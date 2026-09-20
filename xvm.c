#include "xvm.h"
#include <inttypes.h>

void xvm_init(XVm* vm)
{
	vm->chunk = NULL;
	vm->ip = NULL;
	vm->stack_top = vm->stack;
	vm->global_count = 0;
	vm->print_trace = false;
}

void xvm_free(XVm* vm)
{
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
}

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

static inline uint16_t read_short(XVm* vm)
{
	uint16_t val = (uint16_t)((vm->ip[0] << 8) | vm->ip[1]);
	vm->ip += 2;
	return val;
}

static inline int32_t read_int(XVm* vm)
{
	int32_t val = (int32_t)((vm->ip[0] << 24) | (vm->ip[1] << 16) | (vm->ip[2] << 8) | vm->ip[3]);
	vm->ip += 4;
	return val;
}

XVmResult xvm_run(XVm* vm, XIrChunk* chunk)
{
	vm->chunk = chunk;
	vm->ip = chunk->code;
	vm->stack_top = vm->stack;

	while (true)
	{
		if (vm->ip >= chunk->code + chunk->count)
		{
			break;
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
			xir_disassemble_instruction(chunk, (int)(vm->ip - chunk->code));
		}

		uint8_t instruction = *vm->ip++;

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
				int32_t val = read_int(vm);
				xvm_push(vm, xval_int(val));
				break;
			}

		case OP_CONST_FLOAT:
		case OP_CONST_STR:
			{
				uint16_t c_idx = read_short(vm);
				if (c_idx < chunk->constants.count)
				{
					xvm_push(vm, chunk->constants.values[c_idx]);
				}
				else
				{
					xvm_push(vm, xval_null());
				}
				break;
			}

		case OP_LOAD_GLOBAL:
			{
				uint16_t s_idx = read_short(vm);
				const char* name = (s_idx < chunk->symbols.count) ? chunk->symbols.symbols[s_idx] : "";
				XValue val = xval_null();
				if (!xvm_get_global(vm, name, &val))
				{
					/* Default to 0 / null */
					val = xval_null();
				}
				xvm_push(vm, val);
				break;
			}

		case OP_STORE_GLOBAL:
			{
				uint16_t s_idx = read_short(vm);
				const char* name = (s_idx < chunk->symbols.count) ? chunk->symbols.symbols[s_idx] : "";
				XValue val = xvm_pop(vm);
				xvm_set_global(vm, name, val);
				break;
			}

		case OP_LOAD_LOCAL:
			{
				uint16_t slot = read_short(vm);
				xvm_push(vm, vm->stack[slot]);
				break;
			}

		case OP_STORE_LOCAL:
			{
				uint16_t slot = read_short(vm);
				vm->stack[slot] = xvm_peek(vm, 0);
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
					double fb = (b.type == VAL_FLOAT) ? b.as.fval : (double)b.as.ival;
					if (fb == 0.0)
					{
						fprintf(stderr, "VM Runtime Error: Division by zero\n");
						return VM_RUNTIME_ERROR;
					}
					double fa = (a.type == VAL_FLOAT) ? a.as.fval : (double)a.as.ival;
					xvm_push(vm, xval_float(fa / fb));
				}
				else
				{
					if (b.as.ival == 0)
					{
						fprintf(stderr, "VM Runtime Error: Division by zero\n");
						return VM_RUNTIME_ERROR;
					}
					xvm_push(vm, xval_int(a.as.ival / b.as.ival));
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
					return VM_RUNTIME_ERROR;
				}
				xvm_push(vm, xval_int(a.as.ival % b.as.ival));
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

		case OP_BIT_NOT:
			{
				XValue a = xvm_pop(vm);
				xvm_push(vm, xval_int(~a.as.ival));
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
				uint16_t offset = read_short(vm);
				vm->ip = chunk->code + offset;
				break;
			}

		case OP_JUMP_IF_FALSE:
			{
				uint16_t offset = read_short(vm);
				XValue cond = xvm_peek(vm, 0);
				if (!xval_is_truthy(cond))
				{
					vm->ip = chunk->code + offset;
				}
				break;
			}

		case OP_JUMP_IF_TRUE:
			{
				uint16_t offset = read_short(vm);
				XValue cond = xvm_peek(vm, 0);
				if (xval_is_truthy(cond))
				{
					vm->ip = chunk->code + offset;
				}
				break;
			}

		case OP_LOOP:
			{
				uint16_t offset = read_short(vm);
				vm->ip = chunk->code + offset;
				break;
			}

		case OP_PRINT:
			{
				uint8_t arg_count = *vm->ip++;
				XValue args[32];
				for (int i = arg_count - 1; i >= 0; i--)
				{
					args[i] = xvm_pop(vm);
				}
				for (int i = 0; i < arg_count; i++)
				{
					xval_print(args[i]);
					if (i < arg_count - 1) printf(" ");
				}
				printf("\n");
				xvm_push(vm, xval_null());
				break;
			}

		case OP_HALT:
		case OP_RETURN:
			return VM_OK;

		default:
			fprintf(stderr, "Unknown opcode in VM: 0x%02x\n", instruction);
			return VM_RUNTIME_ERROR;
		}
	}

	return VM_OK;
}
