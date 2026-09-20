#include "xir.h"
#include <inttypes.h>

/* -------------------------------------------------------------------------
 * Opcode Names
 * ------------------------------------------------------------------------- */
const char* xir_opcode_name(XIrOpCode op)
{
	switch (op)
	{
	case OP_NOP:          return "OP_NOP";
	case OP_CONST_NULL:   return "OP_CONST_NULL";
	case OP_CONST_TRUE:   return "OP_CONST_TRUE";
	case OP_CONST_FALSE:  return "OP_CONST_FALSE";
	case OP_CONST_INT:    return "OP_CONST_INT";
	case OP_CONST_FLOAT:  return "OP_CONST_FLOAT";
	case OP_CONST_STR:    return "OP_CONST_STR";
	case OP_LOAD_GLOBAL:  return "OP_LOAD_GLOBAL";
	case OP_STORE_GLOBAL: return "OP_STORE_GLOBAL";
	case OP_LOAD_LOCAL:   return "OP_LOAD_LOCAL";
	case OP_STORE_LOCAL:  return "OP_STORE_LOCAL";
	case OP_LOAD_FIELD:   return "OP_LOAD_FIELD";
	case OP_STORE_FIELD:  return "OP_STORE_FIELD";
	case OP_LOAD_INDEX:   return "OP_LOAD_INDEX";
	case OP_STORE_INDEX:  return "OP_STORE_INDEX";
	case OP_BUILD_LIST:   return "OP_BUILD_LIST";
	case OP_POP:          return "OP_POP";
	case OP_DUP:          return "OP_DUP";
	case OP_ADD:          return "OP_ADD";
	case OP_SUB:          return "OP_SUB";
	case OP_MUL:          return "OP_MUL";
	case OP_DIV:          return "OP_DIV";
	case OP_MOD:          return "OP_MOD";
	case OP_NEG:          return "OP_NEG";
	case OP_BIT_AND:      return "OP_BIT_AND";
	case OP_BIT_OR:       return "OP_BIT_OR";
	case OP_BIT_XOR:      return "OP_BIT_XOR";
	case OP_BIT_NOT:      return "OP_BIT_NOT";
	case OP_SHL:          return "OP_SHL";
	case OP_SHR:          return "OP_SHR";
	case OP_EQ:           return "OP_EQ";
	case OP_NEQ:          return "OP_NEQ";
	case OP_LT:           return "OP_LT";
	case OP_LTE:          return "OP_LTE";
	case OP_GT:           return "OP_GT";
	case OP_GTE:          return "OP_GTE";
	case OP_NOT:          return "OP_NOT";
	case OP_JUMP:         return "OP_JUMP";
	case OP_JUMP_IF_FALSE:return "OP_JUMP_IF_FALSE";
	case OP_JUMP_IF_TRUE: return "OP_JUMP_IF_TRUE";
	case OP_LOOP:         return "OP_LOOP";
	case OP_CALL:         return "OP_CALL";
	case OP_CALL_METHOD:  return "OP_CALL_METHOD";
	case OP_PRINT:        return "OP_PRINT";
	case OP_RETURN:       return "OP_RETURN";
	case OP_HALT:         return "OP_HALT";
	default:              return "OP_UNKNOWN";
	}
}

/* -------------------------------------------------------------------------
 * Value Construction & Helpers
 * ------------------------------------------------------------------------- */
XValue xval_null(void)
{
	XValue v;
	v.type = VAL_NULL;
	v.as.ival = 0;
	return v;
}

XValue xval_bool(bool b)
{
	XValue v;
	v.type = VAL_BOOL;
	v.as.bval = b;
	return v;
}

XValue xval_int(int64_t i)
{
	XValue v;
	v.type = VAL_INT;
	v.as.ival = i;
	return v;
}

XValue xval_float(double f)
{
	XValue v;
	v.type = VAL_FLOAT;
	v.as.fval = f;
	return v;
}

XValue xval_str(const char* s)
{
	XValue v;
	v.type = VAL_STRING;
	v.as.sval = s ? strdup(s) : strdup("");
	return v;
}

