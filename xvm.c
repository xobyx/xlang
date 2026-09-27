#include "xvm.h"
#include "xcollection.h"
#include "functions.h"
#include <inttypes.h>
#include <time.h>

XVm* g_current_vm = NULL;

/* -------------------------------------------------------------------------
 * Class & VTable Lifecycle
 * ------------------------------------------------------------------------- */
XClass* xclass_create(XVm* vm, const char* name, XClass* base)
{
	XClass* klass = (XClass*)calloc(1, sizeof(XClass));
	klass->name = strdup(name ? name : "Object");
	klass->base = base;
	if (base)
	{
		klass->field_count = base->field_count;
		klass->field_capacity = base->field_count > 0 ? base->field_count : 8;
		klass->fields = (XFieldDesc*)calloc(klass->field_capacity, sizeof(XFieldDesc));
		for (uint32_t i = 0; i < base->field_count; i++)
		{
			klass->fields[i].name = strdup(base->fields[i].name);
			klass->fields[i].slot_idx = base->fields[i].slot_idx;
			klass->fields[i].type_name = base->fields[i].type_name ? strdup(base->fields[i].type_name) : NULL;
		}
	}
	if (vm)
	{
		klass->next = vm->all_classes;
		vm->all_classes = klass;
	}
	return klass;
}

void xclass_add_field(XClass* klass, const char* name, const char* type_name)
{
	if (!klass || !name) return;
	for (uint32_t i = 0; i < klass->field_count; i++)
	{
		if (klass->fields[i].name && strcmp(klass->fields[i].name, name) == 0)
			return;
	}
	if (klass->field_count >= klass->field_capacity)
	{
		klass->field_capacity = klass->field_capacity > 0 ? klass->field_capacity * 2 : 8;
		klass->fields = (XFieldDesc*)realloc(klass->fields, klass->field_capacity * sizeof(XFieldDesc));
	}
	uint32_t idx = klass->field_count++;
	klass->fields[idx].name = strdup(name);
	klass->fields[idx].slot_idx = (uint16_t)idx;
	klass->fields[idx].type_name = type_name ? strdup(type_name) : NULL;
}

int xclass_find_field_slot(const XClass* klass, const char* name)
{
	if (!klass || !name) return -1;
	for (uint32_t i = 0; i < klass->field_count; i++)
	{
		if (klass->fields[i].name && strcmp(klass->fields[i].name, name) == 0)
		{
			return (int)klass->fields[i].slot_idx;
		}
	}
	if (klass->base)
	{
		return xclass_find_field_slot(klass->base, name);
	}
	return -1;
}

void xclass_add_method(XClass* klass, const char* name, int arity, XClosure* closure)
{
	if (!klass || !name) return;
	for (uint32_t i = 0; i < klass->method_count; i++)
	{
		if (klass->methods[i].name && strcmp(klass->methods[i].name, name) == 0 &&
		    (arity == -1 || klass->methods[i].arity == arity))
		{
			klass->methods[i].closure = closure;
			klass->methods[i].arity = arity;
			return;
		}
	}
	if (klass->method_count >= klass->method_capacity)
	{
		klass->method_capacity = klass->method_capacity > 0 ? klass->method_capacity * 2 : 8;
		klass->methods = (XMethod*)realloc(klass->methods, klass->method_capacity * sizeof(XMethod));
	}
	uint32_t idx = klass->method_count++;
	klass->methods[idx].name = strdup(name);
	klass->methods[idx].arity = arity;
	klass->methods[idx].closure = closure;
}

XClosure* xclass_find_method(const XClass* klass, const char* name, int arity)
{
	if (!klass || !name) return NULL;
	for (uint32_t i = 0; i < klass->method_count; i++)
	{
		if (klass->methods[i].name && strcmp(klass->methods[i].name, name) == 0 &&
		    (arity == -1 || klass->methods[i].arity == arity))
		{
			return klass->methods[i].closure;
		}
	}
	for (uint32_t i = 0; i < klass->method_count; i++)
	{
		if (klass->methods[i].name && strcmp(klass->methods[i].name, name) == 0)
		{
			return klass->methods[i].closure;
		}
	}
	if (klass->base)
	{
		return xclass_find_method(klass->base, name, arity);
	}
	return NULL;
}

