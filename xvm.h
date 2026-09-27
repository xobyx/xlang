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

typedef struct XMethod {
	char* name;
	int arity;
	struct XClosure* closure;
} XMethod;

typedef struct XFieldDesc {
	char* name;
	uint16_t slot_idx;
	char* type_name;
} XFieldDesc;

typedef struct XClass {
	char* name;
	struct XClass* base;
	uint32_t field_count;
	uint32_t field_capacity;
	XFieldDesc* fields;
	uint32_t method_count;
	uint32_t method_capacity;
	XMethod* methods;
	struct XClass* next;
} XClass;

typedef struct XInstance {
	XClass* klass;
	int id;
	struct XInstance* next;
	uint32_t field_count;
	XValue fields[];
} XInstance;

struct XVm;
XClosure* xclosure_create(struct XVm* vm, XFunction* function);
void xclosure_free(XClosure* closure);

XClass* xclass_create(struct XVm* vm, const char* name, XClass* base);
void xclass_add_field(XClass* klass, const char* name, const char* type_name);
int xclass_find_field_slot(const XClass* klass, const char* name);
void xclass_add_method(XClass* klass, const char* name, int arity, struct XClosure* closure);
struct XClosure* xclass_find_method(const XClass* klass, const char* name, int arity);
void xclass_free(XClass* klass);
XClass* xvm_find_class(struct XVm* vm, const char* name);

XInstance* xinstance_create_class(struct XVm* vm, XClass* klass, int id);
XInstance* xinstance_create(struct XVm* vm, const char* class_name);
XInstance* xinstance_create_with_id(struct XVm* vm, const char* class_name, int id);
void xinstance_free(XInstance* inst);

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
	XInstance* all_instances;
	XClass* all_classes;
	XClass* class_list;
	XClass* class_map;
	XClass* class_datetime;
	XClass* class_object;

	XVmGlobal globals[VM_GLOBALS_MAX];
	int global_count;

	bool print_trace;
} XVm;

void xvm_init(XVm* vm);
void xvm_free(XVm* vm);

XVmResult xvm_run(XVm* vm, XIrChunk* chunk);

extern XVm* g_current_vm;

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