XValue xval_obj(void* o)
{
	XValue v;
	v.type = VAL_OBJECT;
	v.as.oval = o;
	return v;
}

void xval_print(XValue v)
{
	switch (v.type)
	{
	case VAL_NULL:   printf("null"); break;
	case VAL_BOOL:   printf("%s", v.as.bval ? "true" : "false"); break;
	case VAL_INT:    printf("%" PRId64, v.as.ival); break;
	case VAL_FLOAT:  printf("%g", v.as.fval); break;
	case VAL_STRING: printf("%s", v.as.sval ? v.as.sval : ""); break;
	case VAL_OBJECT: printf("[object %p]", v.as.oval); break;
	}
}

bool xval_is_truthy(XValue v)
{
	switch (v.type)
	{
	case VAL_NULL:   return false;
	case VAL_BOOL:   return v.as.bval;
	case VAL_INT:    return v.as.ival != 0;
	case VAL_FLOAT:  return v.as.fval != 0.0;
	case VAL_STRING: return v.as.sval && v.as.sval[0] != '\0';
	case VAL_OBJECT: return v.as.oval != NULL;
	default:         return false;
	}
}

bool xval_equal(XValue a, XValue b)
{
	if (a.type != b.type)
	{
		/* Allow int and float comparison */
		if (a.type == VAL_INT && b.type == VAL_FLOAT)
			return (double)a.as.ival == b.as.fval;
		if (a.type == VAL_FLOAT && b.type == VAL_INT)
			return a.as.fval == (double)b.as.ival;
		return false;
	}

	switch (a.type)
	{
	case VAL_NULL:   return true;
	case VAL_BOOL:   return a.as.bval == b.as.bval;
	case VAL_INT:    return a.as.ival == b.as.ival;
	case VAL_FLOAT:  return a.as.fval == b.as.fval;
	case VAL_STRING: return strcmp(a.as.sval ? a.as.sval : "", b.as.sval ? b.as.sval : "") == 0;
	case VAL_OBJECT: return a.as.oval == b.as.oval;
	default:         return false;
	}
}

/* -------------------------------------------------------------------------
 * Bytecode Chunk Implementation
 * ------------------------------------------------------------------------- */
void xir_chunk_init(XIrChunk* chunk)
{
	chunk->code = NULL;
	chunk->count = 0;
	chunk->capacity = 0;
	chunk->lines = NULL;

	chunk->constants.values = NULL;
	chunk->constants.count = 0;
	chunk->constants.capacity = 0;

	chunk->symbols.symbols = NULL;
	chunk->symbols.count = 0;
	chunk->symbols.capacity = 0;
}

void xir_chunk_free(XIrChunk* chunk)
{
	if (!chunk) return;
	if (chunk->code) free(chunk->code);
	if (chunk->lines) free(chunk->lines);

	for (int i = 0; i < chunk->constants.count; i++)
	{
		if (chunk->constants.values[i].type == VAL_STRING && chunk->constants.values[i].as.sval)
		{
			free(chunk->constants.values[i].as.sval);
		}
	}
	if (chunk->constants.values) free(chunk->constants.values);

	for (int i = 0; i < chunk->symbols.count; i++)
	{
		if (chunk->symbols.symbols[i]) free(chunk->symbols.symbols[i]);
	}
	if (chunk->symbols.symbols) free(chunk->symbols.symbols);

	xir_chunk_init(chunk);
}

void xir_emit_byte(XIrChunk* chunk, uint8_t b, int line)
{
	if (chunk->count + 1 > chunk->capacity)
	{
		int old_cap = chunk->capacity;
		chunk->capacity = old_cap < 64 ? 64 : old_cap * 2;
		chunk->code = (uint8_t*)realloc(chunk->code, chunk->capacity);
		chunk->lines = (int*)realloc(chunk->lines, chunk->capacity * sizeof(int));
	}
	chunk->code[chunk->count] = b;
	chunk->lines[chunk->count] = line;
	chunk->count++;
}

