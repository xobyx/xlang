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
	case OP_CLOSURE:       return "OP_CLOSURE";
	case OP_GET_UPVALUE:   return "OP_GET_UPVALUE";
	case OP_SET_UPVALUE:   return "OP_SET_UPVALUE";
	case OP_CLOSE_UPVALUE: return "OP_CLOSE_UPVALUE";
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

XValue xval_func(XFunction* fn)
{
	XValue v;
	v.type = VAL_FUNCTION;
	v.as.fnval = fn;
	return v;
}

XFunction* xfunc_create(const char* name, int arity)
{
	XFunction* fn = (XFunction*)malloc(sizeof(XFunction));
	fn->name = name ? strdup(name) : strdup("fn");
	fn->arity = arity;
	fn->upvalue_count = 0;
	xir_chunk_init(&fn->chunk);
	return fn;
}

void xfunc_free(XFunction* fn)
{
	if (!fn) return;
	if (fn->name) free(fn->name);
	xir_chunk_free(&fn->chunk);
	free(fn);
}

XValue xval_closure(XClosure* c)
{
	XValue v;
	v.type = VAL_CLOSURE;
	v.as.closureval = c;
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
	case VAL_FUNCTION:
		if (v.as.fnval)
			printf("<fn %s/%d>", v.as.fnval->name ? v.as.fnval->name : "fn", v.as.fnval->arity);
		else
			printf("<fn null>");
		break;
	case VAL_CLOSURE:
		printf("<closure %p>", (void*)v.as.closureval);
		break;
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
	case VAL_FUNCTION: return true;
	case VAL_CLOSURE: return true;
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
	case VAL_FUNCTION: return a.as.fnval == b.as.fnval;
	case VAL_CLOSURE: return a.as.closureval == b.as.closureval;
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
		else if (chunk->constants.values[i].type == VAL_FUNCTION && chunk->constants.values[i].as.fnval)
		{
			xfunc_free(chunk->constants.values[i].as.fnval);
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

	/* Disassemble any nested functions in the constant pool */
	for (int i = 0; i < chunk->constants.count; i++)
	{
		if (chunk->constants.values[i].type == VAL_FUNCTION && chunk->constants.values[i].as.fnval)
		{
			char sub_name[256];
			snprintf(sub_name, sizeof(sub_name), "%s::<fn %s>", name ? name : "chunk", chunk->constants.values[i].as.fnval->name);
			xir_disassemble_chunk(&chunk->constants.values[i].as.fnval->chunk, sub_name);
		}
	}
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

	case OP_CLOSURE:
		{
			uint16_t c_idx = (chunk->code[offset + 1] << 8) | chunk->code[offset + 2];
			printf("[%d] ", c_idx);
			if (c_idx < chunk->constants.count && chunk->constants.values[c_idx].type == VAL_FUNCTION)
			{
				XFunction* fn = chunk->constants.values[c_idx].as.fnval;
				printf("<fn \033[35m%s\033[0m arity=%d upvalues=%d>\n", fn->name, fn->arity, fn->upvalue_count);
				int cur = offset + 3;
				for (int j = 0; j < fn->upvalue_count; j++)
				{
					uint8_t is_local = chunk->code[cur++];
					uint8_t index = chunk->code[cur++];
					printf("      |                     %s %d\n", is_local ? "local" : "upvalue", index);
				}
				return cur;
			}
			printf("\n");
			return offset + 3;
		}

	case OP_GET_UPVALUE:
	case OP_SET_UPVALUE:
		{
			uint8_t slot = chunk->code[offset + 1];
			printf("upvalue \033[33m%d\033[0m\n", slot);
			return offset + 2;
		}

	case OP_CLOSE_UPVALUE:
		printf("\n");
		return offset + 1;

	default:
		printf("Unknown opcode 0x%02x\n", opcode);
		return offset + 1;
	}
}

/* -------------------------------------------------------------------------
 * Bytecode File Serialization (.xbc)
 * ------------------------------------------------------------------------- */
static void write_u8(FILE* f, uint8_t v) { fwrite(&v, 1, 1, f); }
static void write_u32(FILE* f, uint32_t v) { fwrite(&v, 4, 1, f); }
static void write_i64(FILE* f, int64_t v) { fwrite(&v, 8, 1, f); }
static void write_double(FILE* f, double v) { fwrite(&v, 8, 1, f); }
static void write_str(FILE* f, const char* s)
{
	uint32_t len = s ? (uint32_t)strlen(s) : 0;
	write_u32(f, len);
	if (len > 0) fwrite(s, 1, len, f);
}

static bool read_u8(FILE* f, uint8_t* out) { return fread(out, 1, 1, f) == 1; }
static bool read_u32(FILE* f, uint32_t* out) { return fread(out, 4, 1, f) == 1; }
static bool read_i64(FILE* f, int64_t* out) { return fread(out, 8, 1, f) == 1; }
static bool read_double(FILE* f, double* out) { return fread(out, 8, 1, f) == 1; }
static char* read_str(FILE* f)
{
	uint32_t len = 0;
	if (!read_u32(f, &len)) return NULL;
	char* s = (char*)malloc(len + 1);
	if (!s) return NULL;
	if (len > 0)
	{
		if (fread(s, 1, len, f) != len) { free(s); return NULL; }
	}
	s[len] = '\0';
	return s;
}

