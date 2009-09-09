#ifndef _PURE_GNUMERIC_H
#define _PURE_GNUMERIC_H

/*
 * Interface to Gnumeric internal functions.
 */

#ifdef __cplusplus
extern "C" {
#endif
#include <pure/runtime.h>
#ifdef __cplusplus
}
#endif

#include <gnumeric.h>
#include <cell.h>
#include <expr.h>
#include <expr-impl.h>
#include <func.h>
#include <sheet.h>
#include <value.h>
#include <str.h>
#include <stdlib.h>
#include <string.h>

pure_expr* value2pure(GnmFuncEvalInfo *ei, const GnmValue *v);
GnmValue* pure2value(GnmFuncEvalInfo *ei, pure_expr *x);
GnmValue* call_pure_function(GnmFuncEvalInfo *ei, gint n_args,
			     GnmValue const * const *argv);

#endif /* _PURE_GNUMERIC_H */