void xir_emit_op(XIrChunk* chunk, XIrOpCode op, int line)
{
	xir_emit_byte(chunk, (uint8_t)op, line);
}

void xir_emit_short(XIrChunk* chunk, uint16_t value, int line)
{
	xir_emit_byte(chunk, (uint8_t)((value >> 8) & 0xff), line);
	xir_emit_byte(chunk, (uint8_t)(value & 0xff), line);
}

void xir_emit_int(XIrChunk* chunk, int32_t value, int line)
{
	xir_emit_byte(chunk, (uint8_t)((value >> 24) & 0xff), line);
	xir_emit_byte(chunk, (uint8_t)((value >> 16) & 0xff), line);
	xir_emit_byte(chunk, (uint8_t)((value >> 8) & 0xff), line);
	xir_emit_byte(chunk, (uint8_t)(value & 0xff), line);
}

int xir_emit_jump(XIrChunk* chunk, XIrOpCode jump_op, int line)
{
	xir_emit_op(chunk, jump_op, line);
	xir_emit_byte(chunk, 0xff, line);
	xir_emit_byte(chunk, 0xff, line);
	return chunk->count - 2;
}

void xir_patch_jump(XIrChunk* chunk, int offset)
{
	/* Jump to current end of bytecode */
	uint16_t target = (uint16_t)chunk->count;
	chunk->code[offset] = (uint8_t)((target >> 8) & 0xff);
	chunk->code[offset + 1] = (uint8_t)(target & 0xff);
}

void xir_emit_loop(XIrChunk* chunk, int loop_start, int line)
{
	xir_emit_op(chunk, OP_LOOP, line);
	uint16_t target = (uint16_t)loop_start;
	xir_emit_short(chunk, target, line);
}

int xir_add_constant(XIrChunk* chunk, XValue val)
{
	XIrValuePool* p = &chunk->constants;
	if (p->count + 1 > p->capacity)
	{
		int old_cap = p->capacity;
		p->capacity = old_cap < 16 ? 16 : old_cap * 2;
		p->values = (XValue*)realloc(p->values, p->capacity * sizeof(XValue));
	}
	p->values[p->count] = val;
	return p->count++;
}

int xir_add_symbol(XIrChunk* chunk, const char* name)
{
	if (!name) name = "";
	XIrSymbolTable* t = &chunk->symbols;
	for (int i = 0; i < t->count; i++)
	{
		if (strcmp(t->symbols[i], name) == 0)
			return i;
	}

	if (t->count + 1 > t->capacity)
	{
		int old_cap = t->capacity;
		t->capacity = old_cap < 16 ? 16 : old_cap * 2;
		t->symbols = (char**)realloc(t->symbols, t->capacity * sizeof(char*));
	}
	t->symbols[t->count] = strdup(name);
	return t->count++;
}

/* -------------------------------------------------------------------------
 * Disassembler Implementation
 * ------------------------------------------------------------------------- */
void xir_disassemble_chunk(const XIrChunk* chunk, const char* name)
{
	printf("\n\033[1;36m=== Bytecode Disassembly: %s (%d bytes) ===\033[0m\n", name ? name : "chunk", chunk->count);
	printf("\033[1mOffset  Line  Opcode               Operands\033[0m\n");
	printf("----------------------------------------------------\n");

	for (int offset = 0; offset < chunk->count;)
	{
		offset = xir_disassemble_instruction(chunk, offset);
	}
	printf("\033[1;36m====================================================\033[0m\n\n");
}