void xclass_free(XClass* klass)
{
	if (!klass) return;
	if (klass->name) free(klass->name);
	for (uint32_t i = 0; i < klass->field_count; i++)
	{
		if (klass->fields[i].name) free(klass->fields[i].name);
		if (klass->fields[i].type_name) free(klass->fields[i].type_name);
	}
	if (klass->fields) free(klass->fields);
	for (uint32_t i = 0; i < klass->method_count; i++)
	{
		if (klass->methods[i].name) free(klass->methods[i].name);
	}
	if (klass->methods) free(klass->methods);
	free(klass);
}

XClass* xvm_find_class(XVm* vm, const char* name)
{
	if (!vm || !name) return NULL;
	for (XClass* k = vm->all_classes; k != NULL; k = k->next)
	{
		if (k->name && strcmp(k->name, name) == 0)
			return k;
	}
	return NULL;
}

/* -------------------------------------------------------------------------
 * Instance Lifecycle (Shape / Flexible-Array Contiguous Allocation)
 * ------------------------------------------------------------------------- */
XInstance* xinstance_create_class(XVm* vm, XClass* klass, int id)
{
	uint32_t num_fields = klass ? klass->field_count : 0;
	size_t total_size = sizeof(XInstance) + num_fields * sizeof(XValue);
	XInstance* inst = (XInstance*)calloc(1, total_size);
	inst->klass = klass;
	inst->id = id;
	inst->field_count = num_fields;
	for (uint32_t i = 0; i < num_fields; i++)
	{
		inst->fields[i] = xval_null();
	}
	if (klass)
	{
		int id_slot = xclass_find_field_slot(klass, "id");
		if (id_slot >= 0 && (uint32_t)id_slot < num_fields)
		{
			inst->fields[id_slot] = xval_int(id);
		}
	}
	if (vm)
	{
		inst->next = vm->all_instances;
		vm->all_instances = inst;
	}
	return inst;
}

XInstance* xinstance_create_with_id(XVm* vm, const char* class_name, int id)
{
	XClass* klass = NULL;
	if (vm)
	{
		klass = xvm_find_class(vm, class_name);
		if (!klass && class_name)
		{
			klass = xclass_create(vm, class_name, vm->class_object);
		}
	}
	return xinstance_create_class(vm, klass, id);
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
	vm->all_classes = NULL;
	vm->class_object = xclass_create(vm, "Object", NULL);
	vm->class_list = xclass_create(vm, "List", vm->class_object);
	vm->class_map = xclass_create(vm, "Map", vm->class_object);
	vm->class_datetime = xclass_create(vm, "DateTime", vm->class_object);
	vm->global_count = 0;
	vm->print_trace = false;
	vm->jit_enabled = false;
	vm->jit_threshold = 20;
	vm->jit_engine = NULL;
}

