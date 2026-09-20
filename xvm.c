#include "xvm.h"
#include "xcollection.h"
#include "functions.h"
#include <inttypes.h>
#include <time.h>

/* -------------------------------------------------------------------------
 * Instance Lifecycle
 * ------------------------------------------------------------------------- */
XInstance* xinstance_create_with_id(XVm* vm, const char* class_name, int id)
{
	XInstance* inst = (XInstance*)calloc(1, sizeof(XInstance));
	inst->class_name = strdup(class_name ? class_name : "Object");
	inst->id = id;
	if (vm)
	{
		inst->next = vm->all_instances;
		vm->all_instances = inst;
	}
	return inst;
}

XInstance* xinstance_create(XVm* vm, const char* class_name)
{
	int id = 0;
	if (class_name)
	{
		if (strcmp(class_name, "List") == 0)
		{
			id = x_list_alloc();
		}
		else if (strcmp(class_name, "Map") == 0 || strcmp(class_name, "HashMap") == 0)
		{
			id = x_map_alloc();
		}
		else if (strcmp(class_name, "DateTime") == 0)
		{
			id = (int)time(NULL);
		}
	}
	return xinstance_create_with_id(vm, class_name, id);
}

void xinstance_free(XInstance* inst)
{
	if (!inst) return;
	if (inst->class_name) free(inst->class_name);
	for (int i = 0; i < inst->field_count; i++)
	{
		if (inst->field_names[i]) free(inst->field_names[i]);
	}
	if (inst->field_names) free(inst->field_names);
	if (inst->field_values) free(inst->field_values);
	free(inst);
}

/* -------------------------------------------------------------------------
 * Closure & Upvalue Lifecycle
 * ------------------------------------------------------------------------- */
XClosure* xclosure_create(XVm* vm, XFunction* function)
{
	XClosure* closure = (XClosure*)malloc(sizeof(XClosure));
	closure->function = function;
	closure->upvalue_count = function ? function->upvalue_count : 0;
	if (closure->upvalue_count > 0)
	{
		closure->upvalues = (XUpvalue**)calloc(closure->upvalue_count, sizeof(XUpvalue*));
	}
	else
	{
		closure->upvalues = NULL;
	}

	closure->next = vm ? vm->all_closures : NULL;
	if (vm) vm->all_closures = closure;
	return closure;
}

void xclosure_free(XClosure* closure)
{
	if (!closure) return;
	if (closure->upvalues)
	{
		free(closure->upvalues);
	}
	free(closure);
}

static XUpvalue* capture_upvalue(XVm* vm, XValue* local)
{
	XUpvalue* prev = NULL;
	XUpvalue* curr = vm->open_upvalues;
	while (curr != NULL && curr->location > local)
	{
		prev = curr;
		curr = curr->next;
	}

	if (curr != NULL && curr->location == local)
	{
		return curr;
	}

	XUpvalue* created = (XUpvalue*)malloc(sizeof(XUpvalue));
	created->location = local;
	created->closed = xval_null();
	created->next = curr;

	if (prev == NULL)
	{
		vm->open_upvalues = created;
	}
	else
	{
		prev->next = created;
	}

	/* Track in all_upvalues for leak-free teardown */
	created->all_next = vm->all_upvalues;
	vm->all_upvalues = created;

	return created;
}

static void close_upvalues(XVm* vm, XValue* last)
{
	while (vm->open_upvalues != NULL && vm->open_upvalues->location >= last)
	{
		XUpvalue* upvalue = vm->open_upvalues;
		upvalue->closed = *upvalue->location;
		if (upvalue->closed.type == VAL_STRING && upvalue->closed.as.sval)
		{
			upvalue->closed.as.sval = strdup(upvalue->closed.as.sval);
		}
		upvalue->location = &upvalue->closed;
		vm->open_upvalues = upvalue->next;
	}
}

/* -------------------------------------------------------------------------
 * VM Initialization & Teardown
 * ------------------------------------------------------------------------- */
void xvm_init(XVm* vm)
{
	vm->frame_count = 0;
	vm->stack_top = vm->stack;
	vm->open_upvalues = NULL;
	vm->all_upvalues = NULL;
	vm->all_closures = NULL;
	vm->all_instances = NULL;
	vm->global_count = 0;
	vm->print_trace = false;
}

void xvm_free(XVm* vm)
{
	/* Free all created upvalues */
	XUpvalue* u = vm->all_upvalues;
	while (u != NULL)
	{
		XUpvalue* next = u->all_next;
		if (u->closed.type == VAL_STRING && u->closed.as.sval)
		{
			free(u->closed.as.sval);
		}
		free(u);
		u = next;
	}
	vm->all_upvalues = NULL;
	vm->open_upvalues = NULL;

	/* Free all tracked closures */
	XClosure* c = vm->all_closures;
	while (c != NULL)
	{
		XClosure* next = c->next;
		xclosure_free(c);
		c = next;
	}
	vm->all_closures = NULL;

	/* Free all tracked instances */
	XInstance* inst = vm->all_instances;
	while (inst != NULL)
	{
		XInstance* next = inst->next;
		xinstance_free(inst);
		inst = next;
	}
	vm->all_instances = NULL;

	/* Free global variables */
	for (int i = 0; i < vm->global_count; i++)
	{
		if (vm->globals[i].name) free(vm->globals[i].name);
		if (vm->globals[i].value.type == VAL_STRING && vm->globals[i].value.as.sval)
		{
			free(vm->globals[i].value.as.sval);
		}
	}
	vm->global_count = 0;
	vm->stack_top = vm->stack;
	vm->frame_count = 0;
}

/* -------------------------------------------------------------------------
 * Stack Utilities
 * ------------------------------------------------------------------------- */
void xvm_push(XVm* vm, XValue val)
{
	if (vm->stack_top - vm->stack >= VM_STACK_MAX)
	{
		fprintf(stderr, "VM Stack Overflow\n");
		return;
	}
	*vm->stack_top = val;
	vm->stack_top++;
}

XValue xvm_pop(XVm* vm)
{
	if (vm->stack_top == vm->stack)
	{
		fprintf(stderr, "VM Stack Underflow\n");
		return xval_null();
	}
	vm->stack_top--;
	return *vm->stack_top;
}