int xir_disassemble_instruction(const XIrChunk* chunk, int offset)
{
	printf("%04d    ", offset);

	if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1])
	{
		printf("   |  ");
	}
	else
	{
		printf("%4d  ", chunk->lines[offset]);
	}

	uint8_t opcode = chunk->code[offset];
	printf("%-18s ", xir_opcode_name((XIrOpCode)opcode));

	switch (opcode)
	{
	case OP_CONST_NULL:
	case OP_CONST_TRUE:
	case OP_CONST_FALSE:
	case OP_POP:
	case OP_DUP:
	case OP_ADD:
	case OP_SUB:
	case OP_MUL:
	case OP_DIV:
	case OP_MOD:
	case OP_NEG:
	case OP_BIT_AND:
	case OP_BIT_OR:
	case OP_BIT_XOR:
	case OP_BIT_NOT:
	case OP_SHL:
	case OP_SHR:
	case OP_EQ:
	case OP_NEQ:
	case OP_LT:
	case OP_LTE:
	case OP_GT:
	case OP_GTE:
	case OP_NOT:
	case OP_LOAD_INDEX:
	case OP_STORE_INDEX:
	case OP_RETURN:
	case OP_HALT:
	case OP_NOP:
		printf("\n");
		return offset + 1;

	case OP_CONST_INT:
		{
			int32_t val = (chunk->code[offset + 1] << 24) |
			              (chunk->code[offset + 2] << 16) |
			              (chunk->code[offset + 3] << 8)  |
			              (chunk->code[offset + 4]);
			printf("\033[33m%d\033[0m\n", val);
			return offset + 5;
		}

	case OP_CONST_FLOAT:
	case OP_CONST_STR:
		{
			uint16_t idx = (chunk->code[offset + 1] << 8) | chunk->code[offset + 2];
			printf("[%d] ", idx);
			if (idx < chunk->constants.count)
			{
				printf("'");
				xval_print(chunk->constants.values[idx]);
				printf("'");
			}
			printf("\n");
			return offset + 3;
		}

	case OP_LOAD_GLOBAL:
	case OP_STORE_GLOBAL:
	case OP_LOAD_FIELD:
	case OP_STORE_FIELD:
		{
			uint16_t s_idx = (chunk->code[offset + 1] << 8) | chunk->code[offset + 2];
			const char* sym = (s_idx < chunk->symbols.count) ? chunk->symbols.symbols[s_idx] : "?";
			printf("[%d] ('\033[36m%s\033[0m')\n", s_idx, sym);
			return offset + 3;
		}

	case OP_LOAD_LOCAL:
	case OP_STORE_LOCAL:
		{
			uint16_t slot = (chunk->code[offset + 1] << 8) | chunk->code[offset + 2];
			printf("slot \033[32m%d\033[0m\n", slot);
			return offset + 3;
		}

	case OP_BUILD_LIST:
		{
			uint16_t count = (chunk->code[offset + 1] << 8) | chunk->code[offset + 2];
			printf("%d items\n", count);
			return offset + 3;
		}

	case OP_JUMP:
	case OP_JUMP_IF_FALSE:
	case OP_JUMP_IF_TRUE:
	case OP_LOOP:
		{
			uint16_t target = (chunk->code[offset + 1] << 8) | chunk->code[offset + 2];
			printf("-> \033[1;33m%04d\033[0m\n", target);
			return offset + 3;
		}

	case OP_CALL:
		{
			uint16_t s_idx = (chunk->code[offset + 1] << 8) | chunk->code[offset + 2];
			uint8_t args = chunk->code[offset + 3];
			const char* sym = (s_idx < chunk->symbols.count) ? chunk->symbols.symbols[s_idx] : "?";
			printf("[%d] ('\033[32m%s\033[0m', %d args)\n", s_idx, sym, args);
			return offset + 4;
		}

	case OP_CALL_METHOD:
		{
			uint16_t s_idx = (chunk->code[offset + 1] << 8) | chunk->code[offset + 2];
			uint8_t args = chunk->code[offset + 3];
			const char* sym = (s_idx < chunk->symbols.count) ? chunk->symbols.symbols[s_idx] : "?";
			printf(".%s (%d args)\n", sym, args);
			return offset + 4;
		}

	case OP_PRINT:
		{
			uint8_t args = chunk->code[offset + 1];
			printf("%d args\n", args);
			return offset + 2;
		}

	default:
		printf("Unknown opcode 0x%02x\n", opcode);
		return offset + 1;
	}
}