void xvm_enable_jit(XVm* vm, int threshold)
{
	if (!vm) return;
	vm->jit_enabled = true;
	vm->jit_threshold = (threshold > 0) ? threshold : 20;
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

	/* Free all classes */
	XClass* k = vm->all_classes;
	while (k != NULL)
	{
		XClass* next = k->next;
		xclass_free(k);
		k = next;
	}
	vm->all_classes = NULL;
	vm->class_list = NULL;
	vm->class_map = NULL;
	vm->class_datetime = NULL;
	vm->class_object = NULL;

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

static inline const char* xinstance_class_name(const XInstance* inst)
{
	return (inst && inst->klass && inst->klass->name) ? inst->klass->name : "Object";
}

/* -------------------------------------------------------------------------
 * Var / XValue Interop Bridge
 * ------------------------------------------------------------------------- */
XValue var_to_xvalue(const var* v)
{
	if (v == NULL || v->type_define == NULL) return xval_null();
	if (v->type_define == T_INT)
	{
		return xval_int(v->value_int != NULL ? *v->value_int : 0);
	}
	else if (v->type_define == T_FLOAT)
	{
		return xval_float(v->value_float != NULL ? *v->value_float : 0.0);
	}
	else if (v->type_define == T_LONG)
	{
		return xval_int(v->value_long != NULL ? *v->value_long : 0);
	}
	else if (v->type_define == T_BOOL)
	{
		return xval_bool(v->value_bool != NULL ? *v->value_bool : false);
	}
	else if (v->type_define == T_STRING)
	{
		const char* s = (v->value_str_ptr != NULL && *v->value_str_ptr != NULL) ? *v->value_str_ptr : "";
		return xval_str(s);
	}
	else if (!is_base_type(v->type_define))
	{
		type_instance* ti = v->value_type_instsance ? v->value_type_instsance : (type_instance*)v->values;
		return xval_obj(ti);
	}
	return xval_null();
}

void xvalue_to_var(XValue xv, var* out_v)
{
	if (out_v == NULL) return;
	memset(out_v, 0, sizeof(var));
	out_v->size = 1;
	switch (xv.type)
	{
	case VAL_INT:
		out_v->type_define = T_INT;
		out_v->inline_val.inline_int = (int)xv.as.ival;
		out_v->values = &out_v->inline_val;
		break;
	case VAL_FLOAT:
		out_v->type_define = T_FLOAT;
		out_v->inline_val.inline_float = (float)xv.as.fval;
		out_v->values = &out_v->inline_val;
		break;
	case VAL_BOOL:
		out_v->type_define = T_BOOL;
		out_v->inline_val.inline_bool = xv.as.bval;
		out_v->values = &out_v->inline_val;
		break;
	case VAL_STRING:
		out_v->type_define = T_STRING;
		out_v->inline_val.raw_primitive = (int64_t)(intptr_t)(xv.as.sval ? xv.as.sval : "");
		out_v->values = &out_v->inline_val;
		break;
	case VAL_OBJECT:
		if (xv.as.oval != NULL)
		{
			XInstance* inst = (XInstance*)xv.as.oval;
			const char* cname = xinstance_class_name(inst);
			type_def* td = cname ? get_type_by_name((char*)cname) : NULL;
			out_v->type_define = td ? td : T_OBJECT;
			out_v->inline_val.inline_int = inst->id;
			out_v->values = &out_v->inline_val;
			out_v->holder = (type_instance*)inst;
		}
		break;
	default:
		out_v->type_define = NULL;
		out_v->values = NULL;
		break;
	}
}

/* -------------------------------------------------------------------------
 * Virtual Machine Dispatch Loop
 * ------------------------------------------------------------------------- */
static XVmResult xvm_run_loop(XVm* vm, XIrChunk* chunk)
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
				frame->slots[slot] = xvm_pop(vm);
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
					*frame->closure->upvalues[slot]->location = xvm_pop(vm);
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

		case OP_CLASS:
			{
				uint16_t s_cls = read_short(frame);
				uint16_t s_base = read_short(frame);
				uint16_t fcount = read_short(frame);
				const char* cname = (s_cls < current_chunk->symbols.count) ? current_chunk->symbols.symbols[s_cls] : "";
				const char* bname = (s_base != 0xFFFF && s_base < current_chunk->symbols.count) ? current_chunk->symbols.symbols[s_base] : NULL;

				XClass* base_klass = bname ? xvm_find_class(vm, bname) : vm->class_object;
				if (!base_klass) base_klass = vm->class_object;

				XClass* klass = xvm_find_class(vm, cname);
				if (!klass)
				{
					klass = xclass_create(vm, cname, base_klass);
				}
				else
				{
					klass->base = base_klass;
					if (base_klass && base_klass->field_count > 0 && klass->field_count == 0)
					{
						klass->field_count = base_klass->field_count;
						klass->field_capacity = base_klass->field_count > 0 ? base_klass->field_count : 8;
						klass->fields = (XFieldDesc*)calloc(klass->field_capacity, sizeof(XFieldDesc));
						for (uint32_t i = 0; i < base_klass->field_count; i++)
						{
							klass->fields[i].name = strdup(base_klass->fields[i].name);
							klass->fields[i].slot_idx = base_klass->fields[i].slot_idx;
							klass->fields[i].type_name = base_klass->fields[i].type_name ? strdup(base_klass->fields[i].type_name) : NULL;
						}
					}
				}

				for (uint16_t i = 0; i < fcount; i++)
				{
					uint16_t s_f = read_short(frame);
					const char* fname = (s_f < current_chunk->symbols.count) ? current_chunk->symbols.symbols[s_f] : "";
					xclass_add_field(klass, fname, NULL);
				}
				break;
			}

		case OP_METHOD:
			{
				uint16_t s_cls = read_short(frame);
				uint16_t s_m = read_short(frame);
				uint8_t arity = *frame->ip++;
				const char* cname = (s_cls < current_chunk->symbols.count) ? current_chunk->symbols.symbols[s_cls] : "";
				const char* mname = (s_m < current_chunk->symbols.count) ? current_chunk->symbols.symbols[s_m] : "";

				XValue closure_val = xvm_pop(vm);
				if (closure_val.type == VAL_CLOSURE && closure_val.as.closureval)
				{
					XClass* klass = xvm_find_class(vm, cname);
					if (!klass)
					{
						klass = xclass_create(vm, cname, vm->class_object);
					}
					xclass_add_method(klass, mname, (int)arity, closure_val.as.closureval);
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

		case OP_LOAD_FIELD_SLOT:
			{
				uint16_t slot = read_short(frame);
				XValue obj = xvm_pop(vm);
				if (obj.type == VAL_OBJECT && obj.as.oval != NULL)
				{
					XInstance* inst = (XInstance*)obj.as.oval;
					if (slot < inst->field_count)
					{
						xvm_push(vm, inst->fields[slot]);
						break;
					}
				}
				xvm_push(vm, xval_null());
				break;
			}

		case OP_STORE_FIELD_SLOT:
			{
				uint16_t slot = read_short(frame);
				XValue val = xvm_pop(vm);
				XValue obj = xvm_pop(vm);
				if (obj.type == VAL_OBJECT && obj.as.oval != NULL)
				{
					XInstance* inst = (XInstance*)obj.as.oval;
					if (slot < inst->field_count)
					{
						inst->fields[slot] = val;
						if (inst->klass && slot < inst->klass->field_count &&
						    inst->klass->fields[slot].name && strcmp(inst->klass->fields[slot].name, "id") == 0 &&
						    val.type == VAL_INT)
						{
							inst->id = (int)val.as.ival;
						}
					}
				}
				xvm_push(vm, val);
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
					const char* cname = xinstance_class_name(inst);
					if (strcmp(field_name, "id") == 0 ||
					    (strcmp(cname, "DateTime") == 0 && strcmp(field_name, "timestamp") == 0))
					{
						xvm_push(vm, xval_int(inst->id));
						break;
					}
					if (inst->klass)
					{
						int slot = xclass_find_field_slot(inst->klass, field_name);
						if (slot >= 0 && (uint32_t)slot < inst->field_count)
						{
							xvm_push(vm, inst->fields[slot]);
							break;
						}
					}
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
					const char* cname = xinstance_class_name(inst);
					if ((strcmp(field_name, "id") == 0 ||
					     (strcmp(cname, "DateTime") == 0 && strcmp(field_name, "timestamp") == 0)) &&
					    val.type == VAL_INT)
					{
						inst->id = (int)val.as.ival;
						xvm_push(vm, val);
						break;
					}
					if (inst->klass)
					{
						int slot = xclass_find_field_slot(inst->klass, field_name);
						if (slot >= 0 && (uint32_t)slot < inst->field_count)
						{
							inst->fields[slot] = val;
						}
					}
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
					const char* cname = xinstance_class_name(inst);
					if ((inst->klass == vm->class_list || strcmp(cname, "List") == 0) && idx_val.type == VAL_INT)
					{
						int idx = (int)idx_val.as.ival;
						int itype = x_list_item_type(inst->id, idx);
						if (itype == 1) xvm_push(vm, xval_int(x_list_item_int(inst->id, idx)));
						else if (itype == 2) xvm_push(vm, xval_float(x_list_item_float(inst->id, idx)));
						else xvm_push(vm, xval_str(x_list_item_str(inst->id, idx)));
						break;
					}
					else if ((inst->klass == vm->class_map || strcmp(cname, "Map") == 0 || strcmp(cname, "HashMap") == 0) && idx_val.type == VAL_STRING)
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
					const char* cname = xinstance_class_name(inst);
					if ((inst->klass == vm->class_list || strcmp(cname, "List") == 0) && idx_val.type == VAL_INT)
					{
						int idx = (int)idx_val.as.ival;
						if (val.type == VAL_INT) x_list_set_item_int(inst->id, idx, (int)val.as.ival);
						else if (val.type == VAL_FLOAT) x_list_set_item_float(inst->id, idx, (float)val.as.fval);
						else if (val.type == VAL_STRING) x_list_set_item_str(inst->id, idx, val.as.sval ? val.as.sval : "");
					}
					else if ((inst->klass == vm->class_map || strcmp(cname, "Map") == 0 || strcmp(cname, "HashMap") == 0) && idx_val.type == VAL_STRING)
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
				current_chunk->exec_count++;
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
					char arity_name[256];
					snprintf(arity_name, sizeof(arity_name), "%s#%d", name, arg_count);
					if (!xvm_get_global(vm, arity_name, &fn_val) && !xvm_get_global(vm, name, &fn_val))
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
							for (int i = 0; i < arg_count; i++)
							{
								xvalue_to_var(n_args[i], &fc.func_parmeters[i]);
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
										obj_id = type_instance_get_id(ti);
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
							if (fc._return.type_define != NULL && is_base_type(fc._return.type_define))
								xvm_push(vm, var_to_xvalue(&fc._return));
							else if (fc._return.type_define != NULL && !is_base_type(fc._return.type_define))
							{
								const char* tname = fc._return.type_define->type_name ? fc._return.type_define->type_name : "Object";
								int obj_id = 0;
								type_instance* ti = fc._return.value_type_instsance ? fc._return.value_type_instsance : (type_instance*)fc._return.values;
								if (ti != NULL)
								{
									obj_id = type_instance_get_id(ti);
								}
								XInstance* inst = xinstance_create_with_id(vm, tname, obj_id);
								if (ti != NULL && inst != NULL)
								{
									for (uint32_t f = 0; f < ti->field_count; f++)
									{
										var* fld = &ti->fields[f];
										int slot = -1;
										if (fld->name && inst->klass)
											slot = xclass_find_field_slot(inst->klass, fld->name);
										if (slot < 0 && f < inst->field_count)
											slot = (int)f;
										if (slot >= 0 && (uint32_t)slot < inst->field_count)
										{
											inst->fields[slot] = var_to_xvalue(fld);
										}
									}
								}
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

				if (callee_closure->function)
				{
					XFunction* fn = callee_closure->function;
					fn->call_count++;

					if (vm->jit_enabled && fn->jit_native_entry != NULL)
					{
						if (arg_count == 0)
						{
							int64_t (*f0)(void) = (int64_t (*)(void))fn->jit_native_entry;
							int64_t ret = f0();
							if (s_idx == 0xFFFF) xvm_pop(vm);
							xvm_push(vm, xval_int(ret));
							break;
						}
						else if (arg_count == 1)
						{
							XValue a0 = xvm_pop(vm);
							if (s_idx == 0xFFFF) xvm_pop(vm);
							int64_t v0 = (a0.type == VAL_FLOAT) ? (int64_t)a0.as.fval : ((a0.type == VAL_STRING) ? (intptr_t)a0.as.sval : a0.as.ival);
							int64_t (*f1)(int64_t) = (int64_t (*)(int64_t))fn->jit_native_entry;
							int64_t ret = f1(v0);
							xvm_push(vm, xval_int(ret));
							break;
						}
						else if (arg_count == 2)
						{
							XValue a1 = xvm_pop(vm);
							XValue a0 = xvm_pop(vm);
							if (s_idx == 0xFFFF) xvm_pop(vm);
							int64_t v0 = (a0.type == VAL_FLOAT) ? (int64_t)a0.as.fval : ((a0.type == VAL_STRING) ? (intptr_t)a0.as.sval : a0.as.ival);
							int64_t v1 = (a1.type == VAL_FLOAT) ? (int64_t)a1.as.fval : ((a1.type == VAL_STRING) ? (intptr_t)a1.as.sval : a1.as.ival);
							int64_t (*f2)(int64_t, int64_t) = (int64_t (*)(int64_t, int64_t))fn->jit_native_entry;
							int64_t ret = f2(v0, v1);
							xvm_push(vm, xval_int(ret));
							break;
						}
					}

					if (arg_count != fn->arity)
					{
						fprintf(stderr, "VM Runtime Error: Function '%s' expects %d arguments, but got %d\n",
						        fn->name ? fn->name : "fn",
						        fn->arity, arg_count);
						return VM_RUNTIME_ERROR;
					}
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
					const char* cname = xinstance_class_name(inst);
					if (inst->klass == vm->class_list || strcmp(cname, "List") == 0)
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
						else if (strcmp(method_name, "set") == 0 || strcmp(method_name, "set_int") == 0 || strcmp(method_name, "set_float") == 0)
						{
							int idx = (arg_count >= 1 && args[0].type == VAL_INT) ? (int)args[0].as.ival : 0;
							if (arg_count >= 2)
							{
								if (args[1].type == VAL_INT) x_list_set_item_int(inst->id, idx, (int)args[1].as.ival);
								else if (args[1].type == VAL_FLOAT) x_list_set_item_float(inst->id, idx, (float)args[1].as.fval);
								else x_list_set_item_str(inst->id, idx, args[1].as.sval ? args[1].as.sval : "");
							}
							xvm_push(vm, xval_int(1));
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
					else if (inst->klass == vm->class_map || strcmp(cname, "Map") == 0 || strcmp(cname, "HashMap") == 0)
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
					else if (inst->klass == vm->class_datetime || strcmp(cname, "DateTime") == 0)
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

					/* User / imported script class method: lookup method in klass vtable first */
					XClosure* cl = NULL;
					if (inst->klass)
					{
						cl = xclass_find_method(inst->klass, method_name, arg_count);
					}
						if (!cl)
						{
							/* Fallback: lookup "ClassName.methodName#arity" then "ClassName.methodName" */
							char user_mname[256];
							snprintf(user_mname, sizeof(user_mname), "%s.%s#%d", cname, method_name, arg_count);
							XValue mfn = xval_null();
							if (!xvm_get_global(vm, user_mname, &mfn))
							{
								snprintf(user_mname, sizeof(user_mname), "%s.%s", cname, method_name);
								xvm_get_global(vm, user_mname, &mfn);
							}
							if (mfn.type == VAL_CLOSURE) cl = mfn.as.closureval;
							else if (mfn.type == VAL_FUNCTION && mfn.as.fnval) cl = xclosure_create(vm, mfn.as.fnval);
						}

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
				else if (receiver.type == VAL_STRING)
				{
					if (strcmp(method_name, "length") == 0 || strcmp(method_name, "size") == 0 || strcmp(method_name, "len") == 0)
					{
						xvm_push(vm, xval_int(receiver.as.sval ? (int64_t)strlen(receiver.as.sval) : 0));
						break;
					}
					else if (strcmp(method_name, "get") == 0 && arg_count >= 1 && args[0].type == VAL_INT)
					{
						int idx = (int)args[0].as.ival;
						const char* s = receiver.as.sval ? receiver.as.sval : "";
						size_t slen = strlen(s);
						if (idx >= 0 && (size_t)idx < slen)
						{
							char ch[2] = { s[idx], '\0' };
							xvm_push(vm, xval_str(ch));
						}
						else
						{
							xvm_push(vm, xval_str(""));
						}
						break;
					}

					func_deftion* str_fn = NULL;
					for (int fi = 0; fi < 30; fi++)
					{
						if (T_STRING->d_functions[fi].func_name != NULL &&
						    strcmp(T_STRING->d_functions[fi].func_name, method_name) == 0)
						{
							str_fn = &T_STRING->d_functions[fi];
							break;
						}
					}
					if (str_fn != NULL && str_fn->func_code != NULL)
					{
						fcall fc;
						memset(&fc, 0, sizeof(fcall));
						fc.deftion = str_fn;
						fc.parm_count_c = arg_count;

						var ctx;
						memset(&ctx, 0, sizeof(var));
						ctx.type_define = T_STRING;
						char* s_ctx = (char*)(receiver.as.sval ? receiver.as.sval : "");
						ctx.value_str_ptr = &s_ctx;
						ctx.values = &s_ctx;
						fc.context = &ctx;

						for (int i = 0; i < arg_count; i++)
						{
							xvalue_to_var(args[i], &fc.func_parmeters[i]);
						}

						str_fn->func_code(&fc);

						if (fc._return.type_define != NULL && is_base_type(fc._return.type_define))
							xvm_push(vm, var_to_xvalue(&fc._return));
						else if (fc._return.type_define != NULL && !is_base_type(fc._return.type_define))
						{
							const char* tname = fc._return.type_define->type_name ? fc._return.type_define->type_name : "Object";
							int obj_id = 0;
							type_instance* ti = fc._return.value_type_instsance ? fc._return.value_type_instsance : (type_instance*)fc._return.values;
							if (ti != NULL)
							{
								obj_id = type_instance_get_id(ti);
							}
							else if (fc._return.value_int != NULL)
							{
								obj_id = *fc._return.value_int;
							}
							XInstance* inst = xinstance_create_with_id(vm, tname, obj_id);
							xvm_push(vm, xval_obj(inst));
						}
						else
							xvm_push(vm, xval_null());
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

XVmResult xvm_run(XVm* vm, XIrChunk* chunk)
{
	if (!vm || !chunk) return VM_RUNTIME_ERROR;
	XVm* prev_vm = g_current_vm;
	g_current_vm = vm;
	XVmResult res = xvm_run_loop(vm, chunk);
	g_current_vm = prev_vm;
	return res;
}