XValue xvm_peek(XVm* vm, int distance)
{
	return vm->stack_top[-1 - distance];
}

/* -------------------------------------------------------------------------
 * Global Variable Storage
 * ------------------------------------------------------------------------- */
void xvm_set_global(XVm* vm, const char* name, XValue val)
{
	if (!name) return;
	XValue store_val = val;
	if (val.type == VAL_STRING && val.as.sval)
	{
		store_val.as.sval = strdup(val.as.sval);
	}

	for (int i = 0; i < vm->global_count; i++)
	{
		if (strcmp(vm->globals[i].name, name) == 0)
		{
			if (vm->globals[i].value.type == VAL_STRING && vm->globals[i].value.as.sval)
			{
				free(vm->globals[i].value.as.sval);
			}
			vm->globals[i].value = store_val;
			return;
		}
	}

	if (vm->global_count < VM_GLOBALS_MAX)
	{
		vm->globals[vm->global_count].name = strdup(name);
		vm->globals[vm->global_count].value = store_val;
		vm->global_count++;
	}
	else if (store_val.type == VAL_STRING && store_val.as.sval)
	{
		free(store_val.as.sval);
	}
}

bool xvm_get_global(XVm* vm, const char* name, XValue* out_val)
{
	if (!name) return false;
	for (int i = 0; i < vm->global_count; i++)
	{
		if (strcmp(vm->globals[i].name, name) == 0)
		{
			if (out_val) *out_val = vm->globals[i].value;
			return true;
		}
	}
	return false;
}

/* -------------------------------------------------------------------------
 * Bytecode Operand Readers
 * ------------------------------------------------------------------------- */
static inline uint16_t read_short(XCallFrame* frame)
{
	uint16_t val = (uint16_t)((frame->ip[0] << 8) | frame->ip[1]);
	frame->ip += 2;
	return val;
}

static inline int32_t read_int(XCallFrame* frame)
{
	int32_t val = (int32_t)((frame->ip[0] << 24) | (frame->ip[1] << 16) | (frame->ip[2] << 8) | frame->ip[3]);
	frame->ip += 4;
	return val;
}

/* -------------------------------------------------------------------------
 * Virtual Machine Dispatch Loop
 * ------------------------------------------------------------------------- */
