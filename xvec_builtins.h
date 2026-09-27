#ifndef XVEC_BUILTINS_H
#define XVEC_BUILTINS_H

#include "xvm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize and register all core Vectorcall built-in functions */
void xvec_builtins_init(void);
void xvm_register_vector_func(const char* name, XVectorFn fn, int arity);

/* Core VM-Aware Builtins */
XValue vec_assert(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_print(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_print_f(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_println(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_eval(XVm* vm, XValue receiver, int argc, const XValue* args);

/* Reflection Builtins */
XValue vec_typeof(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_type_name(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_has_field(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_get_field(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_set_field(XVm* vm, XValue receiver, int argc, const XValue* args);

/* System & GC Builtins */
XValue vec_clock_ms(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_gc_collect(XVm* vm, XValue receiver, int argc, const XValue* args);

#ifdef __cplusplus
}
#endif

#endif /* XVEC_BUILTINS_H */