bool xir_serialize_chunk(const XIrChunk* chunk, FILE* f)
{
	if (!chunk || !f) return false;

	/* 1. Code buffer */
	write_u32(f, (uint32_t)chunk->count);
	if (chunk->count > 0)
	{
		fwrite(chunk->code, 1, chunk->count, f);
		for (int i = 0; i < chunk->count; i++)
		{
			write_u32(f, (uint32_t)chunk->lines[i]);
		}
	}

	/* 2. Constants */
	write_u32(f, (uint32_t)chunk->constants.count);
	for (int i = 0; i < chunk->constants.count; i++)
	{
		XValue val = chunk->constants.values[i];
		write_u8(f, (uint8_t)val.type);
		switch (val.type)
		{
		case VAL_NULL: break;
		case VAL_BOOL: write_u8(f, val.as.bval ? 1 : 0); break;
		case VAL_INT: write_i64(f, val.as.ival); break;
		case VAL_FLOAT: write_double(f, val.as.fval); break;
		case VAL_STRING: write_str(f, val.as.sval); break;
		case VAL_FUNCTION:
			if (val.as.fnval)
			{
				write_str(f, val.as.fnval->name);
				write_u32(f, (uint32_t)val.as.fnval->arity);
				write_u32(f, (uint32_t)val.as.fnval->upvalue_count);
				if (!xir_serialize_chunk(&val.as.fnval->chunk, f)) return false;
			}
			else
			{
				write_str(f, "");
				write_u32(f, 0);
				write_u32(f, 0);
				XIrChunk empty;
				xir_chunk_init(&empty);
				xir_serialize_chunk(&empty, f);
			}
			break;
		default:
			write_u8(f, 0);
			break;
		}
	}

	/* 3. Symbols */
	write_u32(f, (uint32_t)chunk->symbols.count);
	for (int i = 0; i < chunk->symbols.count; i++)
	{
		write_str(f, chunk->symbols.symbols[i]);
	}

	return true;
}

bool xir_deserialize_chunk(XIrChunk* chunk, FILE* f)
{
	if (!chunk || !f) return false;
	xir_chunk_init(chunk);

	/* 1. Code buffer */
	uint32_t code_count = 0;
	if (!read_u32(f, &code_count)) return false;
	chunk->count = (int)code_count;
	chunk->capacity = chunk->count > 0 ? chunk->count : 1;
	chunk->code = (uint8_t*)malloc(chunk->capacity);
	chunk->lines = (int*)malloc(chunk->capacity * sizeof(int));
	if (code_count > 0)
	{
		if (fread(chunk->code, 1, code_count, f) != code_count) return false;
		for (uint32_t i = 0; i < code_count; i++)
		{
			uint32_t line = 0;
			if (!read_u32(f, &line)) return false;
			chunk->lines[i] = (int)line;
		}
	}

	/* 2. Constants */
	uint32_t const_count = 0;
	if (!read_u32(f, &const_count)) return false;
	for (uint32_t i = 0; i < const_count; i++)
	{
		uint8_t type_byte = 0;
		if (!read_u8(f, &type_byte)) return false;
		XValue val = xval_null();
		switch ((XValueType)type_byte)
		{
		case VAL_NULL: val = xval_null(); break;
		case VAL_BOOL:
			{
				uint8_t b = 0;
				if (!read_u8(f, &b)) return false;
				val = xval_bool(b != 0);
				break;
			}
		case VAL_INT:
			{
				int64_t iv = 0;
				if (!read_i64(f, &iv)) return false;
				val = xval_int(iv);
				break;
			}
		case VAL_FLOAT:
			{
				double fv = 0.0;
				if (!read_double(f, &fv)) return false;
				val = xval_float(fv);
				break;
			}
		case VAL_STRING:
			{
				char* s = read_str(f);
				if (!s) return false;
				val = xval_str(s);
				free(s);
				break;
			}
		case VAL_FUNCTION:
			{
				char* fname = read_str(f);
				if (!fname) return false;
				uint32_t arity = 0, upvalues = 0;
				if (!read_u32(f, &arity) || !read_u32(f, &upvalues)) { free(fname); return false; }
				XFunction* fn = xfunc_create(fname, (int)arity);
				fn->upvalue_count = (int)upvalues;
				free(fname);
				if (!xir_deserialize_chunk(&fn->chunk, f)) { xfunc_free(fn); return false; }
				val = xval_func(fn);
				break;
			}
		default:
			val = xval_null();
			break;
		}
		xir_add_constant(chunk, val);
	}

	/* 3. Symbols */
	uint32_t sym_count = 0;
	if (!read_u32(f, &sym_count)) return false;
	for (uint32_t i = 0; i < sym_count; i++)
	{
		char* sym = read_str(f);
		if (!sym) return false;
		xir_add_symbol(chunk, sym);
		free(sym);
	}

	return true;
}

bool xir_save_file(const XIrChunk* chunk, const char* filepath)
{
	if (!chunk || !filepath) return false;
	FILE* f = fopen(filepath, "wb");
	if (!f) return false;

	uint32_t magic = XBC_MAGIC;
	uint32_t version = XBC_VERSION;
	write_u32(f, magic);
	write_u32(f, version);
	write_u32(f, 0); /* flags */
	write_u32(f, 0); /* reserved */

	bool ok = xir_serialize_chunk(chunk, f);
	fclose(f);
	return ok;
}

bool xir_load_file(XIrChunk* chunk, const char* filepath)
{
	if (!chunk || !filepath) return false;
	FILE* f = fopen(filepath, "rb");
	if (!f) return false;

	uint32_t magic = 0, version = 0, r1 = 0, r2 = 0;
	if (!read_u32(f, &magic) || magic != XBC_MAGIC) { fclose(f); return false; }
	if (!read_u32(f, &version) || version != XBC_VERSION) { fclose(f); return false; }
	read_u32(f, &r1);
	read_u32(f, &r2);

	bool ok = xir_deserialize_chunk(chunk, f);
	fclose(f);
	return ok;
}
