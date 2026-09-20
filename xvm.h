#ifndef XVM_H
#define XVM_H

#include "xir.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VM_STACK_MAX 2048
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

typedef struct XVm {
	XIrChunk* chunk;
	uint8_t* ip;

	XValue stack[VM_STACK_MAX];
	XValue* stack_top;

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