XVmResult xvm_run(XVm* vm, XIrChunk* chunk)
{
	if (!vm || !chunk) return VM_RUNTIME_ERROR;

	/* Create synthetic root function for the top-level chunk */
	XFunction top_fn;
	top_fn.name = "main";
	top_fn.arity = 0;
	top_fn.upvalue_count = 0;
	top_fn.chunk = *chunk;

	XClosure* top_closure = xclosure_create(vm, &top_fn);

	vm->stack_top = vm->stack;
	vm->frame_count = 1;
	XCallFrame* frame = &vm->frames[0];
	frame->closure = top_closure;
	frame->ip = chunk->code;
	frame->slots = vm->stack;
	frame->return_slot = vm->stack;

	while (true)
	{
		XIrChunk* current_chunk = &frame->closure->function->chunk;

		if (frame->ip >= current_chunk->code + current_chunk->count)
		{
			if (vm->frame_count <= 1)
			{
				break;
			}
			/* Implicit return from function */
			close_upvalues(vm, frame->slots);
			XValue* ret_dest = frame->return_slot;
			vm->frame_count--;
			vm->stack_top = ret_dest;
			xvm_push(vm, xval_null());
			frame = &vm->frames[vm->frame_count - 1];
			continue;
		}

		if (vm->print_trace)
		{
			printf("          ");
			for (XValue* slot = vm->stack; slot < vm->stack_top; slot++)
			{
				printf("[ ");
				xval_print(*slot);
				printf(" ]");
			}
			printf("\n");
			xir_disassemble_instruction(current_chunk, (int)(frame->ip - current_chunk->code));
		}

		uint8_t instruction = *frame->ip++;

		switch ((XIrOpCode)instruction)
		{
		case OP_NOP:
			break;

		case OP_CONST_NULL:
			xvm_push(vm, xval_null());
			break;

		case OP_CONST_TRUE:
			xvm_push(vm, xval_bool(true));
			break;

		case OP_CONST_FALSE:
			xvm_push(vm, xval_bool(false));
			break;

		case OP_CONST_INT:
			{
				int32_t val = read_int(frame);
				xvm_push(vm, xval_int(val));
				break;
			}

		case OP_CONST_FLOAT:
		case OP_CONST_STR:
			{
				uint16_t c_idx = read_short(frame);
				if (c_idx < current_chunk->constants.count)
				{
					xvm_push(vm, current_chunk->constants.values[c_idx]);
				}
				else
				{
					xvm_push(vm, xval_null());
				}
				break;
			}

		case OP_LOAD_GLOBAL:
			{
				uint16_t s_idx = read_short(frame);
				const char* name = (s_idx < current_chunk->symbols.count) ? current_chunk->symbols.symbols[s_idx] : "";
				XValue val = xval_null();
				if (!xvm_get_global(vm, name, &val))
				{
					val = xval_null();
				}
				xvm_push(vm, val);
				break;
			}

		case OP_STORE_GLOBAL:
			{
				uint16_t s_idx = read_short(frame);
				const char* name = (s_idx < current_chunk->symbols.count) ? current_chunk->symbols.symbols[s_idx] : "";
				XValue val = xvm_pop(vm);
				xvm_set_global(vm, name, val);
				break;
			}

		case OP_LOAD_LOCAL:
			{
				uint16_t slot = read_short(frame);
				xvm_push(vm, frame->slots[slot]);
				break;
			}

		case OP_STORE_LOCAL:
			{
				uint16_t slot = read_short(frame);
				frame->slots[slot] = xvm_peek(vm, 0);
				break;
			}

		case OP_GET_UPVALUE:
			{
				uint8_t slot = *frame->ip++;
				if (frame->closure && slot < frame->closure->upvalue_count && frame->closure->upvalues[slot])
				{
					xvm_push(vm, *frame->closure->upvalues[slot]->location);
				}
				else
				{
					xvm_push(vm, xval_null());
				}
				break;
			}

		case OP_SET_UPVALUE:
			{
				uint8_t slot = *frame->ip++;
				if (frame->closure && slot < frame->closure->upvalue_count && frame->closure->upvalues[slot])
				{
					*frame->closure->upvalues[slot]->location = xvm_peek(vm, 0);
				}
				break;
			}

		case OP_CLOSE_UPVALUE:
			{
				close_upvalues(vm, vm->stack_top - 1);
				xvm_pop(vm);
				break;
			}

		case OP_CLOSURE:
			{
				uint16_t c_idx = read_short(frame);
				if (c_idx < current_chunk->constants.count &&
				    current_chunk->constants.values[c_idx].type == VAL_FUNCTION)
				{
					XFunction* fn = current_chunk->constants.values[c_idx].as.fnval;
					XClosure* closure = xclosure_create(vm, fn);
					for (int i = 0; i < fn->upvalue_count; i++)
					{
						uint8_t is_local = *frame->ip++;
						uint8_t index = *frame->ip++;
						if (is_local)
						{
							closure->upvalues[i] = capture_upvalue(vm, frame->slots + index);
						}
						else if (frame->closure && index < frame->closure->upvalue_count)
						{
							closure->upvalues[i] = frame->closure->upvalues[index];
						}
					}
					xvm_push(vm, xval_closure(closure));
				}
				else
				{
					xvm_push(vm, xval_null());
				}
				break;
			}

		case OP_NEW_INSTANCE:
			{
				uint16_t s_idx = read_short(frame);
				uint8_t arg_count = *frame->ip++;
				const char* class_name = (s_idx < current_chunk->symbols.count) ? current_chunk->symbols.symbols[s_idx] : "";
				XValue args[32];
				for (int i = arg_count - 1; i >= 0; i--)
				{
					args[i] = xvm_pop(vm);
				}

				XInstance* inst = NULL;
				if (strcmp(class_name, "DateTime") == 0)
				{
					int ts = (arg_count >= 1 && args[0].type == VAL_INT) ? (int)args[0].as.ival : (int)time(NULL);
					inst = xinstance_create_with_id(vm, class_name, ts);
				}
				else if (strcmp(class_name, "List") == 0)
				{
					inst = xinstance_create_with_id(vm, class_name, x_list_alloc());
				}
				else if (strcmp(class_name, "Map") == 0 || strcmp(class_name, "HashMap") == 0)
				{
					inst = xinstance_create_with_id(vm, class_name, x_map_alloc());
				}
				else
				{
					inst = xinstance_create(vm, class_name);
				}
				xvm_push(vm, xval_obj(inst));
				break;
			}

		case OP_BUILD_LIST:
			{
				uint16_t count = read_short(frame);
				XInstance* inst = xinstance_create(vm, "List");
				XValue items[64];
				for (int i = count - 1; i >= 0; i--)
				{
					items[i] = xvm_pop(vm);
				}
				for (int i = 0; i < count; i++)
				{
					if (items[i].type == VAL_INT) x_list_append_int(inst->id, (int)items[i].as.ival);
					else if (items[i].type == VAL_FLOAT) x_list_append_float(inst->id, (float)items[i].as.fval);
					else if (items[i].type == VAL_STRING) x_list_append_str(inst->id, items[i].as.sval ? items[i].as.sval : "");
				}
				xvm_push(vm, xval_obj(inst));
				break;
			}

		case OP_LOAD_FIELD:
			{
				uint16_t s_idx = read_short(frame);
				const char* field_name = (s_idx < current_chunk->symbols.count) ? current_chunk->symbols.symbols[s_idx] : "";
				XValue obj = xvm_pop(vm);
				if (obj.type == VAL_OBJECT && obj.as.oval != NULL)
				{
					XInstance* inst = (XInstance*)obj.as.oval;
					if (strcmp(field_name, "id") == 0 ||
					    (strcmp(inst->class_name, "DateTime") == 0 && strcmp(field_name, "timestamp") == 0))
					{
						xvm_push(vm, xval_int(inst->id));
						break;
					}
					bool found = false;
					for (int i = 0; i < inst->field_count; i++)
					{
						if (strcmp(inst->field_names[i], field_name) == 0)
						{
							xvm_push(vm, inst->field_values[i]);
							found = true;
							break;
						}
					}
					if (found) break;
				}
				xvm_push(vm, xval_null());
				break;
			}

		case OP_STORE_FIELD:
			{
				uint16_t s_idx = read_short(frame);
				const char* field_name = (s_idx < current_chunk->symbols.count) ? current_chunk->symbols.symbols[s_idx] : "";
				XValue val = xvm_pop(vm);
				XValue obj = xvm_pop(vm);
				if (obj.type == VAL_OBJECT && obj.as.oval != NULL)
				{
					XInstance* inst = (XInstance*)obj.as.oval;
					if ((strcmp(field_name, "id") == 0 ||
					     (strcmp(inst->class_name, "DateTime") == 0 && strcmp(field_name, "timestamp") == 0)) &&
					    val.type == VAL_INT)
					{
						inst->id = (int)val.as.ival;
						xvm_push(vm, val);
						break;
					}
					int idx = -1;
					for (int i = 0; i < inst->field_count; i++)
					{
						if (strcmp(inst->field_names[i], field_name) == 0)
						{
							idx = i;
							break;
						}
					}
					if (idx == -1)
					{
						inst->field_count++;
						inst->field_names = (char**)realloc(inst->field_names, inst->field_count * sizeof(char*));
						inst->field_values = (XValue*)realloc(inst->field_values, inst->field_count * sizeof(XValue));
						idx = inst->field_count - 1;
						inst->field_names[idx] = strdup(field_name);
					}
					inst->field_values[idx] = val;
				}
				xvm_push(vm, val);
				break;
			}

		case OP_LOAD_INDEX:
			{
				XValue idx_val = xvm_pop(vm);
				XValue target = xvm_pop(vm);
				if (target.type == VAL_OBJECT && target.as.oval != NULL)
				{
					XInstance* inst = (XInstance*)target.as.oval;
					if (strcmp(inst->class_name, "List") == 0 && idx_val.type == VAL_INT)
					{
						int idx = (int)idx_val.as.ival;
						int itype = x_list_item_type(inst->id, idx);
						if (itype == 1) xvm_push(vm, xval_int(x_list_item_int(inst->id, idx)));
						else if (itype == 2) xvm_push(vm, xval_float(x_list_item_float(inst->id, idx)));
						else xvm_push(vm, xval_str(x_list_item_str(inst->id, idx)));
						break;
					}
					else if ((strcmp(inst->class_name, "Map") == 0 || strcmp(inst->class_name, "HashMap") == 0) && idx_val.type == VAL_STRING)
					{
						const char* key = idx_val.as.sval ? idx_val.as.sval : "";
						int mtype = x_map_fetch_type(inst->id, key);
						if (mtype == 1) xvm_push(vm, xval_int(x_map_fetch_int(inst->id, key)));
						else if (mtype == 2) xvm_push(vm, xval_float(x_map_fetch_float(inst->id, key)));
						else xvm_push(vm, xval_str(x_map_fetch_str(inst->id, key)));
						break;
					}
				}
				xvm_push(vm, xval_null());
				break;
			}

		case OP_STORE_INDEX:
			{
				XValue val = xvm_pop(vm);
				XValue idx_val = xvm_pop(vm);
				XValue target = xvm_pop(vm);
				if (target.type == VAL_OBJECT && target.as.oval != NULL)
				{
					XInstance* inst = (XInstance*)target.as.oval;
					if ((strcmp(inst->class_name, "Map") == 0 || strcmp(inst->class_name, "HashMap") == 0) && idx_val.type == VAL_STRING)
					{
						const char* key = idx_val.as.sval ? idx_val.as.sval : "";
						if (val.type == VAL_INT) x_map_insert_int(inst->id, key, (int)val.as.ival);
						else if (val.type == VAL_FLOAT) x_map_insert_float(inst->id, key, (float)val.as.fval);
						else x_map_insert_str(inst->id, key, val.as.sval ? val.as.sval : "");
					}
				}
				xvm_push(vm, val);
				break;
			}

		case OP_POP:
			xvm_pop(vm);
			break;

		case OP_DUP:
			xvm_push(vm, xvm_peek(vm, 0));
			break;

		case OP_ADD:
			{
				XValue b = xvm_pop(vm);
				XValue a = xvm_pop(vm);

				if (a.type == VAL_STRING || b.type == VAL_STRING)
				{
					char buf_a[128] = {0};
					char buf_b[128] = {0};
					const char* sa = "";
					const char* sb = "";

					if (a.type == VAL_STRING) sa = a.as.sval ? a.as.sval : "";
					else if (a.type == VAL_INT) { snprintf(buf_a, sizeof(buf_a), "%" PRId64, a.as.ival); sa = buf_a; }
					else if (a.type == VAL_FLOAT) { snprintf(buf_a, sizeof(buf_a), "%g", a.as.fval); sa = buf_a; }
					else if (a.type == VAL_BOOL) { sa = a.as.bval ? "true" : "false"; }

					if (b.type == VAL_STRING) sb = b.as.sval ? b.as.sval : "";
					else if (b.type == VAL_INT) { snprintf(buf_b, sizeof(buf_b), "%" PRId64, b.as.ival); sb = buf_b; }
					else if (b.type == VAL_FLOAT) { snprintf(buf_b, sizeof(buf_b), "%g", b.as.fval); sb = buf_b; }
					else if (b.type == VAL_BOOL) { sb = b.as.bval ? "true" : "false"; }

					size_t len = strlen(sa) + strlen(sb) + 1;
					char* cat = (char*)malloc(len);
					snprintf(cat, len, "%s%s", sa, sb);
					XValue res = xval_str(cat);
					free(cat);
					xvm_push(vm, res);
				}
				else if (a.type == VAL_FLOAT || b.type == VAL_FLOAT)
				{
					double fa = (a.type == VAL_FLOAT) ? a.as.fval : (double)a.as.ival;
					double fb = (b.type == VAL_FLOAT) ? b.as.fval : (double)b.as.ival;
					xvm_push(vm, xval_float(fa + fb));
				}
				else
				{
					xvm_push(vm, xval_int(a.as.ival + b.as.ival));
				}
				break;
			}

		case OP_SUB:
			{
				XValue b = xvm_pop(vm);
				XValue a = xvm_pop(vm);
				if (a.type == VAL_FLOAT || b.type == VAL_FLOAT)
				{
					double fa = (a.type == VAL_FLOAT) ? a.as.fval : (double)a.as.ival;
					double fb = (b.type == VAL_FLOAT) ? b.as.fval : (double)b.as.ival;
					xvm_push(vm, xval_float(fa - fb));
				}
				else
				{
					xvm_push(vm, xval_int(a.as.ival - b.as.ival));
				}
				break;
			}

		case OP_MUL:
			{
				XValue b = xvm_pop(vm);
				XValue a = xvm_pop(vm);
				if (a.type == VAL_FLOAT || b.type == VAL_FLOAT)
				{
					double fa = (a.type == VAL_FLOAT) ? a.as.fval : (double)a.as.ival;
					double fb = (b.type == VAL_FLOAT) ? b.as.fval : (double)b.as.ival;
					xvm_push(vm, xval_float(fa * fb));
				}
				else
				{
					xvm_push(vm, xval_int(a.as.ival * b.as.ival));
				}
				break;
			}

		case OP_DIV:
			{
				XValue b = xvm_pop(vm);
				XValue a = xvm_pop(vm);
				if (a.type == VAL_FLOAT || b.type == VAL_FLOAT)
				{
					double fa = (a.type == VAL_FLOAT) ? a.as.fval : (double)a.as.ival;
					double fb = (b.type == VAL_FLOAT) ? b.as.fval : (double)b.as.ival;
					if (fb == 0.0)
					{
						fprintf(stderr, "VM Runtime Error: Division by zero\n");
						xvm_push(vm, xval_float(0.0));
					}
					else
					{
						xvm_push(vm, xval_float(fa / fb));
					}
				}
				else
				{
					if (b.as.ival == 0)
					{
						fprintf(stderr, "VM Runtime Error: Division by zero\n");
						xvm_push(vm, xval_int(0));
					}
					else
					{
						xvm_push(vm, xval_int(a.as.ival / b.as.ival));
					}
				}
				break;
			}

		case OP_MOD:
			{
				XValue b = xvm_pop(vm);
				XValue a = xvm_pop(vm);
				if (b.as.ival == 0)
				{
					fprintf(stderr, "VM Runtime Error: Modulo by zero\n");
					xvm_push(vm, xval_int(0));
				}
				else
				{
					xvm_push(vm, xval_int(a.as.ival % b.as.ival));
				}
				break;
			}

		case OP_NEG:
			{
				XValue a = xvm_pop(vm);
				if (a.type == VAL_FLOAT)
					xvm_push(vm, xval_float(-a.as.fval));
				else
					xvm_push(vm, xval_int(-a.as.ival));
				break;
			}

		case OP_NOT:
			{
				XValue a = xvm_pop(vm);
				xvm_push(vm, xval_bool(!xval_is_truthy(a)));
				break;
			}

		case OP_BIT_NOT:
			{
				XValue a = xvm_pop(vm);
				xvm_push(vm, xval_int(~a.as.ival));
				break;
			}

		case OP_BIT_AND:
			{
				XValue b = xvm_pop(vm);
				XValue a = xvm_pop(vm);
				xvm_push(vm, xval_int(a.as.ival & b.as.ival));
				break;
			}

		case OP_BIT_OR:
			{
				XValue b = xvm_pop(vm);
				XValue a = xvm_pop(vm);
				xvm_push(vm, xval_int(a.as.ival | b.as.ival));
				break;
			}

		case OP_BIT_XOR:
			{
				XValue b = xvm_pop(vm);
				XValue a = xvm_pop(vm);
				xvm_push(vm, xval_int(a.as.ival ^ b.as.ival));
				break;
			}

		case OP_SHL:
			{
				XValue b = xvm_pop(vm);
				XValue a = xvm_pop(vm);
				xvm_push(vm, xval_int(a.as.ival << b.as.ival));
				break;
			}

		case OP_SHR:
			{
				XValue b = xvm_pop(vm);
				XValue a = xvm_pop(vm);
				xvm_push(vm, xval_int(a.as.ival >> b.as.ival));
				break;
			}

		case OP_EQ:
			{
				XValue b = xvm_pop(vm);
				XValue a = xvm_pop(vm);
				xvm_push(vm, xval_bool(xval_equal(a, b)));
				break;
			}

		case OP_NEQ:
			{
				XValue b = xvm_pop(vm);
				XValue a = xvm_pop(vm);
				xvm_push(vm, xval_bool(!xval_equal(a, b)));
				break;
			}

		case OP_LT:
			{
				XValue b = xvm_pop(vm);
				XValue a = xvm_pop(vm);
				if (a.type == VAL_FLOAT || b.type == VAL_FLOAT)
				{
					double fa = (a.type == VAL_FLOAT) ? a.as.fval : (double)a.as.ival;
					double fb = (b.type == VAL_FLOAT) ? b.as.fval : (double)b.as.ival;
					xvm_push(vm, xval_bool(fa < fb));
				}
				else
				{
					xvm_push(vm, xval_bool(a.as.ival < b.as.ival));
				}
				break;
			}

		case OP_LTE:
			{
				XValue b = xvm_pop(vm);
				XValue a = xvm_pop(vm);
				if (a.type == VAL_FLOAT || b.type == VAL_FLOAT)
				{
					double fa = (a.type == VAL_FLOAT) ? a.as.fval : (double)a.as.ival;
					double fb = (b.type == VAL_FLOAT) ? b.as.fval : (double)b.as.ival;
					xvm_push(vm, xval_bool(fa <= fb));
				}
				else
				{
					xvm_push(vm, xval_bool(a.as.ival <= b.as.ival));
				}
				break;
			}

		case OP_GT:
			{
				XValue b = xvm_pop(vm);
				XValue a = xvm_pop(vm);
				if (a.type == VAL_FLOAT || b.type == VAL_FLOAT)
				{
					double fa = (a.type == VAL_FLOAT) ? a.as.fval : (double)a.as.ival;
					double fb = (b.type == VAL_FLOAT) ? b.as.fval : (double)b.as.ival;
					xvm_push(vm, xval_bool(fa > fb));
				}
				else
				{
					xvm_push(vm, xval_bool(a.as.ival > b.as.ival));
				}
				break;
			}

		case OP_GTE:
			{
				XValue b = xvm_pop(vm);
				XValue a = xvm_pop(vm);
				if (a.type == VAL_FLOAT || b.type == VAL_FLOAT)
				{
					double fa = (a.type == VAL_FLOAT) ? a.as.fval : (double)a.as.ival;
					double fb = (b.type == VAL_FLOAT) ? b.as.fval : (double)b.as.ival;
					xvm_push(vm, xval_bool(fa >= fb));
				}
				else
				{
					xvm_push(vm, xval_bool(a.as.ival >= b.as.ival));
				}
				break;
			}

		case OP_JUMP:
			{
				uint16_t offset = read_short(frame);
				frame->ip = current_chunk->code + offset;
				break;
			}

		case OP_JUMP_IF_FALSE:
			{
				uint16_t offset = read_short(frame);
				XValue cond = xvm_peek(vm, 0);
				if (!xval_is_truthy(cond))
				{
					frame->ip = current_chunk->code + offset;
				}
				break;
			}

		case OP_JUMP_IF_TRUE:
			{
				uint16_t offset = read_short(frame);
				XValue cond = xvm_peek(vm, 0);
				if (xval_is_truthy(cond))
				{
					frame->ip = current_chunk->code + offset;
				}
				break;
			}

		case OP_LOOP:
			{
				uint16_t offset = read_short(frame);
				frame->ip = current_chunk->code + offset;
				break;
			}

		case OP_CALL:
			{
				uint16_t s_idx = read_short(frame);
				uint8_t arg_count = *frame->ip++;
				XClosure* callee_closure = NULL;

				if (s_idx == 0xFFFF)
				{
					/* Callee is on stack right beneath the arguments */
					XValue callee_val = *(vm->stack_top - 1 - arg_count);
					if (callee_val.type == VAL_CLOSURE)
					{
						callee_closure = callee_val.as.closureval;
					}
					else if (callee_val.type == VAL_FUNCTION && callee_val.as.fnval)
					{
						callee_closure = xclosure_create(vm, callee_val.as.fnval);
					}
					else
					{
						fprintf(stderr, "VM Runtime Error: Attempted to call non-function\n");
						return VM_RUNTIME_ERROR;
					}
				}
				else
				{
					const char* name = (s_idx < current_chunk->symbols.count) ? current_chunk->symbols.symbols[s_idx] : "";
					XValue fn_val = xval_null();
					if (!xvm_get_global(vm, name, &fn_val))
					{
						func_deftion* native_fn = get_func_by_name((char*)name);
						if (native_fn != NULL && native_fn->func_code != NULL)
						{
							XValue n_args[32];
							for (int i = arg_count - 1; i >= 0; i--)
							{
								n_args[i] = xvm_pop(vm);
							}
							fcall fc;
							memset(&fc, 0, sizeof(fcall));
							fc.deftion = native_fn;
							fc.parm_count_c = arg_count;
							int i_buf[32];
							float f_buf[32];
							char* s_buf[32];
							bool b_buf[32];
							for (int i = 0; i < arg_count; i++)
							{
								if (n_args[i].type == VAL_INT)
								{
									i_buf[i] = (int)n_args[i].as.ival;
									fc.func_parmeters[i].type_define = T_INT;
									fc.func_parmeters[i].value_int = &i_buf[i];
									fc.func_parmeters[i].values = &i_buf[i];
								}
								else if (n_args[i].type == VAL_FLOAT)
								{
									f_buf[i] = (float)n_args[i].as.fval;
									fc.func_parmeters[i].type_define = T_FLOAT;
									fc.func_parmeters[i].value_float = &f_buf[i];
									fc.func_parmeters[i].values = &f_buf[i];
								}
								else if (n_args[i].type == VAL_STRING)
								{
									s_buf[i] = n_args[i].as.sval;
									fc.func_parmeters[i].type_define = T_STRING;
									fc.func_parmeters[i].value_str_ptr = &s_buf[i];
									fc.func_parmeters[i].values = &s_buf[i];
								}
								else if (n_args[i].type == VAL_BOOL)
								{
									b_buf[i] = n_args[i].as.bval;
									fc.func_parmeters[i].type_define = T_BOOL;
									fc.func_parmeters[i].value_bool = &b_buf[i];
									fc.func_parmeters[i].values = &b_buf[i];
								}
								else if (n_args[i].type == VAL_OBJECT && n_args[i].as.oval != NULL)
								{
									XInstance* inst = (XInstance*)n_args[i].as.oval;
									i_buf[i] = inst->id;
									type_def* td = get_type_by_name(inst->class_name ? inst->class_name : "");
									fc.func_parmeters[i].type_define = td ? td : T_INT;
									fc.func_parmeters[i].value_int = &i_buf[i];
									fc.func_parmeters[i].values = &i_buf[i];
								}
							}
							native_fn->func_code(&fc);
							if (strcmp(name, "json_parse") == 0)
							{
								int obj_id = -1;
								const char* tname = (fc._return.type_define && !is_base_type(fc._return.type_define) && fc._return.type_define->type_name) ? fc._return.type_define->type_name : "Map";
								if (fc._return.type_define != NULL && !is_base_type(fc._return.type_define))
								{
									type_instance* ti = fc._return.value_type_instsance ? fc._return.value_type_instsance : (type_instance*)fc._return.values;
									if (ti != NULL)
									{
										var* id_prop = get_var_by_name_on_stack("id", &ti->propertys);
										if (id_prop != NULL && id_prop->value_int != NULL)
											obj_id = *id_prop->value_int;
									}
								}
								else if (fc._return.value_int != NULL)
								{
									obj_id = *fc._return.value_int;
								}

								if (obj_id >= 0)
								{
									XInstance* inst = xinstance_create_with_id(vm, tname, obj_id);
									xvm_push(vm, xval_obj(inst));
								}
								else
								{
									xvm_push(vm, xval_null());
								}
								break;
							}
							if (fc._return.type_define == T_INT && fc._return.value_int)
								xvm_push(vm, xval_int(*fc._return.value_int));
							else if (fc._return.type_define == T_FLOAT && fc._return.value_float)
								xvm_push(vm, xval_float(*fc._return.value_float));
							else if (fc._return.type_define == T_STRING && fc._return.value_str_ptr && *fc._return.value_str_ptr)
								xvm_push(vm, xval_str(*fc._return.value_str_ptr));
							else if (fc._return.type_define == T_BOOL && fc._return.value_bool)
								xvm_push(vm, xval_bool(*fc._return.value_bool));
							else if (fc._return.type_define != NULL && !is_base_type(fc._return.type_define))
							{
								const char* tname = fc._return.type_define->type_name ? fc._return.type_define->type_name : "Object";
								int obj_id = 0;
								type_instance* ti = fc._return.value_type_instsance ? fc._return.value_type_instsance : (type_instance*)fc._return.values;
								if (ti != NULL)
								{
									var* id_prop = get_var_by_name_on_stack("id", &ti->propertys);
									if (id_prop != NULL && id_prop->value_int != NULL)
										obj_id = *id_prop->value_int;
								}
								XInstance* inst = xinstance_create_with_id(vm, tname, obj_id);
								xvm_push(vm, xval_obj(inst));
							}
							else
								xvm_push(vm, xval_null());
							break;
						}

						fprintf(stderr, "VM Runtime Error: Undefined function '%s'\n", name);
						return VM_RUNTIME_ERROR;
					}
					if (fn_val.type == VAL_CLOSURE)
					{
						callee_closure = fn_val.as.closureval;
					}
					else if (fn_val.type == VAL_FUNCTION && fn_val.as.fnval)
					{
						callee_closure = xclosure_create(vm, fn_val.as.fnval);
					}
					else
					{
						fprintf(stderr, "VM Runtime Error: '%s' is not callable\n", name);
						return VM_RUNTIME_ERROR;
					}
				}

				if (callee_closure->function && arg_count != callee_closure->function->arity)
				{
					fprintf(stderr, "VM Runtime Error: Function '%s' expects %d arguments, but got %d\n",
					        callee_closure->function->name ? callee_closure->function->name : "fn",
					        callee_closure->function->arity, arg_count);
					return VM_RUNTIME_ERROR;
				}

				if (vm->frame_count >= VM_FRAMES_MAX)
				{
					fprintf(stderr, "VM Stack Overflow: Call frame depth exceeded (%d)\n", VM_FRAMES_MAX);
					return VM_RUNTIME_ERROR;
				}

				XCallFrame* new_frame = &vm->frames[vm->frame_count++];
				new_frame->closure = callee_closure;
				new_frame->ip = callee_closure->function->chunk.code;
				new_frame->slots = vm->stack_top - arg_count;
				new_frame->return_slot = (s_idx == 0xFFFF) ? (vm->stack_top - 1 - arg_count) : (vm->stack_top - arg_count);
				frame = new_frame;
				break;
			}

		case OP_CALL_METHOD:
			{
				uint16_t s_idx = read_short(frame);
				uint8_t arg_count = *frame->ip++;
				const char* method_name = (s_idx < current_chunk->symbols.count) ? current_chunk->symbols.symbols[s_idx] : "";

				XValue args[32];
				for (int i = arg_count - 1; i >= 0; i--)
				{
					args[i] = xvm_pop(vm);
				}
				XValue receiver = xvm_pop(vm);

				if (receiver.type == VAL_OBJECT && receiver.as.oval != NULL)
				{
					XInstance* inst = (XInstance*)receiver.as.oval;
					if (strcmp(inst->class_name, "List") == 0)
					{
						if (strcmp(method_name, "add") == 0 || strcmp(method_name, "add_int") == 0 || strcmp(method_name, "add_float") == 0)
						{
							if (arg_count >= 1)
							{
								if (args[0].type == VAL_INT) x_list_append_int(inst->id, (int)args[0].as.ival);
								else if (args[0].type == VAL_FLOAT) x_list_append_float(inst->id, (float)args[0].as.fval);
								else x_list_append_str(inst->id, args[0].as.sval ? args[0].as.sval : "");
							}
							xvm_push(vm, xval_int(x_list_count(inst->id)));
							break;
						}
						else if (strcmp(method_name, "get") == 0 || strcmp(method_name, "get_int") == 0 || strcmp(method_name, "get_float") == 0)
						{
							int idx = (arg_count >= 1 && args[0].type == VAL_INT) ? (int)args[0].as.ival : 0;
							int itype = x_list_item_type(inst->id, idx);
							if (itype == 1) xvm_push(vm, xval_int(x_list_item_int(inst->id, idx)));
							else if (itype == 2) xvm_push(vm, xval_float(x_list_item_float(inst->id, idx)));
							else xvm_push(vm, xval_str(x_list_item_str(inst->id, idx)));
							break;
						}
						else if (strcmp(method_name, "size") == 0 || strcmp(method_name, "length") == 0)
						{
							xvm_push(vm, xval_int(x_list_count(inst->id)));
							break;
						}
						else if (strcmp(method_name, "free") == 0)
						{
							x_list_free_id(inst->id);
							xvm_push(vm, xval_int(0));
							break;
						}
					}
					else if (strcmp(inst->class_name, "Map") == 0 || strcmp(inst->class_name, "HashMap") == 0)
					{
						if (strcmp(method_name, "put") == 0 || strcmp(method_name, "set") == 0 ||
						    strcmp(method_name, "put_int") == 0 || strcmp(method_name, "put_float") == 0)
						{
							const char* key = (arg_count >= 1 && args[0].type == VAL_STRING) ? args[0].as.sval : "";
							if (arg_count >= 2)
							{
								if (args[1].type == VAL_INT) x_map_insert_int(inst->id, key, (int)args[1].as.ival);
								else if (args[1].type == VAL_FLOAT) x_map_insert_float(inst->id, key, (float)args[1].as.fval);
								else x_map_insert_str(inst->id, key, args[1].as.sval ? args[1].as.sval : "");
							}
							xvm_push(vm, xval_int(1));
							break;
						}
						else if (strcmp(method_name, "get") == 0 || strcmp(method_name, "get_int") == 0 || strcmp(method_name, "get_float") == 0)
						{
							const char* key = (arg_count >= 1 && args[0].type == VAL_STRING) ? args[0].as.sval : "";
							int mtype = x_map_fetch_type(inst->id, key);
							if (mtype == 1) xvm_push(vm, xval_int(x_map_fetch_int(inst->id, key)));
							else if (mtype == 2) xvm_push(vm, xval_float(x_map_fetch_float(inst->id, key)));
							else xvm_push(vm, xval_str(x_map_fetch_str(inst->id, key)));
							break;
						}
						else if (strcmp(method_name, "has") == 0 || strcmp(method_name, "contains") == 0)
						{
							const char* key = (arg_count >= 1 && args[0].type == VAL_STRING) ? args[0].as.sval : "";
							xvm_push(vm, xval_bool(x_map_contains_key(inst->id, key)));
							break;
						}
						else if (strcmp(method_name, "size") == 0 || strcmp(method_name, "length") == 0)
						{
							xvm_push(vm, xval_int(x_map_count(inst->id)));
							break;
						}
						else if (strcmp(method_name, "free") == 0)
						{
							x_map_free_id(inst->id);
							xvm_push(vm, xval_int(0));
							break;
						}
					}
					else if (strcmp(inst->class_name, "DateTime") == 0)
					{
						time_t t = (time_t)inst->id;
						if (t <= 0) t = time(NULL);
						struct tm tm_info;
						localtime_r(&t, &tm_info);

						if (strcmp(method_name, "year") == 0)
						{
							xvm_push(vm, xval_int(tm_info.tm_year + 1900));
							break;
						}
						else if (strcmp(method_name, "month") == 0)
						{
							xvm_push(vm, xval_int(tm_info.tm_mon + 1));
							break;
						}
						else if (strcmp(method_name, "day") == 0)
						{
							xvm_push(vm, xval_int(tm_info.tm_mday));
							break;
						}
						else if (strcmp(method_name, "hour") == 0)
						{
							xvm_push(vm, xval_int(tm_info.tm_hour));
							break;
						}
						else if (strcmp(method_name, "minute") == 0)
						{
							xvm_push(vm, xval_int(tm_info.tm_min));
							break;
						}
						else if (strcmp(method_name, "second") == 0)
						{
							xvm_push(vm, xval_int(tm_info.tm_sec));
							break;
						}
						else if (strcmp(method_name, "to_str") == 0)
						{
							char buf[64];
							strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_info);
							xvm_push(vm, xval_str(buf));
							break;
						}
						else if (strcmp(method_name, "format") == 0)
						{
							const char* fmt = (arg_count >= 1 && args[0].type == VAL_STRING) ? args[0].as.sval : "%Y-%m-%d %H:%M:%S";
							char buf[256];
							strftime(buf, sizeof(buf), fmt ? fmt : "%Y-%m-%d %H:%M:%S", &tm_info);
							xvm_push(vm, xval_str(buf));
							break;
						}
					}
					else
					{
						/* User class method: lookup "ClassName.methodName" */
						char user_mname[256];
						snprintf(user_mname, sizeof(user_mname), "%s.%s", inst->class_name, method_name);
						XValue mfn = xval_null();
						if (xvm_get_global(vm, user_mname, &mfn))
						{
							XClosure* cl = NULL;
							if (mfn.type == VAL_CLOSURE) cl = mfn.as.closureval;
							else if (mfn.type == VAL_FUNCTION && mfn.as.fnval) cl = xclosure_create(vm, mfn.as.fnval);
							if (cl != NULL)
							{
								if (vm->frame_count >= VM_FRAMES_MAX)
								{
									fprintf(stderr, "VM Stack Overflow\n");
									return VM_RUNTIME_ERROR;
								}
								xvm_push(vm, receiver);
								for (int i = 0; i < arg_count; i++)
								{
									xvm_push(vm, args[i]);
								}
								XCallFrame* new_frame = &vm->frames[vm->frame_count++];
								new_frame->closure = cl;
								new_frame->ip = cl->function->chunk.code;
								new_frame->slots = vm->stack_top - (arg_count + 1);
								new_frame->return_slot = vm->stack_top - (arg_count + 1);
								frame = new_frame;
								break;
							}
						}
					}
				}
				else if (receiver.type == VAL_STRING)
				{
					if (strcmp(method_name, "length") == 0 || strcmp(method_name, "size") == 0 || strcmp(method_name, "len") == 0)
					{
						xvm_push(vm, xval_int(receiver.as.sval ? (int64_t)strlen(receiver.as.sval) : 0));
						break;
					}
				}

				xvm_push(vm, xval_null());
				break;
			}

		case OP_RETURN:
			{
				XValue result = xvm_pop(vm);
				close_upvalues(vm, frame->slots);
				XValue* ret_dest = frame->return_slot;
				vm->frame_count--;
				if (vm->frame_count == 0)
				{
					return VM_OK;
				}
				vm->stack_top = ret_dest;
				xvm_push(vm, result);
				frame = &vm->frames[vm->frame_count - 1];
				break;
			}

		case OP_PRINT:
			{
				uint8_t arg_count = *frame->ip++;
				XValue args[32];
				for (int i = arg_count - 1; i >= 0; i--)
				{
					args[i] = xvm_pop(vm);
				}

				if (arg_count > 0 && args[0].type == VAL_STRING && strchr(args[0].as.sval ? args[0].as.sval : "", '%') != NULL)
				{
					const char* fmt = args[0].as.sval;
					int arg_idx = 1;
					for (const char* p = fmt; *p != '\0'; p++)
					{
						if (*p == '%' && *(p + 1) != '\0')
						{
							p++;
							if (*p == '%')
							{
								putchar('%');
								continue;
							}
							if (arg_idx < arg_count)
							{
								XValue arg = args[arg_idx++];
								if (arg.type == VAL_INT) printf("%ld", (long)arg.as.ival);
								else if (arg.type == VAL_FLOAT) printf("%f", arg.as.fval);
								else if (arg.type == VAL_STRING) printf("%s", arg.as.sval ? arg.as.sval : "null");
								else if (arg.type == VAL_BOOL) printf("%s", arg.as.bval ? "True" : "False");
								else if (arg.type == VAL_NULL) printf("null");
								else xval_print(arg);
							}
						}
						else
						{
							putchar(*p);
						}
					}
					size_t flen = strlen(fmt);
					if (flen == 0 || fmt[flen - 1] != '\n')
					{
						putchar('\n');
					}
				}
				else
				{
					for (int i = 0; i < arg_count; i++)
					{
						xval_print(args[i]);
						if (i < arg_count - 1) printf(" ");
					}
					bool ends_newline = false;
					if (arg_count == 1 && args[0].type == VAL_STRING && args[0].as.sval)
					{
						size_t len = strlen(args[0].as.sval);
						if (len > 0 && args[0].as.sval[len - 1] == '\n') ends_newline = true;
					}
					if (!ends_newline)
					{
						printf("\n");
					}
				}
				xvm_push(vm, xval_null());
				break;
			}

		case OP_HALT:
			if (vm->frame_count <= 1)
			{
				return VM_OK;
			}
			else
			{
				close_upvalues(vm, frame->slots);
				XValue* ret_dest = frame->return_slot;
				vm->frame_count--;
				vm->stack_top = ret_dest;
				xvm_push(vm, xval_null());
				frame = &vm->frames[vm->frame_count - 1];
				break;
			}

		default:
			fprintf(stderr, "Unknown opcode in VM: 0x%02x\n", instruction);
			return VM_RUNTIME_ERROR;
		}
	}

	return VM_OK;
}
