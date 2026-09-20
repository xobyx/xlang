#ifndef XIR_H
#define XIR_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Bytecode / IR Opcodes
 * ------------------------------------------------------------------------- */
typedef enum XIrOpCode {
	OP_NOP = 0,

	/* Constants */
	OP_CONST_NULL,
	OP_CONST_TRUE,
	OP_CONST_FALSE,
	OP_CONST_INT,     /* operand: 32-bit int */
	OP_CONST_FLOAT,   /* operand: uint16 constant pool index */
	OP_CONST_STR,     /* operand: uint16 constant pool index */

	/* Variables & Scopes */
	OP_LOAD_GLOBAL,   /* operand: uint16 symbol index */
	OP_STORE_GLOBAL,  /* operand: uint16 symbol index */
	OP_LOAD_LOCAL,    /* operand: uint16 local slot index */
	OP_STORE_LOCAL,   /* operand: uint16 local slot index */

	/* Members & Collections */
	OP_LOAD_FIELD,    /* operand: uint16 symbol index */
	OP_STORE_FIELD,   /* operand: uint16 symbol index */
	OP_LOAD_INDEX,
	OP_STORE_INDEX,
	OP_BUILD_LIST,    /* operand: uint16 element count */

	/* Stack Operations */
	OP_POP,
	OP_DUP,

	/* Arithmetic */
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_MOD,
	OP_NEG,

	/* Bitwise */
	OP_BIT_AND,
	OP_BIT_OR,
	OP_BIT_XOR,
	OP_BIT_NOT,
	OP_SHL,
	OP_SHR,

	/* Comparison & Logic */
	OP_EQ,
	OP_NEQ,
	OP_LT,
	OP_LTE,
	OP_GT,
	OP_GTE,
	OP_NOT,

	/* Control Flow (jumps use 16-bit relative or absolute offset) */
	OP_JUMP,          /* operand: uint16 target offset */
	OP_JUMP_IF_FALSE, /* operand: uint16 target offset */
	OP_JUMP_IF_TRUE,  /* operand: uint16 target offset */
	OP_LOOP,          /* operand: uint16 backward target offset */

	/* Invocations */
	OP_CALL,          /* operand: uint16 symbol index, uint8 arg_count */
	OP_CALL_METHOD,   /* operand: uint16 symbol index, uint8 arg_count */
	OP_PRINT,         /* operand: uint8 arg_count */
	OP_RETURN,

	/* Execution control */
	OP_HALT
} XIrOpCode;

const char* xir_opcode_name(XIrOpCode op);

/* -------------------------------------------------------------------------
 * Value Representation in IR Constant Pool & VM
 * ------------------------------------------------------------------------- */
typedef enum XValueType {
	VAL_NULL = 0,
	VAL_BOOL,
	VAL_INT,
	VAL_FLOAT,
	VAL_STRING,
	VAL_OBJECT
} XValueType;

typedef struct XValue {
	XValueType type;
	union {
		bool bval;
		int64_t ival;
		double fval;
		char* sval;
		void* oval;
	} as;
} XValue;

XValue xval_null(void);
XValue xval_bool(bool b);
XValue xval_int(int64_t i);
XValue xval_float(double f);
XValue xval_str(const char* s);
XValue xval_obj(void* o);

void xval_print(XValue v);
bool xval_is_truthy(XValue v);
bool xval_equal(XValue a, XValue b);

/* -------------------------------------------------------------------------
 * Constant Pool & Symbol Table
 * ------------------------------------------------------------------------- */
typedef struct XIrValuePool {
	XValue* values;
	int count;
	int capacity;
} XIrValuePool;

typedef struct XIrSymbolTable {
	char** symbols;
	int count;
	int capacity;
} XIrSymbolTable;

/* -------------------------------------------------------------------------
 * Bytecode Chunk
 * ------------------------------------------------------------------------- */
typedef struct XIrChunk {
	uint8_t* code;
	int count;
	int capacity;

	int* lines;

	XIrValuePool constants;
	XIrSymbolTable symbols;
} XIrChunk;

void xir_chunk_init(XIrChunk* chunk);
void xir_chunk_free(XIrChunk* chunk);

/* Emit primitives */
void xir_emit_byte(XIrChunk* chunk, uint8_t b, int line);
void xir_emit_op(XIrChunk* chunk, XIrOpCode op, int line);
void xir_emit_short(XIrChunk* chunk, uint16_t value, int line);
void xir_emit_int(XIrChunk* chunk, int32_t value, int line);

/* Jump patching */
int xir_emit_jump(XIrChunk* chunk, XIrOpCode jump_op, int line);
void xir_patch_jump(XIrChunk* chunk, int jump_offset);
void xir_emit_loop(XIrChunk* chunk, int loop_start, int line);

/* Pool additions (returns index) */
int xir_add_constant(XIrChunk* chunk, XValue val);
int xir_add_symbol(XIrChunk* chunk, const char* name);

/* -------------------------------------------------------------------------
 * Disassembler
 * ------------------------------------------------------------------------- */
void xir_disassemble_chunk(const XIrChunk* chunk, const char* name);
int xir_disassemble_instruction(const XIrChunk* chunk, int offset);

#ifdef __cplusplus
}
#endif

#endif /* XIR_H */
