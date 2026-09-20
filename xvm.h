#ifndef XVM_H
#define XVM_H

#include "xir.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VM_FRAMES_MAX 256
#define VM_STACK_MAX (VM_FRAMES_MAX * 256)
#define VM_GLOBALS_MAX 512

typedef enum XVmResult {
	VM_OK = 0,
	VM_COMPILE_ERROR,
	VM_RUNTIME_ERROR
} XVmResult;

typedef struct XVmGlobal {
	char* name;
	XValue value;
} XVmGlobal;

typedef struct XUpvalue {
	XValue* location;
	XValue closed;
	struct XUpvalue* next;
	struct XUpvalue* all_next;
} XUpvalue;

struct XClosure {
	XFunction* function;
	XUpvalue** upvalues;
	int upvalue_count;
	struct XClosure* next;
};

struct XVm;
XClosure* xclosure_create(struct XVm* vm, XFunction* function);
void xclosure_free(XClosure* closure);

typedef struct XCallFrame {
	XClosure* closure;
	uint8_t* ip;
	XValue* slots;
	XValue* return_slot;
} XCallFrame;

typedef struct XVm {
	XCallFrame frames[VM_FRAMES_MAX];
	int frame_count;

	XValue stack[VM_STACK_MAX];
	XValue* stack_top;

	XUpvalue* open_upvalues;
	XUpvalue* all_upvalues;
	XClosure* all_closures;

	XVmGlobal globals[VM_GLOBALS_MAX];
	int global_count;

	bool print_trace;
} XVm;

void xvm_init(XVm* vm);
void xvm_free(XVm* vm);

XVmResult xvm_run(XVm* vm, XIrChunk* chunk);

/* Global variable management */
void xvm_set_global(XVm* vm, const char* name, XValue val);
bool xvm_get_global(XVm* vm, const char* name, XValue* out_val);

/* Stack utilities */
void xvm_push(XVm* vm, XValue val);
XValue xvm_pop(XVm* vm);
XValue xvm_peek(XVm* vm, int distance);

#ifdef __cplusplus
}
#endif

#endif /* XVM_H */
