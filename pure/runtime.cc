// vim: set shiftwidth=2 softtabstop=2:

/* AIX requires this to be the first thing in the file.  */
#ifndef __GNUC__
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
#pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
char *alloca ();
#   endif
#  endif
# endif
#endif
#ifdef __MINGW32__
#include <malloc.h>
#endif

#include "runtime.h"
#include "expr.hh"
#include "interpreter.hh"
#include "util.hh"
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <iostream>
#include <sstream>

#include "config.h"
#include "funcall.h"

#ifdef HAVE_GSL
#include <gsl/gsl_errno.h>
#include <gsl/gsl_matrix.h>
#endif

#include <complex> //only the type is used, not any methods
typedef std::complex<double> Complex;

/* Implement the GSL-like operations on symbolic matrices that we need. */

static gsl_matrix_symbolic* 
gsl_matrix_symbolic_alloc(const size_t n1, const size_t n2)
{
  gsl_block_symbolic* block;
  gsl_matrix_symbolic* m;
  if (n1 == 0 || n2 == 0)
    return 0;
  m = (gsl_matrix_symbolic*)malloc(sizeof(gsl_matrix_symbolic));
  if (m == 0)
    return 0;
  block = (gsl_block_symbolic*)malloc(sizeof(gsl_block_symbolic));
  if (block == 0) {
    free(m);
    return 0;
  }
  block->size = n1*n2;
  block->data = (pure_expr**)malloc(block->size*sizeof(pure_expr*)) ;
  if (block->data == 0) {
    free(m);
    free(block);
    return 0;
  }
  m->data = block->data;
  m->size1 = n1;
  m->size2 = n2;
  m->tda = n2; 
  m->block = block;
  m->owner = 1;
  return m;
}

static gsl_matrix_symbolic*
gsl_matrix_symbolic_calloc(const size_t n1, const size_t n2)
{
  gsl_matrix_symbolic* m = gsl_matrix_symbolic_alloc(n1, n2);
  if (m == 0) return 0;
  memset(m->data, 0, m->block->size*sizeof(pure_expr*));
  return m;
}

static void gsl_matrix_symbolic_free(gsl_matrix_symbolic *m)
{
  if (m->owner) free(m->block);
  free(m);
}

static gsl_matrix_symbolic_view
gsl_matrix_symbolic_submatrix(gsl_matrix_symbolic *m, 
			      const size_t i, const size_t j, 
			      const size_t n1, const size_t n2)
{
  gsl_matrix_symbolic_view view = {{0, 0, 0, 0, 0, 0}}; 
  if (i >= m->size1 || j >= m->size2 || n1 == 0 || n2 == 0 ||
      i + n1 > m->size1 || j + n2 > m->size2)
    return view;
  else {
     gsl_matrix_symbolic s = {0, 0, 0, 0, 0, 0};
     s.data = m->data + i * m->tda + j;
     s.size1 = n1;
     s.size2 = n2;
     s.tda = m->tda;
     s.block = m->block;
     s.owner = 0;
     view.matrix = s;
     return view;
  }
}

static int
gsl_matrix_symbolic_memcpy(gsl_matrix_symbolic *dest,
			   const gsl_matrix_symbolic *src)
{
  const size_t src_size1 = src->size1;
  const size_t src_size2 = src->size2;
  const size_t dest_size1 = dest->size1;
  const size_t dest_size2 = dest->size2;
  if (src_size1 != dest_size1 || src_size2 != dest_size2)
    return -1;
  else {
    const size_t src_tda = src->tda ;
    const size_t dest_tda = dest->tda ;
    size_t i;
    for (i = 0; i < src_size1 ; i++)
      memcpy(dest->data+dest_tda*i, src->data+src_tda*i,
	     src_size2*sizeof(pure_expr*));
    return 0;
  }
}

// Hooks to report stack overflows and other kinds of hard errors.
#define checkstk(test) if (interpreter::stackmax > 0 &&			\
      interpreter::stackdir*(&test - interpreter::baseptr)		\
      >= interpreter::stackmax)						\
    pure_throw(stack_exception())
#define checkall(test) if (interpreter::stackmax > 0 &&			\
      interpreter::stackdir*(&test - interpreter::baseptr)		\
      >= interpreter::stackmax) {					\
    interpreter::brkmask = 0;						\
    pure_throw(stack_exception());					\
  } else if (interpreter::brkmask)					\
    interpreter::brkmask = 0;						\
  else if (interpreter::brkflag)					\
    pure_throw(signal_exception(interpreter::brkflag))

// Debug expression allocations. Warns about expression memory leaks.
// NOTE: Bookkeeping starts and ends at each toplevel pure_invoke call.
// Enabling this code will make the interpreter *much* slower.
#if MEMDEBUG
#if MEMDEBUG>1 // enable this to print each and every expression (de)allocation
#define MEMDEBUG_NEW(x)  interpreter::g_interp->mem_allocations.insert(x); \
  cerr << "NEW:  " << (void*)x << ": " << x << endl;
#define MEMDEBUG_FREE(x) interpreter::g_interp->mem_allocations.erase(x); \
  cerr << "FREE: " << (void*)x << ": " << x << endl;
#else
#define MEMDEBUG_NEW(x)  interpreter::g_interp->mem_allocations.insert(x);
#define MEMDEBUG_FREE(x) interpreter::g_interp->mem_allocations.erase(x);
#endif
#define MEMDEBUG_INIT if (interpreter::g_interp->estk.empty())	\
    interpreter::g_interp->mem_allocations.clear();
#define MEMDEBUG_SUMMARY(ret) if (interpreter::g_interp->estk.empty()) {\
    mem_mark(ret);							\
    if (!interpreter::g_interp->mem_allocations.empty()) {		\
      cerr << "** WARNING: leaked expressions:\n";			\
      for (set<pure_expr*>::iterator x =				\
	     interpreter::g_interp->mem_allocations.begin();		\
	   x != interpreter::g_interp->mem_allocations.end(); x++)	\
	cerr << (void*)(*x) << " (refc = " << (*x)->refc << "): "	\
	     << (*x) << endl;						\
      interpreter::g_interp->mem_allocations.clear();			\
    }									\
  }
static void mem_mark(pure_expr *x)
{
  interpreter::g_interp->mem_allocations.erase(x);
  if (x->tag == EXPR::APP) {
    mem_mark(x->data.x[0]);
    mem_mark(x->data.x[1]);
  } else if (x->tag >= 0 && x->data.clos) {
    for (size_t i = 0; i < x->data.clos->m; i++)
      mem_mark(x->data.clos->env[i]);
  }
}
#else
#define MEMDEBUG_NEW(x)
#define MEMDEBUG_FREE(x)
#define MEMDEBUG_INIT
#define MEMDEBUG_SUMMARY(ret)
#endif

// Debug shadow stack manipulations. Prints pushes and pops of stack frames.
// NOTE: Enabling this code generates *lots* of debugging output.
#if DEBUG>2
#define SSTK_DEBUG 1
#else
#define SSTK_DEBUG 0
#endif

static inline pure_expr* pure_apply2(pure_expr *x, pure_expr *y)
{
  // Count references and construct a function application.
  pure_new_args(2, x, y);
  return pure_apply(x, y);
}

static inline pure_expr* signal_exception(int sig)
{
  if (!interpreter::g_interp) return 0;
  pure_expr *f = pure_const(interpreter::g_interp->symtab.signal_sym().f);
  pure_expr *x = pure_int(sig);
  return pure_apply2(f, x);
}

static inline pure_expr* stack_exception()
{
  if (!interpreter::g_interp) return 0;
  return pure_const(interpreter::g_interp->symtab.segfault_sym().f);
}

static inline pure_expr* bad_matrix_exception(pure_expr *x)
{
  if (!interpreter::g_interp) return 0;
  pure_expr *f = pure_const(interpreter::g_interp->symtab.bad_matrix_sym().f);
  if (x)
    return pure_apply2(f, x);
  else
    return f;
}

static inline pure_expr *get_sentry(pure_expr *x)
{
  if (x==0)
    return 0;
  else if (x->tag == EXPR::APP || x->tag == EXPR::PTR)
    return x->data.x[2];
  else
    return 0;
}

static inline void call_sentry(pure_expr *x)
{
  if (x->tag == EXPR::APP || x->tag == EXPR::PTR) {
    pure_expr *s = x->data.x[2];
    if (s) {
      ++x->refc;
      pure_freenew(pure_apply2(s, x));
      --x->refc;
    }
  }
}

static inline void free_sentry(pure_expr *x)
{
  if (x->tag == EXPR::APP || x->tag == EXPR::PTR) {
    pure_expr *s = x->data.x[2];
    if (s) pure_free(s);
  }
}

// Expression pointers are allocated in larger chunks for better performance.
// NOTE: Only internal fields get initialized by new_expr(), the remaining
// fields *must* be initialized as appropriate by the caller.

static inline pure_expr *new_expr()
{
  interpreter& interp = *interpreter::g_interp;
  pure_expr *x = interp.exps;
  if (x)
    interp.exps = x->xp;
  else if (interp.mem && interp.mem->p-interp.mem->x < MEMSIZE)
    x = interp.mem->p++;
  else {
    pure_mem *mem = interp.mem;
    interp.mem = new pure_mem;
    interp.mem->next = mem;
    interp.mem->p = interp.mem->x;
    x = interp.mem->p++;
  }
  x->refc = 0;
  x->xp = interp.tmps;
  x->data.x[2] = 0; // initialize the sentry
  interp.tmps = x;
  return x;
}

static inline void free_expr(pure_expr *x)
{
  interpreter& interp = *interpreter::g_interp;
  x->xp = interp.exps;
  interp.exps = x;
  MEMDEBUG_FREE(x)
}

static inline
pure_expr *pure_new_internal(pure_expr *x)
{
  interpreter& interp = *interpreter::g_interp;
  assert(x && "pure_new: null expression");
  assert((x->refc==0 || !x->xp) && "pure_new: corrupt expression data");
#if DEBUG>2
  if (x->tag >= 0 && x->data.clos)
    cerr << "pure_new: " << (x->data.clos->local?"local":"global")
	 << " closure " << x << " (" << (void*)x << "), refc = "
	 << x->refc << endl;
#endif
  if (x->refc++ == 0) {
    // remove x from the list of temporaries
    if (interp.tmps == x)
      interp.tmps = x->xp;
    else {
      // walk the list to find the place where x has to be unlinked
      pure_expr *tmps = interp.tmps;
      while (tmps && tmps->xp != x) tmps = tmps->xp;
      assert(tmps);
      tmps->xp = x->xp;
    }
    x->xp = 0;
  }
  return x;
}

/* Mass updates of refcounts. We need to optimize this case to avoid quadratic
   complexity due to repeated searches of the tmps list. This prevents some
   serious performance issues, in particular in symbolic matrix primitives.
   Thanks to Scott E. Dillard for pointing this out. */

/* To make this fast, we currently employ a rather simple heuristic which only
   optimizes the most common cases (however, these seem to hold in most
   primitives where it matters):

   - The vector elements have a nonzero reference count already (as determined
     by looking at the first element). In this case updates don't refer to the
     tmps list and will be fast anyway (no optimization needed).

   - The *last* vector element is near the beginning of the tmps list (as
     determined by looking at the first three elements of the tmps list).
     This indicates that the elements have been created in order and should
     be updated in reverse.

   - The *first* vector element is near the beginning of the tmps list (as
     determined by looking at the first three elements of the tmps list).
     This indicates that the elements have been created in reverse and should
     be updated in order.

   If a primitive falls outside this scheme then it may still suffer from
   quadratic complexity. In such a case it is best if you create the elements
   with a nonzero reference count instead, and decrement them later. */

static inline short peek(pure_expr *const tmps,
			 pure_expr *const first, pure_expr *const last)
{
  pure_expr *t = tmps;
  short count = 0;
  while (t && count < 3)
    if (t == last)
      return -1;
    else if (t == first)
      return 1;
    else {
      t = t->xp; count++;
    }
  return 0;
}

static inline void pure_new_vect(const size_t n, pure_expr **xs)
{
  short rc;
  if (n == 0)
    return;
  else if (xs[0]->refc > 0)
    rc = 0;
  else {
    interpreter& interp = *interpreter::g_interp;
    pure_expr *tmps = interp.tmps;
    pure_expr *first = xs[0], *last = xs[n-1];
    rc = peek(tmps, first, last);
  }
  if (rc <= 0)
    for (size_t i = n; i > 0; )
      pure_new_internal(xs[--i]);
  else
    for (size_t i = 0; i < n; i++)
      pure_new_internal(xs[i]);
}

/* Also handle the case of non-contiguous expression vectors which might arise
   in symbolic matrix operations. */

static inline void pure_new_vect2(const size_t n, const size_t m,
				  const size_t stride, pure_expr **xs)
{
  if (stride == m)
    pure_new_vect(n*m, xs);
  short rc;
  if (n == 0 || m == 0)
    return;
  else if (xs[0]->refc > 0)
    rc = 0;
  else {
    interpreter& interp = *interpreter::g_interp;
    pure_expr *tmps = interp.tmps;
    pure_expr *first = xs[0], *last = xs[(n-1)*stride+m-1];
    rc = peek(tmps, first, last);
  }
  if (rc <= 0)
    for (size_t i = n; i > 0; ) {
      const size_t k = (--i)*stride;
      for (size_t j = m; j > 0; )
	pure_new_internal(xs[k+(--j)]);
    }
  else
    for (size_t i = 0; i < n; i++) {
      const size_t k = i*stride;
      for (size_t j = 0; j < m; j++)
	pure_new_internal(xs[k+j]);
    }
}

static void pure_free_clos(pure_expr *x)
{
#if DEBUG>3
  cerr << "pure_free, freeing environment: "
       << (x->data.clos->local?"local":"global") << " closure "
       << x << " (" << (void*)x << "), refc = " << x->refc << endl;
#endif
  if (x->data.clos->ep) {
    Env *env = (Env*)x->data.clos->ep;
    assert(env->refc > 0);
    if (--env->refc == 0) delete env;
  }
  if (x->data.clos->env) {
    for (size_t i = 0; i < x->data.clos->m; i++)
      pure_free(x->data.clos->env[i]);
    delete[] x->data.clos->env;
  }
  delete x->data.clos;
}

static pure_closure *pure_copy_clos(pure_closure *clos)
{
  assert(clos);
  pure_closure *ret = new pure_closure;
  ret->local = clos->local;
  ret->thunked = clos->thunked;
  ret->n = clos->n;
  ret->m = clos->m;
  ret->fp = clos->fp;
  ret->ep = clos->ep;
  if (clos->ep) ((Env*)clos->ep)->refc++;
  if (clos->m == 0)
    ret->env = 0;
  else {
    ret->env = new pure_expr*[clos->m];
    for (size_t i = 0; i < clos->m; i++) {
      ret->env[i] = clos->env[i];
      assert(clos->env[i]->refc > 0);
      clos->env[i]->refc++;
    }
  }
  return ret;
}

static void pure_free_matrix(pure_expr *x)
{
  if (!x->data.mat.p) return;
  assert(x->data.mat.refc && "pure_free_matrix: corrupt data");
  assert(*x->data.mat.refc > 0 && "pure_free_matrix: unreferenced data");
  bool owner = --*x->data.mat.refc == 0;
  switch (x->tag) {
  case EXPR::MATRIX: {
    gsl_matrix_symbolic *m = (gsl_matrix_symbolic*)x->data.mat.p;
    if (owner) {
      const size_t k = m->size1, l = m->size2, tda = m->tda;;
      for (size_t i = 0; i < k; i++)
	for (size_t j = 0; j < l; j++)
	  pure_free(m->data[i*tda+j]);
    }
    m->owner = owner;
    gsl_matrix_symbolic_free(m);
    break;
  }
#ifdef HAVE_GSL
  case EXPR::DMATRIX: {
    gsl_matrix *m = (gsl_matrix*)x->data.mat.p;
    m->owner = owner && m->block;
    gsl_matrix_free(m);
    break;
  }
  case EXPR::CMATRIX: {
    gsl_matrix_complex *m = (gsl_matrix_complex*)x->data.mat.p;
    m->owner = owner && m->block;
    gsl_matrix_complex_free(m);
    break;
  }
  case EXPR::IMATRIX: {
    gsl_matrix_int *m = (gsl_matrix_int*)x->data.mat.p;
    m->owner = owner && m->block;
    gsl_matrix_int_free(m);
    break;
  }
#endif
  default:
    break;
  }
  if (owner) delete x->data.mat.refc;
}

#if 1

/* This is implemented (mostly) non-recursively to prevent stack overflows,
   using the xp field to form a stack on the fly. */

static inline
void pure_free_internal(pure_expr *x)
{
  assert(x && "pure_free: null expression");
  assert(x->refc > 0 && "pure_free: unreferenced expression");
  assert(!x->xp && "pure_free: corrupt expression data");
  pure_expr *xp = 0, *y;
 loop:
  if (--x->refc == 0) {
    call_sentry(x);
    switch (x->tag) {
    case EXPR::APP:
      y = x->data.x[0];
      assert(!x->xp);
      x->xp = xp; xp = x; x = y;
      goto loop;
    case EXPR::INT:
    case EXPR::DBL:
    case EXPR::PTR:
      // nothing to do
      break;
    case EXPR::BIGINT:
      mpz_clear(x->data.z);
      break;
    case EXPR::STR:
      free(x->data.s);
      break;
    case EXPR::MATRIX:
    case EXPR::DMATRIX:
    case EXPR::CMATRIX:
    case EXPR::IMATRIX:
      pure_free_matrix(x);
      break;
    default:
      assert(x->tag >= 0);
      if (x->data.clos) pure_free_clos(x);
      break;
    }
  }
  while (xp && x == xp->data.x[1]) {
    if (x->refc == 0) {
      free_sentry(x);
      free_expr(x);
    }
    x = xp; xp = x->xp;
  }
  if (x->refc == 0) {
    free_sentry(x);
    free_expr(x);
  }
  if (xp) {
    x = xp->data.x[1];
    goto loop;
  }
}

#else

/* This version is only included here for reference and debugging purposes,
   normal builds should use the non-recursive version above. */

static
void pure_free_internal(pure_expr *x)
{
  if (--x->refc == 0) {
    call_sentry(x);
    switch (x->tag) {
    case EXPR::APP:
      pure_free_internal(x->data.x[0]);
      pure_free_internal(x->data.x[1]);
      break;
    case EXPR::INT:
    case EXPR::DBL:
      break;
    case EXPR::BIGINT:
      mpz_clear(x->data.z);
      break;
    case EXPR::STR:
      free(x->data.s);
      break;
    case EXPR::PTR:
      break;
    default:
      assert(x->tag >= 0);
      if (x->data.clos) pure_free_clos(x);
      break;
    }
    free_sentry(x);
    free_expr(x);
  }
}

#endif

static void inline
pure_unref_internal(pure_expr *x)
{
  assert(x && "pure_unref: null expression");
  assert(x->refc > 0 && "pure_unref: unreferenced expression");
  if (--x->refc == 0 && !x->xp) {
    interpreter& interp = *interpreter::g_interp;
    // check whether x is already on the tmps list
    pure_expr *tmps = interp.tmps;
    while (tmps && tmps != x) tmps = tmps->xp;
    if (!tmps) {
      // put x on the tmps list again
      x->xp = interp.tmps;
      interp.tmps = x;
    }
  }
}

/* PUBLIC API. **************************************************************/

extern "C"
int32_t pure_sym(const char *s)
{
  assert(s);
  interpreter& interp = *interpreter::g_interp;
  const char *t = strstr(s, "::");
  string id = t?s:"::"+string(s), qual = t?id.substr(0, t-s):"";
  const symbol* sym;
  if (qual != *interp.symtab.current_namespace) {
    /* Switch to the desired namespace so that we also get the private
       symbols. */
    string *save_namespace = interp.symtab.current_namespace;
    interp.symtab.current_namespace = &qual;
    sym = interp.symtab.sym(id);
    interp.symtab.current_namespace = save_namespace;
  } else
    sym = interp.symtab.sym(id);
  if (sym)
    return sym->f;
  else
    return 0;
}

extern "C"
int32_t pure_getsym(const char *s)
{
  assert(s);
  interpreter& interp = *interpreter::g_interp;
  string id = strstr(s, "::")?s:"::"+string(s);
  const symbol* sym = interp.symtab.lookup(id);
  if (sym)
    return sym->f;
  else
    return 0;
}

extern "C"
const char *pure_sym_pname(int32_t tag)
{
  assert(tag>0);
  interpreter& interp = *interpreter::g_interp;
  const symbol& sym = interp.symtab.sym(tag);
  return sym.s.c_str();
}

extern "C"
int8_t pure_sym_nprec(int32_t tag)
{
  assert(tag>0);
  interpreter& interp = *interpreter::g_interp;
  const symbol& sym = interp.symtab.sym(tag);
  return nprec(sym.prec, sym.fix);
}

extern "C"
pure_expr *pure_symbol(int32_t tag)
{
  assert(tag>0);
  interpreter& interp = *interpreter::g_interp;
  const symbol& sym = interp.symtab.sym(tag);
  // Check for an existing global variable for this symbol.
  GlobalVar& v = interp.globalvars[tag];
  if (!v.v) {
    // The variable doesn't exist yet (we have a new symbol), create it.
    string lab;
    // Create a name for the variable (cf. interpreter::mkvarlabel).
    if (sym.prec < 10 || sym.fix == nullary)
      lab = "$("+sym.s+")";
    else
      lab = "$"+sym.s;
    // Create a global variable bound to the symbol for now.
    v.v = new llvm::GlobalVariable
      (interp.ExprPtrTy, false, llvm::GlobalVariable::InternalLinkage,
       llvm::ConstantPointerNull::get(interp.ExprPtrTy),
       lab.c_str(), interp.module);
    interp.JIT->addGlobalMapping(v.v, &v.x);
    v.x = pure_new_internal(pure_const(tag));
    // Since we just created this variable, it doesn't have any closure bound
    // to it yet, so it's safe to just return the symbol as is.
    return v.x;
  } else {
    // The symbol already has a definition, so we might have to evaluate it on
    // the fly.
    map<int32_t,ExternInfo>::const_iterator it = interp.externals.find(tag);
    if (it != interp.externals.end()) {
      // We have an external. This case must be treated separately, since v
      // just points to a box for the Pure function symbol rather than the
      // external wrapper function itself.
      const ExternInfo& info = it->second;
      size_t n = info.argtypes.size();
      void *f = interp.JIT->getPointerToFunction(info.f);
      if (f) {
	if (n == 0)
	  // Parameterless external, do a direct call.
	  return ((pure_expr *(*)(void))f) ();
	else
	  // External with parameters. Build an fbox for the external, return
	  // this as the value of the symbol.
	  return pure_clos(false, false, tag, n, f, 0, 0);
      }
      // If we come here, the external wrapper failed to compile for some
      // reason, just proceed as if it was an ordinary Pure function.
    }
    // We have an ordinary Pure symbol. There might be a parameterless closure
    // bound to it, pure_call takes care of that case.
    return pure_call(v.x);
  }
}

extern "C"
pure_expr *pure_int(int32_t i)
{
  pure_expr *x = new_expr();
  x->tag = EXPR::INT;
  x->data.i = i;
  MEMDEBUG_NEW(x)
  return x;
}

extern "C"
pure_expr *pure_mpz(const mpz_t z)
{
  pure_expr *x = new_expr();
  x->tag = EXPR::BIGINT;
  mpz_init_set(x->data.z, z);
  MEMDEBUG_NEW(x)
  return x;
}

extern "C"
pure_expr *pure_double(double d)
{
  pure_expr *x = new_expr();
  x->tag = EXPR::DBL;
  x->data.d = d;
  MEMDEBUG_NEW(x)
  return x;
}

extern "C"
pure_expr *pure_pointer(void *p)
{
  pure_expr *x = new_expr();
  x->tag = EXPR::PTR;
  x->data.p = p;
  MEMDEBUG_NEW(x)
  return x;
}

extern "C"
pure_expr *pure_expr_pointer(void)
{
  pure_expr **p = (pure_expr**)malloc(sizeof(pure_expr*));
  if (p) {
    *p = 0;
    return pure_pointer(p);
  } else
    return 0;
}

extern "C"
pure_expr *pure_string_dup(const char *s)
{
  if (!s) return pure_pointer(0);
  pure_expr *x = new_expr();
  x->tag = EXPR::STR;
  x->data.s = strdup(s);
  MEMDEBUG_NEW(x)
  return x;
}

extern "C"
pure_expr *pure_cstring_dup(const char *s)
{
  if (!s) return pure_pointer(0);
  pure_expr *x = new_expr();
  x->tag = EXPR::STR;
  x->data.s = toutf8(s, 0);
  MEMDEBUG_NEW(x)
  return x;
}

extern "C"
pure_expr *pure_string(char *s)
{
  if (!s) return pure_pointer(0);
  pure_expr *x = new_expr();
  x->tag = EXPR::STR;
  x->data.s = s;
  MEMDEBUG_NEW(x)
  return x;
}

extern "C"
pure_expr *pure_cstring(char *s)
{
  if (!s) return pure_pointer(0);
  pure_expr *x = pure_cstring_dup(s);
  free(s);
  return x;
}

#ifdef HAVE_GSL
/* GSL doesn't really support empty matrices, so we fake them by allocating 1
   dummy row or column if the corresponding dimension is actually zero. */
static inline gsl_matrix*
create_double_matrix(size_t nrows, size_t ncols)
{
  if (nrows == 0 || ncols == 0 ) {
    size_t nrows1 = (nrows>0)?nrows:1;
    size_t ncols1 = (ncols>0)?ncols:1;
    gsl_matrix *m = gsl_matrix_calloc(nrows1, ncols1);
    if (!m) return 0;
    m->size1 = nrows; m->size2 = ncols;
    return m;
  } else
    return gsl_matrix_alloc(nrows, ncols);
}

static inline gsl_matrix_complex*
create_complex_matrix(size_t nrows, size_t ncols)
{
  if (nrows == 0 || ncols == 0 ) {
    size_t nrows1 = (nrows>0)?nrows:1;
    size_t ncols1 = (ncols>0)?ncols:1;
    gsl_matrix_complex *m = gsl_matrix_complex_calloc(nrows1, ncols1);
    if (!m) return 0;
    m->size1 = nrows; m->size2 = ncols;
    return m;
  } else
    return gsl_matrix_complex_alloc(nrows, ncols);
}

static inline gsl_matrix_int*
create_int_matrix(size_t nrows, size_t ncols)
{
  if (nrows == 0 || ncols == 0 ) {
    size_t nrows1 = (nrows>0)?nrows:1;
    size_t ncols1 = (ncols>0)?ncols:1;
    gsl_matrix_int *m = gsl_matrix_int_calloc(nrows1, ncols1);
    if (!m) return 0;
    m->size1 = nrows; m->size2 = ncols;
    return m;
  } else
    return gsl_matrix_int_alloc(nrows, ncols);
}
#endif

static inline gsl_matrix_symbolic*
create_symbolic_matrix(size_t nrows, size_t ncols)
{
  if (nrows == 0 || ncols == 0 ) {
    size_t nrows1 = (nrows>0)?nrows:1;
    size_t ncols1 = (ncols>0)?ncols:1;
    gsl_matrix_symbolic *m = gsl_matrix_symbolic_calloc(nrows1, ncols1);
    if (!m) return 0;
    m->size1 = nrows; m->size2 = ncols;
    return m;
  } else
    return gsl_matrix_symbolic_alloc(nrows, ncols);
}

extern "C"
pure_expr *pure_symbolic_matrix(void *p)
{
  gsl_matrix_symbolic *m = (gsl_matrix_symbolic*)p;
  if (!m || !m->owner) return 0;
  m->owner = 0;
  pure_expr *x = new_expr();
  x->tag = EXPR::MATRIX;
  x->data.mat.p = p;
  // count references
  const size_t k = m->size1, l = m->size2, tda = m->tda;
  pure_new_vect2(k, l, tda, m->data);
  x->data.mat.refc = new uint32_t;
  *x->data.mat.refc = 1;
  MEMDEBUG_NEW(x)
  return x;
}

extern "C"
pure_expr *pure_double_matrix(void *p)
{
#ifdef HAVE_GSL
  gsl_matrix *m = (gsl_matrix*)p;
  if (!m || !m->owner) return 0;
  m->owner = 0;
  pure_expr *x = new_expr();
  x->tag = EXPR::DMATRIX;
  x->data.mat.p = p;
  x->data.mat.refc = new uint32_t;
  *x->data.mat.refc = 1;
  MEMDEBUG_NEW(x)
  return x;
#else
  return 0;
#endif
}

extern "C"
pure_expr *pure_complex_matrix(void *p)
{
#ifdef HAVE_GSL
  gsl_matrix_complex *m = (gsl_matrix_complex*)p;
  if (!m || !m->owner) return 0;
  m->owner = 0;
  pure_expr *x = new_expr();
  x->tag = EXPR::CMATRIX;
  x->data.mat.p = p;
  x->data.mat.refc = new uint32_t;
  *x->data.mat.refc = 1;
  MEMDEBUG_NEW(x)
  return x;
#else
  return 0;
#endif
}

extern "C"
pure_expr *pure_int_matrix(void *p)
{
#ifdef HAVE_GSL
  gsl_matrix_int *m = (gsl_matrix_int*)p;
  if (!m || !m->owner) return 0;
  m->owner = 0;
  pure_expr *x = new_expr();
  x->tag = EXPR::IMATRIX;
  x->data.mat.p = p;
  x->data.mat.refc = new uint32_t;
  *x->data.mat.refc = 1;
  MEMDEBUG_NEW(x)
  return x;
#else
  return 0;
#endif
}

extern "C"
pure_expr *pure_symbolic_matrix_dup(const void *p)
{
  gsl_matrix_symbolic *m1 = (gsl_matrix_symbolic*)p;
  if (!m1) return 0;
  gsl_matrix_symbolic *m2 = create_symbolic_matrix(m1->size1, m1->size2);
  if (!m2) return 0;
  if (m1->size1 > 0 && m1->size2 > 0)
    gsl_matrix_symbolic_memcpy(m2, m1);
  return pure_symbolic_matrix(m2);
}

extern "C"
pure_expr *pure_double_matrix_dup(const void *p)
{
#ifdef HAVE_GSL
  gsl_matrix *m1 = (gsl_matrix*)p;
  if (!m1) return 0;
  gsl_matrix *m2 = create_double_matrix(m1->size1, m1->size2);
  if (!m2) return 0;
  if (m1->size1 > 0 && m1->size2 > 0)
    gsl_matrix_memcpy(m2, m1);
  return pure_double_matrix(m2);
#else
  return 0;
#endif
}

extern "C"
pure_expr *pure_complex_matrix_dup(const void *p)
{
#ifdef HAVE_GSL
  gsl_matrix_complex *m1 = (gsl_matrix_complex*)p;
  if (!m1) return 0;
  gsl_matrix_complex *m2 = create_complex_matrix(m1->size1, m1->size2);
  if (!m2) return 0;
  if (m1->size1 > 0 && m1->size2 > 0)
    gsl_matrix_complex_memcpy(m2, m1);
  return pure_complex_matrix(m2);
#else
  return 0;
#endif
}

extern "C"
pure_expr *pure_int_matrix_dup(const void *p)
{
#ifdef HAVE_GSL
  gsl_matrix_int *m1 = (gsl_matrix_int*)p;
  if (!m1) return 0;
  gsl_matrix_int *m2 = create_int_matrix(m1->size1, m1->size2);
  if (!m2) return 0;
  if (m1->size1 > 0 && m1->size2 > 0)
    gsl_matrix_int_memcpy(m2, m1);
  return pure_int_matrix(m2);
#else
  return 0;
#endif
}

static inline bool is_complex(pure_expr *x)
{
  if (x->tag != EXPR::APP) return false;
  pure_expr *u = x->data.x[0], *v = x->data.x[1];
  if (u->tag == EXPR::APP) {
    interpreter& interp = *interpreter::g_interp;
    pure_expr *f = u->data.x[0];
    symbol &rect = interp.symtab.complex_rect_sym(),
      &polar = interp.symtab.complex_polar_sym();
    if (f->tag != rect.f && f->tag != polar.f)
      return false;
    u = u->data.x[1];
    if ((u->tag != EXPR::INT && u->tag != EXPR::DBL) ||
	(v->tag != EXPR::INT && v->tag != EXPR::DBL))
      return false;
    else
      return true;
  } else
    return false;
}

static inline bool get_complex(pure_expr *x, double& a, double& b)
{
  if (x->tag != EXPR::APP) return false;
  pure_expr *u = x->data.x[0], *v = x->data.x[1];
  if (u->tag == EXPR::APP) {
    interpreter& interp = *interpreter::g_interp;
    pure_expr *f = u->data.x[0];
    symbol &rect = interp.symtab.complex_rect_sym(),
      &polar = interp.symtab.complex_polar_sym();
    if (f->tag != rect.f && f->tag != polar.f)
      return false;
    u = u->data.x[1];
    switch (u->tag) {
    case EXPR::INT:
      a = (double)u->data.i;
      break;
    case EXPR::DBL:
      a = u->data.d;
      break;
    default:
      return false;
    }
    switch (v->tag) {
    case EXPR::INT:
      b = (double)v->data.i;
      break;
    case EXPR::DBL:
      b = v->data.d;
      break;
    default:
      return false;
    }
    if (f->tag == polar.f) {
      double r = a, t = b;
      a = r*cos(t); b = r*sin(t);
    }
    return true;
  } else
    return false;
}

static inline pure_expr *make_complex2(symbol& rect, double a, double b)
{
  return pure_appl(pure_symbol(rect.f), 2, pure_double(a), pure_double(b));
}

static inline pure_expr *make_complex(double a, double b)
{
  interpreter& interp = *interpreter::g_interp;
  symbol &rect = interp.symtab.complex_rect_sym();
  return make_complex2(rect, a, b);
}

#ifdef HAVE_GSL
static pure_expr*
double_matrix_rows(size_t nrows, size_t ncols, size_t n, pure_expr **xs)
{
  gsl_matrix *mat = create_double_matrix(nrows, ncols);
  if (!mat) return 0;
  double *data = mat->data;
  size_t tda = mat->tda;
  pure_new_vect(n, xs);
  for (size_t count = 0, i = 0; count < n; count++) {
    pure_expr *x = xs[count];
    switch (x->tag) {
    case EXPR::INT:
      data[i++*tda] = (double)x->data.i;
      break;
    case EXPR::BIGINT:
      data[i++*tda] = mpz_get_d(x->data.z);
      break;
    case EXPR::DBL:
      data[i++*tda] = x->data.d;
      break;
    case EXPR::DMATRIX: {
      gsl_matrix *mat1 = (gsl_matrix*)x->data.mat.p;
      if (mat1)
	for (size_t j = 0; j < mat1->size1; i++, j++)
	  memcpy(data+i*tda, mat1->data+j*mat1->tda, ncols*sizeof(double));
      break;
    }
    case EXPR::IMATRIX: {
      gsl_matrix_int *mat1 = (gsl_matrix_int*)x->data.mat.p;
      if (mat1)
	for (size_t j = 0; j < mat1->size1; i++, j++)
	  for (size_t k = 0; k < mat1->size2; k++)
	    data[i*tda+k] = (double)mat1->data[j*mat1->tda+k];
      break;
    }
    case EXPR::CMATRIX:
    case EXPR::MATRIX:
      // empty matrix, skip
      break;
    default:
      assert(0 && "bad matrix element");
      break;
    }
  }
  for (size_t i = 0; i < n; i++)
    pure_free_internal(xs[i]);
  return pure_double_matrix(mat);
}

static pure_expr*
double_matrix_columns(size_t nrows, size_t ncols, size_t n, pure_expr **xs)
{
  gsl_matrix *mat = create_double_matrix(nrows, ncols);
  if (!mat) return 0;
  double *data = mat->data;
  size_t tda = mat->tda;
  pure_new_vect(n, xs);
  for (size_t count = 0, i = 0; count < n; count++) {
    pure_expr *x = xs[count];
    switch (x->tag) {
    case EXPR::INT:
      data[i++] = (double)x->data.i;
      break;
    case EXPR::BIGINT:
      data[i++] = mpz_get_d(x->data.z);
      break;
    case EXPR::DBL:
      data[i++] = x->data.d;
      break;
    case EXPR::DMATRIX: {
      gsl_matrix *mat1 = (gsl_matrix*)x->data.mat.p;
      if (mat1)
	for (size_t j = 0; j < mat1->size1; j++)
	  memcpy(data+j*tda+i, mat1->data+j*mat1->tda,
		 mat1->size2*sizeof(double));
      i += mat1->size2;
      break;
    }
    case EXPR::IMATRIX: {
      gsl_matrix_int *mat1 = (gsl_matrix_int*)x->data.mat.p;
      if (mat1)
	for (size_t j = 0; j < mat1->size1; j++)
	  for (size_t k = 0; k < mat1->size2; k++)
	    data[j*tda+k+i] = (double)mat1->data[j*mat1->tda+k];
      i += mat1->size2;
      break;
    }
    case EXPR::CMATRIX:
    case EXPR::MATRIX:
      // empty matrix, skip
      break;
    default:
      assert(0 && "bad matrix element");
      break;
    }
  }
  for (size_t i = 0; i < n; i++)
    pure_free_internal(xs[i]);
  return pure_double_matrix(mat);
}

static pure_expr*
complex_matrix_rows(size_t nrows, size_t ncols, size_t n, pure_expr **xs)
{
  gsl_matrix_complex *mat = create_complex_matrix(nrows, ncols);
  if (!mat) return 0;
  double *data = mat->data;
  size_t tda = mat->tda;
  pure_new_vect(n, xs);
  for (size_t count = 0, i = 0; count < n; count++) {
    pure_expr *x = xs[count];
    switch (x->tag) {
    case EXPR::INT:
      data[2*i*tda] = (double)x->data.i;
      data[2*i*tda+1] = 0.0;
      i++;
      break;
    case EXPR::BIGINT:
      data[2*i*tda] = mpz_get_d(x->data.z);
      data[2*i*tda+1] = 0.0;
      i++;
      break;
    case EXPR::DBL:
      data[2*i*tda] = x->data.d;
      data[2*i*tda+1] = 0.0;
      i++;
      break;
    case EXPR::APP: {
      double a, b;
      if (get_complex(x, a, b)) {
	data[2*i*tda] = a;
	data[2*i*tda+1] = b;
	i++;
      } else {
	assert(0 && "bad matrix element");
      }
      break;
    }
    case EXPR::DMATRIX: {
      gsl_matrix *mat1 = (gsl_matrix*)x->data.mat.p;
      if (mat1)
	for (size_t j = 0; j < mat1->size1; i++, j++)
	  for (size_t k = 0; k < mat1->size2; k++) {
	    data[2*(i*tda+k)] = mat1->data[j*mat1->tda+k];
	    data[2*(i*tda+k)+1] = 0.0;
	  }
      break;
    }
    case EXPR::IMATRIX: {
      gsl_matrix_int *mat1 = (gsl_matrix_int*)x->data.mat.p;
      if (mat1)
	for (size_t j = 0; j < mat1->size1; i++, j++)
	  for (size_t k = 0; k < mat1->size2; k++) {
	    data[2*(i*tda+k)] = (double)mat1->data[j*mat1->tda+k];
	    data[2*(i*tda+k)+1] = 0.0;
	  }
      break;
    }
    case EXPR::CMATRIX: {
      gsl_matrix_complex *mat1 = (gsl_matrix_complex*)x->data.mat.p;
      if (mat1)
	for (size_t j = 0; j < mat1->size1; i++, j++)
	  memcpy(data+2*i*tda, mat1->data+2*j*mat1->tda,
		 ncols*2*sizeof(double));
      break;
    }
    case EXPR::MATRIX:
      // empty matrix, skip
      break;
    default:
      assert(0 && "bad matrix element");
      break;
    }
  }
  for (size_t i = 0; i < n; i++)
    pure_free_internal(xs[i]);
  return pure_complex_matrix(mat);
}

static pure_expr*
complex_matrix_columns(size_t nrows, size_t ncols, size_t n, pure_expr **xs)
{
  gsl_matrix_complex *mat = create_complex_matrix(nrows, ncols);
  if (!mat) return 0;
  double *data = mat->data;
  size_t tda = mat->tda;
  pure_new_vect(n, xs);
  for (size_t count = 0, i = 0; count < n; count++) {
    pure_expr *x = xs[count];
    switch (x->tag) {
    case EXPR::INT:
      data[2*i] = (double)x->data.i;
      data[2*i+1] = 0.0;
      i++;
      break;
    case EXPR::BIGINT:
      data[2*i] = mpz_get_d(x->data.z);
      data[2*i+1] = 0.0;
      i++;
      break;
    case EXPR::DBL:
      data[2*i] = x->data.d;
      data[2*i+1] = 0.0;
      i++;
      break;
    case EXPR::APP: {
      double a, b;
      if (get_complex(x, a, b)) {
	data[2*i] = a;
	data[2*i+1] = b;
	i++;
      } else {
	assert(0 && "bad matrix element");
      }
      break;
    }
    case EXPR::DMATRIX: {
      gsl_matrix *mat1 = (gsl_matrix*)x->data.mat.p;
      if (mat1)
	for (size_t j = 0; j < mat1->size1; j++)
	  for (size_t k = 0; k < mat1->size2; k++) {
	    data[2*(j*tda+k+i)] = mat1->data[j*mat1->tda+k];
	    data[2*(j*tda+k+i)+1] = 0.0;
	  }
      i += mat1->size2;
      break;
    }
    case EXPR::IMATRIX: {
      gsl_matrix_int *mat1 = (gsl_matrix_int*)x->data.mat.p;
      if (mat1)
	for (size_t j = 0; j < mat1->size1; j++)
	  for (size_t k = 0; k < mat1->size2; k++) {
	    data[2*(j*tda+k+i)] = (double)mat1->data[j*mat1->tda+k];
	    data[2*(j*tda+k+i)+1] = 0.0;
	  }
      i += mat1->size2;
      break;
    }
    case EXPR::CMATRIX: {
      gsl_matrix_complex *mat1 = (gsl_matrix_complex*)x->data.mat.p;
      if (mat1)
	for (size_t j = 0; j < mat1->size1; j++)
	  memcpy(data+2*(j*tda+i), mat1->data+2*j*mat1->tda,
		 mat1->size2*2*sizeof(double));
      i += mat1->size2;
      break;
    }
    case EXPR::MATRIX:
      // empty matrix, skip
      break;
    default:
      assert(0 && "bad matrix element");
      break;
    }
  }
  for (size_t i = 0; i < n; i++)
    pure_free_internal(xs[i]);
  return pure_complex_matrix(mat);
}

static pure_expr*
int_matrix_rows(size_t nrows, size_t ncols, size_t n, pure_expr **xs)
{
  gsl_matrix_int *mat = create_int_matrix(nrows, ncols);
  if (!mat) return 0;
  int *data = mat->data;
  size_t tda = mat->tda;
  pure_new_vect(n, xs);
  for (size_t count = 0, i = 0; count < n; count++) {
    pure_expr *x = xs[count];
    switch (x->tag) {
    case EXPR::INT:
      data[i++*tda] = x->data.i;
      break;
    case EXPR::BIGINT:
      data[i++*tda] = pure_get_int(x);
      break;
    case EXPR::DBL:
      data[i++*tda] = (int)x->data.d;
      break;
    case EXPR::DMATRIX: {
      gsl_matrix *mat1 = (gsl_matrix*)x->data.mat.p;
      if (mat1)
	for (size_t j = 0; j < mat1->size1; i++, j++)
	  for (size_t k = 0; k < mat1->size2; k++)
	    data[i*tda+k] = (int)mat1->data[j*mat1->tda+k];
      break;
    }
    case EXPR::IMATRIX: {
      gsl_matrix_int *mat1 = (gsl_matrix_int*)x->data.mat.p;
      if (mat1)
	for (size_t j = 0; j < mat1->size1; i++, j++)
	  memcpy(data+i*tda, mat1->data+j*mat1->tda, ncols*sizeof(int));
      break;
    }
    case EXPR::CMATRIX:
    case EXPR::MATRIX:
      // empty matrix, skip
      break;
    default:
      assert(0 && "bad matrix element");
      break;
    }
  }
  for (size_t i = 0; i < n; i++)
    pure_free_internal(xs[i]);
  return pure_int_matrix(mat);
}

static pure_expr*
int_matrix_columns(size_t nrows, size_t ncols, size_t n, pure_expr **xs)
{
  gsl_matrix_int *mat = create_int_matrix(nrows, ncols);
  if (!mat) return 0;
  int *data = mat->data;
  size_t tda = mat->tda;
  pure_new_vect(n, xs);
  for (size_t count = 0, i = 0; count < n; count++) {
    pure_expr *x = xs[count];
    switch (x->tag) {
    case EXPR::INT:
      data[i++] = x->data.i;
      break;
    case EXPR::BIGINT:
      data[i++] = pure_get_int(x);
      break;
    case EXPR::DBL:
      data[i++] = (int)x->data.d;
      break;
    case EXPR::DMATRIX: {
      gsl_matrix *mat1 = (gsl_matrix*)x->data.mat.p;
      if (mat1)
	for (size_t j = 0; j < mat1->size1; j++)
	  for (size_t k = 0; k < mat1->size2; k++)
	    data[j*tda+k+i] = (int)mat1->data[j*mat1->tda+k];
      i += mat1->size2;
      break;
    }
    case EXPR::IMATRIX: {
      gsl_matrix_int *mat1 = (gsl_matrix_int*)x->data.mat.p;
      if (mat1)
	for (size_t j = 0; j < mat1->size1; j++)
	  memcpy(data+j*tda+i, mat1->data+j*mat1->tda,
		 mat1->size2*sizeof(int));
      i += mat1->size2;
      break;
    }
    case EXPR::CMATRIX:
    case EXPR::MATRIX:
      // empty matrix, skip
      break;
    default:
      assert(0 && "bad matrix element");
      break;
    }
  }
  for (size_t i = 0; i < n; i++)
    pure_free_internal(xs[i]);
  return pure_int_matrix(mat);
}
#endif

static pure_expr*
symbolic_matrix_rows(size_t nrows, size_t ncols, size_t n, pure_expr **xs)
{
  gsl_matrix_symbolic *mat = create_symbolic_matrix(nrows, ncols);
  if (!mat) return 0;
  pure_expr **data = mat->data;
  size_t tda = mat->tda;
  pure_new_vect(n, xs);
  for (size_t count = 0, i = 0; count < n; count++) {
    pure_expr *x = xs[count];
    switch (x->tag) {
    case EXPR::MATRIX: {
      gsl_matrix_symbolic *mat1 = (gsl_matrix_symbolic*)x->data.mat.p;
      if (mat1)
	for (size_t j = 0; j < mat1->size1; i++, j++)
	  memcpy(data+i*tda, mat1->data+j*mat1->tda, ncols*sizeof(pure_expr*));
      break;
    }
#ifdef HAVE_GSL
    case EXPR::DMATRIX: {
      gsl_matrix *mat1 = (gsl_matrix*)x->data.mat.p;
      if (mat1)
	for (size_t j = 0; j < mat1->size1; i++, j++)
	  for (size_t k = 0; k < mat1->size2; k++)
	    data[i*tda+k] = pure_double(mat1->data[j*mat1->tda+k]);
      break;
    }
    case EXPR::IMATRIX: {
      gsl_matrix_int *mat1 = (gsl_matrix_int*)x->data.mat.p;
      if (mat1)
	for (size_t j = 0; j < mat1->size1; i++, j++)
	  for (size_t k = 0; k < mat1->size2; k++)
	    data[i*tda+k] = pure_int(mat1->data[j*mat1->tda+k]);
      break;
    }
    case EXPR::CMATRIX: {
      gsl_matrix_complex *mat1 = (gsl_matrix_complex*)x->data.mat.p;
      if (mat1) {
	interpreter& interp = *interpreter::g_interp;
	symbol& rect = interp.symtab.complex_rect_sym();
	for (size_t j = 0; j < mat1->size1; i++, j++)
	  for (size_t k = 0; k < mat1->size2; k++) {
	    size_t l = 2*(j*mat1->tda+k);
	    data[i*tda+k] =
	      make_complex2(rect, mat1->data[l], mat1->data[l+1]);
	  }
      }
      break;
    }
#endif
    default:
      data[i++*tda] = x;
      break;
    }
  }
  pure_expr *ret = pure_symbolic_matrix(mat);
  for (size_t i = 0; i < n; i++)
    pure_free_internal(xs[i]);
  return ret;
}

static pure_expr*
symbolic_matrix_columns(size_t nrows, size_t ncols, size_t n, pure_expr **xs)
{
  gsl_matrix_symbolic *mat = create_symbolic_matrix(nrows, ncols);
  if (!mat) return 0;
  pure_expr **data = mat->data;
  size_t tda = mat->tda;
  pure_new_vect(n, xs);
  for (size_t count = 0, i = 0; count < n; count++) {
    pure_expr *x = xs[count];
    switch (x->tag) {
    case EXPR::MATRIX: {
      gsl_matrix_symbolic *mat1 = (gsl_matrix_symbolic*)x->data.mat.p;
      if (mat1)
	for (size_t j = 0; j < mat1->size1; j++)
	  memcpy(data+j*tda+i, mat1->data+j*mat1->tda,
		 mat1->size2*sizeof(pure_expr*));
      i += mat1->size2;
      break;
    }
#ifdef HAVE_GSL
    case EXPR::DMATRIX: {
      gsl_matrix *mat1 = (gsl_matrix*)x->data.mat.p;
      if (mat1)
	for (size_t j = 0; j < mat1->size1; j++)
	  for (size_t k = 0; k < mat1->size2; k++)
	    data[j*tda+k+i] = pure_double(mat1->data[j*mat1->tda+k]);
      i += mat1->size2;
      break;
    }
    case EXPR::IMATRIX: {
      gsl_matrix_int *mat1 = (gsl_matrix_int*)x->data.mat.p;
      if (mat1)
	for (size_t j = 0; j < mat1->size1; j++)
	  for (size_t k = 0; k < mat1->size2; k++)
	    data[j*tda+k+i] = pure_int(mat1->data[j*mat1->tda+k]);
      i += mat1->size2;
      break;
    }
    case EXPR::CMATRIX: {
      gsl_matrix_complex *mat1 = (gsl_matrix_complex*)x->data.mat.p;
      if (mat1) {
	interpreter& interp = *interpreter::g_interp;
	symbol& rect = interp.symtab.complex_rect_sym();
	for (size_t j = 0; j < mat1->size1; j++)
	  for (size_t k = 0; k < mat1->size2; k++) {
	    size_t l = 2*(j*mat1->tda+k);
	    data[j*tda+k+i] =
	      make_complex2(rect, mat1->data[l], mat1->data[l+1]);
	  }
      }
      i += mat1->size2;
      break;
    }
#endif
    default:
      data[i++] = x;
      break;
    }
  }
  pure_expr *ret = pure_symbolic_matrix(mat);
  for (size_t i = 0; i < n; i++)
    pure_free_internal(xs[i]);
  return ret;
}

static inline void set_target_type(int32_t& target, int32_t t)
{
  if (target == 0)
    target = t;
  else if (target != t)
    target = EXPR::MATRIX;
}

extern "C"
pure_expr *pure_matrix_rowsl(uint32_t n, ...)
{
  va_list ap;
  pure_expr **xs = (pure_expr**)alloca(n*sizeof(pure_expr*));
  va_start(ap, n);
  for (size_t i = 0; i < n; i++)
    xs[i] = va_arg(ap, pure_expr*);
  va_end(ap);
  return pure_matrix_rowsv(n, xs);
}

extern "C"
pure_expr *pure_matrix_rowsv(uint32_t n, pure_expr **xs)
{
  int k = -1;
  size_t nrows = 0, ncols = 0;
  int32_t target = 0;
  bool have_matrix = false;
  for (size_t i = 0; i < n; i++) {
    pure_expr *x = xs[i];
    switch (x->tag) {
    case EXPR::MATRIX: {
      gsl_matrix_symbolic *mp = (gsl_matrix_symbolic*)x->data.mat.p;
      if (mp->size1 > 0 && mp->size2 > 0) {
	if (k >= 0 && mp->size2 != (size_t)k)
	  return 0;
	nrows += mp->size1; k = mp->size2;
	set_target_type(target, EXPR::MATRIX);
	have_matrix = true;
      }
      break;
    }
#ifdef HAVE_GSL
    case EXPR::DBL:
      set_target_type(target, EXPR::DMATRIX);
      if (k >= 0 && k != 1) return 0;
      nrows++; k = 1;
      break;
    case EXPR::INT:
      set_target_type(target, EXPR::IMATRIX);
      if (k >= 0 && k != 1) return 0;
      nrows++; k = 1;
      break;
    case EXPR::APP: {
      double a, b;
      if (k >= 0 && k != 1) return 0;
      nrows++; k = 1;
      if (get_complex(x, a, b))
	set_target_type(target, EXPR::CMATRIX);
      else
	set_target_type(target, EXPR::MATRIX);
      break;
    }
    case EXPR::DMATRIX: {
      gsl_matrix *mp = (gsl_matrix*)x->data.mat.p;
      if (mp->size1 > 0 && mp->size2 > 0) {
	if (k >= 0 && mp->size2 != (size_t)k)
	  return 0;
	nrows += mp->size1; k = mp->size2;
	set_target_type(target, EXPR::DMATRIX);
	have_matrix = true;
      }
      break;
    }
    case EXPR::CMATRIX: {
      gsl_matrix_complex *mp = (gsl_matrix_complex*)x->data.mat.p;
      if (mp->size1 > 0 && mp->size2 > 0) {
	if (k >= 0 && mp->size2 != (size_t)k)
	  return 0;
	nrows += mp->size1; k = mp->size2;
	set_target_type(target, EXPR::CMATRIX);
	have_matrix = true;
      }
      break;
    }
    case EXPR::IMATRIX: {
      gsl_matrix_int *mp = (gsl_matrix_int*)x->data.mat.p;
      if (mp->size1 > 0 && mp->size2 > 0) {
	if (k >= 0 && mp->size2 != (size_t)k)
	  return 0;
	nrows += mp->size1; k = mp->size2;
	set_target_type(target, EXPR::IMATRIX);
	have_matrix = true;
      }
      break;
    }
#endif
    default:
      if (k >= 0 && k != 1) return 0;
      nrows++; k = 1;
      set_target_type(target, EXPR::MATRIX);
      break;
    }
  }
  if (n == 1 && have_matrix) return xs[0];
  if (k < 0) k = 0;
  ncols = k;
  if (target == 0) target = EXPR::MATRIX;
  switch (target) {
  case EXPR::MATRIX:
    return symbolic_matrix_rows(nrows, ncols, n, xs);
#ifdef HAVE_GSL
  case EXPR::DMATRIX:
    return double_matrix_rows(nrows, ncols, n, xs);
  case EXPR::CMATRIX:
    return complex_matrix_rows(nrows, ncols, n, xs);
  case EXPR::IMATRIX:
    return int_matrix_rows(nrows, ncols, n, xs);
#endif
  default:
    return 0;
  }
}

extern "C"
pure_expr *pure_matrix_columnsl(uint32_t n, ...)
{
  va_list ap;
  pure_expr **xs = (pure_expr**)alloca(n*sizeof(pure_expr*));
  va_start(ap, n);
  for (size_t i = 0; i < n; i++)
    xs[i] = va_arg(ap, pure_expr*);
  va_end(ap);
  return pure_matrix_columnsv(n, xs);
}

extern "C"
pure_expr *pure_matrix_columnsv(uint32_t n, pure_expr **xs)
{
  int k = -1;
  size_t nrows = 0, ncols = 0;
  int32_t target = 0;
  bool have_matrix = false;
  for (size_t i = 0; i < n; i++) {
    pure_expr *x = xs[i];
    switch (x->tag) {
    case EXPR::MATRIX: {
      gsl_matrix_symbolic *mp = (gsl_matrix_symbolic*)x->data.mat.p;
      if (mp->size1 > 0 && mp->size2 > 0) {
	if (k >= 0 && mp->size1 != (size_t)k)
	  return 0;
	ncols += mp->size2; k = mp->size1;
	set_target_type(target, EXPR::MATRIX);
	have_matrix = true;
      }
      break;
    }
#ifdef HAVE_GSL
    case EXPR::DBL:
      set_target_type(target, EXPR::DMATRIX);
      if (k >= 0 && k != 1) return 0;
      ncols++; k = 1;
      break;
    case EXPR::INT:
      set_target_type(target, EXPR::IMATRIX);
      if (k >= 0 && k != 1) return 0;
      ncols++; k = 1;
      break;
    case EXPR::APP: {
      double a, b;
      if (k >= 0 && k != 1) return 0;
      ncols++; k = 1;
      if (get_complex(x, a, b))
	set_target_type(target, EXPR::CMATRIX);
      else
	set_target_type(target, EXPR::MATRIX);
      break;
    }
    case EXPR::DMATRIX: {
      gsl_matrix *mp = (gsl_matrix*)x->data.mat.p;
      if (mp->size1 > 0 && mp->size2 > 0) {
	if (k >= 0 && mp->size1 != (size_t)k)
	  return 0;
	ncols += mp->size2; k = mp->size1;
	set_target_type(target, EXPR::DMATRIX);
	have_matrix = true;
      }
      break;
    }
    case EXPR::CMATRIX: {
      gsl_matrix_complex *mp = (gsl_matrix_complex*)x->data.mat.p;
      if (mp->size1 > 0 && mp->size2 > 0) {
	if (k >= 0 && mp->size1 != (size_t)k)
	  return 0;
	ncols += mp->size2; k = mp->size1;
	set_target_type(target, EXPR::CMATRIX);
	have_matrix = true;
      }
      break;
    }
    case EXPR::IMATRIX: {
      gsl_matrix_int *mp = (gsl_matrix_int*)x->data.mat.p;
      if (mp->size1 > 0 && mp->size2 > 0) {
	if (k >= 0 && mp->size1 != (size_t)k)
	  return 0;
	ncols += mp->size2; k = mp->size1;
	set_target_type(target, EXPR::IMATRIX);
	have_matrix = true;
      }
      break;
    }
#endif
    default:
      if (k >= 0 && k != 1) return 0;
      ncols++; k = 1;
      set_target_type(target, EXPR::MATRIX);
      break;
    }
  }
  if (n == 1 && have_matrix) return xs[0];
  if (k < 0) k = 0;
  nrows = k;
  if (target == 0) target = EXPR::MATRIX;
  switch (target) {
  case EXPR::MATRIX:
    return symbolic_matrix_columns(nrows, ncols, n, xs);
#ifdef HAVE_GSL
  case EXPR::DMATRIX:
    return double_matrix_columns(nrows, ncols, n, xs);
  case EXPR::CMATRIX:
    return complex_matrix_columns(nrows, ncols, n, xs);
  case EXPR::IMATRIX:
    return int_matrix_columns(nrows, ncols, n, xs);
#endif
  default:
    return 0;
  }
}

extern "C"
pure_expr *pure_app(pure_expr *fun, pure_expr *arg)
{
  return pure_apply2(fun, arg);
}

extern "C"
pure_expr *pure_appl(pure_expr *fun, size_t argc, ...)
{
  if (argc == 0) return fun;
  va_list ap;
  va_start(ap, argc);
  pure_expr **args = (pure_expr**)alloca(argc*sizeof(pure_expr*));
  for (size_t i = 0; i < argc; i++)
    args[i] = va_arg(ap, pure_expr*);
  return pure_appv(fun, argc, args);
}

extern "C"
pure_expr *pure_appv(pure_expr *fun, size_t argc, pure_expr **args)
{
  pure_expr *y = fun;
  for (size_t i = 0; i < argc; i++)
    y = pure_apply2(y, args[i]);
  return y;
}

static inline pure_expr *mk_nil()
{
  interpreter& interp = *interpreter::g_interp;
  return pure_symbol(interp.symtab.nil_sym().f);
}

static inline pure_expr *mk_cons(pure_expr *x, pure_expr *y)
{
  interpreter& interp = *interpreter::g_interp;
  pure_expr *f = pure_const(interp.symtab.cons_sym().f);
  return pure_apply2(pure_apply2(f, x), y);
}

static inline pure_expr *mk_void()
{
  interpreter& interp = *interpreter::g_interp;
  return pure_symbol(interp.symtab.void_sym().f);
}

static inline pure_expr *mk_pair(pure_expr *x, pure_expr *y)
{
  interpreter& interp = *interpreter::g_interp;
  pure_expr *f = pure_symbol(interp.symtab.pair_sym().f);
  return pure_apply2(pure_apply2(f, x), y);
}

extern "C"
pure_expr *pure_listl(size_t size, ...)
{
  if (size == 0) return mk_nil();
  va_list ap;
  va_start(ap, size);
  pure_expr **elems = (pure_expr**)alloca(size*sizeof(pure_expr*));
  for (size_t i = 0; i < size; i++)
    elems[i] = va_arg(ap, pure_expr*);
  return pure_listv(size, elems);
}

extern "C"
pure_expr *pure_listv(size_t size, pure_expr **elems)
{
  pure_expr *y = mk_nil();
  for (size_t i = size; i-- > 0; )
    y = mk_cons(elems[i], y);
  return y;
}

extern "C"
pure_expr *pure_tuplel(size_t size, ...)
{
  if (size == 0) return mk_void();
  va_list ap;
  va_start(ap, size);
  pure_expr **elems = (pure_expr**)alloca(size*sizeof(pure_expr*));
  for (size_t i = 0; i < size; i++)
    elems[i] = va_arg(ap, pure_expr*);
  return pure_tuplev(size, elems);
}

extern "C"
pure_expr *pure_tuplev(size_t size, pure_expr **elems)
{
  if (size == 0) return mk_void();
  pure_expr *y = elems[--size];
  for (size_t i = size; i-- > 0; )
    y = mk_pair(elems[i], y);
  return y;
}

extern "C"
bool pure_is_symbol(const pure_expr *x, int32_t *sym)
{
  assert(x);
  if (x->tag >= 0) {
    if (sym) *sym = x->tag;
    return true;
  } else
    return false;
}

extern "C"
bool pure_is_int(const pure_expr *x, int32_t *i)
{
  assert(x);
  if (x->tag == EXPR::INT) {
    if (i) *i = x->data.i;
    return true;
  } else
    return false;
}

extern "C"
bool pure_is_mpz(const pure_expr *x, mpz_t *z)
{
  assert(x);
  if (x->tag == EXPR::BIGINT) {
    if (z) mpz_init_set(*z, x->data.z);
    return true;
  } else
    return false;
}

extern "C"
bool pure_is_double(const pure_expr *x, double *d)
{
  assert(x);
  if (x->tag == EXPR::DBL) {
    if (d) *d = x->data.d;
    return true;
  } else
    return false;
}

extern "C"
bool pure_is_pointer(const pure_expr *x, void **p)
{
  assert(x);
  if (x->tag == EXPR::PTR) {
    if (p) *p = x->data.p;
    return true;
  } else
    return false;
}

extern "C"
bool pure_is_string(const pure_expr *x, const char **s)
{
  assert(x);
  if (x->tag == EXPR::STR) {
    if (s) *s = x->data.s;
    return true;
  } else
    return false;
}

extern "C"
bool pure_is_string_dup(const pure_expr *x, char **s)
{
  assert(x);
  if (x->tag == EXPR::STR) {
    if (s) *s = strdup(x->data.s);
    return true;
  } else
    return false;
}

extern "C"
bool pure_is_cstring_dup(const pure_expr *x, char **s)
{
  assert(x);
  if (x->tag == EXPR::STR) {
    if (s) *s = fromutf8(x->data.s);
    return true;
  } else
    return false;
}

extern "C"
bool pure_is_symbolic_matrix(const pure_expr *x, void **p)
{
  if (x->tag == EXPR::MATRIX) {
    *p = x->data.mat.p;
    return true;
  } else
    return false;
}

extern "C"
bool pure_is_double_matrix(const pure_expr *x, void **p)
{
  if (x->tag == EXPR::DMATRIX) {
    *p = x->data.mat.p;
    return true;
  } else
    return false;
}

extern "C"
bool pure_is_complex_matrix(const pure_expr *x, void **p)
{
  if (x->tag == EXPR::CMATRIX) {
    *p = x->data.mat.p;
    return true;
  } else
    return false;
}

extern "C"
bool pure_is_int_matrix(const pure_expr *x, void **p)
{
  if (x->tag == EXPR::IMATRIX) {
    *p = x->data.mat.p;
    return true;
  } else
    return false;
}

extern "C"
bool pure_is_app(const pure_expr *x, pure_expr **fun, pure_expr **arg)
{
  assert(x);
  if (x->tag == EXPR::APP) {
    if (fun) *fun = x->data.x[0];
    if (arg) *arg = x->data.x[1];
    return true;
  } else
    return false;
}

extern "C"
bool pure_is_appv(pure_expr *x, pure_expr **_fun,
		  size_t *_argc, pure_expr ***_args)
{
  assert(x);
  pure_expr *u = x, *y, *z;
  size_t argc = 0;
  while (pure_is_app(u, &y, &z)) {
    argc++;
    u = y;
  }
  if (_fun) *_fun = u;
  if (_argc) *_argc = argc;
  if (_args) {
    if (argc > 0) {
      pure_expr **args = (pure_expr**)malloc(argc*sizeof(pure_expr*));
      assert(args);
      size_t i = argc;
      u = x;
      while (pure_is_app(u, &y, &z)) {
	args[--i] = z;
	u = y;
      }
      *_args = args;
    } else
      *_args = 0;
  }
  return true;
}

static inline bool is_nil(pure_expr *x)
{
  interpreter& interp = *interpreter::g_interp;
  return x->tag == interp.symtab.nil_sym().f;
}

static inline bool is_cons(pure_expr *x, pure_expr*& y, pure_expr*& z)
{
  interpreter& interp = *interpreter::g_interp;
  if (x->tag == EXPR::APP && x->data.x[0]->tag == EXPR::APP &&
      x->data.x[0]->data.x[0]->tag == interp.symtab.cons_sym().f) {
    y = x->data.x[0]->data.x[1];
    z = x->data.x[1];
    return true;
  } else
    return false;
}

static inline bool is_void(pure_expr *x)
{
  interpreter& interp = *interpreter::g_interp;
  return x->tag == interp.symtab.void_sym().f;
}

static inline bool is_pair(pure_expr *x, pure_expr*& y, pure_expr*& z)
{
  interpreter& interp = *interpreter::g_interp;
  if (x->tag == EXPR::APP && x->data.x[0]->tag == EXPR::APP &&
      x->data.x[0]->data.x[0]->tag == interp.symtab.pair_sym().f) {
    y = x->data.x[0]->data.x[1];
    z = x->data.x[1];
    return true;
  } else
    return false;
}

extern "C"
bool pure_is_listv(pure_expr *x, size_t *_size, pure_expr ***_elems)
{
  assert(x);
  pure_expr *u = x, *y, *z;
  size_t size = 0;
  while (is_cons(u, y, z)) {
    size++;
    u = z;
  }
  if (!is_nil(u)) return false;
  if (_size) *_size = size;
  if (_elems) {
    if (size > 0) {
      pure_expr **elems = (pure_expr**)malloc(size*sizeof(pure_expr*));
      assert(elems);
      size_t i = 0;
      u = x;
      while (is_cons(u, y, z)) {
	elems[i++] = y;
	u = z;
      }
      *_elems = elems;
    } else
      *_elems = 0;
  }
  return true;
}

extern "C"
bool pure_is_tuplev(pure_expr *x, size_t *_size, pure_expr ***_elems)
{
  assert(x);
  /* FIXME: This implementation assumes that tuples are right-recursive. If we
     change the tuple implementation in the prelude then this code has to be
     adapted accordingly. */
  pure_expr *u = x, *y, *z;
  size_t size = 1;
  if (is_void(x)) {
    if (_size) *_size = 0;
    if (_elems) *_elems = 0;
    return true;
  }
  while (is_pair(u, y, z)) {
    size++;
    u = z;
  }
  if (_size) *_size = size;
  if (_elems) {
    pure_expr **elems = (pure_expr**)malloc(size*sizeof(pure_expr*));
    assert(elems);
    size_t i = 0;
    u = x;
    while (is_pair(u, y, z)) {
      elems[i++] = y;
      u = z;
    }
    elems[i++] = u;
    *_elems = elems;
  }
  return true;
}

extern "C"
pure_expr *pure_complex(double c[2])
{
  return make_complex(c[0], c[1]);
}

extern "C"
bool pure_is_complex(pure_expr *x, double *c)
{
  double a, b;
  if (get_complex(x, a, b)) {
    if (c) {
      c[0] = a;
      c[1] = b;
    }
    return true;
  } else
    return false;
}

extern "C"
pure_expr *pure_new(pure_expr *x)
{
  return pure_new_internal(x);
}

extern "C"
void pure_free(pure_expr *x)
{
  pure_free_internal(x);
}

extern "C"
void pure_freenew(pure_expr *x)
{
  if (x->refc == 0)
    pure_free_internal(pure_new_internal(x));
}

extern "C"
void pure_ref(pure_expr *x)
{
  x->refc++;
}

extern "C"
void pure_unref(pure_expr *x)
{
  pure_unref_internal(x);
}

extern "C"
pure_expr *pure_sentry(pure_expr *sentry, pure_expr *x)
{
  if (x==0)
    return 0;
  else if (x->tag == EXPR::APP || x->tag == EXPR::PTR) {
    if (x->data.x[2])
      pure_free_internal(x->data.x[2]);
    x->data.x[2] = sentry?pure_new_internal(sentry):0;
    return x;
  } else
    return 0;
}

extern "C"
pure_expr *pure_get_sentry(pure_expr *x)
{
  return get_sentry(x);
}

extern "C"
pure_expr *pure_clear_sentry(pure_expr *x)
{
  return pure_sentry(0, x);
}

extern "C"
bool pure_let(int32_t sym, pure_expr *x)
{
  if (sym <= 0 || !x) return false;
  try {
    interpreter& interp = *interpreter::g_interp;
    interp.defn(sym, x);
    return true;
  } catch (err &e) {
    return false;
  }
}

extern "C"
bool pure_def(int32_t sym, pure_expr *x)
{
  if (sym <= 0 || !x) return false;
  try {
    interpreter& interp = *interpreter::g_interp;
    interp.const_defn(sym, x);
    return true;
  } catch (err &e) {
    return false;
  }
}

extern "C"
bool pure_clear(int32_t sym)
{
  if (sym > 0) {
    interpreter& interp = *interpreter::g_interp;
    interp.clear();
    return true;
  } else
    return false;
}

extern "C"
uint32_t pure_save()
{
  interpreter& interp = *interpreter::g_interp;
  if (interp.temp < 0xffffffffU)
    return ++interp.temp;
  else
    return 0;
}

extern "C"
uint32_t pure_restore()
{
  interpreter& interp = *interpreter::g_interp;
  uint32_t level = interp.temp;
  interp.clear();
  if (level > 0 && interp.temp > level-1) --interp.temp;
  return interp.temp;
}

extern "C"
pure_expr *pure_eval(const char *s)
{
  assert(s);
  interpreter& interp = *interpreter::g_interp;
  interp.errmsg.clear();
  pure_expr *res = interp.runstr(string(s)+";");
  interp.result = 0;
  if (res) {
    pure_unref_internal(res);
    return res;
  } else if (interp.errmsg.empty())
    return mk_void();
  else
    return 0;
}

extern "C"
char *pure_evalcmd(const char *s)
{
  assert(s);
  interpreter& interp = *interpreter::g_interp;
  ostream *l_output = interp.output;
  ostringstream sout;
  interp.errmsg.clear();
  interp.output = &sout;
  pure_expr *res = interp.runstr(string(s));
  interp.result = 0;
  interp.output = l_output;
  if (res)
    pure_free_internal(res);
  if (!interp.errmsg.empty() || sout.str().empty())
    return 0;
  else
    return strdup(sout.str().c_str());
}

#ifndef HOST
#define HOST "unknown"
#endif
#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "0.0"
#endif
#ifndef PURELIB
#define PURELIB "/usr/local/lib/pure-" PACKAGE_VERSION
#endif

#include <llvm/Target/TargetOptions.h>

#include <sys/types.h>
#include <sys/stat.h>

static inline bool chkfile(const string& s)
{
  struct stat st;
  return !stat(s.c_str(), &st) && !S_ISDIR(st.st_mode);
}

static void add_path(list<string>& dirs, const string& path)
{
  size_t pos = 0;
  while (pos != string::npos) {
#ifdef _WIN32
    size_t end = path.find(';', pos);
#else
    size_t end = path.find(':', pos);
#endif
    string s;
    if (end == string::npos) {
      s = path.substr(pos);
      pos = end;
    } else {
      s = path.substr(pos, end-pos);
      pos = end+1;
    }
    if (!s.empty()) {
      if (s[s.size()-1] != '/') s.append("/");
      dirs.push_back(s);
    }
  }
}

static string unixize(const string& s)
{
  string t = s;
#ifdef _WIN32
  for (size_t i = 0, n = t.size(); i<n; i++)
    if (t[i] == '\\')
      t[i] = '/';
#endif
  return t;
}

extern "C"
pure_interp *pure_create_interp(int argc, char *argv[])
{
  // This is pretty much the same as pure.cc:main(), except that some options
  // are ignored and there's no user interaction.
  char base;
  interpreter *_interp = new interpreter, &interp = *_interp;
  int count = 0;
  bool want_prelude = true, have_prelude = false;
  // We use some stuff which is not safe to call while another interpreter is
  // active, so we temporarily switch to the new interpreter now.
  interpreter *s_interp = interpreter::g_interp;
  interpreter::g_interp = _interp;
  // This is used in advisory stack checks.
  if (!interpreter::baseptr) interpreter::baseptr = &base;
  // get some settings from the environment
  const char *env;
  if ((env = getenv("HOME")))
    interp.histfile = string(env)+"/.pure_history";
  if ((env = getenv("PURE_PS")))
    interp.ps = string(env);
  if ((env = getenv("PURE_STACK"))) {
    char *end;
    size_t n = strtoul(env, &end, 0);
    if (!*end) interpreter::stackmax = n*1024;
  }
  if ((env = getenv("PURELIB"))) {
    string s = unixize(env);
    if (!s.empty() && s[s.size()-1] != '/') s.append("/");
    interp.libdir = s;
  } else
    interp.libdir = string(PURELIB)+"/";
  string prelude = interp.libdir+string("prelude.pure");
#if USE_FASTCC
  // This global option is needed to get tail call optimization (you'll also
  // need to have USE_FASTCC in interpreter.hh enabled).
  llvm::PerformTailCallOpt = true;
#endif
#if defined(HAVE_GSL) && DEBUG<2
  // Turn off GSL's own error handler which aborts the program.
  gsl_set_error_handler_off();
#endif
  // scan the command line options
  list<string> myargs;
  if (argv && *argv) for (char **args = ++argv; *args; ++args)
    if (*args == string("-h") || *args == string("--help"))
      /* ignored */;
    else if (*args == string("--version"))
      /* ignored */;
    else if (*args == string("-c"))
      /* ignored */;
    else if (*args == string("-i"))
      /* ignored */;
    else if (*args == string("-n") || *args == string("--noprelude"))
      want_prelude = false;
    else if (*args == string("--norc"))
      /* ignored */;
    else if (*args == string("--noediting"))
      /* ignored */;
    else if (*args == string("-q"))
      /* ignored */;
    else if (string(*args).substr(0,2) == "-o") {
      string s = string(*args).substr(2);
      if (s.empty()) {
	if (!*++args) {
	  cerr << "pure_create_interp: -o lacks filename argument\n";
	  delete _interp;
	  return 0;
	}
      }
      /* ignored */
    } else if (string(*args).substr(0,2) == "-I") {
      string s = string(*args).substr(2);
      if (s.empty()) {
	if (!*++args) {
	  cerr << "pure_create_interp: -I lacks directory argument\n";
	  delete _interp;
	  return 0;
	}
	s = *args;
      }
      s = unixize(s);
      if (!s.empty()) {
	if (s[s.size()-1] != '/') s.append("/");
	interp.includedirs.push_back(s);
      }
    } else if (string(*args).substr(0,2) == "-L") {
      string s = string(*args).substr(2);
      if (s.empty()) {
	if (!*++args) {
	  cerr << "pure_create_interp: -L lacks directory argument\n";
	  delete _interp;
	  return 0;
	}
	s = *args;
      }
      s = unixize(s);
      if (!s.empty()) {
	if (s[s.size()-1] != '/') s.append("/");
	interp.librarydirs.push_back(s);
      }
    } else if (string(*args).substr(0,2) == "-v") {
      string s = string(*args).substr(2);
      if (s.empty()) continue;
      char *end;
      strtoul(s.c_str(), &end, 0);
      if (*end) {
	cerr << "pure_create_interp: invalid option " << *args << endl;
	delete _interp;
	return 0;
      }
    } else if (*args == string("-x")) {
      while (*++args) myargs.push_back(*args);
      break;
    } else if (*args == string("--")) {
      while (*++args) myargs.push_back(*args);
      break;
    } else if (**args == '-') {
      cerr << "pure_create_interp: invalid option " << *args << endl;
      delete _interp;
      return 0;
    }
  if ((env = getenv("PURE_INCLUDE")))
    add_path(interp.includedirs, unixize(env));
  if ((env = getenv("PURE_LIBRARY")))
    add_path(interp.librarydirs, unixize(env));
  interp.init_sys_vars(PACKAGE_VERSION, HOST, myargs);
  if (want_prelude) {
    // load the prelude if we can find it
    if (chkfile(prelude)) {
      have_prelude = true;
      try { interp.run(prelude, false); } catch (err &e) {
	cerr << "pure_create_interp: " << e.what() << endl;
	delete _interp;
	return 0;
      }
      interp.compile();
    }
  }
  // load scripts specified on the command line
  if (argv) for (; *argv; ++argv)
    if (string(*argv).substr(0,2) == "-v") {
      uint8_t level = 1;
      string s = string(*argv).substr(2);
      if (!s.empty()) level = (uint8_t)strtoul(s.c_str(), 0, 0);
      interp.verbose = level;
    } else if (*argv == string("-x")) {
      if (*++argv) {
	count++; interp.modname = *argv;
	try { interp.run(*argv, false); } catch (err &e) {
	  cerr << "pure_create_interp: " << e.what() << endl;
	  delete _interp;
	  return 0;
	}
      } else {
	cerr << "pure_create_interp: missing script name\n";
	delete _interp;
	return 0;
      }
      break;
    } else if (*argv == string("--"))
      break;
    else if (string(*argv).substr(0,2) == "-o" ||
	     string(*argv).substr(0,2) == "-I" ||
	     string(*argv).substr(0,2) == "-L") {
      string s = string(*argv).substr(2);
      if (s.empty()) ++argv;
    } else if (**argv == '-')
      ;
    else if (**argv) {
      if (count++ == 0) interp.modname = *argv;
      try { interp.run(*argv, false); } catch (err &e) {
	cerr << "pure_create_interp: " << e.what() << endl;
	delete _interp;
	return 0;
      }
    }
  interp.symtab.init_builtins();
  if (s_interp) interpreter::g_interp = s_interp;
  return (pure_interp*)_interp;
}

extern "C"
void pure_delete_interp(pure_interp *interp)
{
  assert(interp);
  interpreter *_interp = (interpreter*)interp;
  delete _interp;
}

extern "C"
void pure_switch_interp(pure_interp *interp)
{
  assert(interp);
  interpreter::g_interp = (interpreter*)interp;
}

extern "C"
pure_interp *pure_current_interp()
{
  return (pure_interp*)interpreter::g_interp;
}

extern "C"
void pure_interp_compile(pure_interp *interp)
{
  assert(interp);
  interpreter *_interp = (interpreter*)interp;
  _interp->compile();
  llvm::Module *module = _interp->module;
  for (llvm::Module::iterator it = module->begin(), end = module->end();
       it != end; ++it) {
    llvm::Function *fn = &*it;
    if (!fn->isDeclaration()) _interp->JIT->getPointerToFunction(fn);
  }
}

/* END OF PUBLIC API. *******************************************************/

extern "C"
pure_interp *pure_interp_main(int argc, char *argv[],
			      int32_t nsyms, char *syms,
			      pure_expr ***vars, void **vals, int32_t *arities,
			      pure_expr ***sstk, void **fptr)
{
#if 0
  std::cout << "\n** argc = " << argc << endl;
  for (int i = 0; i < argc; i++)
    std::cout << "argv[" << i << "] = '"
	      << (argv[i]?argv[i]:"(null)") << "'\n";
  std::cout << "\n** " << nsyms << " symbols\n";
  std::cout << syms;
  std::cout << "\n** vars table " << vars << endl;
  for (int32_t f = 1; f < nsyms; f++)
    if (vars[f])
      std::cout << f << " = " << vars[f] << endl;
  std::cout << "\n** vals table " << vals << endl;
  for (int32_t f = 1; f < nsyms; f++)
    if (vals[f])
      std::cout << f << " = " << vals[f] << " (" << arities[f] << ")\n";
  std::cout << "\n** sstk = " << sstk << ", fptr = " << fptr << endl;
#endif
  char base;
  interpreter *_interp =
    new interpreter(nsyms, syms, vars, vals, arities, sstk, fptr),
    &interp = *_interp;
  interpreter::g_interp = _interp;
  if (!interpreter::baseptr) interpreter::baseptr = &base;
  // get some settings from the environment
  const char *env;
  if ((env = getenv("PURE_STACK"))) {
    char *end;
    size_t n = strtoul(env, &end, 0);
    if (!*end) interpreter::stackmax = n*1024;
  }
  if ((env = getenv("PURELIB"))) {
    string s = unixize(env);
    if (!s.empty() && s[s.size()-1] != '/') s.append("/");
    interp.libdir = s;
  } else
    interp.libdir = string(PURELIB)+"/";
  if ((env = getenv("PURE_INCLUDE")))
    add_path(interp.includedirs, unixize(env));
  if ((env = getenv("PURE_LIBRARY")))
    add_path(interp.librarydirs, unixize(env));
#if USE_FASTCC
  // This global option is needed to get tail call optimization (you'll also
  // need to have USE_FASTCC in interpreter.hh enabled).
  llvm::PerformTailCallOpt = true;
#endif
#if defined(HAVE_GSL) && DEBUG<2
  // Turn off GSL's own error handler which aborts the program.
  gsl_set_error_handler_off();
#endif
  // scan the command line options
  list<string> myargs;
  for (char **args = argv; *args; ++args)
    myargs.push_back(*args);
  interp.symtab.init_builtins();
  interp.init_sys_vars(PACKAGE_VERSION, HOST, myargs);
  return (pure_interp*)_interp;
}

extern "C"
pure_expr *pure_const(int32_t tag)
{
  // XXXFIXME: We should cache these on a per interpreter basis, so that only
  // a single expression node exists for each symbol.
  pure_expr *x = new_expr();
  x->tag = tag;
  x->data.clos = 0;
  MEMDEBUG_NEW(x)
  return x;
}

extern "C"
pure_expr *pure_clos(bool local, bool thunked, int32_t tag, uint32_t n,
		     void *f, void *e, uint32_t m, /* m x pure_expr* */ ...)
{
  // Parameterless closures are always thunked, otherwise they would already
  // have been executed.
  if (n==0) thunked = true;
  pure_expr *x = new_expr();
  x->tag = tag;
  x->data.clos = new pure_closure;
  x->data.clos->local = local;
  x->data.clos->thunked = thunked;
  x->data.clos->n = n;
  x->data.clos->m = m;
  x->data.clos->fp = f;
  x->data.clos->ep = e;
  if (e) ((Env*)e)->refc++;
  if (m == 0)
    x->data.clos->env = 0;
  else {
    x->data.clos->env = new pure_expr*[m];
    va_list ap;
    va_start(ap, m);
    for (size_t i = 0; i < m; i++) {
      x->data.clos->env[i] = va_arg(ap, pure_expr*);
      assert(x->data.clos->env[i]->refc > 0);
    }
    va_end(ap);
  }
  MEMDEBUG_NEW(x)
  return x;
}

extern "C"
pure_expr *pure_long(int64_t l)
{
  int sgn = (l>0)?1:(l<0)?-1:0;
  uint64_t v = (uint64_t)(l>=0?l:-l);
  if (sizeof(mp_limb_t) == 8) {
    // 8 byte limbs, value fits in a single limb.
    limb_t u[1] = { v };
    return pure_bigint(sgn, u);
  } else {
    // 4 byte limbs, put least significant word in the first limb.
    limb_t u[2] = { (uint32_t)v, (uint32_t)(v>>32) };
    return pure_bigint(sgn+sgn, u);
  }
}

static void make_bigint(mpz_t z, int32_t size, const limb_t *limbs)
{
  // FIXME: For efficiency, we poke directly into the mpz struct here, this
  // might need to be reviewed for future GMP revisions.
  int sz = size>=0?size:-size, sgn = size>0?1:size<0?-1:0, sz0 = 0;
  // normalize: the most significant limb should be nonzero
  for (int i = 0; i < sz; i++) if (limbs[i] != 0) sz0 = i+1;
  sz = sz0; size = sgn*sz;
  mpz_init(z);
  if (sz > 0) _mpz_realloc(z, sz);
  assert(sz == 0 || z->_mp_d);
  for (int i = 0; i < sz; i++) z->_mp_d[i] = limbs[i];
  z->_mp_size = size;
}

extern "C"
pure_expr *pure_bigint(int32_t size, const limb_t *limbs)
{
  pure_expr *x = new_expr();
  x->tag = EXPR::BIGINT;
  make_bigint(x->data.z, size, limbs);
  MEMDEBUG_NEW(x)
  return x;
}

extern "C"
int32_t pure_cmp_bigint(pure_expr *x, int32_t size, const limb_t *limbs)
{
  assert(x && x->tag == EXPR::BIGINT);
  mpz_t z;
  make_bigint(z, size, limbs);
  int res = mpz_cmp(x->data.z, z);
  mpz_clear(z);
  return res;
}

extern "C"
int32_t pure_cmp_string(pure_expr *x, const char *s)
{
  assert(x && x->tag == EXPR::STR);
  return strcmp(x->data.s, s);
}

list<char*> temps; // XXXFIXME: This should be TLD.

char *pure_get_cstring(pure_expr *x)
{
  assert(x && x->tag == EXPR::STR);
  char *s = fromutf8(x->data.s, 0);
  assert(s);
  temps.push_back(s);
  return s;
}

extern "C"
void pure_free_cstrings()
{
  for (list<char*>::iterator t = temps.begin(); t != temps.end(); t++)
    if (*t) free(*t);
  temps.clear();
}

extern "C"
int64_t pure_get_long(pure_expr *x)
{
  uint64_t v =
    (sizeof(mp_limb_t) == 8) ? (uint64_t)mpz_getlimbn(x->data.z, 0) :
    (mpz_getlimbn(x->data.z, 0) +
     (((uint64_t)mpz_getlimbn(x->data.z, 1))<<32));
  return (mpz_sgn(x->data.z) < 0) ? -(int64_t)v : (int64_t)v;
}

extern "C"
int32_t pure_get_int(pure_expr *x)
{
  uint32_t v =
    (sizeof(mp_limb_t) == 8) ? (uint32_t)(uint64_t)mpz_getlimbn(x->data.z, 0) :
    mpz_getlimbn(x->data.z, 0);
  return (mpz_sgn(x->data.z) < 0) ? -(int32_t)v : (int32_t)v;
}

extern "C"
void *pure_get_bigint(pure_expr *x)
{
  assert(x && x->tag == EXPR::BIGINT);
  return &x->data.z;
}

extern "C"
void *pure_get_matrix(pure_expr *x)
{
  assert(x && x->tag == EXPR::MATRIX || x->tag == EXPR::DMATRIX ||
	 x->tag == EXPR::CMATRIX || x->tag == EXPR::IMATRIX);
  return x->data.mat.p;
}

extern "C"
void *pure_get_matrix_data(pure_expr *x)
{
  switch (x->tag) {
  case EXPR::MATRIX: {
    gsl_matrix_symbolic *m = (gsl_matrix_symbolic*)x->data.mat.p;
    return m->data;
  }
#ifdef HAVE_GSL
  case EXPR::DMATRIX: {
    gsl_matrix *m = (gsl_matrix*)x->data.mat.p;
    return m->data;
  }
  case EXPR::CMATRIX: {
    gsl_matrix_complex *m = (gsl_matrix_complex*)x->data.mat.p;
    return m->data;
  }
  case EXPR::IMATRIX:{
    gsl_matrix_int *m = (gsl_matrix_int*)x->data.mat.p;
    return m->data;
  }
#endif
  default:
    assert(0 && "not a matrix");
    return 0;
  }
}

list<void*> cvector_temps; // XXXFIXME: This should be TLD.

void *pure_get_matrix_data_byte(pure_expr *x)
{
  void *v = matrix_to_byte_array(0, x);
  cvector_temps.push_back(v);
  return v;
}

void *pure_get_matrix_data_short(pure_expr *x)
{
  void *v = matrix_to_short_array(0, x);
  cvector_temps.push_back(v);
  return v;
}

void *pure_get_matrix_data_int(pure_expr *x)
{
  void *v = matrix_to_int_array(0, x);
  cvector_temps.push_back(v);
  return v;
}

void *pure_get_matrix_data_float(pure_expr *x)
{
  void *v = matrix_to_float_array(0, x);
  cvector_temps.push_back(v);
  return v;
}

void *pure_get_matrix_data_double(pure_expr *x)
{
  void *v = matrix_to_double_array(0, x);
  cvector_temps.push_back(v);
  return v;
}

extern "C"
void pure_free_cvectors()
{
  for (list<void*>::iterator t = cvector_temps.begin();
       t != cvector_temps.end(); t++)
    if (*t) free(*t);
  cvector_temps.clear();
}

extern "C"
pure_expr *pure_matrix_rows(uint32_t n, ...)
{
  va_list ap;
  pure_expr **xs = (pure_expr**)alloca(n*sizeof(pure_expr*));
  int k = -1;
  size_t nrows = 0, ncols = 0;
  int32_t target = 0;
  bool have_matrix = false;
  pure_expr *x = 0;
  va_start(ap, n);
  for (size_t i = 0; i < n; i++)
    xs[i] = va_arg(ap, pure_expr*);
  va_end(ap);
  for (size_t i = 0; i < n; i++) {
    x = xs[i];
    switch (x->tag) {
    case EXPR::MATRIX: {
      gsl_matrix_symbolic *mp = (gsl_matrix_symbolic*)x->data.mat.p;
      if (mp->size1 > 0 && mp->size2 > 0) {
	if (k >= 0 && mp->size2 != (size_t)k)
	  goto err;
	nrows += mp->size1; k = mp->size2;
	set_target_type(target, EXPR::MATRIX);
	have_matrix = true;
      }
      break;
    }
#ifdef HAVE_GSL
    case EXPR::DBL:
      set_target_type(target, EXPR::DMATRIX);
      if (k >= 0 && k != 1) goto err;
      nrows++; k = 1;
      break;
    case EXPR::INT:
      set_target_type(target, EXPR::IMATRIX);
      if (k >= 0 && k != 1) goto err;
      nrows++; k = 1;
      break;
    case EXPR::APP: {
      double a, b;
      if (k >= 0 && k != 1) goto err;
      nrows++; k = 1;
      if (get_complex(x, a, b))
	set_target_type(target, EXPR::CMATRIX);
      else
	set_target_type(target, EXPR::MATRIX);
      break;
    }
    case EXPR::DMATRIX: {
      gsl_matrix *mp = (gsl_matrix*)x->data.mat.p;
      if (mp->size1 > 0 && mp->size2 > 0) {
	if (k >= 0 && mp->size2 != (size_t)k)
	  goto err;
	nrows += mp->size1; k = mp->size2;
	set_target_type(target, EXPR::DMATRIX);
	have_matrix = true;
      }
      break;
    }
    case EXPR::CMATRIX: {
      gsl_matrix_complex *mp = (gsl_matrix_complex*)x->data.mat.p;
      if (mp->size1 > 0 && mp->size2 > 0) {
	if (k >= 0 && mp->size2 != (size_t)k)
	  goto err;
	nrows += mp->size1; k = mp->size2;
	set_target_type(target, EXPR::CMATRIX);
	have_matrix = true;
      }
      break;
    }
    case EXPR::IMATRIX: {
      gsl_matrix_int *mp = (gsl_matrix_int*)x->data.mat.p;
      if (mp->size1 > 0 && mp->size2 > 0) {
	if (k >= 0 && mp->size2 != (size_t)k)
	  goto err;
	nrows += mp->size1; k = mp->size2;
	set_target_type(target, EXPR::IMATRIX);
	have_matrix = true;
      }
      break;
    }
#endif
    default:
      if (k >= 0 && k != 1) goto err;
      nrows++; k = 1;
      set_target_type(target, EXPR::MATRIX);
      break;
    }
  }
  if (n == 1 && have_matrix) return xs[0];
  if (k < 0) k = 0;
  ncols = k;
  if (target == 0) target = EXPR::MATRIX;
  switch (target) {
  case EXPR::MATRIX:
    return symbolic_matrix_rows(nrows, ncols, n, xs);
#ifdef HAVE_GSL
  case EXPR::DMATRIX:
    return double_matrix_rows(nrows, ncols, n, xs);
  case EXPR::CMATRIX:
    return complex_matrix_rows(nrows, ncols, n, xs);
  case EXPR::IMATRIX:
    return int_matrix_rows(nrows, ncols, n, xs);
#endif
  default:
    assert(0 && "this can't happen");
    return 0;
  }
 err:
  /* This is called without a shadow stack frame, so we do our own cleanup
     here to avoid having temporaries hanging around indefinitely. */
  if (x) x->refc++;
  pure_new_vect(n, xs);
  for (size_t i = 0; i < n; i++)
    pure_free_internal(xs[i]);
  pure_unref_internal(x);
  pure_throw(bad_matrix_exception(x));
  return 0;
}

extern "C"
pure_expr *pure_matrix_columns(uint32_t n, ...)
{
  va_list ap;
  pure_expr **xs = (pure_expr**)alloca(n*sizeof(pure_expr*));
  int k = -1;
  size_t nrows = 0, ncols = 0;
  int32_t target = 0;
  bool have_matrix = false;
  pure_expr *x = 0;
  va_start(ap, n);
  for (size_t i = 0; i < n; i++)
    xs[i] = va_arg(ap, pure_expr*);
  va_end(ap);
  for (size_t i = 0; i < n; i++) {
    x = xs[i];
    switch (x->tag) {
    case EXPR::MATRIX: {
      gsl_matrix_symbolic *mp = (gsl_matrix_symbolic*)x->data.mat.p;
      if (mp->size1 > 0 && mp->size2 > 0) {
	if (k >= 0 && mp->size1 != (size_t)k)
	  goto err;
	ncols += mp->size2; k = mp->size1;
	set_target_type(target, EXPR::MATRIX);
	have_matrix = true;
      }
      break;
    }
#ifdef HAVE_GSL
    case EXPR::DBL:
      set_target_type(target, EXPR::DMATRIX);
      if (k >= 0 && k != 1) goto err;
      ncols++; k = 1;
      break;
    case EXPR::INT:
      set_target_type(target, EXPR::IMATRIX);
      if (k >= 0 && k != 1) goto err;
      ncols++; k = 1;
      break;
    case EXPR::APP: {
      double a, b;
      if (k >= 0 && k != 1) goto err;
      ncols++; k = 1;
      if (get_complex(x, a, b))
	set_target_type(target, EXPR::CMATRIX);
      else
	set_target_type(target, EXPR::MATRIX);
      break;
    }
    case EXPR::DMATRIX: {
      gsl_matrix *mp = (gsl_matrix*)x->data.mat.p;
      if (mp->size1 > 0 && mp->size2 > 0) {
	if (k >= 0 && mp->size1 != (size_t)k)
	  goto err;
	ncols += mp->size2; k = mp->size1;
	set_target_type(target, EXPR::DMATRIX);
	have_matrix = true;
      }
      break;
    }
    case EXPR::CMATRIX: {
      gsl_matrix_complex *mp = (gsl_matrix_complex*)x->data.mat.p;
      if (mp->size1 > 0 && mp->size2 > 0) {
	if (k >= 0 && mp->size1 != (size_t)k)
	  goto err;
	ncols += mp->size2; k = mp->size1;
	set_target_type(target, EXPR::CMATRIX);
	have_matrix = true;
      }
      break;
    }
    case EXPR::IMATRIX: {
      gsl_matrix_int *mp = (gsl_matrix_int*)x->data.mat.p;
      if (mp->size1 > 0 && mp->size2 > 0) {
	if (k >= 0 && mp->size1 != (size_t)k)
	  goto err;
	ncols += mp->size2; k = mp->size1;
	set_target_type(target, EXPR::IMATRIX);
	have_matrix = true;
      }
      break;
    }
#endif
    default:
      if (k >= 0 && k != 1) goto err;
      ncols++; k = 1;
      set_target_type(target, EXPR::MATRIX);
      break;
    }
  }
  if (n == 1 && have_matrix) return xs[0];
  if (k < 0) k = 0;
  nrows = k;
  if (target == 0) target = EXPR::MATRIX;
  switch (target) {
  case EXPR::MATRIX:
    return symbolic_matrix_columns(nrows, ncols, n, xs);
#ifdef HAVE_GSL
  case EXPR::DMATRIX:
    return double_matrix_columns(nrows, ncols, n, xs);
  case EXPR::CMATRIX:
    return complex_matrix_columns(nrows, ncols, n, xs);
  case EXPR::IMATRIX:
    return int_matrix_columns(nrows, ncols, n, xs);
#endif
  default:
    assert(0 && "this can't happen");
    return 0;
  }
 err:
  /* This is called without a shadow stack frame, so we do our own cleanup
     here to avoid having temporaries hanging around indefinitely. */
  if (x) x->refc++;
  pure_new_vect(n, xs);
  for (size_t i = 0; i < n; i++)
    pure_free_internal(xs[i]);
  pure_unref_internal(x);
  pure_throw(bad_matrix_exception(x));
  return 0;
}

extern "C"
pure_expr *pure_call(pure_expr *x)
{
  char test;
  assert(x);
  if (x->tag > 0 && x->data.clos && x->data.clos->n == 0) {
    void *fp = x->data.clos->fp;
#if DEBUG>1
    cerr << "pure_call: calling " << x << " -> " << fp << endl;
#endif
    assert(x->refc > 0 && !x->data.clos->local);
    // parameterless call
    checkall(test);
    return ((pure_expr*(*)())fp)();
  } else {
#if DEBUG>2
    if (x->tag >= 0 && x->data.clos)
      cerr << "pure_call: returning " << x << " -> " << x->data.clos->fp
	   << " (" << x->data.clos->n << " args)" << endl;
    else
      cerr << "pure_call: returning " << x << endl;
#endif
    return x;
  }
}

static inline void resize_sstk(pure_expr**& sstk, size_t& cap,
			       size_t sz, size_t n)
{
  size_t newsz = sz+n;
  if (newsz > cap) {
    while (newsz > cap) {
      assert((cap << 1) > cap);
      cap = cap << 1;
    }
    sstk = (pure_expr**)realloc(sstk, cap*sizeof(pure_expr*));
    assert(sstk);
  }
}

#define is_thunk(x) ((x)->tag == 0 && (x)->data.clos && (x)->data.clos->n == 0)

extern "C"
pure_expr *pure_force(pure_expr *x)
{
  char test;
  assert(x);
  if (is_thunk(x)) {
    // parameterless anonymous closure (thunk)
    assert(x->data.clos->thunked);
    pure_expr *ret;
    interpreter& interp = *interpreter::g_interp;
    void *fp = x->data.clos->fp;
    size_t m = x->data.clos->m;
    uint32_t env = 0;
    assert(x->refc > 0);
    // construct a stack frame for the function call
    if (m>0) {
      size_t sz = interp.sstk_sz;
      resize_sstk(interp.sstk, interp.sstk_cap, sz, m+1);
      pure_expr **sstk = interp.sstk;
      env = sz+1;
      sstk[sz++] = 0;
      for (size_t j = 0; j < m; j++) {
	sstk[sz++] = x->data.clos->env[j];
	assert(x->data.clos->env[j]->refc > 0);
	x->data.clos->env[j]->refc++;
      }
#if SSTK_DEBUG
      cerr << "++ stack: (sz = " << sz << ")\n";
      for (size_t i = 0; i < sz; i++) {
	pure_expr *x = sstk[i];
	if (i == interp.sstk_sz) cerr << "** pushed:\n";
	if (x)
	  cerr << i << ": " << (void*)x << ": " << x << endl;
	else
	  cerr << i << ": " << "** frame **\n";
      }
#endif
      interp.sstk_sz = sz;
    }
#if DEBUG>1
    cerr << "pure_force: calling " << x << " -> " << fp << endl;
    for (size_t j = 0; j < m; j++)
      cerr << "env#" << j << " = " << x->data.clos->env[j] << " -> " << (void*)x->data.clos->env[j] << ", refc = " << x->data.clos->env[j]->refc << endl;
#endif
    // parameterless call
    checkall(test);
    if (m>0)
      ret = ((pure_expr*(*)(uint32_t))fp)(env);
    else
      ret = ((pure_expr*(*)())fp)();
#if DEBUG>1
    cerr << "pure_force: result " << x << " = " << ret << " -> " << (void*)ret << ", refc = " << ret->refc << endl;
#endif
    // check whether the result is again a thunk, then we have to evaluate
    // that recursively
    if (is_thunk(ret))
      pure_force(pure_new_internal(ret));
    pure_new_internal(ret);
    // memoize the result
    assert(x!=ret);
    if (x->data.clos->ep)
      /* XXXKLUDGE: This thunk was created inside a global variable
	 definition. Unlink the thunk from its parent environment (effectively
	 making the environment permanent), so that we don't leave dangling
	 pointers to the thunk procedure, which are bound to cause segfaults
	 later. (This leaks memory on the parent environment if the variable
	 is cleared later, but since this can only happen on the interactive
	 command line, that's a minor issue.) */
      x->data.clos->ep = 0;
    pure_free_clos(x);
    x->tag = ret->tag;
    x->data = ret->data;
    switch (x->tag) {
    case EXPR::APP:
      pure_new_internal(x->data.x[0]);
      pure_new_internal(x->data.x[1]);
    case EXPR::PTR:
      if (x->data.x[2]) pure_new_internal(x->data.x[2]);
      break;
    case EXPR::STR:
      x->data.s = strdup(x->data.s);
      break;
    default:
      if (x->tag >= 0 && x->data.clos)
	x->data.clos = pure_copy_clos(x->data.clos);
      break;
    }
    pure_free_internal(ret);
    return x;
  } else {
#if DEBUG>2
    if (x->tag >= 0 && x->data.clos)
      cerr << "pure_force: returning " << x << " -> " << x->data.clos->fp
	   << " (" << x->data.clos->n << " args)" << endl;
    else
      cerr << "pure_force: returning " << x << endl;
#endif
    return x;
  }
}

extern "C"
pure_expr *pure_apply(pure_expr *x, pure_expr *y)
{
  char test;
  assert(x && y && x->refc > 0 && y->refc > 0);
  // if the function in this call is a thunk, evaluate it now
  if (is_thunk(x)) pure_force(x);
  // travel down the spine, count arguments
  pure_expr *f = x, *f0, *ret;
  uint32_t n = 1;
  while (f->tag == EXPR::APP) { f = f->data.x[0]; n++; }
  f0 = f;
  if (f->tag >= 0 && f->data.clos && !f->data.clos->thunked &&
      f->data.clos->n == n) {
    // saturated call; execute it now
    interpreter& interp = *interpreter::g_interp;
    void *fp = f->data.clos->fp;
    size_t m = f->data.clos->m;
    uint32_t env = 0;
    void **argv = (void**)alloca(n*sizeof(void*));
    assert(argv && "pure_apply: stack overflow");
    assert(n <= MAXARGS && "pure_apply: function call exceeds maximum #args");
    assert(f->data.clos->local || m == 0);
    // collect arguments
    f = x;
    for (size_t j = 1; f->tag == EXPR::APP; j++, f = f->data.x[0]) {
      assert(f->data.x[1]->refc > 0);
      argv[n-1-j] = f->data.x[1]; f->data.x[1]->refc++;
    }
    argv[n-1] = y;
    // make sure that we do not gc the function before calling it
    f0->refc++; pure_free_internal(x);
    // first push the function object on the shadow stack so that it's
    // garbage-collected in case of an exception
    resize_sstk(interp.sstk, interp.sstk_cap, interp.sstk_sz, n+m+2);
    interp.sstk[interp.sstk_sz++] = f0;
    // construct a stack frame for the function call
    {
      size_t sz = interp.sstk_sz;
      resize_sstk(interp.sstk, interp.sstk_cap, sz, n+m+1);
      pure_expr **sstk = interp.sstk;
      if (m>0) env = sz+n+1;
      sstk[sz++] = 0;
      for (size_t j = 0; j < n; j++)
	sstk[sz++] = (pure_expr*)argv[j];
      for (size_t j = 0; j < m; j++) {
	sstk[sz++] = f0->data.clos->env[j];
	assert(f0->data.clos->env[j]->refc > 0);
	f0->data.clos->env[j]->refc++;
      }
#if SSTK_DEBUG
      cerr << "++ stack: (sz = " << sz << ")\n";
      for (size_t i = 0; i < sz; i++) {
	pure_expr *x = sstk[i];
	if (i == interp.sstk_sz) cerr << "** pushed:\n";
	if (x)
	  cerr << i << ": " << (void*)x << ": " << x << endl;
	else
	  cerr << i << ": " << "** frame **\n";
      }
#endif
      interp.sstk_sz = sz;
    }
#if DEBUG>1
    cerr << "pure_apply: calling " << f0 << " -> " << fp << endl;
    for (size_t j = 0; j < n; j++)
      cerr << "arg#" << j << " = " << (pure_expr*)argv[j] << " -> " << argv[j] << ", refc = " << ((pure_expr*)argv[j])->refc << endl;
    for (size_t j = 0; j < m; j++)
      cerr << "env#" << j << " = " << f0->data.clos->env[j] << " -> " << (void*)f0->data.clos->env[j] << ", refc = " << f0->data.clos->env[j]->refc << endl;
#endif
    checkall(test);
    if (m>0)
      xfuncall(ret, fp, n, env, argv)
    else
      funcall(ret, fp, n, argv)
#if DEBUG>1
	cerr << "pure_apply: result " << f0 << " = " << ret << " -> " << (void*)ret << ", refc = " << ret->refc << endl;
#endif
    /* Temporarily increase the reference count on the result in case it comes
       from the environment, so that we don't gc it with the function object.
       We only need to do this if the result isn't a temporary and we actually
       have an environment. */
    {
      bool keep = m>0 && ret->refc>0;
      if (keep) ret->refc++;
      // pop the function object from the shadow stack
      pure_free_internal(interp.sstk[--interp.sstk_sz]);
      if (keep) pure_unref_internal(ret);
    }
    return ret;
  } else {
    // construct a literal application node
    f = new_expr();
    f->tag = EXPR::APP;
    f->data.x[0] = x;
    f->data.x[1] = y;
    MEMDEBUG_NEW(f)
    return f;
  }
}

extern "C"
void pure_throw(pure_expr* e)
{
  interpreter::brkflag = 0;
  interpreter& interp = *interpreter::g_interp;
  if (interp.estk.empty())
    abort(); // no exception handler, bail out
  else {
    interp.estk.front().e = e;
    longjmp(interp.estk.front().jmp, 1);
  }
}

#include <signal.h>

extern "C"
void pure_sigfpe(void)
{
  pure_throw(signal_exception(SIGFPE));
}

static void sig_handler(int sig)
{
  interpreter::brkflag = sig;
}

extern "C"
void pure_trap(int32_t action, int32_t sig)
{
  if (action > 0)
    signal(sig, sig_handler);
  else if (action < 0)
    signal(sig, SIG_IGN);
  else
    signal(sig, SIG_DFL);
}

extern "C"
pure_expr *pure_catch(pure_expr *h, pure_expr *x)
{
  char test;
  assert(h && x);
  if (x->tag >= 0 && x->data.clos && x->data.clos->n == 0) {
    interpreter& interp = *interpreter::g_interp;
    void *fp = x->data.clos->fp;
#if DEBUG>1
    cerr << "pure_catch: calling " << x << " -> " << fp << endl;
#endif
    assert(h->refc > 0 && x->refc > 0);
    size_t m = x->data.clos->m;
    assert(x->data.clos->local || m == 0);
    pure_expr **env = 0;
    size_t oldsz = interp.sstk_sz;;
    if (m>0) {
      // construct a stack frame
      size_t sz = oldsz;
      resize_sstk(interp.sstk, interp.sstk_cap, sz, m+1);
      pure_expr **sstk = interp.sstk; env = sstk+sz+1;
      sstk[sz++] = 0;
      for (size_t j = 0; j < m; j++) {
	sstk[sz++] = x->data.clos->env[j];
	assert(env[j]->refc > 0); env[j]->refc++;
      }
#if SSTK_DEBUG
      cerr << "++ stack: (sz = " << sz << ")\n";
      for (size_t i = 0; i < sz; i++) {
	pure_expr *x = sstk[i];
	if (i == interp.sstk_sz) cerr << "** pushed:\n";
	if (x)
	  cerr << i << ": " << (void*)x << ": " << x << endl;
	else
	  cerr << i << ": " << "** frame **\n";
      }
#endif
      interp.sstk_sz = sz;
    }
    checkstk(test);
    // Push an exception.
    pure_exception ex; ex.e = 0; ex.sz = oldsz;
    interp.estk.push_front(ex);
    // Call the function now. Catch exceptions generated by the runtime.
    if (setjmp(interp.estk.front().jmp)) {
      // caught an exception
      size_t sz = interp.estk.front().sz;
      pure_expr *e = interp.estk.front().e;
      interp.estk.pop_front();
      if (e) pure_new_internal(e);
#if 0
      /* This doesn't seem to be safe here. Defer until later. */
      // collect garbage
      pure_expr *tmps = interp.tmps;
      while (tmps) {
	pure_expr *next = tmps->xp;
	pure_freenew(tmps);
	tmps = next;
      }
#endif
      for (size_t i = interp.sstk_sz; i-- > sz; )
	if (interp.sstk[i] && interp.sstk[i]->refc > 0)
	  pure_free_internal(interp.sstk[i]);
      interp.sstk_sz = sz;
      if (!e)
	e = pure_new_internal(mk_void());
      assert(e && e->refc > 0);
#if DEBUG>1
      cerr << "pure_catch: exception " << (void*)e << " (refc = " << e->refc
	   << "): " << e << endl;
#endif
      pure_free_internal(x);
      // mask further breaks until the handler starts executing
      interp.brkmask = 1;
      pure_expr *res = pure_apply(h, e);
      return res;
    } else {
      pure_expr *res;
      if (env)
	// pass environment
	res = ((pure_expr*(*)(uint32_t))fp)(env-interp.sstk);
      else
	// parameterless call
	res = ((pure_expr*(*)())fp)();
      // normal return
      interp.estk.pop_front();
#if DEBUG>2
      pure_expr *tmps = interp.tmps;
      while (tmps) {
	if (tmps != res) cerr << "uncollected temporary: " << tmps << endl;
	tmps = tmps->xp;
      }
#endif
      assert(res);
      res->refc++;
      pure_free_internal(h); pure_free_internal(x);
      pure_unref_internal(res);
      return res;
    }
  } else {
    pure_free_internal(h);
    pure_unref_internal(x);
    return x;
  }
}

extern "C"
pure_expr *pure_invoke(void *f, pure_expr** _e)
{
  assert(_e);
  pure_expr*& e = *_e;
  interpreter& interp = *interpreter::g_interp;
  // Cast the function pointer to the right type (takes no arguments, returns
  // a pure_expr*), so we can call it as a native function.
  pure_expr *(*fp)() = (pure_expr*(*)())f;
#if DEBUG>1
  cerr << "pure_invoke: calling " << f << endl;
#endif
  MEMDEBUG_INIT
  // Push an exception.
  pure_exception ex; ex.e = 0; ex.sz = interp.sstk_sz;
  interp.estk.push_front(ex);
  // Call the function now. Catch exceptions generated by the runtime.
  if (setjmp(interp.estk.front().jmp)) {
    // caught an exception
    size_t sz = interp.estk.front().sz;
    e = interp.estk.front().e;
    interp.estk.pop_front();
    if (e) pure_new_internal(e);
#if 0
    /* This doesn't seem to be safe here. Defer until later. */
    // collect garbage
    pure_expr *tmps = interp.tmps;
    while (tmps) {
      pure_expr *next = tmps->xp;
      pure_freenew(tmps);
      tmps = next;
    }
#endif
    for (size_t i = interp.sstk_sz; i-- > sz; )
      if (interp.sstk[i] && interp.sstk[i]->refc > 0)
	pure_free_internal(interp.sstk[i]);
    interp.sstk_sz = sz;
#if DEBUG>1
    if (e)
      cerr << "pure_invoke: exception " << (void*)e << " (refc = " << e->refc
	   << "): " << e << endl;
#endif
    MEMDEBUG_SUMMARY(e)
    return 0;
  } else {
    pure_expr *res = fp();
    // normal return
    interp.estk.pop_front();
    MEMDEBUG_SUMMARY(res)
#if DEBUG>2
    pure_expr *tmps = interp.tmps;
    while (tmps) {
      if (tmps != res) cerr << "uncollected temporary: " << tmps << endl;
      tmps = tmps->xp;
    }
#endif
    return res;
  }
}

extern "C"
void pure_new_args(uint32_t n, ...)
{
  va_list ap;
  va_start(ap, n);
  while (n-- > 0) {
    pure_expr *x = va_arg(ap, pure_expr*);
    if (x->refc > 0)
      x->refc++;
    else
      pure_new_internal(x);
  };
  va_end(ap);
}

extern "C"
void pure_free_args(pure_expr *x, uint32_t n, ...)
{
  va_list ap;
  if (x) x->refc++;
  va_start(ap, n);
  while (n-- > 0) {
    pure_expr *x = va_arg(ap, pure_expr*);
    if (x->refc > 1)
      x->refc--;
    else
      pure_free_internal(x);
  };
  va_end(ap);
}

extern "C"
uint32_t pure_push_args(uint32_t n, uint32_t m, ...)
{
  va_list ap;
  interpreter& interp = *interpreter::g_interp;
  size_t sz = interp.sstk_sz;
  resize_sstk(interp.sstk, interp.sstk_cap, sz, n+m+1);
  pure_expr **sstk = interp.sstk; uint32_t env = (m>0)?sz+n+1:0;
  // mark the beginning of this frame
  sstk[sz++] = 0;
  pure_expr **frame = sstk+sz;
  const uint32_t k = n+m;
  va_start(ap, m);
  for (uint32_t i = 0; i < k; i++) {
    pure_expr *x = va_arg(ap, pure_expr*);
    sstk[sz++] = x;
  };
  va_end(ap);
  /* The reference counts are updated in reverse which is faster in the common
     case that temporary arguments have been generated in order. */
  for (uint32_t i = k; i > 0; ) {
    pure_expr *x = frame[--i];
    if (x->refc > 0)
      x->refc++;
    else
      pure_new_internal(x);
  }
#if SSTK_DEBUG
  cerr << "++ stack: (sz = " << sz << ")\n";
  for (size_t i = 0; i < sz; i++) {
    pure_expr *x = sstk[i];
    if (i == interp.sstk_sz) cerr << "** pushed:\n";
    if (x)
      cerr << i << ": " << (void*)x << ": " << x << endl;
    else
      cerr << i << ": " << "** frame **\n";
  }
#endif
  interp.sstk_sz = sz;
  // return a pointer to the environment:
  return env;
}

extern "C"
void pure_pop_args(pure_expr *x, uint32_t n, uint32_t m)
{
  interpreter& interp = *interpreter::g_interp;
  pure_expr **sstk = interp.sstk;
  size_t sz = interp.sstk_sz;
#if !defined(NDEBUG) || SSTK_DEBUG
  size_t oldsz = sz;
#endif
  sz -= n+m+1;
  assert(sz < oldsz && !sstk[sz]);
#if SSTK_DEBUG
  cerr << "++ stack: (oldsz = " << oldsz << ")\n";
  for (size_t i = 0; i < oldsz; i++) {
    pure_expr *x = sstk[i];
    if (i == sz) cerr << "** popped:\n";
    if (x)
      cerr << i << ": " << (void*)x << ": " << x << endl;
    else
      cerr << i << ": " << "** frame **\n";
  }
#endif
  if (x) x->refc++;
  for (size_t i = 0; i < n+m; i++) {
    pure_expr *x = sstk[sz+1+i];
    assert(x);
    if (x->refc > 1)
      x->refc--;
    else
      pure_free_internal(x);
  };
  interp.sstk_sz = sz;
}

extern "C"
void pure_pop_tail_args(pure_expr *x, uint32_t n, uint32_t m)
{
  interpreter& interp = *interpreter::g_interp;
  pure_expr **sstk = interp.sstk;
  size_t sz, lastsz = interp.sstk_sz, oldsz = lastsz;
  while (lastsz > 0 && sstk[--lastsz]) ;
  assert(lastsz < oldsz && !sstk[lastsz]);
  sz = lastsz-(n+m+1);
  assert(sz < lastsz && !sstk[sz]);
#if SSTK_DEBUG
  cerr << "++ stack: (oldsz = " << oldsz << ", lastsz = " << lastsz << ")\n";
  for (size_t i = 0; i < oldsz; i++) {
    pure_expr *x = sstk[i];
    if (i == sz) cerr << "** popped:\n";
    if (i == lastsz) cerr << "** moved:\n";
    if (x)
      cerr << i << ": " << (void*)x << ": " << x << endl;
    else
      cerr << i << ": " << "** frame **\n";
  }
#endif
  if (x) x->refc++;
  for (size_t i = 0; i < n+m; i++) {
    pure_expr *x = sstk[sz+1+i];
    assert(x);
    if (x->refc > 1)
      x->refc--;
    else
      pure_free_internal(x);
  };
  memmove(sstk+sz, sstk+lastsz, (oldsz-lastsz)*sizeof(pure_expr*));
  interp.sstk_sz -= n+m+1;
}

extern "C"
void pure_push_arg(pure_expr *x)
{
  interpreter& interp = *interpreter::g_interp;
  size_t sz = interp.sstk_sz;
  resize_sstk(interp.sstk, interp.sstk_cap, sz, 2);
  pure_expr** sstk = interp.sstk;
  sstk[sz++] = 0; sstk[sz++] = x;
  if (x->refc > 0)
    x->refc++;
  else
    pure_new_internal(x);
#if SSTK_DEBUG
  cerr << "++ stack: (sz = " << sz << ")\n";
  for (size_t i = 0; i < sz; i++) {
    pure_expr *x = sstk[i];
    if (i == interp.sstk_sz) cerr << "** pushed:\n";
    if (x)
      cerr << i << ": " << (void*)x << ": " << x << endl;
    else
      cerr << i << ": " << "** frame **\n";
  }
#endif
  interp.sstk_sz = sz;
}

extern "C"
void pure_pop_arg()
{
#if SSTK_DEBUG
  pure_pop_args(0, 1, 0);
#else
  interpreter& interp = *interpreter::g_interp;
  pure_expr *x = interp.sstk[interp.sstk_sz-1];
  if (x->refc > 1)
    x->refc--;
  else
    pure_free_internal(x);
  interp.sstk_sz -= 2;
#endif
}

extern "C"
void pure_pop_tail_arg()
{
#if SSTK_DEBUG
  pure_pop_tail_args(0, 1, 0);
#else
  interpreter& interp = *interpreter::g_interp;
  pure_expr **sstk = interp.sstk;
  size_t lastsz = interp.sstk_sz, oldsz = lastsz;
  while (lastsz > 0 && sstk[--lastsz]) ;
  pure_expr *x = interp.sstk[lastsz-1];
  if (x->refc > 1)
    x->refc--;
  else
    pure_free_internal(x);
  memmove(sstk+lastsz-2, sstk+lastsz, (oldsz-lastsz)*sizeof(pure_expr*));
  interp.sstk_sz -= 2;
#endif
}

extern "C"
void pure_debug(int32_t tag, const char *format, ...)
{
  cout << "break at ";
  if (tag > 0)
    cout << interpreter::g_interp->symtab.sym(tag).s;
  else
    cout << "<<anonymous closure>>";
  cout << ": ";
  va_list ap;
  va_start(ap, format);
  vprintf(format, ap);
  va_end(ap);
  static bool init = false;
  if (!init) {
    cout << "\n(Press 'x' to exit the interpreter, <cr> to continue, <eof> to run unattended.)";
    init = true;
  }
  cout << "\n: ";
  char ans;
  cin >> noskipws >> ans;
  bool bail_out = ans=='x';
  while (cin.good() && ans != '\n') cin >> noskipws >> ans;
  if (!cin.good()) cout << "\n";
  if (bail_out) exit(0);
}

/* LIBRARY API. *************************************************************/

extern "C"
pure_expr *pure_byte_string(const char *s)
{
  if (!s) return pure_pointer(0);
  return pure_pointer(strdup(s));
}

extern "C"
pure_expr *pure_byte_cstring(const char *s)
{
  if (!s) return pure_pointer(0);
  return pure_pointer(fromutf8(s));
}

extern "C"
pure_expr *pure_intval(pure_expr *x)
{
  assert(x);
  if (is_thunk(x)) pure_force(x);
  switch (x->tag) {
  case EXPR::INT:	return x;
  case EXPR::BIGINT:	return pure_int(pure_get_int(x));
  case EXPR::DBL:	return pure_int((int32_t)x->data.d);
#if SIZEOF_VOID_P==8
    // Must cast to 64 bit here first, since on 64 bit systems g++ gives an
    // error when directly casting a 64 bit pointer to a 32 bit integer.
  case EXPR::PTR:	return pure_int((uint32_t)(uint64_t)x->data.p);
#else
  case EXPR::PTR:	return pure_int((uint32_t)x->data.p);
#endif
  default:		return 0;
  }
}

extern "C"
pure_expr *pure_dblval(pure_expr *x)
{
  assert(x);
  if (is_thunk(x)) pure_force(x);
  switch (x->tag) {
  case EXPR::INT:	return pure_double((double)x->data.i);
  case EXPR::BIGINT:	return pure_double(mpz_get_d(x->data.z));
  case EXPR::DBL:	return x;
  default:		return 0;
  }
}

extern "C"
pure_expr *pure_pointerval(pure_expr *x)
{
  assert(x);
  if (is_thunk(x)) pure_force(x);
  switch (x->tag) {
  case EXPR::PTR:	return x;
  case EXPR::STR:	return pure_pointer(x->data.s);
  case EXPR::INT:	return pure_pointer((void*)x->data.i);
  case EXPR::BIGINT:
    if (sizeof(mp_limb_t) == 8)
#if SIZEOF_VOID_P==8
      return pure_pointer((void*)mpz_getlimbn(x->data.z, 0));
#else
      return pure_pointer((void*)(uint32_t)mpz_getlimbn(x->data.z, 0));
#endif
    else
#if SIZEOF_VOID_P==8
      return pure_pointer((void*)(uint64_t)pure_get_long(x));
#else
      return pure_pointer((void*)(uint32_t)pure_get_int(x));
#endif
  case EXPR::MATRIX: {
    gsl_matrix_symbolic *m = (gsl_matrix_symbolic*)x->data.mat.p;
    return pure_pointer(m->data);
  }
#ifdef HAVE_GSL
  case EXPR::DMATRIX: {
    gsl_matrix *m = (gsl_matrix*)x->data.mat.p;
    return pure_pointer(m->data);
  }
  case EXPR::CMATRIX: {
    gsl_matrix_complex *m = (gsl_matrix_complex*)x->data.mat.p;
    return pure_pointer(m->data);
  }
  case EXPR::IMATRIX:{
    gsl_matrix_int *m = (gsl_matrix_int*)x->data.mat.p;
    return pure_pointer(m->data);
  }
#endif
  default:		return 0;
  }
}

static pure_expr *pointer_to_bigint(void *p)
{
  if (sizeof(mp_limb_t) == 8) {
    // In this case the pointer value ought to fit into a single limb.
#if SIZEOF_VOID_P==8
    limb_t u[1] = { (uint64_t)p };
#else
    limb_t u[1] = { (uint64_t)(uint32_t)p };
#endif
    return pure_bigint(1, u);
  }
  // 4 byte limbs.
#if SIZEOF_VOID_P==8
  // 8 byte pointers, put least significant word in the first limb.
  assert(sizeof(void*) == 8);
  limb_t u[2] = { (uint32_t)(uint64_t)p, (uint32_t)(((uint64_t)p)>>32) };
  return pure_bigint(2, u);
#else
  // 4 byte pointers.
  limb_t u[1] = { (uint32_t)p };
  return pure_bigint(1, u);
#endif
}

extern "C"
pure_expr *pure_bigintval(pure_expr *x)
{
  assert(x);
  if (is_thunk(x)) pure_force(x);
  if (x->tag == EXPR::BIGINT)
    return x;
  else if (x->tag == EXPR::PTR)
    return pointer_to_bigint(x->data.p);
  else if (x->tag != EXPR::INT && x->tag != EXPR::DBL)
    return 0;
  else if (x->tag == EXPR::DBL &&
	   (is_nan(x->data.d) || is_nan(x->data.d-x->data.d)))
    pure_sigfpe();
  pure_expr *y = pure_bigint(0, 0);
  mpz_t& z = y->data.z;
  if (x->tag == EXPR::INT)
    mpz_set_si(z, x->data.i);
  else if (x->tag == EXPR::DBL)
    mpz_set_d(z, x->data.d);
  return y;
}

extern "C"
pure_expr *pure_rational(double d)
{
  pure_expr *u = pure_bigint(0, 0);
  pure_expr *v = pure_bigint(0, 0);
  mpz_t& x = u->data.z;
  mpz_t& y = v->data.z;
  mpq_t q;
  mpq_init(q);
  mpq_set_d(q, d);
  mpq_get_num(x, q);
  mpq_get_den(y, q);
  mpq_clear(q);
  return pure_tuplel(2, u, v);
}

// This is the ``Mersenne Twister'' random number generator MT19937, which
// generates pseudorandom integers uniformly distributed in 0..(2^32 - 1)
// starting from any odd seed in 0..(2^32 - 1).  This version is a recode
// by Shawn Cokus (Cokus@math.washington.edu) on March 8, 1998 of a version by
// Takuji Nishimura (who had suggestions from Topher Cooper and Marc Rieffel in
// July-August 1997).
//
// Effectiveness of the recoding (on Goedel2.math.washington.edu, a DEC Alpha
// running OSF/1) using GCC -O3 as a compiler: before recoding: 51.6 sec. to
// generate 300 million random numbers; after recoding: 24.0 sec. for the same
// (i.e., 46.5% of original time), so speed is now about 12.5 million random
// number generations per second on this machine.
//
// According to the URL <http://www.math.keio.ac.jp/~matumoto/emt.html>
// (and paraphrasing a bit in places), the Mersenne Twister is ``designed
// with consideration of the flaws of various existing generators,'' has
// a period of 2^19937 - 1, gives a sequence that is 623-dimensionally
// equidistributed, and ``has passed many stringent tests, including the
// die-hard test of G. Marsaglia and the load test of P. Hellekalek and
// S. Wegenkittl.''  It is efficient in memory usage (typically using 2506
// to 5012 bytes of static data, depending on data type sizes, and the code
// is quite short as well).  It generates random numbers in batches of 624
// at a time, so the caching and pipelining of modern systems is exploited.
// It is also divide- and mod-free.
//
// This library is free software; you can redistribute it and/or modify it
// under the terms of the GNU Library General Public License as published by
// the Free Software Foundation (either version 2 of the License or, at your
// option, any later version).  This library is distributed in the hope that
// it will be useful, but WITHOUT ANY WARRANTY, without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
// the GNU Library General Public License for more details.  You should have
// received a copy of the GNU Library General Public License along with this
// library; if not, write to the Free Software Foundation, Inc., 59 Temple
// Place, Suite 330, Boston, MA 02111-1307, USA.
//
// The code as Shawn received it included the following notice:
//
//   Copyright (C) 1997 Makoto Matsumoto and Takuji Nishimura.  When
//   you use this, send an e-mail to <matumoto@math.keio.ac.jp> with
//   an appropriate reference to your work.
//
// It would be nice to CC: <Cokus@math.washington.edu> when you write.
//

// See http://www.math.keio.ac.jp/~matumoto/emt.html for the original sources.

#define N              (624)
#define M              (397)
#define K              (0x9908B0DFU)
#define hiBit(u)       ((u) & 0x80000000U)
#define loBit(u)       ((u) & 0x00000001U)
#define loBits(u)      ((u) & 0x7FFFFFFFU)
#define mixBits(u, v)  (hiBit(u)|loBits(v))

// TLD?
static uint32_t stateMT[N+1];
static uint32_t *nextMT;
static int leftMT = -1;

void pure_srandom(uint32_t seed)
{
  // MT works best with odd seeds, so we enforce that here.
  register uint32_t x = (seed | 1U) & 0xFFFFFFFFU, *s = stateMT;
  register int j;

  for (leftMT=0, *s++=x, j=N; --j; *s++ = (x*=69069U) & 0xFFFFFFFFU);
}

static uint32_t reloadMT(void)
{
  register uint32_t *p0=stateMT, *p2=stateMT+2, *pM=stateMT+M, s0, s1;
  register int j;

  if (leftMT < -1)
    pure_srandom(4357U);

  leftMT=N-1, nextMT=stateMT+1;

  for (s0=stateMT[0], s1=stateMT[1], j=N-M+1; --j; s0=s1, s1=*p2++)
    *p0++ = *pM++ ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? K : 0U);

  for (pM=stateMT, j=M; --j; s0=s1, s1=*p2++)
    *p0++ = *pM++ ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? K : 0U);

  s1=stateMT[0], *p0 = *pM ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? K : 0U);
  s1 ^= (s1 >> 11);
  s1 ^= (s1 <<  7) & 0x9D2C5680U;
  s1 ^= (s1 << 15) & 0xEFC60000U;
  return(s1 ^ (s1 >> 18));
}

uint32_t pure_random(void)
{
  uint32_t y;
  
  if(--leftMT < 0)
    return reloadMT();

  y  = *nextMT++;
  y ^= (y >> 11);
  y ^= (y <<  7) & 0x9D2C5680U;
  y ^= (y << 15) & 0xEFC60000U;
  return (y ^ (y >> 18));
}

#undef N
#undef M
#undef K

extern "C"
pure_expr *bigint_neg(mpz_t x)
{
  pure_expr *u = pure_bigint(0, 0);
  mpz_t& z = u->data.z;
  mpz_neg(z, x);
  return u;
}

extern "C"
pure_expr *bigint_add(mpz_t x, mpz_t y)
{
  pure_expr *u = pure_bigint(0, 0);
  mpz_t& z = u->data.z;
  mpz_add(z, x, y);
  return u;
}

extern "C"
pure_expr *bigint_sub(mpz_t x, mpz_t y)
{
  pure_expr *u = pure_bigint(0, 0);
  mpz_t& z = u->data.z;
  mpz_sub(z, x, y);
  return u;
}

extern "C"
pure_expr *bigint_mul(mpz_t x, mpz_t y)
{
  pure_expr *u = pure_bigint(0, 0);
  mpz_t& z = u->data.z;
  mpz_mul(z, x, y);
  return u;
}

// These raise a SIGFPE signal exception for division by zero.

extern "C"
pure_expr *bigint_div(mpz_t x, mpz_t y)
{
  if (mpz_sgn(y) == 0) pure_sigfpe();
  pure_expr *u = pure_bigint(0, 0);
  mpz_t& z = u->data.z;
  mpz_tdiv_q(z, x, y);
  return u;
}

extern "C"
pure_expr *bigint_mod(mpz_t x, mpz_t y)
{
  if (mpz_sgn(y) == 0) pure_sigfpe();
  pure_expr *u = pure_bigint(0, 0);
  mpz_t& z = u->data.z;
  mpz_tdiv_r(z, x, y);
  return u;
}

extern "C"
pure_expr *bigint_pow(mpz_t x, uint32_t y)
{
  pure_expr *u = pure_bigint(0, 0);
  mpz_t& z = u->data.z;
  mpz_pow_ui(z, x, y);
  return u;
}

extern "C"
pure_expr *bigint_shl(mpz_t x, int32_t y)
{
  pure_expr *u = pure_bigint(0, 0);
  mpz_t& z = u->data.z;
  mpz_mul_2exp(z, x, y);
  return u;
}

extern "C"
pure_expr *bigint_shr(mpz_t x, int32_t y)
{
  pure_expr *u = pure_bigint(0, 0);
  mpz_t& z = u->data.z;
  mpz_fdiv_q_2exp(z, x, y);
  return u;
}

extern "C"
pure_expr *bigint_not(mpz_t x)
{
  pure_expr *u = pure_bigint(0, 0);
  mpz_t& z = u->data.z;
  mpz_com(z, x);
  return u;
}

extern "C"
pure_expr *bigint_and(mpz_t x, mpz_t y)
{
  pure_expr *u = pure_bigint(0, 0);
  mpz_t& z = u->data.z;
  mpz_and(z, x, y);
  return u;
}

extern "C"
pure_expr *bigint_or(mpz_t x, mpz_t y)
{
  pure_expr *u = pure_bigint(0, 0);
  mpz_t& z = u->data.z;
  mpz_ior(z, x, y);
  return u;
}

extern "C"
pure_expr *bigint_gcd(mpz_t x, mpz_t y)
{
  pure_expr *u = pure_bigint(0, 0);
  mpz_t& z = u->data.z;
  mpz_gcd(z, x, y);
  return u;
}

extern "C"
pure_expr *bigint_lcm(mpz_t x, mpz_t y)
{
  pure_expr *u = pure_bigint(0, 0);
  mpz_t& z = u->data.z;
  mpz_lcm(z, x, y);
  return u;
}

extern "C"
int32_t bigint_cmp(mpz_t x, mpz_t y)
{
  return mpz_cmp(x, y);
}

extern "C"
bool string_null(const char *s)
{
  assert(s);
  return *s==0;
}

extern "C"
uint32_t string_size(const char *s)
{
  assert(s);
  return u8strlen(s);
}

extern "C"
pure_expr *string_char_at(const char *s, uint32_t n)
{
  assert(s);
  unsigned long c = u8strchar(s, n);
  if (c == 0) return 0;
  char buf[5];
  return pure_string_dup(u8char(buf, c));
}

static void add_char(pure_expr*** x, unsigned long c)
{
  interpreter& interp = *interpreter::g_interp;
  char buf[5];
  pure_expr *y = new_expr(), *z = pure_string_dup(u8char(buf, c));
  y->tag = EXPR::APP;
  y->data.x[0] = pure_new_internal(pure_const(interp.symtab.cons_sym().f));
  y->data.x[1] = pure_new_internal(z);
  **x = pure_new_internal(new_expr());
  (**x)->tag = EXPR::APP;
  (**x)->data.x[0] = pure_new_internal(y);
  *x = (**x)->data.x+1;
}

extern "C"
pure_expr *string_chars(const char *s)
{
  assert(s);
  pure_expr *x, **y = &x, ***z = &y;
  u8dostr(s, (void(*)(void*,unsigned long))add_char, z);
  **z = pure_new_internal(mk_nil());
  pure_unref_internal(x);
  return x;
}

extern "C"
pure_expr *string_chr(uint32_t n)
{
  char buf[5];
  u8char(buf, n);
  if (u8charcode(buf) > 0)
    return pure_string_dup(buf);
  else
    return 0;
}

extern "C"
pure_expr *string_ord(const char *c)
{
  assert(c);
  long n = u8charcode(c);
  if (n > 0)
    return pure_int(n);
  else
    return 0;
}

extern "C"
pure_expr *string_concat(const char* s, const char *t)
{
  assert(s && t);
  size_t p = strlen(s), q = strlen(t);
  char *buf = new char[p+q+1];
  strcpy(buf, s); strcpy(buf+p, t);
  pure_expr *x = new_expr();
  x->tag = EXPR::STR;
  x->data.s = buf;
  MEMDEBUG_NEW(x)
  return x;
}

extern "C"
pure_expr *string_concat_list(pure_expr *xs)
{
  // linear-time concatenation of a list of strings
  assert(xs);
  if (is_thunk(xs)) pure_force(xs);
  // calculate the size of the result string
  pure_expr *ys = xs, *z, *zs;
  size_t n = 0;
  while (is_cons(ys, z, zs)) {
    if (is_thunk(z)) pure_force(z);
    if (z->tag != EXPR::STR) break;
    n += strlen(z->data.s);
    ys = zs;
    if (is_thunk(ys)) pure_force(ys);
  }
  if (!is_nil(ys)) return 0;
  // allocate the result string
  char *buf = new char[n+1]; buf[0] = 0;
  // concatenate
  ys = xs; n = 0;
  while (is_cons(ys, z, zs) && z->tag == EXPR::STR) {
    strcpy(buf+n, z->data.s);
    n += strlen(z->data.s);
    ys = zs;
  }
  // return the result
  pure_expr *x = new_expr();
  x->tag = EXPR::STR;
  x->data.s = buf;
  MEMDEBUG_NEW(x)
  return x;
}

extern "C"
pure_expr *string_substr(const char* s, uint32_t pos, uint32_t size)
{
  assert(s);
  const char *p = u8strcharpos(s, pos), *q = u8strcharpos(p, size);
  size_t n = q-p;
  char *buf = new char[n+1];
  strncpy(buf, p, n); buf[n] = 0;
  pure_expr *x = new_expr();
  x->tag = EXPR::STR;
  x->data.s = buf;
  MEMDEBUG_NEW(x)
  return x;
}

extern "C"
int32_t string_index(const char* s, const char *t)
{
  assert(s && t);
  const char *p = strstr(s, t);
  if (p)
    return u8strpos(s, p);
  else
    return -1;
}

extern "C"
char *str(const pure_expr *x)
{
  assert(x);
  ostringstream os;
  try {
    os << x;
    return strdup(os.str().c_str());
  } catch (err &e) {
    return 0;
  }
}

extern "C"
pure_expr *eval(pure_expr *x)
{
  assert(x);
  char *s;
  if (pure_is_cstring_dup(x, &s)) {
    interpreter& interp = *interpreter::g_interp;
    interp.errmsg.clear();
    pure_expr *res = interp.runstr(string(s)+";");
    free(s);
    interp.result = 0;
    if (res) {
      pure_unref_internal(res);
      return res;
    } else if (interp.errmsg.empty())
      return mk_void();
    else
      return 0;
  } else {
    pure_expr *res = 0, *e = 0;
    interpreter& interp = *interpreter::g_interp;
    try {
      expr y = interp.pure_expr_to_expr(x);
      res = interp.eval(y, e);
    } catch (err &e) {
      return x;
    }
    if (res) {
      assert(!e);
      return res;
    } else if (e) {
      pure_unref_internal(e);
      pure_throw(e);
      return 0;
    } else
      return 0;
  }
}

extern "C"
pure_expr *evalcmd(pure_expr *x)
{
  assert(x);
  char *s;
  if (pure_is_cstring_dup(x, &s)) {
    interpreter& interp = *interpreter::g_interp;
    ostream *l_output = interp.output;
    ostringstream sout;
    interp.errmsg.clear();
    interp.output = &sout;
    pure_expr *res = interp.runstr(string(s));
    free(s);
    interp.result = 0;
    interp.output = l_output;
    if (res)
      pure_free_internal(res);
    else if (!interp.errmsg.empty())
      return 0;
    return pure_cstring_dup(sout.str().c_str());
  } else
    return 0;
}

extern "C"
const char *lasterr()
{
  interpreter& interp = *interpreter::g_interp;
  return interp.errmsg.c_str();
}

extern "C"
uint32_t matrix_size(pure_expr *x)
{
  switch (x->tag) {
  case EXPR::MATRIX: {
    gsl_matrix_symbolic *m = (gsl_matrix_symbolic*)x->data.mat.p;
    return m->size1*m->size2;
  }
#ifdef HAVE_GSL
  case EXPR::DMATRIX: {
    gsl_matrix *m = (gsl_matrix*)x->data.mat.p;
    return m->size1*m->size2;
  }
  case EXPR::CMATRIX: {
    gsl_matrix_complex *m = (gsl_matrix_complex*)x->data.mat.p;
    return m->size1*m->size2;
  }
  case EXPR::IMATRIX: {
    gsl_matrix_int *m = (gsl_matrix_int*)x->data.mat.p;
    return m->size1*m->size2;
  }
#endif
  default:
    return 0;
  }
}

extern "C"
pure_expr *matrix_dim(pure_expr *x)
{
  switch (x->tag) {
  case EXPR::MATRIX: {
    gsl_matrix_symbolic *m = (gsl_matrix_symbolic*)x->data.mat.p;
    return pure_tuplel(2, pure_int(m->size1), pure_int(m->size2));
  }
#ifdef HAVE_GSL
  case EXPR::DMATRIX: {
    gsl_matrix *m = (gsl_matrix*)x->data.mat.p;
    return pure_tuplel(2, pure_int(m->size1), pure_int(m->size2));
  }
  case EXPR::CMATRIX: {
    gsl_matrix_complex *m = (gsl_matrix_complex*)x->data.mat.p;
    return pure_tuplel(2, pure_int(m->size1), pure_int(m->size2));
  }
  case EXPR::IMATRIX: {
    gsl_matrix_int *m = (gsl_matrix_int*)x->data.mat.p;
    return pure_tuplel(2, pure_int(m->size1), pure_int(m->size2));
  }
#endif
  default:
    return 0;
  }
}

extern "C"
uint32_t matrix_stride(pure_expr *x)
{
  switch (x->tag) {
  case EXPR::MATRIX: {
    gsl_matrix_symbolic *m = (gsl_matrix_symbolic*)x->data.mat.p;
    return m->tda;
  }
#ifdef HAVE_GSL
  case EXPR::DMATRIX: {
    gsl_matrix *m = (gsl_matrix*)x->data.mat.p;
    return m->tda;
  }
  case EXPR::CMATRIX: {
    gsl_matrix_complex *m = (gsl_matrix_complex*)x->data.mat.p;
    return m->tda;
  }
  case EXPR::IMATRIX: {
    gsl_matrix_int *m = (gsl_matrix_int*)x->data.mat.p;
    return m->tda;
  }
#endif
  default:
    return 0;
  }
}

extern "C"
int32_t matrix_type(pure_expr *x)
{
  uint32_t t = (uint32_t)x->tag;
  if ((t&0xfffffff0) == (uint32_t)EXPR::MATRIX)
    // the lowest nibble is the subtype tag
    return (int32_t)(t&0xf);
  else
    return -1;
}

extern "C"
pure_expr *matrix_elem_at(pure_expr *x, int32_t _i)
{
  switch (x->tag) {
  case EXPR::MATRIX: {
    gsl_matrix_symbolic *m = (gsl_matrix_symbolic*)x->data.mat.p;
    if (m->tda > m->size2) {
      const size_t i = _i/m->size2, j = _i%m->size2, k = i*m->tda+j;
      return m->data[k];
    } else
      return m->data[_i];
  }
#ifdef HAVE_GSL
  case EXPR::DMATRIX: {
    gsl_matrix *m = (gsl_matrix*)x->data.mat.p;
    if (m->tda > m->size2) {
      const size_t i = _i/m->size2, j = _i%m->size2, k = i*m->tda+j;
      return pure_double(m->data[k]);
    } else
      return pure_double(m->data[_i]);
  }
  case EXPR::CMATRIX: {
    gsl_matrix_complex *m = (gsl_matrix_complex*)x->data.mat.p;
    if (m->tda > m->size2) {
      const size_t i = _i/m->size2, j = _i%m->size2, k = 2*(i*m->tda+j);
      return make_complex(m->data[k], m->data[k+1]);
    } else {
      const size_t k = 2*_i;
      return make_complex(m->data[k], m->data[k+1]);
    }
  }
  case EXPR::IMATRIX: {
    gsl_matrix_int *m = (gsl_matrix_int*)x->data.mat.p;
    if (m->tda > m->size2) {
      const size_t i = _i/m->size2, j = _i%m->size2, k = i*m->tda+j;
      return pure_int(m->data[k]);
    } else
      return pure_int(m->data[_i]);
  }
#endif
  default:
    return 0;
  }
}

extern "C"
pure_expr *matrix_elem_at2(pure_expr *x, int32_t i, int32_t j)
{
  switch (x->tag) {
  case EXPR::MATRIX: {
    gsl_matrix_symbolic *m = (gsl_matrix_symbolic*)x->data.mat.p;
    const size_t k = i*m->tda+j;
    return m->data[k];
  }
#ifdef HAVE_GSL
  case EXPR::DMATRIX: {
    gsl_matrix *m = (gsl_matrix*)x->data.mat.p;
    const size_t k = i*m->tda+j;
    return pure_double(m->data[k]);
  }
  case EXPR::CMATRIX: {
    gsl_matrix_complex *m = (gsl_matrix_complex*)x->data.mat.p;
    const size_t k = 2*(i*m->tda+j);
    return make_complex(m->data[k], m->data[k+1]);
  }
  case EXPR::IMATRIX: {
    gsl_matrix_int *m = (gsl_matrix_int*)x->data.mat.p;
    const size_t k = i*m->tda+j;
    return pure_int(m->data[k]);
  }
#endif
  default:
    return 0;
  }
}

extern "C"
pure_expr *matrix_slice(pure_expr *x, int32_t i1, int32_t j1,
			int32_t i2, int32_t j2)
{
  void *p = 0;
  if (i1<0) i1 = 0; if (j1<0) j1 = 0;
  switch (x->tag) {
  case EXPR::MATRIX: {
    gsl_matrix_symbolic *m = (gsl_matrix_symbolic*)x->data.mat.p;
    if (i2 >= (int)m->size1) i2 = m->size1-1;
    if (j2 >= (int)m->size2) j2 = m->size2-1;
    size_t n1 = (i1<(int)m->size1 && i2>=i1)?(i2+1-i1):0,
      n2 = (j1<(int)m->size2 && j2>=j1)?(j2+1-j1):0;
    if (n1 == 0 || n2 == 0) // empty matrix
      return pure_symbolic_matrix(create_symbolic_matrix(n1, n2));
    gsl_matrix_symbolic_view v =
      gsl_matrix_symbolic_submatrix(m, i1, j1, n1, n2);
    // take a copy of the view matrix
    gsl_matrix_symbolic *m1 =
      (gsl_matrix_symbolic*)malloc(sizeof(gsl_matrix_symbolic));
    assert(m1 && v.matrix.data);
    *m1 = v.matrix;
    p = m1;
    break;
  }
#ifdef HAVE_GSL
  case EXPR::DMATRIX: {
    gsl_matrix *m = (gsl_matrix*)x->data.mat.p;
    if (i2 >= (int)m->size1) i2 = m->size1-1;
    if (j2 >= (int)m->size2) j2 = m->size2-1;
    size_t n1 = (i1<(int)m->size1 && i2>=i1)?(i2+1-i1):0,
      n2 = (j1<(int)m->size2 && j2>=j1)?(j2+1-j1):0;
    if (n1 == 0 || n2 == 0) // empty matrix
      return pure_double_matrix(create_double_matrix(n1, n2));
    gsl_matrix_view v = gsl_matrix_submatrix(m, i1, j1, n1, n2);
    // take a copy of the view matrix
    gsl_matrix *m1 = (gsl_matrix*)malloc(sizeof(gsl_matrix));
    assert(m1 && v.matrix.data);
    *m1 = v.matrix;
    p = m1;
    break;
  }
  case EXPR::CMATRIX: {
    gsl_matrix_complex *m = (gsl_matrix_complex*)x->data.mat.p;
    if (i2 >= (int)m->size1) i2 = m->size1-1;
    if (j2 >= (int)m->size2) j2 = m->size2-1;
    size_t n1 = (i1<(int)m->size1 && i2>=i1)?(i2+1-i1):0,
      n2 = (j1<(int)m->size2 && j2>=j1)?(j2+1-j1):0;
    if (n1 == 0 || n2 == 0) // empty matrix
      return pure_complex_matrix(create_complex_matrix(n1, n2));
    gsl_matrix_complex_view v =
      gsl_matrix_complex_submatrix(m, i1, j1, n1, n2);
    // take a copy of the view matrix
    gsl_matrix_complex *m1 =
      (gsl_matrix_complex*)malloc(sizeof(gsl_matrix_complex));
    assert(m1 && v.matrix.data);
    *m1 = v.matrix;
    p = m1;
    break;
  }
  case EXPR::IMATRIX: {
    gsl_matrix_int *m = (gsl_matrix_int*)x->data.mat.p;
    if (i2 >= (int)m->size1) i2 = m->size1-1;
    if (j2 >= (int)m->size2) j2 = m->size2-1;
    size_t n1 = (i1<(int)m->size1 && i2>=i1)?(i2+1-i1):0,
      n2 = (j1<(int)m->size2 && j2>=j1)?(j2+1-j1):0;
    if (n1 == 0 || n2 == 0) // empty matrix
      return pure_int_matrix(create_int_matrix(n1, n2));
    gsl_matrix_int_view v = gsl_matrix_int_submatrix(m, i1, j1, n1, n2);
    // take a copy of the view matrix
    gsl_matrix_int *m1 = (gsl_matrix_int*)malloc(sizeof(gsl_matrix_int));
    assert(m1 && v.matrix.data);
    *m1 = v.matrix;
    p = m1;
    break;
  }
#endif
  default:
    return 0;
  }
  // create a new expression for the slice, update the reference counter for
  // the underlying GSL matrix
  pure_expr *y = new_expr();
  y->tag = x->tag;
  y->data.mat.p = p;
  y->data.mat.refc = x->data.mat.refc;
  (*y->data.mat.refc)++;
  MEMDEBUG_NEW(y)
  return y;
}

extern "C"
pure_expr *matrix_diag(pure_expr *x)
{
  switch (x->tag) {
  case EXPR::MATRIX: {
    gsl_matrix_symbolic *m = (gsl_matrix_symbolic*)x->data.mat.p;
    const size_t n1 = m->size1, n2 = m->size2, n = (n1<n2)?n1:n2;
    gsl_matrix_symbolic *m1 = create_symbolic_matrix(1, n);
    for (size_t i = 0; i < n; i++)
      m1->data[i] = m->data[i*(m->tda+1)];
    return pure_symbolic_matrix(m1);
  }
#ifdef HAVE_GSL
  case EXPR::DMATRIX: {
    gsl_matrix *m = (gsl_matrix*)x->data.mat.p;
    const size_t n1 = m->size1, n2 = m->size2, n = (n1<n2)?n1:n2;
    gsl_matrix *m1 = create_double_matrix(1, n);
    for (size_t i = 0; i < n; i++)
      m1->data[i] = m->data[i*(m->tda+1)];
    return pure_double_matrix(m1);
  }
  case EXPR::CMATRIX: {
    gsl_matrix_complex *m = (gsl_matrix_complex*)x->data.mat.p;
    const size_t n1 = m->size1, n2 = m->size2, n = (n1<n2)?n1:n2;
    gsl_matrix_complex *m1 = create_complex_matrix(1, n);
    for (size_t i = 0; i < n; i++) {
      const size_t k = 2*i*(m->tda+1);
      m1->data[2*i] = m->data[k];
      m1->data[2*i+1] = m->data[k+1];
    }
    return pure_complex_matrix(m1);
  }
  case EXPR::IMATRIX: {
    gsl_matrix_int *m = (gsl_matrix_int*)x->data.mat.p;
    const size_t n1 = m->size1, n2 = m->size2, n = (n1<n2)?n1:n2;
    gsl_matrix_int *m1 = create_int_matrix(1, n);
    for (size_t i = 0; i < n; i++)
      m1->data[i] = m->data[i*(m->tda+1)];
    return pure_int_matrix(m1);
  }
#endif
  default:
    return 0;
  }
}

extern "C"
pure_expr *matrix_subdiag(pure_expr *x, int32_t k)
{
  if (k<0) return matrix_supdiag(x, -k);
  switch (x->tag) {
  case EXPR::MATRIX: {
    gsl_matrix_symbolic *m = (gsl_matrix_symbolic*)x->data.mat.p;
    const size_t n1 = m->size1, n2 = m->size2, n0 = (n1<n2)?n1:n2;
    const size_t n = (n0>(size_t)k)?n0-k:0, k0 = k*m->tda;
    gsl_matrix_symbolic *m1 = create_symbolic_matrix(1, n);
    for (size_t i = 0; i < n; i++)
      m1->data[i] = m->data[k0+i*(m->tda+1)];
    return pure_symbolic_matrix(m1);
  }
#ifdef HAVE_GSL
  case EXPR::DMATRIX: {
    gsl_matrix *m = (gsl_matrix*)x->data.mat.p;
    const size_t n1 = m->size1, n2 = m->size2, n0 = (n1<n2)?n1:n2;
    const size_t n = (n0>(size_t)k)?n0-k:0, k0 = k*m->tda;
    gsl_matrix *m1 = create_double_matrix(1, n);
    for (size_t i = 0; i < n; i++)
      m1->data[i] = m->data[k0+i*(m->tda+1)];
    return pure_double_matrix(m1);
  }
  case EXPR::CMATRIX: {
    gsl_matrix_complex *m = (gsl_matrix_complex*)x->data.mat.p;
    const size_t n1 = m->size1, n2 = m->size2, n0 = (n1<n2)?n1:n2;
    const size_t n = (n0>(size_t)k)?n0-k:0, k0 = k*m->tda;
    gsl_matrix_complex *m1 = create_complex_matrix(1, n);
    for (size_t i = 0; i < n; i++) {
      const size_t k = 2*k0+2*i*(m->tda+1);
      m1->data[2*i] = m->data[k];
      m1->data[2*i+1] = m->data[k+1];
    }
    return pure_complex_matrix(m1);
  }
  case EXPR::IMATRIX: {
    gsl_matrix_int *m = (gsl_matrix_int*)x->data.mat.p;
    const size_t n1 = m->size1, n2 = m->size2, n0 = (n1<n2)?n1:n2;
    const size_t n = (n0>(size_t)k)?n0-k:0, k0 = k*m->tda;
    gsl_matrix_int *m1 = create_int_matrix(1, n);
    for (size_t i = 0; i < n; i++)
      m1->data[i] = m->data[k0+i*(m->tda+1)];
    return pure_int_matrix(m1);
  }
#endif
  default:
    return 0;
  }
}

extern "C"
pure_expr *matrix_supdiag(pure_expr *x, int32_t k)
{
  if (k<0) return matrix_subdiag(x, -k);
  switch (x->tag) {
  case EXPR::MATRIX: {
    gsl_matrix_symbolic *m = (gsl_matrix_symbolic*)x->data.mat.p;
    const size_t n1 = m->size1, n2 = m->size2, n0 = (n1<n2)?n1:n2;
    const size_t n = (n0>(size_t)k)?n0-k:0, k0 = k;
    gsl_matrix_symbolic *m1 = create_symbolic_matrix(1, n);
    for (size_t i = 0; i < n; i++)
      m1->data[i] = m->data[k0+i*(m->tda+1)];
    return pure_symbolic_matrix(m1);
  }
#ifdef HAVE_GSL
  case EXPR::DMATRIX: {
    gsl_matrix *m = (gsl_matrix*)x->data.mat.p;
    const size_t n1 = m->size1, n2 = m->size2, n0 = (n1<n2)?n1:n2;
    const size_t n = (n0>(size_t)k)?n0-k:0, k0 = k;
    gsl_matrix *m1 = create_double_matrix(1, n);
    for (size_t i = 0; i < n; i++)
      m1->data[i] = m->data[k0+i*(m->tda+1)];
    return pure_double_matrix(m1);
  }
  case EXPR::CMATRIX: {
    gsl_matrix_complex *m = (gsl_matrix_complex*)x->data.mat.p;
    const size_t n1 = m->size1, n2 = m->size2, n0 = (n1<n2)?n1:n2;
    const size_t n = (n0>(size_t)k)?n0-k:0, k0 = k;
    gsl_matrix_complex *m1 = create_complex_matrix(1, n);
    for (size_t i = 0; i < n; i++) {
      const size_t k = 2*k0+2*i*(m->tda+1);
      m1->data[2*i] = m->data[k];
      m1->data[2*i+1] = m->data[k+1];
    }
    return pure_complex_matrix(m1);
  }
  case EXPR::IMATRIX: {
    gsl_matrix_int *m = (gsl_matrix_int*)x->data.mat.p;
    const size_t n1 = m->size1, n2 = m->size2, n0 = (n1<n2)?n1:n2;
    const size_t n = (n0>(size_t)k)?n0-k:0, k0 = k;
    gsl_matrix_int *m1 = create_int_matrix(1, n);
    for (size_t i = 0; i < n; i++)
      m1->data[i] = m->data[k0+i*(m->tda+1)];
    return pure_int_matrix(m1);
  }
#endif
  default:
    return 0;
  }
}

extern "C"
pure_expr *matrix_diagm(pure_expr *x)
{
  switch (x->tag) {
  case EXPR::MATRIX: {
    gsl_matrix_symbolic *m = (gsl_matrix_symbolic*)x->data.mat.p;
    const size_t n1 = m->size1, n = m->size2;
    if (n1 != 1) return 0;
    gsl_matrix_symbolic *m1 = create_symbolic_matrix(n, n);
    pure_expr *zero = pure_int(0);
    assert(zero);
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < n; j++)
	m1->data[i*m1->tda+j] = zero;
    for (size_t i = 0; i < n; i++)
      m1->data[i*(m1->tda+1)] = m->data[i];
    return pure_symbolic_matrix(m1);
  }
#ifdef HAVE_GSL
  case EXPR::DMATRIX: {
    gsl_matrix *m = (gsl_matrix*)x->data.mat.p;
    const size_t n1 = m->size1, n = m->size2;
    if (n1 != 1) return 0;
    gsl_matrix *m1 = create_double_matrix(n, n);
    memset(m1->data, 0, m1->block->size*sizeof(double));
    for (size_t i = 0; i < n; i++)
      m1->data[i*(m1->tda+1)] = m->data[i];
    return pure_double_matrix(m1);
  }
  case EXPR::CMATRIX: {
    gsl_matrix_complex *m = (gsl_matrix_complex*)x->data.mat.p;
    const size_t n1 = m->size1, n = m->size2;
    if (n1 != 1) return 0;
    gsl_matrix_complex *m1 = create_complex_matrix(n, n);
    memset(m1->data, 0, m1->block->size*2*sizeof(double));
    for (size_t i = 0; i < n; i++) {
      const size_t k = 2*i*(m1->tda+1);
      m1->data[k] = m->data[2*i];
      m1->data[k+1] = m->data[2*i+1];
    }
    return pure_complex_matrix(m1);
  }
  case EXPR::IMATRIX: {
    gsl_matrix_int *m = (gsl_matrix_int*)x->data.mat.p;
    const size_t n1 = m->size1, n = m->size2;
    if (n1 != 1) return 0;
    gsl_matrix_int *m1 = create_int_matrix(n, n);
    memset(m1->data, 0, m1->block->size*sizeof(int));
    for (size_t i = 0; i < n; i++)
      m1->data[i*(m1->tda+1)] = m->data[i];
    return pure_int_matrix(m1);
  }
#endif
  default:
    return 0;
  }
}

extern "C"
pure_expr *matrix_subdiagm(pure_expr *x, int32_t k)
{
  if (k<0) return matrix_supdiagm(x, -k);
  switch (x->tag) {
  case EXPR::MATRIX: {
    gsl_matrix_symbolic *m = (gsl_matrix_symbolic*)x->data.mat.p;
    const size_t n1 = m->size1, n = m->size2;
    if (n1 != 1) return 0;
    gsl_matrix_symbolic *m1 = create_symbolic_matrix(n+k, n+k);
    const size_t k0 = k*m1->tda;
    pure_expr *zero = pure_int(0);
    assert(zero);
    for (size_t i = 0; i < n+k; i++)
      for (size_t j = 0; j < n+k; j++)
	m1->data[i*m1->tda+j] = zero;
    for (size_t i = 0; i < n; i++)
      m1->data[k0+i*(m1->tda+1)] = m->data[i];
    return pure_symbolic_matrix(m1);
  }
#ifdef HAVE_GSL
  case EXPR::DMATRIX: {
    gsl_matrix *m = (gsl_matrix*)x->data.mat.p;
    const size_t n1 = m->size1, n = m->size2;
    if (n1 != 1) return 0;
    gsl_matrix *m1 = create_double_matrix(n+k, n+k);
    const size_t k0 = k*m1->tda;
    memset(m1->data, 0, m1->block->size*sizeof(double));
    for (size_t i = 0; i < n; i++)
      m1->data[k0+i*(m1->tda+1)] = m->data[i];
    return pure_double_matrix(m1);
  }
  case EXPR::CMATRIX: {
    gsl_matrix_complex *m = (gsl_matrix_complex*)x->data.mat.p;
    const size_t n1 = m->size1, n = m->size2;
    if (n1 != 1) return 0;
    gsl_matrix_complex *m1 = create_complex_matrix(n+k, n+k);
    const size_t k0 = k*m1->tda;
    memset(m1->data, 0, m1->block->size*2*sizeof(double));
    for (size_t i = 0; i < n; i++) {
      const size_t k = 2*k0+2*i*(m1->tda+1);
      m1->data[k] = m->data[2*i];
      m1->data[k+1] = m->data[2*i+1];
    }
    return pure_complex_matrix(m1);
  }
  case EXPR::IMATRIX: {
    gsl_matrix_int *m = (gsl_matrix_int*)x->data.mat.p;
    const size_t n1 = m->size1, n = m->size2;
    if (n1 != 1) return 0;
    gsl_matrix_int *m1 = create_int_matrix(n+k, n+k);
    const size_t k0 = k*m1->tda;
    memset(m1->data, 0, m1->block->size*sizeof(int));
    for (size_t i = 0; i < n; i++)
      m1->data[k0+i*(m1->tda+1)] = m->data[i];
    return pure_int_matrix(m1);
  }
#endif
  default:
    return 0;
  }
}

extern "C"
pure_expr *matrix_supdiagm(pure_expr *x, int32_t k)
{
  if (k<0) return matrix_subdiagm(x, -k);
  switch (x->tag) {
  case EXPR::MATRIX: {
    gsl_matrix_symbolic *m = (gsl_matrix_symbolic*)x->data.mat.p;
    const size_t n1 = m->size1, n = m->size2;
    if (n1 != 1) return 0;
    gsl_matrix_symbolic *m1 = create_symbolic_matrix(n+k, n+k);
    const size_t k0 = k;
    pure_expr *zero = pure_int(0);
    assert(zero);
    for (size_t i = 0; i < n+k; i++)
      for (size_t j = 0; j < n+k; j++)
	m1->data[i*m1->tda+j] = zero;
    for (size_t i = 0; i < n; i++)
      m1->data[k0+i*(m1->tda+1)] = m->data[i];
    return pure_symbolic_matrix(m1);
  }
#ifdef HAVE_GSL
  case EXPR::DMATRIX: {
    gsl_matrix *m = (gsl_matrix*)x->data.mat.p;
    const size_t n1 = m->size1, n = m->size2;
    if (n1 != 1) return 0;
    gsl_matrix *m1 = create_double_matrix(n+k, n+k);
    const size_t k0 = k;
    memset(m1->data, 0, m1->block->size*sizeof(double));
    for (size_t i = 0; i < n; i++)
      m1->data[k0+i*(m1->tda+1)] = m->data[i];
    return pure_double_matrix(m1);
  }
  case EXPR::CMATRIX: {
    gsl_matrix_complex *m = (gsl_matrix_complex*)x->data.mat.p;
    const size_t n1 = m->size1, n = m->size2;
    if (n1 != 1) return 0;
    gsl_matrix_complex *m1 = create_complex_matrix(n+k, n+k);
    const size_t k0 = k;
    memset(m1->data, 0, m1->block->size*2*sizeof(double));
    for (size_t i = 0; i < n; i++) {
      const size_t k = 2*k0+2*i*(m1->tda+1);
      m1->data[k] = m->data[2*i];
      m1->data[k+1] = m->data[2*i+1];
    }
    return pure_complex_matrix(m1);
  }
  case EXPR::IMATRIX: {
    gsl_matrix_int *m = (gsl_matrix_int*)x->data.mat.p;
    const size_t n1 = m->size1, n = m->size2;
    if (n1 != 1) return 0;
    gsl_matrix_int *m1 = create_int_matrix(n+k, n+k);
    const size_t k0 = k;
    memset(m1->data, 0, m1->block->size*sizeof(int));
    for (size_t i = 0; i < n; i++)
      m1->data[k0+i*(m1->tda+1)] = m->data[i];
    return pure_int_matrix(m1);
  }
#endif
  default:
    return 0;
  }
}

extern "C"
pure_expr *matrix_redim(pure_expr *x, int32_t n, int32_t m)
{
  void *p = 0;
  if (n<0 || m<0) return 0;
  const size_t n1 = (size_t)n, n2 = (size_t)m;
  switch (x->tag) {
  case EXPR::MATRIX: {
    gsl_matrix_symbolic *m = (gsl_matrix_symbolic*)x->data.mat.p;
    if (n1*n2!=m->size1*m->size2) return 0;
    if (m->tda == (m->size2>0?m->size2:1)) {
      // No copying necessary, just create a new view of this matrix.
      gsl_matrix_symbolic *m1 =
	(gsl_matrix_symbolic*)malloc(sizeof(gsl_matrix_symbolic));
      assert(m1);
      *m1 = *m;
      m1->size1 = n1; m1->tda = m1->size2 = n2;
      if (m1->tda == 0) m1->tda = 1;
      p = m1;
    } else {
      // Create a new matrix containing the same elements.
      pure_expr *y = pure_symbolic_matrix_dup(m);
      if (y) {
	gsl_matrix_symbolic *m = (gsl_matrix_symbolic*)y->data.mat.p;
	m->size1 = n1; m->tda = m->size2 = n2;
	if (m->tda == 0) m->tda = 1;
      }
      return y;
    }
    break;
  }
#ifdef HAVE_GSL
  case EXPR::DMATRIX: {
    gsl_matrix *m = (gsl_matrix*)x->data.mat.p;
    if (n1*n2!=m->size1*m->size2) return 0;
    if (m->tda == (m->size2>0?m->size2:1)) {
      // No copying necessary, just create a new view of this matrix.
      gsl_matrix *m1 = (gsl_matrix*)malloc(sizeof(gsl_matrix));
      assert(m1);
      *m1 = *m;
      m1->size1 = n1; m1->tda = m1->size2 = n2;
      if (m1->tda == 0) m1->tda = 1;
      p = m1;
    } else {
      // Create a new matrix containing the same elements.
      pure_expr *y = pure_double_matrix_dup(m);
      if (y) {
	gsl_matrix *m = (gsl_matrix*)y->data.mat.p;
	m->size1 = n1; m->tda = m->size2 = n2;
	if (m->tda == 0) m->tda = 1;
      }
      return y;
    }
    break;
  }
  case EXPR::CMATRIX: {
    gsl_matrix_complex *m = (gsl_matrix_complex*)x->data.mat.p;
    if (n1*n2!=m->size1*m->size2) return 0;
    if (m->tda == (m->size2>0?m->size2:1)) {
      // No copying necessary, just create a new view of this matrix.
      gsl_matrix_complex *m1 =
	(gsl_matrix_complex*)malloc(sizeof(gsl_matrix_complex));
      assert(m1);
      *m1 = *m;
      m1->size1 = n1; m1->tda = m1->size2 = n2;
      if (m1->tda == 0) m1->tda = 1;
      p = m1;
    } else {
      // Create a new matrix containing the same elements.
      pure_expr *y = pure_complex_matrix_dup(m);
      if (y) {
	gsl_matrix_complex *m = (gsl_matrix_complex*)y->data.mat.p;
	m->size1 = n1; m->tda = m->size2 = n2;
	if (m->tda == 0) m->tda = 1;
      }
      return y;
    }
    break;
  }
  case EXPR::IMATRIX: {
    gsl_matrix_int *m = (gsl_matrix_int*)x->data.mat.p;
    if (n1*n2!=m->size1*m->size2) return 0;
    if (m->tda == (m->size2>0?m->size2:1)) {
      // No copying necessary, just create a new view of this matrix.
      gsl_matrix_int *m1 =
	(gsl_matrix_int*)malloc(sizeof(gsl_matrix_int));
      assert(m1);
      *m1 = *m;
      m1->size1 = n1; m1->tda = m1->size2 = n2;
      if (m1->tda == 0) m1->tda = 1;
      p = m1;
    } else {
      // Create a new matrix containing the same elements.
      pure_expr *y = pure_int_matrix_dup(m);
      if (y) {
	gsl_matrix_int *m = (gsl_matrix_int*)y->data.mat.p;
	m->size1 = n1; m->tda = m->size2 = n2;
	if (m->tda == 0) m->tda = 1;
      }
      return y;
    }
    break;
  }
#endif
  default:
    return 0;
  }
  pure_expr *y = new_expr();
  y->tag = x->tag;
  y->data.mat.p = p;
  y->data.mat.refc = x->data.mat.refc;
  (*y->data.mat.refc)++;
  MEMDEBUG_NEW(y)
  return y;
}

static pure_expr *matrix_rowsv(uint32_t n, pure_expr **xs)
{
  int k = -1;
  size_t nrows = 0, ncols = 0;
  int32_t target = 0;
  bool have_matrix = false;
  pure_expr *x = 0;
  for (size_t i = 0; i < n; i++) {
    x = xs[i];
    switch (x->tag) {
    case EXPR::MATRIX: {
      gsl_matrix_symbolic *mp = (gsl_matrix_symbolic*)x->data.mat.p;
      if (mp->size1 > 0 && mp->size2 > 0) {
	if (k >= 0 && mp->size2 != (size_t)k)
	  goto err;
	nrows += mp->size1; k = mp->size2;
	set_target_type(target, EXPR::MATRIX);
	have_matrix = true;
      }
      break;
    }
#ifdef HAVE_GSL
    case EXPR::DBL:
      set_target_type(target, EXPR::DMATRIX);
      if (k >= 0 && k != 1) goto err;
      nrows++; k = 1;
      break;
    case EXPR::INT:
      set_target_type(target, EXPR::IMATRIX);
      if (k >= 0 && k != 1) goto err;
      nrows++; k = 1;
      break;
    case EXPR::APP: {
      double a, b;
      if (k >= 0 && k != 1) goto err;
      nrows++; k = 1;
      if (get_complex(x, a, b))
	set_target_type(target, EXPR::CMATRIX);
      else
	set_target_type(target, EXPR::MATRIX);
      break;
    }
    case EXPR::DMATRIX: {
      gsl_matrix *mp = (gsl_matrix*)x->data.mat.p;
      if (mp->size1 > 0 && mp->size2 > 0) {
	if (k >= 0 && mp->size2 != (size_t)k)
	  goto err;
	nrows += mp->size1; k = mp->size2;
	set_target_type(target, EXPR::DMATRIX);
	have_matrix = true;
      }
      break;
    }
    case EXPR::CMATRIX: {
      gsl_matrix_complex *mp = (gsl_matrix_complex*)x->data.mat.p;
      if (mp->size1 > 0 && mp->size2 > 0) {
	if (k >= 0 && mp->size2 != (size_t)k)
	  goto err;
	nrows += mp->size1; k = mp->size2;
	set_target_type(target, EXPR::CMATRIX);
	have_matrix = true;
      }
      break;
    }
    case EXPR::IMATRIX: {
      gsl_matrix_int *mp = (gsl_matrix_int*)x->data.mat.p;
      if (mp->size1 > 0 && mp->size2 > 0) {
	if (k >= 0 && mp->size2 != (size_t)k)
	  goto err;
	nrows += mp->size1; k = mp->size2;
	set_target_type(target, EXPR::IMATRIX);
	have_matrix = true;
      }
      break;
    }
#endif
    default:
      if (k >= 0 && k != 1) goto err;
      nrows++; k = 1;
      set_target_type(target, EXPR::MATRIX);
      break;
    }
  }
  if (n == 1 && have_matrix) return xs[0];
  if (k < 0) k = 0;
  ncols = k;
  if (target == 0) target = EXPR::MATRIX;
  switch (target) {
  case EXPR::MATRIX:
    return symbolic_matrix_rows(nrows, ncols, n, xs);
#ifdef HAVE_GSL
  case EXPR::DMATRIX:
    return double_matrix_rows(nrows, ncols, n, xs);
  case EXPR::CMATRIX:
    return complex_matrix_rows(nrows, ncols, n, xs);
  case EXPR::IMATRIX:
    return int_matrix_rows(nrows, ncols, n, xs);
#endif
  default:
    assert(0 && "this can't happen");
    return 0;
  }
 err:
  pure_throw(bad_matrix_exception(x));
  return 0;
}

static pure_expr *matrix_columnsv(uint32_t n, pure_expr **xs)
{
  int k = -1;
  size_t nrows = 0, ncols = 0;
  int32_t target = 0;
  bool have_matrix = false;
  pure_expr *x = 0;
  for (size_t i = 0; i < n; i++) {
    x = xs[i];
    switch (x->tag) {
    case EXPR::MATRIX: {
      gsl_matrix_symbolic *mp = (gsl_matrix_symbolic*)x->data.mat.p;
      if (mp->size1 > 0 && mp->size2 > 0) {
	if (k >= 0 && mp->size1 != (size_t)k)
	  goto err;
	ncols += mp->size2; k = mp->size1;
	set_target_type(target, EXPR::MATRIX);
	have_matrix = true;
      }
      break;
    }
#ifdef HAVE_GSL
    case EXPR::DBL:
      set_target_type(target, EXPR::DMATRIX);
      if (k >= 0 && k != 1) goto err;
      ncols++; k = 1;
      break;
    case EXPR::INT:
      set_target_type(target, EXPR::IMATRIX);
      if (k >= 0 && k != 1) goto err;
      ncols++; k = 1;
      break;
    case EXPR::APP: {
      double a, b;
      if (k >= 0 && k != 1) goto err;
      ncols++; k = 1;
      if (get_complex(x, a, b))
	set_target_type(target, EXPR::CMATRIX);
      else
	set_target_type(target, EXPR::MATRIX);
      break;
    }
    case EXPR::DMATRIX: {
      gsl_matrix *mp = (gsl_matrix*)x->data.mat.p;
      if (mp->size1 > 0 && mp->size2 > 0) {
	if (k >= 0 && mp->size1 != (size_t)k)
	  goto err;
	ncols += mp->size2; k = mp->size1;
	set_target_type(target, EXPR::DMATRIX);
	have_matrix = true;
      }
      break;
    }
    case EXPR::CMATRIX: {
      gsl_matrix_complex *mp = (gsl_matrix_complex*)x->data.mat.p;
      if (mp->size1 > 0 && mp->size2 > 0) {
	if (k >= 0 && mp->size1 != (size_t)k)
	  goto err;
	ncols += mp->size2; k = mp->size1;
	set_target_type(target, EXPR::CMATRIX);
	have_matrix = true;
      }
      break;
    }
    case EXPR::IMATRIX: {
      gsl_matrix_int *mp = (gsl_matrix_int*)x->data.mat.p;
      if (mp->size1 > 0 && mp->size2 > 0) {
	if (k >= 0 && mp->size1 != (size_t)k)
	  goto err;
	ncols += mp->size2; k = mp->size1;
	set_target_type(target, EXPR::IMATRIX);
	have_matrix = true;
      }
      break;
    }
#endif
    default:
      if (k >= 0 && k != 1) goto err;
      ncols++; k = 1;
      set_target_type(target, EXPR::MATRIX);
      break;
    }
  }
  if (n == 1 && have_matrix) return xs[0];
  if (k < 0) k = 0;
  nrows = k;
  if (target == 0) target = EXPR::MATRIX;
  switch (target) {
  case EXPR::MATRIX:
    return symbolic_matrix_columns(nrows, ncols, n, xs);
#ifdef HAVE_GSL
  case EXPR::DMATRIX:
    return double_matrix_columns(nrows, ncols, n, xs);
  case EXPR::CMATRIX:
    return complex_matrix_columns(nrows, ncols, n, xs);
  case EXPR::IMATRIX:
    return int_matrix_columns(nrows, ncols, n, xs);
#endif
  default:
    assert(0 && "this can't happen");
    return 0;
  }
 err:
  pure_throw(bad_matrix_exception(x));
  return 0;
}

extern "C"
pure_expr *matrix_matcat(pure_expr *x)
{
  switch (x->tag) {
  case EXPR::MATRIX: {
    gsl_matrix_symbolic *mp = (gsl_matrix_symbolic*)x->data.mat.p;
    if (mp->size1 == 0 || mp->size2 == 0) {
      // empty output matrix
      gsl_matrix_symbolic *sm = create_symbolic_matrix(0, 0);
      return pure_symbolic_matrix(sm);
    }
    pure_expr **ys = (pure_expr**)malloc(mp->size1*sizeof(pure_expr*));
    if (!ys)
      return 0; // FIXME: should maybe throw an exception here
    for (size_t i = 0; i < mp->size1; i++) {
      pure_expr **xs = mp->data+i*mp->tda;
      if (!(ys[i] = pure_matrix_columnsv(mp->size2, xs))) {
	for (size_t j = 0; j < i; j++)
	  pure_freenew(ys[j]);
	free(ys);
	return 0;
      }
    }
    pure_expr *y = pure_matrix_rowsv(mp->size1, ys);
    free(ys);
    return y;
  }
#ifdef HAVE_GSL
  case EXPR::DMATRIX:
  case EXPR::CMATRIX:
  case EXPR::IMATRIX:
    return x;
  default:
    return 0;
#endif
  }
}

extern "C"
pure_expr *matrix_rows(pure_expr *xs)
{
  size_t n;
  pure_expr **elems;
  if (pure_is_listv(xs, &n, &elems)) {
    pure_expr *ret = matrix_rowsv(n, elems);
    if (elems) free(elems);
    return ret;
  } else
    return 0;
}

extern "C"
pure_expr *matrix_columns(pure_expr *xs)
{
  size_t n;
  pure_expr **elems;
  if (pure_is_listv(xs, &n, &elems)) {
    pure_expr *ret = matrix_columnsv(n, elems);
    if (elems) free(elems);
    return ret;
  } else
    return 0;
}

extern "C"
pure_expr *matrix_transpose(pure_expr *x)
{
  switch (x->tag) {
  case EXPR::MATRIX: {
    gsl_matrix_symbolic *m1 = (gsl_matrix_symbolic*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    gsl_matrix_symbolic *m2 = create_symbolic_matrix(m, n);
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++)
	m2->data[j*m2->tda+i] = m1->data[i*m1->tda+j];
    return pure_symbolic_matrix(m2);
  }
#ifdef HAVE_GSL
  case EXPR::DMATRIX: {
    gsl_matrix *m1 = (gsl_matrix*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    gsl_matrix *m2 = create_double_matrix(m, n);
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++)
	m2->data[j*m2->tda+i] = m1->data[i*m1->tda+j];
    return pure_double_matrix(m2);
  }
  case EXPR::CMATRIX: {
    gsl_matrix_complex *m1 = (gsl_matrix_complex*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    gsl_matrix_complex *m2 = create_complex_matrix(m, n);
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++) {
	size_t k = 2*(i*m1->tda+j), l = 2*(j*m2->tda+i);
	m2->data[l] = m1->data[k];
	m2->data[l+1] = m1->data[k+1];
      }
    return pure_complex_matrix(m2);
  }
  case EXPR::IMATRIX: {
    gsl_matrix_int *m1 = (gsl_matrix_int*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    gsl_matrix_int *m2 = create_int_matrix(m, n);
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++)
	m2->data[j*m2->tda+i] = m1->data[i*m1->tda+j];
    return pure_int_matrix(m2);
  }
#endif
  default:
    return 0;
  }
}

extern "C"
pure_expr *matrix_double(pure_expr *x)
{
#ifdef HAVE_GSL
  switch (x->tag) {
  case EXPR::MATRIX: {
    gsl_matrix_symbolic *m1 = (gsl_matrix_symbolic*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++)
	if (m1->data[i*m1->tda+j]->tag != EXPR::DBL)
	  return 0;
    gsl_matrix *m2 = create_double_matrix(n, m);
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++)
	m2->data[i*m2->tda+j] = m1->data[i*m1->tda+j]->data.d;
    return pure_double_matrix(m2);
  }
  case EXPR::DMATRIX:
    return x;
  case EXPR::IMATRIX: {
    gsl_matrix_int *m1 = (gsl_matrix_int*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    gsl_matrix *m2 = create_double_matrix(n, m);
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++)
	m2->data[i*m2->tda+j] = (double)m1->data[i*m1->tda+j];
    return pure_double_matrix(m2);
  }
  case EXPR::CMATRIX: {
    gsl_matrix_complex *m1 = (gsl_matrix_complex*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    gsl_matrix *m2 = create_double_matrix(n, 2*m);
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++) {
	size_t k = i*m2->tda+2*j;
	size_t l = 2*(i*m1->tda+j);
	m2->data[k] = m1->data[l];
	m2->data[k+1] = m1->data[l+1];
      }
    return pure_double_matrix(m2);
  }
  default: {
    size_t n;
    pure_expr **xs;
    if (pure_is_listv(x, &n, &xs)) {
      for (size_t i = 0; i < n; i++)
	if (xs[i]->tag != EXPR::DBL) {
	  free(xs);
	  return 0;
	}
      gsl_matrix *m2 = create_double_matrix(1, n);
      for (size_t i = 0; i < n; i++)
	m2->data[i] = xs[i]->data.d;
      if (xs) free(xs);
      return pure_double_matrix(m2);
    } else
      return 0;
  }
  }
#else
  return 0;
#endif
}

extern "C"
pure_expr *matrix_complex(pure_expr *x)
{
#ifdef HAVE_GSL
  switch (x->tag) {
  case EXPR::MATRIX: {
    gsl_matrix_symbolic *m1 = (gsl_matrix_symbolic*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++)
	if (!is_complex(m1->data[i*m1->tda+j]))
	  return 0;
    gsl_matrix_complex *m2 = create_complex_matrix(n, m);
    double a = 0.0, b = 0.0;
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++) {
	size_t k = 2*(i*m2->tda+j);
	get_complex(m1->data[i*m1->tda+j], a, b);
	m2->data[k] = a;
	m2->data[k+1] = b;
      }
    return pure_complex_matrix(m2);
  }
  case EXPR::DMATRIX: {
    gsl_matrix *m1 = (gsl_matrix*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    gsl_matrix_complex *m2 = create_complex_matrix(n, m);
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++) {
	size_t k = 2*(i*m2->tda+j);
	m2->data[k] = m1->data[i*m1->tda+j];
	m2->data[k+1] = 0.0;
      }
    return pure_complex_matrix(m2);
  }
  case EXPR::IMATRIX: {
    gsl_matrix_int *m1 = (gsl_matrix_int*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    gsl_matrix_complex *m2 = create_complex_matrix(n, m);
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++) {
	size_t k = 2*(i*m2->tda+j);
	m2->data[k] = (double)m1->data[i*m1->tda+j];
	m2->data[k+1] = 0.0;
      }
    return pure_complex_matrix(m2);
  }
  case EXPR::CMATRIX:
    return x;
  default: {
    size_t n;
    pure_expr **xs;
    if (pure_is_listv(x, &n, &xs)) {
      for (size_t i = 0; i < n; i++)
	if (!is_complex(xs[i])) {
	  free(xs);
	  return 0;
	}
      gsl_matrix_complex *m2 = create_complex_matrix(1, n);
      double a = 0.0, b = 0.0;
      for (size_t i = 0; i < n; i++) {
	size_t k = 2*i;
	get_complex(xs[i], a, b);
	m2->data[k] = a;
	m2->data[k+1] = b;
      }
      if (xs) free(xs);
      return pure_complex_matrix(m2);
    } else
      return 0;
  }
  }
#else
  return 0;
#endif
}

extern "C"
pure_expr *matrix_int(pure_expr *x)
{
#ifdef HAVE_GSL
  switch (x->tag) {
  case EXPR::MATRIX: {
    gsl_matrix_symbolic *m1 = (gsl_matrix_symbolic*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++)
	if (m1->data[i*m1->tda+j]->tag != EXPR::INT)
	  return 0;
    gsl_matrix_int *m2 = create_int_matrix(n, m);
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++)
	m2->data[i*m2->tda+j] = m1->data[i*m1->tda+j]->data.i;
    return pure_int_matrix(m2);
  }
  case EXPR::DMATRIX: {
    gsl_matrix *m1 = (gsl_matrix*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    gsl_matrix_int *m2 = create_int_matrix(n, m);
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++)
	m2->data[i*m2->tda+j] = (int)m1->data[i*m1->tda+j];
    return pure_int_matrix(m2);
  }
  case EXPR::IMATRIX:
    return x;
  case EXPR::CMATRIX: {
    gsl_matrix_complex *m1 = (gsl_matrix_complex*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    gsl_matrix_int *m2 = create_int_matrix(n, 2*m);
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++) {
	size_t k = i*m2->tda+2*j;
	size_t l = 2*(i*m1->tda+j);
	m2->data[k] = (int)m1->data[l];
	m2->data[k+1] = (int)m1->data[l+1];
      }
    return pure_int_matrix(m2);
  }
  default: {
    size_t n;
    pure_expr **xs;
    if (pure_is_listv(x, &n, &xs)) {
      for (size_t i = 0; i < n; i++)
	if (xs[i]->tag != EXPR::INT) {
	  free(xs);
	  return 0;
	}
      gsl_matrix_int *m2 = create_int_matrix(1, n);
      for (size_t i = 0; i < n; i++)
	m2->data[i] = xs[i]->data.i;
      if (xs) free(xs);
      return pure_int_matrix(m2);
    } else
      return 0;
  }
  }
#else
  return 0;
#endif
}

extern "C"
pure_expr *matrix_symbolic(pure_expr *x)
{
#ifdef HAVE_GSL
  switch (x->tag) {
  case EXPR::MATRIX:
    return x;
  case EXPR::DMATRIX: {
    gsl_matrix *m1 = (gsl_matrix*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    gsl_matrix_symbolic *m2 = create_symbolic_matrix(n, m);
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++)
	m2->data[i*m2->tda+j] = pure_double(m1->data[i*m1->tda+j]);
    return pure_symbolic_matrix(m2);
  }
  case EXPR::IMATRIX: {
    gsl_matrix_int *m1 = (gsl_matrix_int*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    gsl_matrix_symbolic *m2 = create_symbolic_matrix(n, m);
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++)
	m2->data[i*m2->tda+j] = pure_int(m1->data[i*m1->tda+j]);
    return pure_symbolic_matrix(m2);
  }
  case EXPR::CMATRIX: {
    gsl_matrix_complex *m1 = (gsl_matrix_complex*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    gsl_matrix_symbolic *m2 = create_symbolic_matrix(n, m);
    interpreter& interp = *interpreter::g_interp;
    symbol &rect = interp.symtab.complex_rect_sym();
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++) {
	size_t l = 2*(i*m1->tda+j);
	m2->data[i*m2->tda+j] = make_complex2(rect, m1->data[l], m1->data[l+1]);
      }
    return pure_symbolic_matrix(m2);
  }
  default: {
    size_t n;
    pure_expr **xs;
    if (pure_is_listv(x, &n, &xs)) {
      gsl_matrix_symbolic *m2 = create_symbolic_matrix(1, n);
      if (xs) {
	memcpy(m2->data, xs, n*sizeof(pure_expr*));
	free(xs);
      }
      return pure_symbolic_matrix(m2);
    } else
      return 0;
  }
  }
#else
  return 0;
#endif
}

extern "C"
pure_expr *matrix_re(pure_expr *x)
{
#ifdef HAVE_GSL
  switch (x->tag) {
  case EXPR::CMATRIX: {
    gsl_matrix_complex *m1 = (gsl_matrix_complex*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    gsl_matrix *m2 = create_double_matrix(n, m);
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++) {
	size_t k = 2*(i*m1->tda+j), l = i*m2->tda+j;
	m2->data[l] = m1->data[k];
      }
    return pure_double_matrix(m2);
  }
  case EXPR::DMATRIX:
  case EXPR::IMATRIX:
    return x;
  default:
    return 0;
  }
#else
  return 0;
#endif
}

extern "C"
pure_expr *matrix_im(pure_expr *x)
{
#ifdef HAVE_GSL
  switch (x->tag) {
  case EXPR::CMATRIX: {
    gsl_matrix_complex *m1 = (gsl_matrix_complex*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    gsl_matrix *m2 = create_double_matrix(n, m);
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++) {
	size_t k = 2*(i*m1->tda+j), l = i*m2->tda+j;
	m2->data[l] = m1->data[k+1];
      }
    return pure_double_matrix(m2);
  }
  case EXPR::DMATRIX: {
    gsl_matrix *m1 = (gsl_matrix*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    gsl_matrix *m2 = create_double_matrix(n, m);
    memset(m2->data, 0, n*m*sizeof(double));
    return pure_double_matrix(m2);
  }
  case EXPR::IMATRIX: {
    gsl_matrix_int *m1 = (gsl_matrix_int*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    gsl_matrix_int *m2 = create_int_matrix(n, m);
    memset(m2->data, 0, n*m*sizeof(int));
    return pure_int_matrix(m2);
  }
  default:
    return 0;
  }
#else
  return 0;
#endif
}

extern "C"
pure_expr *matrix_conj(pure_expr *x)
{
#ifdef HAVE_GSL
  switch (x->tag) {
  case EXPR::CMATRIX: {
    gsl_matrix_complex *m1 = (gsl_matrix_complex*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    gsl_matrix_complex *m2 = create_complex_matrix(n, m);
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++) {
	size_t k = 2*(i*m1->tda+j), l = 2*(i*m2->tda+j);
	m2->data[l] = m1->data[k];
	m2->data[l+1] = -m1->data[k+1];
      }
    return pure_complex_matrix(m2);
  }
  case EXPR::DMATRIX:
  case EXPR::IMATRIX:
    return x;
  default:
    return 0;
  }
#else
  return 0;
#endif
}

extern "C"
pure_expr *matrix_from_double_array_nodup(uint32_t n1, uint32_t n2, void *p)
{
#ifdef HAVE_GSL
  if (n1 == 0 || n2 == 0) // empty matrix
    return pure_double_matrix(create_double_matrix(n1, n2));
  if (!p) return 0;
  gsl_matrix_view v = gsl_matrix_view_array((double*)p, n1, n2);
  // take a copy of the view matrix
  gsl_matrix *m = (gsl_matrix*)malloc(sizeof(gsl_matrix));
  assert(m && v.matrix.data);
  *m = v.matrix;
  pure_expr *x = new_expr();
  x->tag = EXPR::DMATRIX;
  x->data.mat.p = m;
  x->data.mat.refc = new uint32_t;
  *x->data.mat.refc = 1;
  MEMDEBUG_NEW(x)
  return x;
#else
  return 0;
#endif
}

extern "C"
pure_expr *matrix_from_complex_array_nodup(uint32_t n1, uint32_t n2, void *p)
{
#ifdef HAVE_GSL
  if (n1 == 0 || n2 == 0) // empty matrix
    return pure_complex_matrix(create_complex_matrix(n1, n2));
  if (!p) return 0;
  gsl_matrix_complex_view v =
    gsl_matrix_complex_view_array((double*)p, n1, n2);
  // take a copy of the view matrix
  gsl_matrix_complex *m =
    (gsl_matrix_complex*)malloc(sizeof(gsl_matrix_complex));
  assert(m && v.matrix.data);
  *m = v.matrix;
  pure_expr *x = new_expr();
  x->tag = EXPR::CMATRIX;
  x->data.mat.p = m;
  x->data.mat.refc = new uint32_t;
  *x->data.mat.refc = 1;
  MEMDEBUG_NEW(x)
  return x;
#else
  return 0;
#endif
}

extern "C"
pure_expr *matrix_from_int_array_nodup(uint32_t n1, uint32_t n2, void *p)
{
#ifdef HAVE_GSL
  if (n1 == 0 || n2 == 0) // empty matrix
    return pure_int_matrix(create_int_matrix(n1, n2));
  if (!p) return 0;
  gsl_matrix_int_view v = gsl_matrix_int_view_array((int*)p, n1, n2);
  // take a copy of the view matrix
  gsl_matrix_int *m = (gsl_matrix_int*)malloc(sizeof(gsl_matrix_int));
  assert(m && v.matrix.data);
  *m = v.matrix;
  pure_expr *x = new_expr();
  x->tag = EXPR::IMATRIX;
  x->data.mat.p = m;
  x->data.mat.refc = new uint32_t;
  *x->data.mat.refc = 1;
  MEMDEBUG_NEW(x)
  return x;
#else
  return 0;
#endif
}

extern "C"
pure_expr *matrix_from_double_array(uint32_t n1, uint32_t n2, void *p)
{
#ifdef HAVE_GSL
  if (n1 == 0 || n2 == 0) // empty matrix
    return pure_double_matrix(create_double_matrix(n1, n2));
  if (!p)
    p = calloc(n1*n2, sizeof(double));
  else {
    void *q = malloc(n1*n2*sizeof(double));
    memcpy(q, p, n1*n2*sizeof(double));
    p = q;
  }
  if (!p) return 0;
  gsl_matrix_view v = gsl_matrix_view_array((double*)p, n1, n2);
  // take a copy of the view matrix
  gsl_matrix *m = (gsl_matrix*)malloc(sizeof(gsl_matrix));
  gsl_block *b = (gsl_block*)malloc(sizeof(gsl_block));
  assert(m && b && v.matrix.data);
  *m = v.matrix;
  b->size = n1*n2;
  b->data = m->data;
  m->block = b;
  pure_expr *x = new_expr();
  x->tag = EXPR::DMATRIX;
  x->data.mat.p = m;
  x->data.mat.refc = new uint32_t;
  *x->data.mat.refc = 1;
  MEMDEBUG_NEW(x)
  return x;
#else
  return 0;
#endif
}

extern "C"
pure_expr *matrix_from_complex_array(uint32_t n1, uint32_t n2, void *p)
{
#ifdef HAVE_GSL
  if (n1 == 0 || n2 == 0) // empty matrix
    return pure_complex_matrix(create_complex_matrix(n1, n2));
  if (!p)
    p = calloc(2*n1*n2, sizeof(double));
  else {
    void *q = malloc(2*n1*n2*sizeof(double));
    memcpy(q, p, 2*n1*n2*sizeof(double));
    p = q;
  }
  if (!p) return 0;
  gsl_matrix_complex_view v =
    gsl_matrix_complex_view_array((double*)p, n1, n2);
  // take a copy of the view matrix
  gsl_matrix_complex *m =
    (gsl_matrix_complex*)malloc(sizeof(gsl_matrix_complex));
  gsl_block_complex *b = (gsl_block_complex*)malloc(sizeof(gsl_block_complex));
  assert(m && b && v.matrix.data);
  *m = v.matrix;
  b->size = n1*n2;
  b->data = m->data;
  m->block = b;
  pure_expr *x = new_expr();
  x->tag = EXPR::CMATRIX;
  x->data.mat.p = m;
  x->data.mat.refc = new uint32_t;
  *x->data.mat.refc = 1;
  MEMDEBUG_NEW(x)
  return x;
#else
  return 0;
#endif
}

extern "C"
pure_expr *matrix_from_int_array(uint32_t n1, uint32_t n2, void *p)
{
#ifdef HAVE_GSL
  if (n1 == 0 || n2 == 0) // empty matrix
    return pure_int_matrix(create_int_matrix(n1, n2));
  if (!p)
    p = calloc(n1*n2, sizeof(int));
  else {
    void *q = malloc(n1*n2*sizeof(int));
    memcpy(q, p, n1*n2*sizeof(int));
    p = q;
  }
  if (!p) return 0;
  gsl_matrix_int_view v = gsl_matrix_int_view_array((int*)p, n1, n2);
  // take a copy of the view matrix
  gsl_matrix_int *m = (gsl_matrix_int*)malloc(sizeof(gsl_matrix_int));
  gsl_block_int *b = (gsl_block_int*)malloc(sizeof(gsl_block_int));
  assert(m && b && v.matrix.data);
  *m = v.matrix;
  b->size = n1*n2;
  b->data = m->data;
  m->block = b;
  pure_expr *x = new_expr();
  x->tag = EXPR::IMATRIX;
  x->data.mat.p = m;
  x->data.mat.refc = new uint32_t;
  *x->data.mat.refc = 1;
  MEMDEBUG_NEW(x)
  return x;
#else
  return 0;
#endif
}

extern "C"
void *matrix_to_double_array(void *p, pure_expr *x)
{
#ifdef HAVE_GSL
  switch (x->tag) {
  case EXPR::CMATRIX: {
    gsl_matrix_complex *m1 = (gsl_matrix_complex*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    if (n==0 || m==0) return p;
    if (!p) p = malloc(2*n*m*sizeof(double));
    if (!p) return 0;
    double *q = (double*)p;
    size_t k = 0;
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++) {
	size_t l = 2*(i*m1->tda+j);
	q[k++] = m1->data[l];
	q[k++] = m1->data[l+1];
      }
    return p;
  }
  case EXPR::DMATRIX: {
    gsl_matrix *m1 = (gsl_matrix*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    if (n==0 || m==0) return p;
    if (!p) p = malloc(n*m*sizeof(double));
    if (!p) return 0;
    double *q = (double*)p;
    size_t k = 0;
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++)
	q[k++] = m1->data[i*m1->tda+j];
    return p;
  }
  case EXPR::IMATRIX: {
    gsl_matrix_int *m1 = (gsl_matrix_int*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    if (n==0 || m==0) return p;
    if (!p) p = malloc(n*m*sizeof(double));
    if (!p) return 0;
    double *q = (double*)p;
    size_t k = 0;
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++)
	q[k++] = (double)m1->data[i*m1->tda+j];
    return p;
  }
  default:
    return 0;
  }
#else
  return 0;
#endif
}

extern "C"
void *matrix_to_complex_array(void *p, pure_expr *x)
{
#ifdef HAVE_GSL
  switch (x->tag) {
  case EXPR::CMATRIX: {
    gsl_matrix_complex *m1 = (gsl_matrix_complex*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    if (n==0 || m==0) return p;
    if (!p) p = malloc(2*n*m*sizeof(double));
    if (!p) return 0;
    double *q = (double*)p;
    size_t k = 0;
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++) {
	size_t l = 2*(i*m1->tda+j);
	q[k++] = m1->data[l];
	q[k++] = m1->data[l+1];
      }
    return p;
  }
  case EXPR::DMATRIX: {
    gsl_matrix *m1 = (gsl_matrix*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    if (n==0 || m==0) return p;
    if (!p) p = malloc(2*n*m*sizeof(double));
    if (!p) return 0;
    double *q = (double*)p;
    size_t k = 0;
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++) {
	q[k++] = m1->data[i*m1->tda+j];
	q[k++] = 0.0;
      }
    return p;
  }
  case EXPR::IMATRIX: {
    gsl_matrix_int *m1 = (gsl_matrix_int*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    if (n==0 || m==0) return p;
    if (!p) p = malloc(2*n*m*sizeof(double));
    if (!p) return 0;
    double *q = (double*)p;
    size_t k = 0;
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++) {
	q[k++] = (double)m1->data[i*m1->tda+j];
	q[k++] = 0.0;
      }
    return p;
  }
  default:
    return 0;
  }
#else
  return 0;
#endif
}

extern "C"
void *matrix_to_int_array(void *p, pure_expr *x)
{
#ifdef HAVE_GSL
  switch (x->tag) {
  case EXPR::CMATRIX: {
    gsl_matrix_complex *m1 = (gsl_matrix_complex*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    if (n==0 || m==0) return p;
    if (!p) p = malloc(2*n*m*sizeof(int));
    if (!p) return 0;
    int *q = (int*)p;
    size_t k = 0;
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++) {
	size_t l = 2*(i*m1->tda+j);
	q[k++] = (int)m1->data[l];
	q[k++] = (int)m1->data[l+1];
      }
    return p;
  }
  case EXPR::DMATRIX: {
    gsl_matrix *m1 = (gsl_matrix*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    if (n==0 || m==0) return p;
    if (!p) p = malloc(n*m*sizeof(int));
    if (!p) return 0;
    int *q = (int*)p;
    size_t k = 0;
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++)
	q[k++] = (int)m1->data[i*m1->tda+j];
    return p;
  }
  case EXPR::IMATRIX: {
    gsl_matrix_int *m1 = (gsl_matrix_int*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    if (n==0 || m==0) return p;
    if (!p) p = malloc(n*m*sizeof(int));
    if (!p) return 0;
    int *q = (int*)p;
    size_t k = 0;
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++)
	q[k++] = m1->data[i*m1->tda+j];
    return p;
  }
  default:
    return 0;
  }
#else
  return 0;
#endif
}

extern "C"
pure_expr *matrix_from_float_array(uint32_t n1, uint32_t n2, void *p)
{
#ifdef HAVE_GSL
  if (n1 == 0 || n2 == 0) // empty matrix
    return pure_double_matrix(create_double_matrix(n1, n2));
  if (!p)
    p = calloc(n1*n2, sizeof(double));
  else {
    void *q = malloc(n1*n2*sizeof(double));
    float *p1 = (float*)p; double *q1 = (double*)q;
    for (size_t i = 0; i < n1*n2; i++) q1[i] = (double)p1[i];
    p = q;
  }
  if (!p) return 0;
  gsl_matrix_view v = gsl_matrix_view_array((double*)p, n1, n2);
  // take a copy of the view matrix
  gsl_matrix *m = (gsl_matrix*)malloc(sizeof(gsl_matrix));
  gsl_block *b = (gsl_block*)malloc(sizeof(gsl_block));
  assert(m && b && v.matrix.data);
  *m = v.matrix;
  b->size = n1*n2;
  b->data = m->data;
  m->block = b;
  pure_expr *x = new_expr();
  x->tag = EXPR::DMATRIX;
  x->data.mat.p = m;
  x->data.mat.refc = new uint32_t;
  *x->data.mat.refc = 1;
  MEMDEBUG_NEW(x)
  return x;
#else
  return 0;
#endif
}

extern "C"
pure_expr *matrix_from_complex_float_array(uint32_t n1, uint32_t n2, void *p)
{
#ifdef HAVE_GSL
  if (n1 == 0 || n2 == 0) // empty matrix
    return pure_complex_matrix(create_complex_matrix(n1, n2));
  if (!p)
    p = calloc(2*n1*n2, sizeof(double));
  else {
    void *q = malloc(2*n1*n2*sizeof(double));
    float *p1 = (float*)p; double *q1 = (double*)q;
    for (size_t i = 0; i < 2*n1*n2; i++) q1[i] = (double)p1[i];
    p = q;
  }
  if (!p) return 0;
  gsl_matrix_complex_view v =
    gsl_matrix_complex_view_array((double*)p, n1, n2);
  // take a copy of the view matrix
  gsl_matrix_complex *m =
    (gsl_matrix_complex*)malloc(sizeof(gsl_matrix_complex));
  gsl_block_complex *b = (gsl_block_complex*)malloc(sizeof(gsl_block_complex));
  assert(m && b && v.matrix.data);
  *m = v.matrix;
  b->size = n1*n2;
  b->data = m->data;
  m->block = b;
  pure_expr *x = new_expr();
  x->tag = EXPR::CMATRIX;
  x->data.mat.p = m;
  x->data.mat.refc = new uint32_t;
  *x->data.mat.refc = 1;
  MEMDEBUG_NEW(x)
  return x;
#else
  return 0;
#endif
}

extern "C"
pure_expr *matrix_from_short_array(uint32_t n1, uint32_t n2, void *p)
{
#ifdef HAVE_GSL
  if (n1 == 0 || n2 == 0) // empty matrix
    return pure_int_matrix(create_int_matrix(n1, n2));
  if (!p)
    p = calloc(n1*n2, sizeof(int));
  else {
    void *q = malloc(n1*n2*sizeof(int));
    short *p1 = (short*)p; int *q1 = (int*)q;
    for (size_t i = 0; i < n1*n2; i++) q1[i] = (int)p1[i];
    p = q;
  }
  if (!p) return 0;
  gsl_matrix_int_view v = gsl_matrix_int_view_array((int*)p, n1, n2);
  // take a copy of the view matrix
  gsl_matrix_int *m = (gsl_matrix_int*)malloc(sizeof(gsl_matrix_int));
  gsl_block_int *b = (gsl_block_int*)malloc(sizeof(gsl_block_int));
  assert(m && b && v.matrix.data);
  *m = v.matrix;
  b->size = n1*n2;
  b->data = m->data;
  m->block = b;
  pure_expr *x = new_expr();
  x->tag = EXPR::IMATRIX;
  x->data.mat.p = m;
  x->data.mat.refc = new uint32_t;
  *x->data.mat.refc = 1;
  MEMDEBUG_NEW(x)
  return x;
#else
  return 0;
#endif
}

extern "C"
pure_expr *matrix_from_byte_array(uint32_t n1, uint32_t n2, void *p)
{
#ifdef HAVE_GSL
  if (n1 == 0 || n2 == 0) // empty matrix
    return pure_int_matrix(create_int_matrix(n1, n2));
  if (!p)
    p = calloc(n1*n2, sizeof(int));
  else {
    void *q = malloc(n1*n2*sizeof(int));
    int8_t *p1 = (int8_t*)p; int *q1 = (int*)q;
    for (size_t i = 0; i < n1*n2; i++) q1[i] = (int)p1[i];
    p = q;
  }
  if (!p) return 0;
  gsl_matrix_int_view v = gsl_matrix_int_view_array((int*)p, n1, n2);
  // take a copy of the view matrix
  gsl_matrix_int *m = (gsl_matrix_int*)malloc(sizeof(gsl_matrix_int));
  gsl_block_int *b = (gsl_block_int*)malloc(sizeof(gsl_block_int));
  assert(m && b && v.matrix.data);
  *m = v.matrix;
  b->size = n1*n2;
  b->data = m->data;
  m->block = b;
  pure_expr *x = new_expr();
  x->tag = EXPR::IMATRIX;
  x->data.mat.p = m;
  x->data.mat.refc = new uint32_t;
  *x->data.mat.refc = 1;
  MEMDEBUG_NEW(x)
  return x;
#else
  return 0;
#endif
}

extern "C"
void *matrix_to_float_array(void *p, pure_expr *x)
{
#ifdef HAVE_GSL
  switch (x->tag) {
  case EXPR::CMATRIX: {
    gsl_matrix_complex *m1 = (gsl_matrix_complex*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    if (n==0 || m==0) return p;
    if (!p) p = malloc(2*n*m*sizeof(float));
    if (!p) return 0;
    float *q = (float*)p;
    size_t k = 0;
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++) {
	size_t l = 2*(i*m1->tda+j);
	q[k++] = (float)m1->data[l];
	q[k++] = (float)m1->data[l+1];
      }
    return p;
  }
  case EXPR::DMATRIX: {
    gsl_matrix *m1 = (gsl_matrix*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    if (n==0 || m==0) return p;
    if (!p) p = malloc(n*m*sizeof(float));
    if (!p) return 0;
    float *q = (float*)p;
    size_t k = 0;
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++)
	q[k++] = (float)m1->data[i*m1->tda+j];
    return p;
  }
  case EXPR::IMATRIX: {
    gsl_matrix_int *m1 = (gsl_matrix_int*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    if (n==0 || m==0) return p;
    if (!p) p = malloc(n*m*sizeof(float));
    if (!p) return 0;
    float *q = (float*)p;
    size_t k = 0;
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++)
	q[k++] = (float)m1->data[i*m1->tda+j];
    return p;
  }
  default:
    return 0;
  }
#else
  return 0;
#endif
}

extern "C"
void *matrix_to_complex_float_array(void *p, pure_expr *x)
{
#ifdef HAVE_GSL
  switch (x->tag) {
  case EXPR::CMATRIX: {
    gsl_matrix_complex *m1 = (gsl_matrix_complex*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    if (n==0 || m==0) return p;
    if (!p) p = malloc(2*n*m*sizeof(float));
    if (!p) return 0;
    float *q = (float*)p;
    size_t k = 0;
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++) {
	size_t l = 2*(i*m1->tda+j);
	q[k++] = (float)m1->data[l];
	q[k++] = (float)m1->data[l+1];
      }
    return p;
  }
  case EXPR::DMATRIX: {
    gsl_matrix *m1 = (gsl_matrix*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    if (n==0 || m==0) return p;
    if (!p) p = malloc(2*n*m*sizeof(float));
    if (!p) return 0;
    float *q = (float*)p;
    size_t k = 0;
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++) {
	q[k++] = (float)m1->data[i*m1->tda+j];
	q[k++] = 0.0;
      }
    return p;
  }
  case EXPR::IMATRIX: {
    gsl_matrix_int *m1 = (gsl_matrix_int*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    if (n==0 || m==0) return p;
    if (!p) p = malloc(2*n*m*sizeof(float));
    if (!p) return 0;
    float *q = (float*)p;
    size_t k = 0;
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++) {
	q[k++] = (float)m1->data[i*m1->tda+j];
	q[k++] = 0.0;
      }
    return p;
  }
  default:
    return 0;
  }
#else
  return 0;
#endif
}

extern "C"
void *matrix_to_short_array(void *p, pure_expr *x)
{
#ifdef HAVE_GSL
  switch (x->tag) {
  case EXPR::CMATRIX: {
    gsl_matrix_complex *m1 = (gsl_matrix_complex*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    if (n==0 || m==0) return p;
    if (!p) p = malloc(2*n*m*sizeof(short));
    if (!p) return 0;
    short *q = (short*)p;
    size_t k = 0;
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++) {
	size_t l = 2*(i*m1->tda+j);
	q[k++] = (short)m1->data[l];
	q[k++] = (short)m1->data[l+1];
      }
    return p;
  }
  case EXPR::DMATRIX: {
    gsl_matrix *m1 = (gsl_matrix*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    if (n==0 || m==0) return p;
    if (!p) p = malloc(n*m*sizeof(short));
    if (!p) return 0;
    short *q = (short*)p;
    size_t k = 0;
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++)
	q[k++] = (short)m1->data[i*m1->tda+j];
    return p;
  }
  case EXPR::IMATRIX: {
    gsl_matrix_int *m1 = (gsl_matrix_int*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    if (n==0 || m==0) return p;
    if (!p) p = malloc(n*m*sizeof(short));
    if (!p) return 0;
    short *q = (short*)p;
    size_t k = 0;
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++)
	q[k++] = (short)m1->data[i*m1->tda+j];
    return p;
  }
  default:
    return 0;
  }
#else
  return 0;
#endif
}

extern "C"
void *matrix_to_byte_array(void *p, pure_expr *x)
{
#ifdef HAVE_GSL
  switch (x->tag) {
  case EXPR::CMATRIX: {
    gsl_matrix_complex *m1 = (gsl_matrix_complex*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    if (n==0 || m==0) return p;
    if (!p) p = malloc(2*n*m*sizeof(int8_t));
    if (!p) return 0;
    int8_t *q = (int8_t*)p;
    size_t k = 0;
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++) {
	size_t l = 2*(i*m1->tda+j);
	q[k++] = (int8_t)m1->data[l];
	q[k++] = (int8_t)m1->data[l+1];
      }
    return p;
  }
  case EXPR::DMATRIX: {
    gsl_matrix *m1 = (gsl_matrix*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    if (n==0 || m==0) return p;
    if (!p) p = malloc(n*m*sizeof(int8_t));
    if (!p) return 0;
    int8_t *q = (int8_t*)p;
    size_t k = 0;
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++)
	q[k++] = (int8_t)m1->data[i*m1->tda+j];
    return p;
  }
  case EXPR::IMATRIX: {
    gsl_matrix_int *m1 = (gsl_matrix_int*)x->data.mat.p;
    size_t n = m1->size1, m = m1->size2;
    if (n==0 || m==0) return p;
    if (!p) p = malloc(n*m*sizeof(int8_t));
    if (!p) return 0;
    int8_t *q = (int8_t*)p;
    size_t k = 0;
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < m; j++)
	q[k++] = (int8_t)m1->data[i*m1->tda+j];
    return p;
  }
  default:
    return 0;
  }
#else
  return 0;
#endif
}

static uint32_t mpz_hash(const mpz_t z)
{
  uint32_t h = 0;
  int i, len = z->_mp_size;
  if (len < 0) len = -len;
  if (sizeof(mp_limb_t) == 8) {
    for (i=0; i<len; i++) {
      h ^= (uint32_t)(uint64_t)z->_mp_d[i];
      h ^= (uint32_t)(((uint64_t)z->_mp_d[i])>>32);
    }
  } else {
    for (i=0; i<len; i++)
      h ^= z->_mp_d[i];
  }
  if (z->_mp_size < 0)
    h = -h;
  return h;
}

static uint32_t double_hash(double d)
{
  uint32_t h;
  char *c;
  size_t i;
  c = (char*)&d;
  for (h=0, i=0; i<sizeof(double); i++) {
    h += c[i] * 971;
  }
  return h;
}

static uint32_t string_hash(char *s)
{
  uint32_t h = 0, g;
  while (*s) {
    h = (h<<4)+*(s++);
    if ((g = (h & 0xf0000000)))	{
      h = h^(g>>24);
      h = h^g;
    }
  }
  return h;
}

extern "C"
uint32_t hash(pure_expr *x)
{
  char test;
  if (is_thunk(x)) pure_force(x);
  switch (x->tag) {
  case EXPR::INT:
    return (uint32_t)x->data.i;
  case EXPR::BIGINT:
    return mpz_hash(x->data.z);
  case EXPR::DBL:
    return double_hash(x->data.d);
  case EXPR::STR:
    return string_hash(x->data.s);
  case EXPR::PTR:
#if SIZEOF_VOID_P==8
    return ((uint32_t)(uint64_t)x->data.p) ^ ((uint32_t)(((uint64_t)x->data.p)>>32));
#else
    return (uint32_t)x->data.p;
#endif
  case EXPR::APP: {
    checkstk(test);
    int h;
    h = hash(x->data.x[0]);
    h = (h<<1) | (h<0 ? 1 : 0);
    h ^= hash(x->data.x[1]);
    return (uint32_t)h;
  }
  default:
    return (uint32_t)x->tag;
  }
}

extern "C"
bool same(pure_expr *x, pure_expr *y)
{
  char test;
 tail:
  if (x == y)
    return 1;
  if (is_thunk(x)) pure_force(x);
  if (is_thunk(y)) pure_force(y);
  if (x->tag != y->tag)
    return 0;
  else if (x->tag >= 0 && y->tag >= 0)
    if (x->data.clos && y->data.clos)
      /* Note that for global functions the function pointers may differ in
	 some cases (specifically in the case of an external which may chain
	 to a Pure definition of the same global). However, in that case we
	 may safely assume that the functions are the same anyway, since they
	 are referred to by the same global symbol. */
      return (!x->data.clos->local && !y->data.clos->local) ||
	(x->data.clos->fp == y->data.clos->fp);
    else
      return x->data.clos == y->data.clos;
  else {
    switch (x->tag) {
    case EXPR::APP: {
      checkstk(test);
      if (!same(x->data.x[0], y->data.x[0]))
	return 0;
      /* Fake a tail call here, so that we do not run out of stack space when
	 comparing large lists or similar right-recursive structures. */
      x = x->data.x[1]; y = y->data.x[1];
      goto tail;
    }
    case EXPR::INT:
      return x->data.i == y->data.i;
    case EXPR::BIGINT:
      return mpz_cmp(x->data.z, y->data.z) == 0;
    case EXPR::DBL:
      return x->data.d == y->data.d || (is_nan(x->data.d) && is_nan(y->data.d));
    case EXPR::STR:
      return strcmp(x->data.s, y->data.s) == 0;
    case EXPR::PTR:
      return x->data.p == y->data.p;
    case EXPR::MATRIX: {
      gsl_matrix_symbolic *m1 = (gsl_matrix_symbolic*)x->data.mat.p;
      gsl_matrix_symbolic *m2 = (gsl_matrix_symbolic*)y->data.mat.p;
      const size_t tda1 = m1->tda, tda2 = m2->tda;
      if (m1->size1 != m2->size1 || m1->size2 != m2->size2)
	return 0;
      checkstk(test);
      for (size_t i = 0; i < m1->size1; i++)
	for (size_t j = 0; j < m1->size2; j++)
	  if (!same(m1->data[i*tda1+j], m2->data[i*tda2+j]))
	    return 0;
      return 1;
    }
#ifdef HAVE_GSL
    case EXPR::DMATRIX: {
      gsl_matrix *m1 = (gsl_matrix*)x->data.mat.p;
      gsl_matrix *m2 = (gsl_matrix*)y->data.mat.p;
      const size_t tda1 = m1->tda, tda2 = m2->tda;
      if (m1->size1 != m2->size1 || m1->size2 != m2->size2)
	return 0;
      for (size_t i = 0; i < m1->size1; i++)
	for (size_t j = 0; j < m1->size2; j++)
	  if (m1->data[i*tda1+j] != m2->data[i*tda2+j])
	    return 0;
      return 1;
    }
    case EXPR::CMATRIX: {
      gsl_matrix_complex *m1 = (gsl_matrix_complex*)x->data.mat.p;
      gsl_matrix_complex *m2 = (gsl_matrix_complex*)y->data.mat.p;
      const size_t tda1 = m1->tda, tda2 = m2->tda;
      if (m1->size1 != m2->size1 || m1->size2 != m2->size2)
	return 0;
      for (size_t i = 0; i < m1->size1; i++)
	for (size_t j = 0; j < m1->size2; j++) {
	  const size_t k1 = 2*(i*tda1+j), k2 = 2*(i*tda2+j);
	  if (m1->data[k1] != m2->data[k2] ||
	      m1->data[k1+1] != m2->data[k2+1])
	    return 0;
	}
      return 1;
    }
    case EXPR::IMATRIX: {
      gsl_matrix_int *m1 = (gsl_matrix_int*)x->data.mat.p;
      gsl_matrix_int *m2 = (gsl_matrix_int*)y->data.mat.p;
      const size_t tda1 = m1->tda, tda2 = m2->tda;
      if (m1->size1 != m2->size1 || m1->size2 != m2->size2)
	return 0;
      for (size_t i = 0; i < m1->size1; i++)
	for (size_t j = 0; j < m1->size2; j++)
	  if (m1->data[i*tda1+j] != m2->data[i*tda2+j])
	    return 0;
      return 1;
    }
#endif
    default:
      return 1;
    }
  }
}

extern "C"
bool funp(const pure_expr *x)
{
  return (x->tag > 0 && x->data.clos);
}

extern "C"
bool lambdap(const pure_expr *x)
{
  return (x->tag == 0 && x->data.clos && x->data.clos->n > 0);
}

extern "C"
bool thunkp(const pure_expr *x)
{
  return (x->tag == 0 && x->data.clos && x->data.clos->n == 0);
}

extern "C"
bool varp(const pure_expr *x)
{
  return (x->tag > 0 && !x->data.clos);
}

extern "C"
pure_expr *arity(const pure_expr *x)
{
  if (x->tag >= 0 && x->data.clos)
    return pure_int(x->data.clos->n);
  else
    return 0;
}

extern "C"
int32_t pointer_get_byte(void *ptr)
{
  int8_t *p = (int8_t*)ptr;
  return *p;
}

extern "C"
int32_t pointer_get_short(void *ptr)
{
  int16_t *p = (int16_t*)ptr;
  return *p;
}

extern "C"
int32_t pointer_get_int(void *ptr)
{
  int32_t *p = (int32_t*)ptr;
  return *p;
}

extern "C"
int64_t pointer_get_long(void *ptr)
{
  int64_t *p = (int64_t*)ptr;
  return *p;
}

extern "C"
double pointer_get_float(void *ptr)
{
  float *p = (float*)ptr;
  return *p;
}

extern "C"
double pointer_get_double(void *ptr)
{
  double *p = (double*)ptr;
  return *p;
}

extern "C"
char *pointer_get_string(void *ptr)
{
  char **p = (char**)ptr;
  return *p;
}

extern "C"
void *pointer_get_pointer(void *ptr)
{
  void **p = (void**)ptr;
  return *p;
}

extern "C"
pure_expr *pointer_get_expr(void *ptr)
{
  pure_expr **p = (pure_expr**)ptr;
  return *p;
}

extern "C"
void pointer_put_byte(void *ptr, int32_t x)
{
  int8_t *p = (int8_t*)ptr;
  *p = x;
}

extern "C"
void pointer_put_short(void *ptr, int32_t x)
{
  int16_t *p = (int16_t*)ptr;
  *p = x;
}

extern "C"
void pointer_put_int(void *ptr, int32_t x)
{
  int32_t *p = (int32_t*)ptr;
  *p = x;
}

extern "C"
void pointer_put_long(void *ptr, int64_t x)
{
  int64_t *p = (int64_t*)ptr;
  *p = x;
}

extern "C"
void pointer_put_float(void *ptr, double x)
{
  float *p = (float*)ptr;
  *p = x;
}

extern "C"
void pointer_put_double(void *ptr, double x)
{
  double *p = (double*)ptr;
  *p = x;
}

extern "C"
void pointer_put_string(void *ptr, const char *x)
{
  char **p = (char**)ptr;
  *p = strdup(x);
}

extern "C"
void pointer_put_pointer(void *ptr, void *x)
{
  void **p = (void**)ptr;
  *p = x;
}

extern "C"
void pointer_put_expr(void *ptr, pure_expr *x)
{
  pure_expr **p = (pure_expr**)ptr;
  *p = x;
}

#include <errno.h>

extern "C"
int pure_errno(void)
{
  return errno;
}

extern "C"
void pure_set_errno(int value)
{
  errno = value;
}

#include <time.h>

extern "C"
int64_t pure_time(void)
{
  return (int64_t)time(NULL);
}

/* Note that the following are not thread-safe as they use statically
   allocated buffers. */

extern "C"
char *pure_ctime(int64_t t)
{
  time_t time = (time_t)t;
  return ctime(&time);
}

extern "C"
struct tm *pure_gmtime(int64_t t)
{
  time_t time = (time_t)t;
  return gmtime(&time);
}

extern "C"
struct tm *pure_localtime(int64_t t)
{
  time_t time = (time_t)t;
  return localtime(&time);
}

extern "C"
char *pure_strftime(const char *format, struct tm *tm)
{
  static char buf[1024];
  if (!strftime(buf, 1024, format, tm))
    /* The interface to strftime is rather brain-damaged since it returns zero
       both in case of a buffer overflow and when the resulting string is
       empty. We just pretend that there cannot be any errors and return an
       empty string in both cases. */
    buf[0] = 0;
  return buf;
}

#ifdef HAVE_GETTIMEOFDAY
#include <sys/time.h>
extern "C"
double pure_gettimeofday(void)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ((double)tv.tv_sec)+((double)tv.tv_usec)*1e-6;
}
#else
#ifdef HAVE_FTIME
#include <sys/timeb.h>
extern "C"
double pure_gettimeofday(void)
{
  struct timeb tb;
  ftime(&tb);
  return ((double)tb.time)+((double)tb.millitm)*1e-3;
}
#else
extern "C"
double pure_gettimeofday(void)
{
  return (double)time(NULL);
}
#endif
#endif

#ifdef __MINGW32__
#include <windows.h>
double pure_nanosleep(double t)
{
  if (t > 0.0) {
    unsigned long secs;
    unsigned short msecs;
    double ip, fp;
    if (t > LONG_MAX) t = LONG_MAX;
    fp = modf(t, &ip);
    secs = (unsigned long)ip;
    msecs = (unsigned short)(fp*1e3);
    Sleep(secs*1000U+msecs);
  }
  return 0.0;
}
#else
double pure_nanosleep(double t)
{
  if (t > 0.0) {
    double ip, fp;
    unsigned long secs;
#ifdef HAVE_NANOSLEEP
    unsigned long nsecs;
    struct timespec req, rem;
    fp = modf(t, &ip);
    if (ip > LONG_MAX) { ip = (double)LONG_MAX; fp = 0.0; }
    secs = (unsigned long)ip;
    nsecs = (unsigned long)(fp*1e9);
    req.tv_sec = secs; req.tv_nsec = nsecs;
    if (nanosleep(&req, &rem))
      return ((double)rem.tv_sec)+((double)rem.tv_nsec)*1e-9;
    else
      return 0.0;
#else
#ifdef HAVE_USLEEP
    unsigned long usecs;
    if (t > LONG_MAX) t = LONG_MAX;
    fp = modf(t, &ip);
    secs = (unsigned long)ip;
    usecs = (unsigned long)(fp*1e6);
    usleep(secs*1000000U+usecs);
    return 0.0;
#else
    fp = modf(t, &ip);
    if (ip > LONG_MAX) ip = (double)LONG_MAX;
    secs = (unsigned long)ip;
    return (double)sleep(secs);
#endif
#endif
  } else
    return 0.0;
}
#endif

#ifdef __MINGW32__
extern "C"
FILE *popen(const char *command, const char *type)
{
  return _popen(command, type);
}

extern "C"
int pclose(FILE *stream)
{
  return _pclose(stream);
}

extern "C"
unsigned int sleep(unsigned int secs)
{
  Sleep(secs*1000);
  return 0;
}
#endif

/* Horrible kludge to get round, trunc and the inverse hyperbolic functions
   from libmingwex.a (these are in C99, but not in the Windows system
   libraries, and LLVM doesn't know how to get them either). */

extern "C"
double __round(double x)
{
  return round(x);
}

extern "C"
double __trunc(double x)
{
  return trunc(x);
}

extern "C"
double __asinh(double x)
{
  return asinh(x);
}

extern "C"
double __acosh(double x)
{
  return acosh(x);
}

extern "C"
double __atanh(double x)
{
  return atanh(x);
}

extern "C"
int pure_fprintf(FILE *fp, const char *format)
{
  return fprintf(fp, format);
}

extern "C"
int pure_fprintf_int(FILE *fp, const char *format, int32_t x)
{
  return fprintf(fp, format, x);
}

extern "C"
int pure_fprintf_double(FILE *fp, const char *format, double x)
{
  return fprintf(fp, format, x);
}

extern "C"
int pure_fprintf_string(FILE *fp, const char *format, const char *x)
{
  return fprintf(fp, format, x);
}

extern "C"
int pure_fprintf_pointer(FILE *fp, const char *format, const void *x)
{
  return fprintf(fp, format, x);
}

extern "C"
int pure_snprintf(char *buf, size_t size, const char *format)
{
  return snprintf(buf, size, format);
}

extern "C"
int pure_snprintf_int(char *buf, size_t size, const char *format, int32_t x)
{
  return snprintf(buf, size, format, x);
}

extern "C"
int pure_snprintf_double(char *buf, size_t size, const char *format, double x)
{
  return snprintf(buf, size, format, x);
}

extern "C"
int pure_snprintf_string(char *buf, size_t size, const char *format, const char *x)
{
  return snprintf(buf, size, format, x);
}

extern "C"
int pure_snprintf_pointer(char *buf, size_t size, const char *format, const void *x)
{
  return snprintf(buf, size, format, x);
}

#define myformat(format) scanf_format((char*)alloca(strlen(format)+3), format)

static inline char *scanf_format(char *buf, const char *format)
{
  strcpy(buf, format); strcat(buf, "%n");
  return buf;
}

extern "C"
int pure_fscanf(FILE *fp, const char *format)
{
  int count = -1;
  fscanf(fp, myformat(format), &count);
  return count;
}

extern "C"
int pure_fscanf_int(FILE *fp, const char *format, int32_t *x)
{
  // wrap this up in case int on the target platform is not 32 bit
  int count = -1, y;
  fscanf(fp, myformat(format), &y, &count);
  if (count >= 0) *x = y;
  return count;
}

extern "C"
int pure_fscanf_double(FILE *fp, const char *format, double *x)
{
  int count = -1;
  fscanf(fp, myformat(format), x, &count);
  return count;
}

extern "C"
int pure_fscanf_string(FILE *fp, const char *format, const char *x)
{
  int count = -1;
  fscanf(fp, myformat(format), x, &count);
  return count;
}

extern "C"
int pure_fscanf_pointer(FILE *fp, const char *format, const void **x)
{
  int count = -1;
  fscanf(fp, myformat(format), x, &count);
  return count;
}

extern "C"
int pure_sscanf(const char *buf, const char *format)
{
  int count = -1;
  sscanf(buf, myformat(format), &count);
  return count;
}

extern "C"
int pure_sscanf_int(const char *buf, const char *format, int32_t *x)
{
  // wrap this up in case int on the target platform is not 32 bit
  int count = -1, y; sscanf(buf, myformat(format), &y, &count);
  if (count >= 0) *x = y;
  return count;
}

extern "C"
int pure_sscanf_double(const char *buf, const char *format, double *x)
{
  int count = -1;
  sscanf(buf, myformat(format), x, &count);
  return count;
}

extern "C"
int pure_sscanf_string(const char *buf, const char *format, char *x)
{
  int count = -1;
  sscanf(buf, myformat(format), x, &count);
  return count;
}

extern "C"
int pure_sscanf_pointer(const char *buf, const char *format, void **x)
{
  int count = -1;
  sscanf(buf, myformat(format), x, &count);
  return count;
}

#include <fnmatch.h>
#include <glob.h>

extern "C"
pure_expr *globlist(const glob_t *pglob)
{
  interpreter& interp = *interpreter::g_interp;
  pure_expr *x = mk_nil();
  int i = pglob->gl_pathc;
  while (--i >= 0) {
    pure_expr *f = pure_const(interp.symtab.cons_sym().f);
    pure_expr *y = pure_cstring_dup(pglob->gl_pathv[i]);
    x = pure_apply2(pure_apply2(f, y), x);
  }
  return x;
}

#include <sys/types.h>
#include <regex.h>

extern "C"
pure_expr *regmatches(const regex_t *preg, int flags)
{
  interpreter& interp = *interpreter::g_interp;
  int n = (flags&REG_NOSUB)?0:preg->re_nsub+1;
  regmatch_t *matches = 0;
  if (n > 0) matches = (regmatch_t*)malloc(n*sizeof(regmatch_t));
  pure_expr *f = pure_const(interp.symtab.pair_sym().f);
  pure_expr *x = pure_apply2(pure_apply2(f, pure_int(n)),
			     pure_pointer(matches));
  return x;
}

static int translate_pos(char *s, int p, int l)
{
  assert(p >= 0 && p <= l);
  char c = s[p]; s[p] = 0;
  char *t = toutf8(s); assert(t);
  int res = strlen(t);
  free(t); s[p] = c;
  return res;
}

extern "C"
pure_expr *reglist(const regex_t *preg, const char *s,
		   const regmatch_t *matches)
{
  interpreter& interp = *interpreter::g_interp;
  if (!matches) return mk_void();
  int n = preg->re_nsub+1;
  pure_expr *x = 0;
  int i = n;
  if (strcmp(default_encoding(), "UTF-8") == 0) {
    // Optimize for the case that the system encoding is utf-8. This should be
    // pretty much standard on Linux/Unix systems these days.
    while (--i >= 0) {
      pure_expr *f = pure_const(interp.symtab.pair_sym().f);
      pure_expr *y1 = pure_int(matches[i].rm_so);
      pure_expr *y2;
      if (matches[i].rm_so >= 0 && matches[i].rm_eo >= matches[i].rm_so) {
	size_t n = matches[i].rm_eo - matches[i].rm_so;
	char *buf = (char*)malloc(n+1); assert(buf);
	strncpy(buf, s+matches[i].rm_so, n); buf[n] = 0;
	y2 = pure_cstring(buf);
      } else
	y2 = pure_cstring_dup("");
      if (x)
	x = pure_apply2(pure_apply2(f, y2), x);
      else
	x = y2;
      x = pure_apply2(pure_apply2(f, y1), x);
    }
    return x;
  }
  // Translate the subject string to the system encoding. We also use this as
  // a work buffer to translate system encoding offsets to the utf-8 encoding
  // on the fly.
  char *t = fromutf8(s); assert(t);
  // Compute the position of the first match.
  assert(matches[0].rm_so >= 0);
  int l0 = strlen(t), p0 = matches[0].rm_so, q0 = translate_pos(t, p0, l0);
  char *u = t+p0;
  int l = l0-p0;
  while (--i >= 0) {
    int p = matches[i].rm_so, q;
    if (p < 0)
      q = -1;
    else {
      assert(p >= p0);
      q = q0 + translate_pos(u, p-p0, l);
    }
    pure_expr *f = pure_const(interp.symtab.pair_sym().f);
    pure_expr *y1 = pure_int(q);
    pure_expr *y2;
    if (q >= 0 && matches[i].rm_eo >= matches[i].rm_so) {
      size_t n = matches[i].rm_eo - matches[i].rm_so;
      char *buf = (char*)malloc(n+1); assert(buf);
      strncpy(buf, t+matches[i].rm_so, n); buf[n] = 0;
      y2 = pure_cstring(buf);
    } else
      y2 = pure_cstring_dup("");
    if (x)
      x = pure_apply2(pure_apply2(f, y2), x);
    else
      x = y2;
    x = pure_apply2(pure_apply2(f, y1), x);
  }
  free(t);
  return x;
}

#include <llvm/System/DynamicLibrary.h>

extern "C"
pure_expr *pure_addr(const char *s)
{
  void *p = llvm::sys::DynamicLibrary::SearchForAddressOfSymbol(s);
  if (p)
    return pure_pointer(p);
  else
    return 0;
}

static inline void
df(interpreter& interp, const char* s, pure_expr *x)
{
  try {
    interp.defn(s, x);
  } catch (err &e) {
    cerr << "warning: " << e.what() << endl;
  }
  MEMDEBUG_FREE(x)
}

static inline void
cdf(interpreter& interp, const char* s, pure_expr *x)
{
  try {
    interp.const_defn(s, x);
  } catch (err &e) {
    cerr << "warning: " << e.what() << endl;
  }
  pure_freenew(x);
}

extern "C"
void pure_sys_vars(void)
{
  interpreter& interp = *interpreter::g_interp;
  // standard I/O streams
  df(interp, "stdin",	pure_pointer(stdin));
  df(interp, "stdout",	pure_pointer(stdout));
  df(interp, "stderr",	pure_pointer(stderr));
  // clock
  cdf(interp, "CLOCKS_PER_SEC",	pure_int(CLOCKS_PER_SEC));
  // fnmatch, glob
  cdf(interp, "FNM_NOESCAPE",	pure_int(FNM_NOESCAPE));
  cdf(interp, "FNM_PATHNAME",	pure_int(FNM_PATHNAME));
  cdf(interp, "FNM_PERIOD",	pure_int(FNM_PERIOD));
  cdf(interp, "FNM_CASEFOLD",	pure_int(FNM_CASEFOLD));
  cdf(interp, "GLOB_SIZE",	pure_int(sizeof(glob_t))); // not in POSIX
  cdf(interp, "GLOB_ERR",	pure_int(GLOB_ERR));
  cdf(interp, "GLOB_MARK",	pure_int(GLOB_MARK));
  cdf(interp, "GLOB_NOSORT",	pure_int(GLOB_NOSORT));
  cdf(interp, "GLOB_NOCHECK",	pure_int(GLOB_NOCHECK));
  cdf(interp, "GLOB_NOESCAPE",	pure_int(GLOB_NOESCAPE));
  /* These seem to be GNU-specific. */
#ifdef GLOB_PERIOD
  cdf(interp, "GLOB_PERIOD",	pure_int(GLOB_PERIOD));
#endif
#ifdef GLOB_ONLYDIR
  cdf(interp, "GLOB_ONLYDIR",	pure_int(GLOB_ONLYDIR));
#endif
  cdf(interp, "GLOB_BRACE",	pure_int(GLOB_BRACE));
  cdf(interp, "GLOB_NOMAGIC",	pure_int(GLOB_NOMAGIC));
  cdf(interp, "GLOB_TILDE",	pure_int(GLOB_TILDE));
  // regex stuff
  cdf(interp, "REG_SIZE",	pure_int(sizeof(regex_t))); // not in POSIX
  cdf(interp, "REG_EXTENDED",	pure_int(REG_EXTENDED));
  cdf(interp, "REG_ICASE",	pure_int(REG_ICASE));
  cdf(interp, "REG_NOSUB",	pure_int(REG_NOSUB));
  cdf(interp, "REG_NEWLINE",	pure_int(REG_NEWLINE));
  cdf(interp, "REG_NOTBOL",	pure_int(REG_NOTBOL));
  cdf(interp, "REG_NOTEOL",	pure_int(REG_NOTEOL));
  // regcomp error codes
  cdf(interp, "REG_BADBR",	pure_int(REG_BADBR));
  cdf(interp, "REG_BADPAT",	pure_int(REG_BADPAT));
  cdf(interp, "REG_BADRPT",	pure_int(REG_BADRPT));
  cdf(interp, "REG_ECOLLATE",	pure_int(REG_ECOLLATE));
  cdf(interp, "REG_ECTYPE",	pure_int(REG_ECTYPE));
  cdf(interp, "REG_EESCAPE",	pure_int(REG_EESCAPE));
  cdf(interp, "REG_ESUBREG",	pure_int(REG_ESUBREG));
  cdf(interp, "REG_EBRACK",	pure_int(REG_EBRACK));
  cdf(interp, "REG_EPAREN",	pure_int(REG_EPAREN));
  cdf(interp, "REG_EBRACE",	pure_int(REG_EBRACE));
  cdf(interp, "REG_ERANGE",	pure_int(REG_ERANGE));
  cdf(interp, "REG_ESPACE",	pure_int(REG_ESPACE));
  // regexec error codes
  cdf(interp, "REG_NOMATCH",	pure_int(REG_NOMATCH));
  // signal actions
  cdf(interp, "SIG_TRAP",	pure_int(1));
  cdf(interp, "SIG_IGN",	pure_int(-1));
  cdf(interp, "SIG_DFL",	pure_int(0));
  // signals
#ifdef SIGHUP
  cdf(interp, "SIGHUP",		pure_int(SIGHUP));
#endif
#ifdef SIGINT
  cdf(interp, "SIGINT",		pure_int(SIGINT));
#endif
#ifdef SIGQUIT
  cdf(interp, "SIGQUIT",	pure_int(SIGQUIT));
#endif
#ifdef SIGILL
  cdf(interp, "SIGILL",		pure_int(SIGILL));
#endif
#ifdef SIGABRT
  cdf(interp, "SIGABRT",	pure_int(SIGABRT));
#endif
#ifdef SIGFPE
  cdf(interp, "SIGFPE",		pure_int(SIGFPE));
#endif
#ifdef SIGKILL
  cdf(interp, "SIGKILL",	pure_int(SIGKILL));
#endif
#ifdef SIGSEGV
  cdf(interp, "SIGSEGV",	pure_int(SIGSEGV));
#endif
#ifdef SIGPIPE
  cdf(interp, "SIGPIPE",	pure_int(SIGPIPE));
#endif
#ifdef SIGALRM
  cdf(interp, "SIGALRM",	pure_int(SIGALRM));
#endif
#ifdef SIGTERM
  cdf(interp, "SIGTERM",	pure_int(SIGTERM));
#endif
#ifdef SIGUSR1
  cdf(interp, "SIGUSR1",	pure_int(SIGUSR1));
#endif
#ifdef SIGUSR2
  cdf(interp, "SIGUSR2",	pure_int(SIGUSR2));
#endif
#ifdef SIGCHLD
  cdf(interp, "SIGCHLD",	pure_int(SIGCHLD));
#endif
#ifdef SIGCONT
  cdf(interp, "SIGCONT",	pure_int(SIGCONT));
#endif
#ifdef SIGSTOP
  cdf(interp, "SIGSTOP",	pure_int(SIGSTOP));
#endif
#ifdef SIGTSTP
  cdf(interp, "SIGTSTP",	pure_int(SIGTSTP));
#endif
#ifdef SIGTTIN
  cdf(interp, "SIGTTIN",	pure_int(SIGTTIN));
#endif
#ifdef SIGTTOU
  cdf(interp, "SIGTTOU",	pure_int(SIGTTOU));
#endif
  // setlocale
#ifdef LC_ALL
  cdf(interp, "LC_ALL",		pure_int(LC_ALL));
#endif
#ifdef LC_COLLATE
  cdf(interp, "LC_COLLATE",	pure_int(LC_COLLATE));
#endif
#ifdef LC_CTYPE
  cdf(interp, "LC_CTYPE",	pure_int(LC_CTYPE));
#endif
#ifdef LC_MESSAGES
  cdf(interp, "LC_MESSAGES",	pure_int(LC_MESSAGES));
#endif
#ifdef LC_MONETARY
  cdf(interp, "LC_MONETARY",	pure_int(LC_MONETARY));
#endif
#ifdef LC_NUMERIC
  cdf(interp, "LC_NUMERIC",	pure_int(LC_NUMERIC));
#endif
#ifdef LC_TIME
  cdf(interp, "LC_TIME",	pure_int(LC_TIME));
#endif
}


/* Optimized matrix functions, by Scott E. Dillard. */

namespace matrix { 
//some templates to ease specialization of matrix functions

//the element type of a matrix
template <typename t> struct element_of {};
template <> struct element_of<gsl_matrix> { typedef double type; };
template <> struct element_of<gsl_matrix_int> { typedef int type; };
template <> struct element_of<gsl_matrix_complex> { typedef Complex type; };
template <> struct element_of<gsl_matrix_symbolic>
{ typedef pure_expr* type; };

//create a matrix
template <typename t> t* create_matrix(size_t n,size_t m);
template <> gsl_matrix* create_matrix<gsl_matrix>(size_t n, size_t m)
{ return create_double_matrix(n,m); }
template <> gsl_matrix_int* create_matrix<gsl_matrix_int>(size_t n, size_t m)
{ return create_int_matrix(n,m); }
template <> gsl_matrix_complex* create_matrix<gsl_matrix_complex>
(size_t n, size_t m) { return create_complex_matrix(n,m); }
template <> gsl_matrix_symbolic* create_matrix<gsl_matrix_symbolic>
(size_t n, size_t m) { return create_symbolic_matrix(n,m); }

//free a matrix
void matrix_free(gsl_matrix *m) { gsl_matrix_free(m); }
void matrix_free(gsl_matrix_int *m) { gsl_matrix_int_free(m); }
void matrix_free(gsl_matrix_complex *m) { gsl_matrix_complex_free(m); }
void matrix_free(gsl_matrix_symbolic *m) { gsl_matrix_symbolic_free(m); }

//convert a matrix element into an expression
static inline pure_expr* to_expr(double & d) { return pure_double(d); }
static inline pure_expr* to_expr(int32_t i) { return pure_int(i); }
static inline pure_expr* to_expr(Complex & c) 
{ 
  return pure_complex(&c.real()); 
  //ASSUMPTION: real, imag fields are contiguous, in order. 
  //This is the case for GNU C++.
}
static inline pure_expr* to_expr(pure_expr* e) { return e; }


//convert an expression into a matrix element
inline bool from_expr( pure_expr* e, double & d )
{ return pure_is_double(e,&d); }
inline bool from_expr( pure_expr* e, int & i )
{ return pure_is_int(e,&i); }
inline bool from_expr( pure_expr* e, Complex & c )
{ return pure_is_complex(e,&c.real()); }


// generix matrix do
template <typename matrix_type>
void matrix_do( pure_expr *f, pure_expr *x ) {
  matrix_type *xm = static_cast<matrix_type*>(x->data.mat.p);
  typedef typename element_of<matrix_type>::type elem_type;

  for (size_t i=0; i<xm->size1; ++i) {
    //pi is input read head
    elem_type *pi = reinterpret_cast<elem_type*>(xm->data)+i*xm->tda; 
    for (size_t j=0; j<xm->size2; ++j,++pi) {
      pure_expr *y = pure_app( f, to_expr(*pi) );
      pure_freenew(y);
    }
  }
}


//numeric map loop : optmize common case that the output of the mapped
//function is all of the same numerical type. If not, fall back to symbolic
//map loop

template < typename in_mat_type,  
           typename out_mat_type >
pure_expr* 
numeric_map_loop
( pure_expr *f,       //the function being mapped
  in_mat_type *in,    //the operand matrix
  out_mat_type *out,  //the result matrix
  size_t *lasti,      //last row and column written to 
  size_t *lastj )     //  before bailing out in the event that
                      //  the output of f is not the right type
{
  typedef typename element_of< in_mat_type>::type  in_elem_type;
  typedef typename element_of<out_mat_type>::type out_elem_type;
  
  //need casts for complex case, when data pointer is double*, but
  //we really want complex*
  in_elem_type  *inp  = (reinterpret_cast<in_elem_type* >(in->data ))+1;
  out_elem_type *outp = (reinterpret_cast<out_elem_type*>(out->data))+1;
  pure_expr *r; //result expression
  out_elem_type v; //result value

  //we already performed the first map, so now we apply f to all but the first
  //column in the first row..
  *lasti=0;
  for (size_t j=1; j<in->size2; ++j,++inp,++outp) {
    *lastj=j;
    r = pure_app( f, to_expr(*inp) );
    if (!from_expr(r,v)) return r;
    *outp = v;
    pure_freenew(r);
  }

  //..then the rest of the rows
  for (size_t i=1; i<in->size1; ++i) {
    *lasti=i;
    inp  = (reinterpret_cast< in_elem_type*>( in->data))+i*in->tda; 
    outp = (reinterpret_cast<out_elem_type*>(out->data))+i*out->tda;
    for (size_t j=0; j<in->size2; ++j,++inp,++outp) {
      *lastj=j;
      r = pure_app( f, to_expr(*inp) );
      if (!from_expr(r,v)) return r;
      *outp = v;
      pure_freenew(r);
    }
  }
  return 0;
}


//symbolic map loop : mapped function results in heterogenous / non-numerical
//types. This function may pick up where numerical_map_loop left off, in that
//case copying out the already-evaluated numbers from the matrix 'num'

template < typename in_mat_type,  //the operand matrix type
           typename num_mat_type >
void 
symbolic_map_loop
( pure_expr *f,            //the function being mapped
  in_mat_type *in,         //the operand matrix

  num_mat_type *num,       //the numerical matrix that _would_ have been 
                           //  the output if the results of f had all been of
                           //  the same numerical type
                           
  gsl_matrix_symbolic *out,//the symbolic matrix that will be the end result

  size_t lasti ,           //some of the map may already be evaluated. 
  size_t lastj ,           //  lasti,lastj was the last element mapped
  pure_expr *last )        //  and last is the result
{
  typedef typename element_of< in_mat_type>::type  in_elem_type;
  typedef typename element_of<num_mat_type>::type num_elem_type;

  in_elem_type *inp = 0;
  pure_expr **outp = 0;
  num_elem_type *nump = 0;

  //copy the already-evaluated stuff out of num
  if (lasti>0 || lastj>0) {
    if (lasti>0) {
      for (size_t i=0; i<lasti; ++i) {
        outp = out->data + i*out->tda;
        nump = (reinterpret_cast<num_elem_type*>(num->data)) + i*num->tda;
        for (size_t j=0; j<in->size2; ++j,++outp,++nump) {
          *outp = to_expr(*nump);
        }
      }
    }
    if (lastj>0) {
      outp = out->data + lasti*out->tda;
      nump = (reinterpret_cast<num_elem_type*>(num->data)) + lasti*num->tda;
      for (size_t j=0; j<lastj; ++j,++outp,++nump) {
        *outp = to_expr(*nump);
      }
    }
  } 

  out->data[lasti*out->tda+lastj] = last;
  lastj++;
  if (lastj>=out->size2) {
    lasti++;
    lastj=0;
    if (lasti>=out->size1) 
      return;
  }

  inp  = (reinterpret_cast<in_elem_type*>(in->data))+lasti*in->tda+lastj;
  outp = out->data+lasti*out->tda+lastj;
  //finish the map
  for (size_t j=lastj; j<in->size2; ++j,++inp,++outp) {
    *outp = pure_app(f,to_expr(*inp));
  }

  for (size_t i=lasti+1; i<in->size1; ++i) {
    inp =  (reinterpret_cast<in_elem_type*>(in->data))+i*in->tda; 
    outp = out->data+i*out->tda;
    for (size_t j=0; j<in->size2; ++j,++inp,++outp) {
      *outp = pure_app(f,to_expr(*inp));
    }
  }
}


//generic matrix map, dispatches on input matrix type (the template parameter)
//and output matrix type, which is guessed from the result type of f (x!0).
//The function speculates that all f(x!i) will be of the same type for all i.
//If not, numeric_map_loop bails out and symbolic_map_loop takes over.

template <typename matrix_type>
pure_expr* matrix_map( pure_expr *f, pure_expr *x )
{
  pure_ref(f);
  pure_ref(x);
  typedef typename element_of<matrix_type>::type elem_type;
  matrix_type *xm = static_cast<matrix_type*>(x->data.mat.p);

  if (xm->size1 == 0 || xm->size2 == 0) {
    // empty output matrix
    gsl_matrix_symbolic *sm = create_symbolic_matrix(xm->size1,xm->size2);
    pure_expr *out = pure_symbolic_matrix(sm);
    pure_unref(f);
    pure_unref(x);
    return out;
  }

  //need casts for complex case, when data pointer is double*, but
  //we really want complex*
  elem_type *inp = reinterpret_cast<elem_type* >(xm->data);
  //make a guess at what the result type is by the first element
  pure_expr *first = pure_app(f,to_expr(*inp));
  int32_t firsti;
  double firstd;
  Complex firstc;
  
  size_t lasti=0,lastj=0;
  pure_expr *out=0; //final result

#if HAVE_GSL
  if (from_expr(first,firstd)) { 
    //DOUBLE
    gsl_matrix *dm = create_double_matrix(xm->size1,xm->size2);
    dm->data[0] = firstd;
    pure_expr *last = numeric_map_loop(f,xm,dm,&lasti,&lastj);
    if (!last) {
      out = pure_double_matrix(dm);
    } else {
      gsl_matrix_symbolic *sm = create_symbolic_matrix(xm->size1,xm->size2);
      symbolic_map_loop(f,xm,dm,sm,lasti,lastj,last);
      gsl_matrix_free(dm);
      out = pure_symbolic_matrix(sm);
    }
  } else if (from_expr(first,firsti)) { 
    //INT
    gsl_matrix_int *im = create_int_matrix(xm->size1,xm->size2);
    im->data[0] = firsti; 
    pure_expr *last = numeric_map_loop(f,xm,im,&lasti,&lastj);
    if (!last) { 
      out = pure_int_matrix(im);
    } else {
      gsl_matrix_symbolic *sm = create_symbolic_matrix(xm->size1,xm->size2);
      symbolic_map_loop(f,xm,im,sm,lasti,lastj,last);
      gsl_matrix_int_free(im);
      out = pure_symbolic_matrix(sm);
    }
  } else if (from_expr(first,firstc)) { 
    //COMPLEX
    gsl_matrix_complex *cm = create_complex_matrix(xm->size1,xm->size2);  
    reinterpret_cast<Complex*>(cm->data)[0] = firstc;
    pure_expr *last = numeric_map_loop(f,xm,cm,&lasti,&lastj);
    if (!last) { 
      out = pure_complex_matrix(cm);
    } else {
      gsl_matrix_symbolic *sm = create_symbolic_matrix(xm->size1,xm->size2);
      symbolic_map_loop(f,xm,cm,sm,lasti,lastj,last);
      gsl_matrix_complex_free(cm);
      out = pure_symbolic_matrix(sm);
    }
  } else 
#endif //HAVE_GSL
  { //for anything else, default to symbolic. 
    gsl_matrix_symbolic *sm = create_symbolic_matrix(xm->size1,xm->size2);
    symbolic_map_loop(f,xm,static_cast<matrix_type*>(0),sm,0,0,first);
    out = pure_symbolic_matrix(sm);
  }

  if (first->refc==0) pure_freenew(first);
  pure_unref(f);
  pure_unref(x);
  return out;
}


//numeric zipwith loop : optmize common case that the output of the zipped
//function is all of the same numerical type. If not, fall back to symbolic
//zipwith loop

template < typename in1_mat_type,  
           typename in2_mat_type,
           typename out_mat_type >
pure_expr* 
numeric_zipwith_loop
( pure_expr *f,       //the function being zipped
  in1_mat_type *in1,   //the 1st operand matrix
  in2_mat_type *in2,   //the 2nd operand matrix
  out_mat_type *out,  //the result matrix
  size_t *lasti,      //last row and column written to 
  size_t *lastj )     //  before bailing out in the event that
                      //  the output of f is not the right type
{
  typedef typename element_of<in1_mat_type>::type in1_elem_type;
  typedef typename element_of<in2_mat_type>::type in2_elem_type;
  typedef typename element_of<out_mat_type>::type out_elem_type;
  
  //need casts for complex case, when data pointer is double*, but
  //we really want complex*
  in1_elem_type *inp1 = (reinterpret_cast<in1_elem_type*>(in1->data ))+1;
  in2_elem_type *inp2 = (reinterpret_cast<in2_elem_type*>(in2->data ))+1;
  out_elem_type *outp = (reinterpret_cast<out_elem_type*>(out->data))+1;
  pure_expr *r; //result expression
  out_elem_type v; //result value

  //we already performed the first zipwith, so now we apply f to all but the
  //first column in the first row..
  *lasti=0;
  for (size_t j=1; j<in1->size2 && j<in2->size2; ++j,++inp1,++inp2,++outp) {
    *lastj=j;
    r = pure_appl( f, 2, to_expr(*inp1), to_expr(*inp2) );
    if (!from_expr(r,v)) return r;
    *outp = v;
    pure_freenew(r);
  }

  //..then the rest of the rows
  for (size_t i=1; i<in1->size1 && i<in2->size1; ++i) {
    *lasti=i;
    inp1 = (reinterpret_cast<in1_elem_type*>(in1->data))+i*in1->tda; 
    inp2 = (reinterpret_cast<in2_elem_type*>(in2->data))+i*in2->tda; 
    outp = (reinterpret_cast<out_elem_type*>(out->data))+i*out->tda;
    for (size_t j=0; j<in1->size2 && j<in2->size2; ++j,++inp1,++inp2,++outp) {
      *lastj=j;
      r = pure_appl( f, 2, to_expr(*inp1), to_expr(*inp2) );
      if (!from_expr(r,v)) return r;
      *outp = v;
      pure_freenew(r);
    }
  }
  return 0;
}


//symbolic zipwith loop : zipped function results in heterogenous /
//non-numerical types. This function may pick up where numerical_zipwith_loop
//left off, in that case copying out the already-evaluated numbers from the
//matrix 'num'

template < typename in1_mat_type,  //the operand matrix type
           typename in2_mat_type,
           typename num_mat_type >
void 
symbolic_zipwith_loop
( pure_expr *f,            //the function being zipped
  in1_mat_type *in1,        //the 1st operand matrix
  in2_mat_type *in2,        //the 2nd operand matrix

  num_mat_type *num,       //the numerical matrix that _would_ have been 
                           //  the output if the results of f had all been of
                           //  the same numerical type
                           
  gsl_matrix_symbolic *out,//the symbolic matrix that will be the end result

  size_t lasti ,           //some of the zipwith may already be evaluated. 
  size_t lastj ,           //  lasti,lastj was the last element zipped
  pure_expr *last )        //  and last is the result
{
  typedef typename element_of<in1_mat_type>::type in1_elem_type;
  typedef typename element_of<in2_mat_type>::type in2_elem_type;
  typedef typename element_of<num_mat_type>::type num_elem_type;

  in1_elem_type *inp1 = 0; 
  in2_elem_type *inp2 = 0;
  pure_expr **outp = 0;
  num_elem_type *nump = 0;

  //copy the already-evaluated stuff out of num
  if (lasti>0 || lastj>0) {
    if (lasti>0) {
      for (size_t i=0; i<lasti; ++i) {
        outp = out->data + i*out->tda;
        nump = (reinterpret_cast<num_elem_type*>(num->data)) + i*num->tda;
        for (size_t j=0; j<in1->size2 && j<in2->size2; ++j,++outp,++nump) {
          *outp = to_expr(*nump);
        }
      }
    }
    if (lastj>0) {
      outp = out->data + lasti*out->tda;
      nump = (reinterpret_cast<num_elem_type*>(num->data)) + lasti*num->tda;
      for (size_t j=0; j<lastj; ++j,++outp,++nump) {
        *outp = to_expr(*nump);
      }
    }
  } 

  out->data[lasti*out->tda+lastj] = last;
  lastj++;
  if (lastj>=out->size2) {
    lasti++;
    lastj=0;
    if (lasti>=out->size1) 
      return;
  }

  inp1 = (reinterpret_cast<in1_elem_type*>(in1->data))+lasti*in1->tda+lastj;
  inp2 = (reinterpret_cast<in2_elem_type*>(in2->data))+lasti*in2->tda+lastj;
  outp = out->data+lasti*out->tda+lastj;
  //finish the zipwith
  for (size_t j=lastj; j<in1->size2 && j<in2->size2; ++j,++inp1,++inp2,++outp)
  {
    *outp = pure_appl(f,2,to_expr(*inp1),to_expr(*inp2));
  }

  for (size_t i=lasti+1; i<in1->size1 && i<in2->size1; ++i) {
    inp1 =  (reinterpret_cast<in1_elem_type*>(in1->data))+i*in1->tda; 
    inp2 =  (reinterpret_cast<in2_elem_type*>(in2->data))+i*in2->tda; 
    outp = out->data+i*out->tda;
    for (size_t j=0; j<in1->size2 && j<in2->size2; ++j,++inp1,++inp2,++outp) {
      *outp = pure_appl(f,2,to_expr(*inp1),to_expr(*inp2));
    }
  }
}


//generic matrix zipwith, dispatches on input matrix type (the template
//parameters) and output matrix type, which is guessed from the result type of
//f (x!0) (y!0). The function speculates that all f (x!i) (y!i) will be of the
//same type for all i. If not, numeric_zipwith_loop bails out and
//symbolic_zipwith_loop takes over.

template <typename matrix1_type, typename matrix2_type>
pure_expr* matrix_zipwith( pure_expr *f, pure_expr *x, pure_expr *y )
{
  pure_ref(f);
  pure_ref(x);
  pure_ref(y);
  typedef typename element_of<matrix1_type>::type elem1_type;
  typedef typename element_of<matrix2_type>::type elem2_type;
  matrix1_type *xm = static_cast<matrix1_type*>(x->data.mat.p);
  matrix2_type *ym = static_cast<matrix2_type*>(y->data.mat.p);

  size_t size1 = xm->size1, size2 = xm->size2;
  if (size1 > ym->size1) size1 = ym->size1;
  if (size2 > ym->size2) size2 = ym->size2;

  if (size1 == 0 || size2 == 0) {
    // empty output matrix
    gsl_matrix_symbolic *sm = create_symbolic_matrix(size1,size2);
    pure_expr *out = pure_symbolic_matrix(sm);
    pure_unref(f);
    pure_unref(x);
    pure_unref(y);
    return out;
  }

  //need casts for complex case, when data pointer is double*, but
  //we really want complex*
  elem1_type *inp1 = reinterpret_cast<elem1_type* >(xm->data);
  elem2_type *inp2 = reinterpret_cast<elem2_type* >(ym->data);
  //make a guess at what the result type is by the first element
  pure_expr *first = pure_appl(f,2,to_expr(*inp1),to_expr(*inp2));
  int32_t firsti;
  double firstd;
  Complex firstc;
  
  size_t lasti=0,lastj=0;
  pure_expr *out=0; //final result

#if HAVE_GSL
  if (from_expr(first,firstd)) { 
    //DOUBLE
    gsl_matrix *dm = create_double_matrix(size1,size2);
    dm->data[0] = firstd;
    pure_expr *last = numeric_zipwith_loop(f,xm,ym,dm,&lasti,&lastj);
    if (!last) {
      out = pure_double_matrix(dm);
    } else {
      gsl_matrix_symbolic *sm = create_symbolic_matrix(size1,size2);
      symbolic_zipwith_loop(f,xm,ym,dm,sm,lasti,lastj,last);
      gsl_matrix_free(dm);
      out = pure_symbolic_matrix(sm);
    }
  } else if (from_expr(first,firsti)) { 
    //INT
    gsl_matrix_int *im = create_int_matrix(size1,size2);
    im->data[0] = firsti; 
    pure_expr *last = numeric_zipwith_loop(f,xm,ym,im,&lasti,&lastj);
    if (!last) { 
      out = pure_int_matrix(im);
    } else {
      gsl_matrix_symbolic *sm = create_symbolic_matrix(size1,size2);
      symbolic_zipwith_loop(f,xm,ym,im,sm,lasti,lastj,last);
      gsl_matrix_int_free(im);
      out = pure_symbolic_matrix(sm);
    }
  } else if (from_expr(first,firstc)) { 
    //COMPLEX
    gsl_matrix_complex *cm = create_complex_matrix(size1,size2);  
    reinterpret_cast<Complex*>(cm->data)[0] = firstc;
    pure_expr *last = numeric_zipwith_loop(f,xm,ym,cm,&lasti,&lastj);
    if (!last) { 
      out = pure_complex_matrix(cm);
    } else {
      gsl_matrix_symbolic *sm = create_symbolic_matrix(size1,size2);
      symbolic_zipwith_loop(f,xm,ym,cm,sm,lasti,lastj,last);
      gsl_matrix_complex_free(cm);
      out = pure_symbolic_matrix(sm);
    }
  } else 
#endif //HAVE_GSL
  { //for anything else, default to symbolic. 
    gsl_matrix_symbolic *sm = create_symbolic_matrix(size1,size2);
    symbolic_zipwith_loop(f,xm,ym,static_cast<matrix1_type*>(0),sm,0,0,first);
    out = pure_symbolic_matrix(sm);
  }

  if (first->refc==0) pure_freenew(first);
  pure_unref(f);
  pure_unref(x);
  pure_unref(y);
  return out;
}


//numeric zipwith3 loop : optmize common case that the output of the zipped
//function is all of the same numerical type. If not, fall back to symbolic
//zipwith3 loop

template < typename in1_mat_type,  
           typename in2_mat_type,
           typename in3_mat_type,
           typename out_mat_type >
pure_expr* 
numeric_zipwith3_loop
( pure_expr *f,       //the function being zipped
  in1_mat_type *in1,   //the 1st operand matrix
  in2_mat_type *in2,   //the 2nd operand matrix
  in3_mat_type *in3,   //the 3rd operand matrix
  out_mat_type *out,  //the result matrix
  size_t *lasti,      //last row and column written to 
  size_t *lastj )     //  before bailing out in the event that
                      //  the output of f is not the right type
{
  typedef typename element_of<in1_mat_type>::type in1_elem_type;
  typedef typename element_of<in2_mat_type>::type in2_elem_type;
  typedef typename element_of<in3_mat_type>::type in3_elem_type;
  typedef typename element_of<out_mat_type>::type out_elem_type;
  
  //need casts for complex case, when data pointer is double*, but
  //we really want complex*
  in1_elem_type *inp1 = (reinterpret_cast<in1_elem_type*>(in1->data ))+1;
  in2_elem_type *inp2 = (reinterpret_cast<in2_elem_type*>(in2->data ))+1;
  in3_elem_type *inp3 = (reinterpret_cast<in3_elem_type*>(in3->data ))+1;
  out_elem_type *outp = (reinterpret_cast<out_elem_type*>(out->data))+1;
  pure_expr *r; //result expression
  out_elem_type v; //result value

  //we already performed the first zipwith3, so now we apply f to all but the
  //first column in the first row..
  *lasti=0;
  for (size_t j=1; j<in1->size2 && j<in2->size2 && j<in3->size2;
       ++j,++inp1,++inp2,++inp3,++outp) {
    *lastj=j;
    r = pure_appl( f, 3, to_expr(*inp1), to_expr(*inp2), to_expr(*inp3) );
    if (!from_expr(r,v)) return r;
    *outp = v;
    pure_freenew(r);
  }

  //..then the rest of the rows
  for (size_t i=1; i<in1->size1 && i<in2->size1 && i<in3->size1; ++i) {
    *lasti=i;
    inp1 = (reinterpret_cast<in1_elem_type*>(in1->data))+i*in1->tda; 
    inp2 = (reinterpret_cast<in2_elem_type*>(in2->data))+i*in2->tda; 
    inp3 = (reinterpret_cast<in3_elem_type*>(in3->data))+i*in3->tda; 
    outp = (reinterpret_cast<out_elem_type*>(out->data))+i*out->tda;
    for (size_t j=0; j<in1->size2 && j<in2->size2 && j<in3->size2;
	 ++j,++inp1,++inp2,++inp3,++outp) {
      *lastj=j;
      r = pure_appl( f, 3, to_expr(*inp1), to_expr(*inp2), to_expr(*inp3) );
      if (!from_expr(r,v)) return r;
      *outp = v;
      pure_freenew(r);
    }
  }
  return 0;
}


//symbolic zipwith3 loop : zipped function results in heterogenous /
//non-numerical types. This function may pick up where numerical_zipwith3_loop
//left off, in that case copying out the already-evaluated numbers from the
//matrix 'num'

template < typename in1_mat_type,  //the operand matrix type
           typename in2_mat_type,
           typename in3_mat_type,
           typename num_mat_type >
void 
symbolic_zipwith3_loop
( pure_expr *f,            //the function being zipped
  in1_mat_type *in1,        //the 1st operand matrix
  in2_mat_type *in2,        //the 2nd operand matrix
  in3_mat_type *in3,        //the 3rd operand matrix

  num_mat_type *num,       //the numerical matrix that _would_ have been 
                           //  the output if the results of f had all been of
                           //  the same numerical type
                           
  gsl_matrix_symbolic *out,//the symbolic matrix that will be the end result

  size_t lasti ,           //some of the zipwith3 may already be evaluated. 
  size_t lastj ,           //  lasti,lastj was the last element zipped
  pure_expr *last )        //  and last is the result
{
  typedef typename element_of<in1_mat_type>::type in1_elem_type;
  typedef typename element_of<in2_mat_type>::type in2_elem_type;
  typedef typename element_of<in3_mat_type>::type in3_elem_type;
  typedef typename element_of<num_mat_type>::type num_elem_type;

  in1_elem_type *inp1 = 0; 
  in2_elem_type *inp2 = 0;
  in3_elem_type *inp3 = 0;
  pure_expr **outp = 0;
  num_elem_type *nump = 0;

  //copy the already-evaluated stuff out of num
  if (lasti>0 || lastj>0) {
    if (lasti>0) {
      for (size_t i=0; i<lasti; ++i) {
        outp = out->data + i*out->tda;
        nump = (reinterpret_cast<num_elem_type*>(num->data)) + i*num->tda;
        for (size_t j=0; j<in1->size2 && j<in2->size2 && j<in3->size2;
	     ++j,++outp,++nump) {
          *outp = to_expr(*nump);
        }
      }
    }
    if (lastj>0) {
      outp = out->data + lasti*out->tda;
      nump = (reinterpret_cast<num_elem_type*>(num->data)) + lasti*num->tda;
      for (size_t j=0; j<lastj; ++j,++outp,++nump) {
        *outp = to_expr(*nump);
      }
    }
  } 

  out->data[lasti*out->tda+lastj] = last;
  lastj++;
  if (lastj>=out->size2) {
    lasti++;
    lastj=0;
    if (lasti>=out->size1) 
      return;
  }

  inp1 = (reinterpret_cast<in1_elem_type*>(in1->data))+lasti*in1->tda+lastj;
  inp2 = (reinterpret_cast<in2_elem_type*>(in2->data))+lasti*in2->tda+lastj;
  inp3 = (reinterpret_cast<in3_elem_type*>(in3->data))+lasti*in3->tda+lastj;
  outp = out->data+lasti*out->tda+lastj;
  //finish the zipwith3
  for (size_t j=lastj; j<in1->size2 && j<in2->size2 && j<in3->size2;
       ++j,++inp1,++inp2,++inp3,++outp)
  {
    *outp = pure_appl(f,3,to_expr(*inp1),to_expr(*inp2),to_expr(*inp3));
  }

  for (size_t i=lasti+1; i<in1->size1 && i<in2->size1 && i<in3->size1; ++i) {
    inp1 =  (reinterpret_cast<in1_elem_type*>(in1->data))+i*in1->tda; 
    inp2 =  (reinterpret_cast<in2_elem_type*>(in2->data))+i*in2->tda; 
    inp3 =  (reinterpret_cast<in3_elem_type*>(in3->data))+i*in3->tda; 
    outp = out->data+i*out->tda;
    for (size_t j=0; j<in1->size2 && j<in2->size2 && j<in3->size2;
	 ++j,++inp1,++inp2,++inp3,++outp) {
      *outp = pure_appl(f,3,to_expr(*inp1),to_expr(*inp2),to_expr(*inp3));
    }
  }
}


//generic matrix zipwith3, dispatches on input matrix type (the template
//parameters) and output matrix type, which is guessed from the result type of
//f (x!0) (y!0). The function speculates that all f (x!i) (y!i) will be of the
//same type for all i. If not, numeric_zipwith3_loop bails out and
//symbolic_zipwith3_loop takes over.

template <typename matrix1_type, typename matrix2_type, typename matrix3_type>
pure_expr* matrix_zipwith3( pure_expr *f, pure_expr *x, pure_expr *y,
			    pure_expr *z )
{
  pure_ref(f);
  pure_ref(x);
  pure_ref(y);
  pure_ref(z);
  typedef typename element_of<matrix1_type>::type elem1_type;
  typedef typename element_of<matrix2_type>::type elem2_type;
  typedef typename element_of<matrix3_type>::type elem3_type;
  matrix1_type *xm = static_cast<matrix1_type*>(x->data.mat.p);
  matrix2_type *ym = static_cast<matrix2_type*>(y->data.mat.p);
  matrix3_type *zm = static_cast<matrix3_type*>(z->data.mat.p);

  size_t size1 = xm->size1, size2 = xm->size2;
  if (size1 > ym->size1) size1 = ym->size1;
  if (size2 > ym->size2) size2 = ym->size2;
  if (size1 > zm->size1) size1 = zm->size1;
  if (size2 > zm->size2) size2 = zm->size2;

  if (size1 == 0 || size2 == 0) {
    // empty output matrix
    gsl_matrix_symbolic *sm = create_symbolic_matrix(size1,size2);
    pure_expr *out = pure_symbolic_matrix(sm);
    pure_unref(f);
    pure_unref(x);
    pure_unref(y);
    pure_unref(z);
    return out;
  }

  //need casts for complex case, when data pointer is double*, but
  //we really want complex*
  elem1_type *inp1 = reinterpret_cast<elem1_type* >(xm->data);
  elem2_type *inp2 = reinterpret_cast<elem2_type* >(ym->data);
  elem3_type *inp3 = reinterpret_cast<elem3_type* >(zm->data);
  //make a guess at what the result type is by the first element
  pure_expr *first = pure_appl(f,3,to_expr(*inp1),to_expr(*inp2),
			       to_expr(*inp3));
  int32_t firsti;
  double firstd;
  Complex firstc;
  
  size_t lasti=0,lastj=0;
  pure_expr *out=0; //final result

#if HAVE_GSL
  if (from_expr(first,firstd)) { 
    //DOUBLE
    gsl_matrix *dm = create_double_matrix(size1,size2);
    dm->data[0] = firstd;
    pure_expr *last = numeric_zipwith3_loop(f,xm,ym,zm,dm,&lasti,&lastj);
    if (!last) {
      out = pure_double_matrix(dm);
    } else {
      gsl_matrix_symbolic *sm = create_symbolic_matrix(size1,size2);
      symbolic_zipwith3_loop(f,xm,ym,zm,dm,sm,lasti,lastj,last);
      gsl_matrix_free(dm);
      out = pure_symbolic_matrix(sm);
    }
  } else if (from_expr(first,firsti)) { 
    //INT
    gsl_matrix_int *im = create_int_matrix(size1,size2);
    im->data[0] = firsti; 
    pure_expr *last = numeric_zipwith3_loop(f,xm,ym,zm,im,&lasti,&lastj);
    if (!last) { 
      out = pure_int_matrix(im);
    } else {
      gsl_matrix_symbolic *sm = create_symbolic_matrix(size1,size2);
      symbolic_zipwith3_loop(f,xm,ym,zm,im,sm,lasti,lastj,last);
      gsl_matrix_int_free(im);
      out = pure_symbolic_matrix(sm);
    }
  } else if (from_expr(first,firstc)) { 
    //COMPLEX
    gsl_matrix_complex *cm = create_complex_matrix(size1,size2);  
    reinterpret_cast<Complex*>(cm->data)[0] = firstc;
    pure_expr *last = numeric_zipwith3_loop(f,xm,ym,zm,cm,&lasti,&lastj);
    if (!last) { 
      out = pure_complex_matrix(cm);
    } else {
      gsl_matrix_symbolic *sm = create_symbolic_matrix(size1,size2);
      symbolic_zipwith3_loop(f,xm,ym,zm,cm,sm,lasti,lastj,last);
      gsl_matrix_complex_free(cm);
      out = pure_symbolic_matrix(sm);
    }
  } else 
#endif //HAVE_GSL
  { //for anything else, default to symbolic. 
    gsl_matrix_symbolic *sm = create_symbolic_matrix(size1,size2);
    symbolic_zipwith3_loop(f,xm,ym,zm,static_cast<matrix1_type*>(0),sm,0,0,
			   first);
    out = pure_symbolic_matrix(sm);
  }

  if (first->refc==0) pure_freenew(first);
  pure_unref(f);
  pure_unref(x);
  pure_unref(y);
  pure_unref(z);
  return out;
}


//numeric scanl loop : optmize common case that the output of the function
//is all of the same numerical type. If not, fall back to symbolic scanl loop

template < typename in_mat_type,  
           typename out_mat_type >
pure_expr* 
numeric_scanl_loop
( pure_expr *f,       //the function being mapped
  pure_expr *z,       //seed value
  bool init,          //1 if the seed value was taken from the matrix
  in_mat_type *in,    //the operand matrix
  out_mat_type *out,  //the result matrix
  size_t *lasti,      //last row and column written to 
  size_t *lastj )     //  before bailing out in the event that
                      //  the output of f is not the right type
{
  typedef typename element_of< in_mat_type>::type  in_elem_type;
  typedef typename element_of<out_mat_type>::type out_elem_type;
  
  out_elem_type v;
  
  if (in->size1 == 0 || in->size2 == 0) return 0;

  out_elem_type *outp = (reinterpret_cast<out_elem_type*>(out->data))+1;
  for (size_t i=0; i<in->size1; ++i) {
    *lasti=i;
    in_elem_type *inp =
      (reinterpret_cast<in_elem_type*>(in->data))+i*in->tda+init; 
    for (size_t j=init; j<in->size2; ++j,++inp,++outp) {
      *lastj=j;
      pure_expr *zz = pure_new(z);
      z = pure_appl( f, 2, z, to_expr(*inp) );
      if (!from_expr(z,v)) return z;
      *outp = v;
      pure_free(zz);
    }
    init = 0;
  }
  return 0;
}


//symbolic scanl loop : function results in heterogenous / non-numerical
//types. This function may pick up where numerical_scanl_loop left off, in
//that case copying out the already-evaluated numbers from the matrix 'num'

template < typename in_mat_type,  //the operand matrix type
           typename num_mat_type >
void 
symbolic_scanl_loop
( pure_expr *f,            //The function being mapped
  pure_expr *z,            //The seed value OR last value produced in
                           //  numerical loop
  bool init,               //1 if the seed value was taken from the matrix
  in_mat_type *in,         //The operand matrix
  num_mat_type *num,       //The numerical matrix that _would_ have been 
                           //  the output if the results of f had all been of
                           //  the same numerical type. Possibly null.
  gsl_matrix_symbolic *out,//The symbolic matrix that will be the end result
  ptrdiff_t lasti ,        //Some of the scan may already be evaluated. 
  ptrdiff_t lastj )        //lasti,lastj was the last argument given to f
                              //and z was the result.
{
  typedef typename element_of< in_mat_type>::type  in_elem_type;
  typedef typename element_of<num_mat_type>::type num_elem_type;

  in_elem_type *inp = 0;
  pure_expr **outp = out->data + (!num||init?0:1); 
    // num==0 indicates that z is the initial seed value

  //copy the already-evaluated stuff out of num
  if (num) {
    assert(num->size1 == 1);
    assert(num->size1 == out->size1);
    num_elem_type *nump = reinterpret_cast<num_elem_type*>(num->data);
    for (size_t j=0; j<lasti*in->size2+lastj; ++j) {
        *(outp++) = to_expr(*(nump++)) ;
    }
  }

  *(outp++) = z;
  lastj++;
  if ((size_t)lastj>=in->size2) {
    lasti++;
    lastj=0;
    if ((size_t)lasti>=in->size1) 
      return;
  }

  inp = reinterpret_cast<in_elem_type*>(in->data)+lasti*in->tda+lastj;
  //finish the scan
  for (size_t j=lastj; j<in->size2; ++j) {
    pure_expr *zz = pure_new(z);
    *(outp++) = z = pure_appl(f,2,z,to_expr(*(inp++)));
    pure_unref(zz);
  }

  for (size_t i=lasti+1; i<in->size1; ++i) {
    inp = reinterpret_cast<in_elem_type*>(in->data)+i*in->tda; 
    for (size_t j=0; j<in->size2; ++j) {
      pure_expr *zz = pure_new(z);
      *(outp++) = z = pure_appl(f,2,z,to_expr(*(inp++)));
      pure_unref(zz);
    }
  }

  assert(out->size1==1);
  for (size_t i=0; i<out->size2; ++i) assert(out->data[i]);
}


//Generic matrix scanl, dispatches on input matrix type (the template
//parameter) and output matrix type, which is guessed from the seed value (z).
//The function speculates that all results will be of the same type.  If not,
//numeric_scanl_loop bails out and symbolic_scanl_loop takes over.

template <typename matrix_type>
pure_expr* matrix_scanl( pure_expr *f, pure_expr *z, pure_expr *x )
{
  pure_ref(f);
  pure_ref(x);

  typedef typename element_of<matrix_type>::type elem_type;
  matrix_type *xm = static_cast<matrix_type*>(x->data.mat.p);
  size_t lasti,lastj;
  pure_expr *out; //result matrix

  double zd;
  int zi;
  Complex zc;

#if HAVE_GSL
  if (from_expr(z,zd)) {
    gsl_matrix *dm = create_double_matrix(1,1+xm->size1*xm->size2);
    dm->data[0] = zd;
    pure_expr *last = numeric_scanl_loop(f,z,0,xm,dm,&lasti,&lastj);
    if (!last) {
      out = pure_double_matrix(dm); 
    } else {
      gsl_matrix_symbolic *sm =
	create_symbolic_matrix(1,1+xm->size1*xm->size2);
      sm->data[0] = z;
      symbolic_scanl_loop(f,last,0,xm,dm,sm,lasti,lastj);
      gsl_matrix_free(dm);
      out = pure_symbolic_matrix(sm);
    }
  } else if (from_expr(z,zi)) {
    gsl_matrix_int *im = create_int_matrix(1,1+xm->size1*xm->size2);
    im->data[0] = zi; 
    pure_expr *last = numeric_scanl_loop(f,z,0,xm,im,&lasti,&lastj);
    if (!last) { 
      out = pure_int_matrix(im);
    } else {
      gsl_matrix_symbolic *sm =
	create_symbolic_matrix(1,1+xm->size1*xm->size2);
      sm->data[0] = z;
      symbolic_scanl_loop(f,last,0,xm,im,sm,lasti,lastj);
      gsl_matrix_int_free(im);
      out = pure_symbolic_matrix(sm);
    }
  } else if (from_expr(z,zc)) {
    gsl_matrix_complex *cm = create_complex_matrix(1,1+xm->size1*xm->size2);
    reinterpret_cast<Complex*>(cm->data)[0] = zc; 
    pure_expr *last = numeric_scanl_loop(f,z,0,xm,cm,&lasti,&lastj);
    if (!last) { 
      out = pure_complex_matrix(cm);
    } else {
      gsl_matrix_symbolic *sm =
	create_symbolic_matrix(1,1+xm->size1*xm->size2);
      sm->data[0] = z;
      symbolic_scanl_loop(f,last,0,xm,cm,sm,lasti,lastj);
      gsl_matrix_complex_free(cm);
      out = pure_symbolic_matrix(sm);
    }
  } else
#endif //HAVE_GSL
  { //for anything else, default to symbolic. 
      gsl_matrix_symbolic *sm =
	create_symbolic_matrix(1,1+xm->size1*xm->size2);
      symbolic_scanl_loop(f,z,0,xm,static_cast<matrix_type*>(0),sm,0,-1);
      out = pure_symbolic_matrix(sm);
  }
  pure_unref(f);
  pure_unref(x);
  return out;
}


// same for scanl1
template <typename matrix_type>
pure_expr* matrix_scanl1( pure_expr *f, pure_expr *x )
{
  matrix_type *xm = static_cast<matrix_type*>(x->data.mat.p);
  if (xm->size1 == 0 || xm->size2 == 0) {
    gsl_matrix_symbolic *sm = create_symbolic_matrix(1, 0);
    return pure_symbolic_matrix(sm);
  }
  pure_ref(f);
  pure_ref(x);

  typedef typename element_of<matrix_type>::type elem_type;
  size_t lasti,lastj;
  pure_expr *out; //result matrix

  double zd;
  int zi;
  Complex zc;

  elem_type *p = reinterpret_cast<elem_type*>(xm->data);
  pure_expr *z = to_expr(*p);

#if HAVE_GSL
  if (from_expr(z,zd)) {
    gsl_matrix *dm = create_double_matrix(1,xm->size1*xm->size2);
    dm->data[0] = zd;
    pure_expr *last = numeric_scanl_loop(f,z,1,xm,dm,&lasti,&lastj);
    if (!last) {
      out = pure_double_matrix(dm); 
    } else {
      gsl_matrix_symbolic *sm =
	create_symbolic_matrix(1,xm->size1*xm->size2);
      symbolic_scanl_loop(f,last,1,xm,dm,sm,lasti,lastj);
      gsl_matrix_free(dm);
      out = pure_symbolic_matrix(sm);
    }
  } else if (from_expr(z,zi)) {
    gsl_matrix_int *im = create_int_matrix(1,xm->size1*xm->size2);
    im->data[0] = zi; 
    pure_expr *last = numeric_scanl_loop(f,z,1,xm,im,&lasti,&lastj);
    if (!last) { 
      out = pure_int_matrix(im);
    } else {
      gsl_matrix_symbolic *sm =
	create_symbolic_matrix(1,xm->size1*xm->size2);
      symbolic_scanl_loop(f,last,1,xm,im,sm,lasti,lastj);
      gsl_matrix_int_free(im);
      out = pure_symbolic_matrix(sm);
    }
  } else if (from_expr(z,zc)) {
    gsl_matrix_complex *cm = create_complex_matrix(1,xm->size1*xm->size2);
    reinterpret_cast<Complex*>(cm->data)[0] = zc; 
    pure_expr *last = numeric_scanl_loop(f,z,1,xm,cm,&lasti,&lastj);
    if (!last) { 
      out = pure_complex_matrix(cm);
    } else {
      gsl_matrix_symbolic *sm =
	create_symbolic_matrix(1,xm->size1*xm->size2);
      symbolic_scanl_loop(f,last,1,xm,cm,sm,lasti,lastj);
      gsl_matrix_complex_free(cm);
      out = pure_symbolic_matrix(sm);
    }
  } else
#endif //HAVE_GSL
  { //for anything else, default to symbolic. 
      gsl_matrix_symbolic *sm =
	create_symbolic_matrix(1,xm->size1*xm->size2);
      symbolic_scanl_loop(f,z,1,xm,static_cast<matrix_type*>(0),sm,0,0);
      out = pure_symbolic_matrix(sm);
  }
  pure_unref(f);
  pure_unref(x);
  return out;
}


//Numeric scanr loop : optmize common case that the output of the function
//is all of the same numerical type. If not, fall back to symbolic scanr loop

template < typename in_mat_type,  
           typename out_mat_type >
pure_expr* 
numeric_scanr_loop
( pure_expr *f,       //the function being mapped
  pure_expr *z,       //seed value
  bool init,          //1 if the seed value was taken from the matrix
  in_mat_type *in,    //the operand matrix
  out_mat_type *out,  //the result matrix
  ptrdiff_t *lasti,   //last row and column written to 
  ptrdiff_t *lastj )  //  before bailing out in the event that
                      //  the output of f is not the right type
{
  typedef typename element_of< in_mat_type>::type  in_elem_type;
  typedef typename element_of<out_mat_type>::type out_elem_type;
  
  if (in->size1 == 0 || in->size2 == 0) return 0;
  
  out_elem_type v;
  out_elem_type *outp = reinterpret_cast<out_elem_type*>(out->data)+
                         in->size1*in->size2-1-init;
  for (ptrdiff_t i=in->size1-1; i>=0; --i) {
    *lasti=i;
    in_elem_type *inp = reinterpret_cast<in_elem_type*>(in->data)+
                         i*in->tda+in->size2-1-init; 
    for (ptrdiff_t j=in->size2-1-init; j>=0; --j,--inp,--outp) {
      *lastj=j;
      pure_expr *zz = pure_new(z);
      z = pure_appl( f, 2, to_expr(*inp), z );
      if (!from_expr(z,v)) return z;
      *outp = v;
      pure_free(zz);
    }
    init = 0;
  }
  return 0;
}


//Symbolic scanr loop : function results in heterogenous / non-numerical
//types. This function may pick up where numerical_scanl_loop left off, in
//that case copying out the already-evaluated numbers from the matrix 'num'

template < typename in_mat_type,  //the operand matrix type
           typename num_mat_type >
void 
symbolic_scanr_loop
( pure_expr *f,            //The function being mapped
  pure_expr *z,            //The seed value OR the last result of the
                           //  numerical loop.
  bool init,               //1 if the seed value was taken from the matrix
  in_mat_type *in,         //The operand matrix
  num_mat_type *num,       //The numerical matrix that would have been 
                           //  the output if the results of f had all been of
                           //  the same numerical type. Possibly null.
  gsl_matrix_symbolic *out,//The symbolic matrix that will be the end result
  ptrdiff_t lasti ,        //Some of the scan may already be evaluated. 
  ptrdiff_t lastj )        //lasti,lastj was the last argument given to f
                              //and z was the result.
{
  typedef typename element_of< in_mat_type>::type  in_elem_type;
  typedef typename element_of<num_mat_type>::type num_elem_type;

  in_elem_type *inp = 0;
  pure_expr **outp = out->data+in->size1*in->size2 - (!num||init?0:1) - init;
    //num==0 indicates that z is the initial seed value

  //copy the already-evaluated stuff out of num
  if (num) {
    assert(num->size1 == 1);
    assert(num->size1 == out->size1);
    num_elem_type *nump = 
      reinterpret_cast<num_elem_type*>(num->data)+num->size2-1;
       
                         //        here we start at -2 because -1 was
                         //vvvvvvv already set before calling
    for ( size_t j=num->size2-2+init; j>lasti*in->size2+lastj; --j )
        *(outp--) = to_expr(*(nump--));
  } 

  *(outp--) = z;
  lastj--;
  if (lastj == -1) {
    lasti--;
    lastj=in->size2-1;
    if (lasti == -1) 
      return;
  }

  inp = reinterpret_cast<in_elem_type*>(in->data)+lasti*in->tda+lastj;
  //finish the scan
  for (ptrdiff_t j=lastj; j>=0; --j) {
    pure_expr *zz = pure_new(z);
    *(outp--) = z = pure_appl(f,2,to_expr(*(inp--)),z);
    pure_unref(zz);
  }

  for (ptrdiff_t i=lasti-1; i>=0; --i) {
    inp = reinterpret_cast<in_elem_type*>(in->data)+i*in->tda+in->size2-1; 
    for (ptrdiff_t j=in->size2-1; j>=0; --j) {
      pure_expr *zz = pure_new(z);
      *(outp--) = z = pure_appl(f,2,to_expr(*(inp--)),z);
      pure_unref(zz);
    }
  }

  assert(out->size1==1);
  for (size_t i=0; i<out->size2; ++i) assert(out->data[i]);
}


//Generic matrix scanr, dispatches on input matrix type (the template
//parameter) and output matrix type, which is guessed from the seed value (z).
//The function speculates that all values will be of the same type for all i.
//If not, numeric_scanr_loop bails out and symbolic_scanr_loop takes over.

template <typename matrix_type>
pure_expr* matrix_scanr( pure_expr *f, pure_expr *z, pure_expr *x )
{
  pure_ref(f);
  pure_ref(x);

  typedef typename element_of<matrix_type>::type elem_type;
  matrix_type *xm = static_cast<matrix_type*>(x->data.mat.p);
  ptrdiff_t lasti,lastj;
  pure_expr *out; //result matrix
  double zd;
  int32_t zi;
  Complex zc;

#if HAVE_GSL
  if (from_expr(z,zd)) {
    //DOUBLE
    gsl_matrix *dm = create_double_matrix(1,1+xm->size1*xm->size2);
    dm->data[xm->size1*xm->size2] = zd;
    pure_expr *last = numeric_scanr_loop(f,z,0,xm,dm,&lasti,&lastj);
    if (!last) {
      out = pure_double_matrix(dm); 
    } else {
      gsl_matrix_symbolic *sm = 
          create_symbolic_matrix(1,1+xm->size1*xm->size2);
      sm->data[xm->size1*xm->size2] = z;
      symbolic_scanr_loop(f,last,0,xm,dm,sm,lasti,lastj);
      gsl_matrix_free(dm);
      out = pure_symbolic_matrix(sm);
    }
  } else if (from_expr(z,zi)) {
    //INT
    gsl_matrix_int *im = create_int_matrix(1,1+xm->size1*xm->size2);
    im->data[xm->size1*xm->size2] = zi; 
    pure_expr *last = numeric_scanr_loop(f,z,0,xm,im,&lasti,&lastj);
    if (!last) { 
      out = pure_int_matrix(im);
    } else {
      gsl_matrix_symbolic *sm = 
          create_symbolic_matrix(1,1+xm->size1*xm->size2);
      sm->data[xm->size1*xm->size2] = z;
      symbolic_scanr_loop(f,last,0,xm,im,sm,lasti,lastj);
      gsl_matrix_int_free(im);
      out = pure_symbolic_matrix(sm);
    }
  } else if (from_expr(z,zc)) {
    //COMPLEX
    gsl_matrix_complex *cm = create_complex_matrix(1,1+xm->size1*xm->size2);
    reinterpret_cast<Complex*>(cm->data)[xm->size1*xm->size2] = zc; 
    pure_expr *last = numeric_scanr_loop(f,z,0,xm,cm,&lasti,&lastj);
    if (!last) { 
      out = pure_complex_matrix(cm);
    } else {
      gsl_matrix_symbolic *sm = 
          create_symbolic_matrix(1,1+xm->size1*xm->size2);
      sm->data[xm->size1*xm->size2] = z;
      symbolic_scanr_loop(f,last,0,xm,cm,sm,lasti,lastj);
      gsl_matrix_complex_free(cm);
      out = pure_symbolic_matrix(sm);
    }
  } else 
#endif //HAVE_GSL
  { 
    //for anything else, default to symbolic. 
    gsl_matrix_symbolic *sm = 
      create_symbolic_matrix(1,1+xm->size1*xm->size2);
    symbolic_scanr_loop(f,z,0,xm,static_cast<matrix_type*>(0),sm,
                        xm->size1-1,xm->size2);
    out = pure_symbolic_matrix(sm);
  }
  pure_unref(f);
  pure_unref(x);
  return out;
}

// same for scanr1
template <typename matrix_type>
pure_expr* matrix_scanr1( pure_expr *f, pure_expr *x )
{
  matrix_type *xm = static_cast<matrix_type*>(x->data.mat.p);
  if (xm->size1 == 0 || xm->size2 == 0) {
    gsl_matrix_symbolic *sm = create_symbolic_matrix(1, 0);
    return pure_symbolic_matrix(sm);
  }
  pure_ref(f);
  pure_ref(x);

  typedef typename element_of<matrix_type>::type elem_type;
  ptrdiff_t lasti,lastj;
  pure_expr *out; //result matrix
  double zd;
  int32_t zi;
  Complex zc;

  elem_type *p = reinterpret_cast<elem_type*>(xm->data+xm->size1*xm->size2-1);
  pure_expr *z = to_expr(*p);

#if HAVE_GSL
  if (from_expr(z,zd)) {
    //DOUBLE
    gsl_matrix *dm = create_double_matrix(1,xm->size1*xm->size2);
    dm->data[xm->size1*xm->size2-1] = zd;
    pure_expr *last = numeric_scanr_loop(f,z,1,xm,dm,&lasti,&lastj);
    if (!last) {
      out = pure_double_matrix(dm); 
    } else {
      gsl_matrix_symbolic *sm = 
          create_symbolic_matrix(1,xm->size1*xm->size2);
      symbolic_scanr_loop(f,last,1,xm,dm,sm,lasti,lastj);
      gsl_matrix_free(dm);
      out = pure_symbolic_matrix(sm);
    }
  } else if (from_expr(z,zi)) {
    //INT
    gsl_matrix_int *im = create_int_matrix(1,xm->size1*xm->size2);
    im->data[xm->size1*xm->size2-1] = zi; 
    pure_expr *last = numeric_scanr_loop(f,z,1,xm,im,&lasti,&lastj);
    if (!last) { 
      out = pure_int_matrix(im);
    } else {
      gsl_matrix_symbolic *sm = 
          create_symbolic_matrix(1,xm->size1*xm->size2);
      symbolic_scanr_loop(f,last,1,xm,im,sm,lasti,lastj);
      gsl_matrix_int_free(im);
      out = pure_symbolic_matrix(sm);
    }
  } else if (from_expr(z,zc)) {
    //COMPLEX
    gsl_matrix_complex *cm = create_complex_matrix(1,xm->size1*xm->size2);
    reinterpret_cast<Complex*>(cm->data)[xm->size1*xm->size2-1] = zc; 
    pure_expr *last = numeric_scanr_loop(f,z,1,xm,cm,&lasti,&lastj);
    if (!last) { 
      out = pure_complex_matrix(cm);
    } else {
      gsl_matrix_symbolic *sm = 
          create_symbolic_matrix(1,xm->size1*xm->size2);
      symbolic_scanr_loop(f,last,1,xm,cm,sm,lasti,lastj);
      gsl_matrix_complex_free(cm);
      out = pure_symbolic_matrix(sm);
    }
  } else 
#endif //HAVE_GSL
  { 
    //for anything else, default to symbolic. 
    gsl_matrix_symbolic *sm = 
      create_symbolic_matrix(1,xm->size1*xm->size2);
    symbolic_scanr_loop(f,z,1,xm,static_cast<matrix_type*>(0),sm,
                        xm->size1-1,xm->size2-1);
    out = pure_symbolic_matrix(sm);
  }
  pure_unref(f);
  pure_unref(x);
  return out;
}


//generic matrix foldl
template <typename matrix_type>
pure_expr* matrix_foldl( pure_expr *f, pure_expr *z, pure_expr *x )
{
  pure_ref(f);
  pure_ref(x);
  matrix_type *xm = static_cast<matrix_type*>(x->data.mat.p);
  typedef typename element_of<matrix_type>::type elem_type;

  for (size_t i=0; i<xm->size1; ++i) {
    elem_type *p = reinterpret_cast<elem_type*>(xm->data)+i*xm->tda;
    for (size_t j=0; j<xm->size2; ++j,++p) {
      pure_expr *zz = pure_new(z);
      z = pure_appl( f, 2, z, to_expr(*p)  );
      pure_free(zz);
    }
  }
  pure_unref(f);
  pure_unref(x);
  return z;
}


//generic matrix foldl1
template <typename matrix_type>
pure_expr* matrix_foldl1( pure_expr *f, pure_expr *x )
{
  matrix_type *xm = static_cast<matrix_type*>(x->data.mat.p);
  if (xm->size1 == 0 || xm->size2 == 0) return 0;
  pure_ref(f);
  pure_ref(x);
  typedef typename element_of<matrix_type>::type elem_type;

  pure_expr *z = 0;
  for (size_t i=0; i<xm->size1; ++i) {
    elem_type *p = reinterpret_cast<elem_type*>(xm->data)+i*xm->tda;
    for (size_t j=0; j<xm->size2; ++j,++p)
      if (z) {
	pure_expr *zz = pure_new(z);
	z = pure_appl( f, 2, z, to_expr(*p)  );
	pure_free(zz);
      } else
	z = to_expr(*p);
  }
  pure_unref(f);
  pure_unref(x);
  return z;
}


//generic matrix foldr
template <typename matrix_type>
pure_expr* matrix_foldr( pure_expr *f, pure_expr *z, pure_expr *x )
{
  pure_ref(f);
  pure_ref(x);
  matrix_type *xm = static_cast<matrix_type*>(x->data.mat.p);
  typedef typename element_of<matrix_type>::type elem_type;

  for (ptrdiff_t i=xm->size1-1; i>=0; --i) {
    elem_type *p =
      reinterpret_cast<elem_type*>(xm->data)+i*xm->tda+xm->size2-1;
    for (ptrdiff_t j=xm->size2-1; j>=0; --j,--p) {
      pure_expr *zz = pure_new(z);
      z = pure_appl( f, 2, to_expr(*p), z  );
      pure_free(zz);
    }
  }
  pure_unref(f);
  pure_unref(x);
  return z;
}


//generic matrix foldr1
template <typename matrix_type>
pure_expr* matrix_foldr1( pure_expr *f, pure_expr *x )
{
  matrix_type *xm = static_cast<matrix_type*>(x->data.mat.p);
  if (xm->size1 == 0 || xm->size2 == 0) return 0;
  pure_ref(f);
  pure_ref(x);
  typedef typename element_of<matrix_type>::type elem_type;

  pure_expr *z = 0;
  for (ptrdiff_t i=xm->size1-1; i>=0; --i) {
    elem_type *p =
      reinterpret_cast<elem_type*>(xm->data)+i*xm->tda+xm->size2-1;
    for (ptrdiff_t j=xm->size2-1; j>=0; --j,--p)
      if (z) {
	pure_expr *zz = pure_new(z);
	z = pure_appl( f, 2, to_expr(*p), z  );
	pure_free(zz);
      } else
	z = to_expr(*p);
  }
  pure_unref(f);
  pure_unref(x);
  return z;
}


//generic matrix filter. Throws failed_cond if p(x!i) is not int for all i.
template <typename matrix_type>
matrix_type* matrix_filter( pure_expr *p, pure_expr *x ) {
  pure_ref(p);
  matrix_type *xm = static_cast<matrix_type*>(x->data.mat.p);
  typedef typename element_of<matrix_type>::type elem_type;

  //the output matrix
  matrix_type *o = create_matrix<matrix_type>(1,xm->size1*xm->size2);
  size_t n; //the size of the output matrix
  elem_type *po = reinterpret_cast<elem_type*>(o->data);
                  //po is output write head
  for (size_t i=0; i<xm->size1; ++i) {
    //pi is input read head
    elem_type *pi = reinterpret_cast<elem_type*>(xm->data)+i*xm->tda; 
    for (size_t j=0; j<xm->size2; ++j,++pi) {
      pure_expr *b = pure_app( p, to_expr(*pi) );
      int32_t bi;
      bool res = pure_is_int(b,&bi);
      pure_freenew(b);
      if (!res) goto exception;
      if (bi) *(po++) = *pi;
    }
  }
  n = po - reinterpret_cast<elem_type*>(o->data);

  //compact the output matrix
  if (n != xm->size1*xm->size2) {
    matrix_type *o2 = create_matrix<matrix_type>(1,n);
    memcpy(o2->data,o->data,n*sizeof(elem_type));
    matrix_free(o);
    o = o2;
  }
  pure_unref(p);
  return o;

  exception:
    pure_unref(p);
    matrix_free(o);
    pure_throw( pure_symbol( 
      interpreter::g_interp->symtab.failed_cond_sym().f ) );
    return 0; 
}


//generic matrix_takewhile/dropwhile. Throws failed_cond.

template <typename matrix_type>
matrix_type* matrix_takewhile( pure_expr *p, pure_expr *x ) {
  matrix_type *xm = static_cast<matrix_type*>(x->data.mat.p);
  typedef typename element_of<matrix_type>::type elem_type;

  size_t count = 0;
  for (size_t i=0; i<xm->size1; ++i) {
    //pi is input read head
    elem_type *pi = reinterpret_cast<elem_type*>(xm->data)+i*xm->tda; 
    for (size_t j=0; j<xm->size2; ++j,++pi) {
      pure_expr *b = pure_app( p, to_expr(*pi) );
      int32_t bi;
      bool res = pure_is_int(b,&bi);
      pure_freenew(b);
      if (!res) goto exception;
      if (!bi) goto done;
      count++;
    }
  }
  goto done;

 exception:
    pure_unref(p);
    pure_throw( pure_symbol( 
      interpreter::g_interp->symtab.failed_cond_sym().f ) );
    return 0; 

 done:
  //the output matrix
  matrix_type *o = create_matrix<matrix_type>(1,count);
  size_t n = count; //the size of the output matrix
  elem_type *po = reinterpret_cast<elem_type*>(o->data);
                  //po is output write head
  count = 0;
  for (size_t i=0; i<xm->size1; ++i) {
    //pi is input read head
    elem_type *pi = reinterpret_cast<elem_type*>(xm->data)+i*xm->tda; 
    for (size_t j=0; j<xm->size2; ++j,++pi,++po) {
      if (count >= n) goto really_done;
      *po = *pi;
      count++;
    }
  }
 really_done:
  return o;
}

template <typename matrix_type>
matrix_type* matrix_dropwhile( pure_expr *p, pure_expr *x ) {
  matrix_type *xm = static_cast<matrix_type*>(x->data.mat.p);
  typedef typename element_of<matrix_type>::type elem_type;

  size_t count = 0, lasti = xm->size1, lastj = xm->size2;
  for (size_t i=0; i<xm->size1; ++i) {
    //pi is input read head
    elem_type *pi = reinterpret_cast<elem_type*>(xm->data)+i*xm->tda; 
    for (size_t j=0; j<xm->size2; ++j,++pi) {
      pure_expr *b = pure_app( p, to_expr(*pi) );
      int32_t bi;
      bool res = pure_is_int(b,&bi);
      pure_freenew(b);
      if (!res) goto exception;
      if (!bi) {
	lasti = i; lastj = j;
	goto done;
      }
      count++;
    }
  }
  goto done;

 exception:
    pure_unref(p);
    pure_throw( pure_symbol( 
      interpreter::g_interp->symtab.failed_cond_sym().f ) );
    return 0; 

 done:
  //the output matrix
  size_t n = xm->size1*xm->size2-count; //the size of the output matrix
  matrix_type *o = create_matrix<matrix_type>(1,n);
  elem_type *po = reinterpret_cast<elem_type*>(o->data);
                  //po is output write head
  for (size_t i=lasti; i<xm->size1; ++i) {
    //pi is input read head
    elem_type *pi = reinterpret_cast<elem_type*>(xm->data)+i*xm->tda+lastj;
    for (size_t j=lastj; j<xm->size2; ++j,++pi,++po) {
      *po = *pi;
    }
    lastj = 0;
  }
  return o;
}


//generic matrix checkers. Throw failed_cond if p(x!i) is not int for all i.

template <typename matrix_type>
bool matrix_all( pure_expr *p, pure_expr *x ) {
  matrix_type *xm = static_cast<matrix_type*>(x->data.mat.p);
  typedef typename element_of<matrix_type>::type elem_type;

  for (size_t i=0; i<xm->size1; ++i) {
    //pi is input read head
    elem_type *pi = reinterpret_cast<elem_type*>(xm->data)+i*xm->tda; 
    for (size_t j=0; j<xm->size2; ++j,++pi) {
      pure_expr *b = pure_app( p, to_expr(*pi) );
      int32_t bi;
      bool res = pure_is_int(b,&bi);
      pure_freenew(b);
      if (!res) goto exception;
      if (!bi) return false;
    }
  }
  return true;

  exception:
    pure_unref(p);
    pure_throw( pure_symbol( 
      interpreter::g_interp->symtab.failed_cond_sym().f ) );
    return false; 
}

template <typename matrix_type>
bool matrix_any( pure_expr *p, pure_expr *x ) {
  matrix_type *xm = static_cast<matrix_type*>(x->data.mat.p);
  typedef typename element_of<matrix_type>::type elem_type;

  for (size_t i=0; i<xm->size1; ++i) {
    //pi is input read head
    elem_type *pi = reinterpret_cast<elem_type*>(xm->data)+i*xm->tda; 
    for (size_t j=0; j<xm->size2; ++j,++pi) {
      pure_expr *b = pure_app( p, to_expr(*pi) );
      int32_t bi;
      bool res = pure_is_int(b,&bi);
      pure_freenew(b);
      if (!res) goto exception;
      if (bi) return true;
    }
  }
  return false;

  exception:
    pure_unref(p);
    pure_throw( pure_symbol( 
      interpreter::g_interp->symtab.failed_cond_sym().f ) );
    return false; 
}


} // namespace matrix


extern "C" 
void matrix_do ( pure_expr *f, pure_expr *x )
{
  switch (x->tag) {
  case EXPR::DMATRIX : return matrix::matrix_do<gsl_matrix>(f,x);
  case EXPR::IMATRIX : return matrix::matrix_do<gsl_matrix_int>(f,x);
  case EXPR::CMATRIX : return matrix::matrix_do<gsl_matrix_complex>(f,x);
  case EXPR::MATRIX  : return matrix::matrix_do<gsl_matrix_symbolic>(f,x);
  }
}

extern "C" 
pure_expr* matrix_map ( pure_expr *f, pure_expr *x )
{
  switch (x->tag) {
  case EXPR::DMATRIX : return matrix::matrix_map<gsl_matrix>(f,x);
  case EXPR::IMATRIX : return matrix::matrix_map<gsl_matrix_int>(f,x);
  case EXPR::CMATRIX : return matrix::matrix_map<gsl_matrix_complex>(f,x);
  case EXPR::MATRIX  : return matrix::matrix_map<gsl_matrix_symbolic>(f,x);
  default : return 0;
  }
}

extern "C" 
pure_expr* matrix_zipwith ( pure_expr *f, pure_expr *x, pure_expr *y )
{
  switch (x->tag) {
  case EXPR::DMATRIX :
    switch (y->tag) {
    case EXPR::DMATRIX : 
      return matrix::matrix_zipwith<gsl_matrix,gsl_matrix>(f,x,y);
    case EXPR::IMATRIX : 
      return matrix::matrix_zipwith<gsl_matrix,gsl_matrix_int>(f,x,y);
    case EXPR::CMATRIX :
      return matrix::matrix_zipwith<gsl_matrix,gsl_matrix_complex>(f,x,y);
    case EXPR::MATRIX :
      return matrix::matrix_zipwith<gsl_matrix,gsl_matrix_symbolic>(f,x,y);
    default : return 0;
    }
  case EXPR::IMATRIX :
    switch (y->tag) {
    case EXPR::DMATRIX : 
      return matrix::matrix_zipwith<gsl_matrix_int,gsl_matrix>(f,x,y);
    case EXPR::IMATRIX : 
      return matrix::matrix_zipwith<gsl_matrix_int,gsl_matrix_int>(f,x,y);
    case EXPR::CMATRIX :
      return matrix::matrix_zipwith<gsl_matrix_int,gsl_matrix_complex>(f,x,y);
    case EXPR::MATRIX :
      return matrix::matrix_zipwith<gsl_matrix_int,gsl_matrix_symbolic>(f,x,y);
    default : return 0;
    }
  case EXPR::CMATRIX :
    switch (y->tag) {
    case EXPR::DMATRIX : 
      return matrix::matrix_zipwith<gsl_matrix_complex,gsl_matrix>(f,x,y);
    case EXPR::IMATRIX : 
      return matrix::matrix_zipwith<gsl_matrix_complex,gsl_matrix_int>(f,x,y);
    case EXPR::CMATRIX :
      return matrix::matrix_zipwith<gsl_matrix_complex,gsl_matrix_complex>(f,x,y);
    case EXPR::MATRIX :
      return matrix::matrix_zipwith<gsl_matrix_complex,gsl_matrix_symbolic>(f,x,y);
    default : return 0;
    }
  case EXPR::MATRIX  :
    switch (y->tag) {
    case EXPR::DMATRIX : 
      return matrix::matrix_zipwith<gsl_matrix_symbolic,gsl_matrix>(f,x,y);
    case EXPR::IMATRIX : 
      return matrix::matrix_zipwith<gsl_matrix_symbolic,gsl_matrix_int>(f,x,y);
    case EXPR::CMATRIX :
      return matrix::matrix_zipwith<gsl_matrix_symbolic,gsl_matrix_complex>(f,x,y);
    case EXPR::MATRIX :
      return matrix::matrix_zipwith<gsl_matrix_symbolic,gsl_matrix_symbolic>(f,x,y);
    default : return 0;
    }
  default : return 0;
  }
}

extern "C" 
pure_expr* matrix_zipwith3 ( pure_expr *f, pure_expr *x, pure_expr *y,
			     pure_expr *z )
{
  switch (x->tag) {
  case EXPR::DMATRIX :
    switch (y->tag) {
    case EXPR::DMATRIX : 
      switch (z->tag) {
      case EXPR::DMATRIX : 
	return matrix::matrix_zipwith3<gsl_matrix,gsl_matrix,gsl_matrix>(f,x,y,z);
      case EXPR::IMATRIX : 
	return matrix::matrix_zipwith3<gsl_matrix,gsl_matrix,gsl_matrix_int>(f,x,y,z);
      case EXPR::CMATRIX :
	return matrix::matrix_zipwith3<gsl_matrix,gsl_matrix,gsl_matrix_complex>(f,x,y,z);
      case EXPR::MATRIX :
	return matrix::matrix_zipwith3<gsl_matrix,gsl_matrix,gsl_matrix_symbolic>(f,x,y,z);
      default : return 0;
      }
    case EXPR::IMATRIX : 
      switch (z->tag) {
      case EXPR::DMATRIX : 
	return matrix::matrix_zipwith3<gsl_matrix,gsl_matrix_int,gsl_matrix>(f,x,y,z);
      case EXPR::IMATRIX : 
	return matrix::matrix_zipwith3<gsl_matrix,gsl_matrix_int,gsl_matrix_int>(f,x,y,z);
      case EXPR::CMATRIX :
	return matrix::matrix_zipwith3<gsl_matrix,gsl_matrix_int,gsl_matrix_complex>(f,x,y,z);
      case EXPR::MATRIX :
	return matrix::matrix_zipwith3<gsl_matrix,gsl_matrix_int,gsl_matrix_symbolic>(f,x,y,z);
      default : return 0;
      }
    case EXPR::CMATRIX :
      switch (z->tag) {
      case EXPR::DMATRIX : 
	return matrix::matrix_zipwith3<gsl_matrix,gsl_matrix_complex,gsl_matrix>(f,x,y,z);
      case EXPR::IMATRIX : 
	return matrix::matrix_zipwith3<gsl_matrix,gsl_matrix_complex,gsl_matrix_int>(f,x,y,z);
      case EXPR::CMATRIX :
	return matrix::matrix_zipwith3<gsl_matrix,gsl_matrix_complex,gsl_matrix_complex>(f,x,y,z);
      case EXPR::MATRIX :
	return matrix::matrix_zipwith3<gsl_matrix,gsl_matrix_complex,gsl_matrix_symbolic>(f,x,y,z);
      default : return 0;
      }
    case EXPR::MATRIX :
      switch (z->tag) {
      case EXPR::DMATRIX : 
	return matrix::matrix_zipwith3<gsl_matrix,gsl_matrix_symbolic,gsl_matrix>(f,x,y,z);
      case EXPR::IMATRIX : 
	return matrix::matrix_zipwith3<gsl_matrix,gsl_matrix_symbolic,gsl_matrix_int>(f,x,y,z);
      case EXPR::CMATRIX :
	return matrix::matrix_zipwith3<gsl_matrix,gsl_matrix_symbolic,gsl_matrix_complex>(f,x,y,z);
      case EXPR::MATRIX :
	return matrix::matrix_zipwith3<gsl_matrix,gsl_matrix_symbolic,gsl_matrix_symbolic>(f,x,y,z);
      default : return 0;
      }
    default : return 0;
    }
  case EXPR::IMATRIX :
    switch (y->tag) {
    case EXPR::DMATRIX : 
      switch (z->tag) {
      case EXPR::DMATRIX : 
	return matrix::matrix_zipwith3<gsl_matrix_int,gsl_matrix,gsl_matrix>(f,x,y,z);
      case EXPR::IMATRIX : 
	return matrix::matrix_zipwith3<gsl_matrix_int,gsl_matrix,gsl_matrix_int>(f,x,y,z);
      case EXPR::CMATRIX :
	return matrix::matrix_zipwith3<gsl_matrix_int,gsl_matrix,gsl_matrix_complex>(f,x,y,z);
      case EXPR::MATRIX :
	return matrix::matrix_zipwith3<gsl_matrix_int,gsl_matrix,gsl_matrix_symbolic>(f,x,y,z);
      default : return 0;
      }
    case EXPR::IMATRIX : 
      switch (z->tag) {
      case EXPR::DMATRIX : 
	return matrix::matrix_zipwith3<gsl_matrix_int,gsl_matrix_int,gsl_matrix>(f,x,y,z);
      case EXPR::IMATRIX : 
	return matrix::matrix_zipwith3<gsl_matrix_int,gsl_matrix_int,gsl_matrix_int>(f,x,y,z);
      case EXPR::CMATRIX :
	return matrix::matrix_zipwith3<gsl_matrix_int,gsl_matrix_int,gsl_matrix_complex>(f,x,y,z);
      case EXPR::MATRIX :
	return matrix::matrix_zipwith3<gsl_matrix_int,gsl_matrix_int,gsl_matrix_symbolic>(f,x,y,z);
      default : return 0;
      }
    case EXPR::CMATRIX :
      switch (z->tag) {
      case EXPR::DMATRIX : 
	return matrix::matrix_zipwith3<gsl_matrix_int,gsl_matrix_complex,gsl_matrix>(f,x,y,z);
      case EXPR::IMATRIX : 
	return matrix::matrix_zipwith3<gsl_matrix_int,gsl_matrix_complex,gsl_matrix_int>(f,x,y,z);
      case EXPR::CMATRIX :
	return matrix::matrix_zipwith3<gsl_matrix_int,gsl_matrix_complex,gsl_matrix_complex>(f,x,y,z);
      case EXPR::MATRIX :
	return matrix::matrix_zipwith3<gsl_matrix_int,gsl_matrix_complex,gsl_matrix_symbolic>(f,x,y,z);
      default : return 0;
      }
    case EXPR::MATRIX :
      switch (z->tag) {
      case EXPR::DMATRIX : 
	return matrix::matrix_zipwith3<gsl_matrix_int,gsl_matrix_symbolic,gsl_matrix>(f,x,y,z);
      case EXPR::IMATRIX : 
	return matrix::matrix_zipwith3<gsl_matrix_int,gsl_matrix_symbolic,gsl_matrix_int>(f,x,y,z);
      case EXPR::CMATRIX :
	return matrix::matrix_zipwith3<gsl_matrix_int,gsl_matrix_symbolic,gsl_matrix_complex>(f,x,y,z);
      case EXPR::MATRIX :
	return matrix::matrix_zipwith3<gsl_matrix_int,gsl_matrix_symbolic,gsl_matrix_symbolic>(f,x,y,z);
      default : return 0;
      }
    default : return 0;
    }
  case EXPR::CMATRIX :
    switch (y->tag) {
    case EXPR::DMATRIX : 
      switch (z->tag) {
      case EXPR::DMATRIX : 
	return matrix::matrix_zipwith3<gsl_matrix_complex,gsl_matrix,gsl_matrix>(f,x,y,z);
      case EXPR::IMATRIX : 
	return matrix::matrix_zipwith3<gsl_matrix_complex,gsl_matrix,gsl_matrix_int>(f,x,y,z);
      case EXPR::CMATRIX :
	return matrix::matrix_zipwith3<gsl_matrix_complex,gsl_matrix,gsl_matrix_complex>(f,x,y,z);
      case EXPR::MATRIX :
	return matrix::matrix_zipwith3<gsl_matrix_complex,gsl_matrix,gsl_matrix_symbolic>(f,x,y,z);
      default : return 0;
      }
    case EXPR::IMATRIX : 
      switch (z->tag) {
      case EXPR::DMATRIX : 
	return matrix::matrix_zipwith3<gsl_matrix_complex,gsl_matrix_int,gsl_matrix>(f,x,y,z);
      case EXPR::IMATRIX : 
	return matrix::matrix_zipwith3<gsl_matrix_complex,gsl_matrix_int,gsl_matrix_int>(f,x,y,z);
      case EXPR::CMATRIX :
	return matrix::matrix_zipwith3<gsl_matrix_complex,gsl_matrix_int,gsl_matrix_complex>(f,x,y,z);
      case EXPR::MATRIX :
	return matrix::matrix_zipwith3<gsl_matrix_complex,gsl_matrix_int,gsl_matrix_symbolic>(f,x,y,z);
      default : return 0;
      }
    case EXPR::CMATRIX :
      switch (z->tag) {
      case EXPR::DMATRIX : 
	return matrix::matrix_zipwith3<gsl_matrix_complex,gsl_matrix_complex,gsl_matrix>(f,x,y,z);
      case EXPR::IMATRIX : 
	return matrix::matrix_zipwith3<gsl_matrix_complex,gsl_matrix_complex,gsl_matrix_int>(f,x,y,z);
      case EXPR::CMATRIX :
	return matrix::matrix_zipwith3<gsl_matrix_complex,gsl_matrix_complex,gsl_matrix_complex>(f,x,y,z);
      case EXPR::MATRIX :
	return matrix::matrix_zipwith3<gsl_matrix_complex,gsl_matrix_complex,gsl_matrix_symbolic>(f,x,y,z);
      default : return 0;
      }
    case EXPR::MATRIX :
      switch (z->tag) {
      case EXPR::DMATRIX : 
	return matrix::matrix_zipwith3<gsl_matrix_complex,gsl_matrix_symbolic,gsl_matrix>(f,x,y,z);
      case EXPR::IMATRIX : 
	return matrix::matrix_zipwith3<gsl_matrix_complex,gsl_matrix_symbolic,gsl_matrix_int>(f,x,y,z);
      case EXPR::CMATRIX :
	return matrix::matrix_zipwith3<gsl_matrix_complex,gsl_matrix_symbolic,gsl_matrix_complex>(f,x,y,z);
      case EXPR::MATRIX :
	return matrix::matrix_zipwith3<gsl_matrix_complex,gsl_matrix_symbolic,gsl_matrix_symbolic>(f,x,y,z);
      default : return 0;
      }
    default : return 0;
    }
  case EXPR::MATRIX  :
    switch (y->tag) {
    case EXPR::DMATRIX : 
      switch (z->tag) {
      case EXPR::DMATRIX : 
	return matrix::matrix_zipwith3<gsl_matrix_symbolic,gsl_matrix,gsl_matrix>(f,x,y,z);
      case EXPR::IMATRIX : 
	return matrix::matrix_zipwith3<gsl_matrix_symbolic,gsl_matrix,gsl_matrix_int>(f,x,y,z);
      case EXPR::CMATRIX :
	return matrix::matrix_zipwith3<gsl_matrix_symbolic,gsl_matrix,gsl_matrix_complex>(f,x,y,z);
      case EXPR::MATRIX :
	return matrix::matrix_zipwith3<gsl_matrix_symbolic,gsl_matrix,gsl_matrix_symbolic>(f,x,y,z);
      default : return 0;
      }
    case EXPR::IMATRIX : 
      switch (z->tag) {
      case EXPR::DMATRIX : 
	return matrix::matrix_zipwith3<gsl_matrix_symbolic,gsl_matrix_int,gsl_matrix>(f,x,y,z);
      case EXPR::IMATRIX : 
	return matrix::matrix_zipwith3<gsl_matrix_symbolic,gsl_matrix_int,gsl_matrix_int>(f,x,y,z);
      case EXPR::CMATRIX :
	return matrix::matrix_zipwith3<gsl_matrix_symbolic,gsl_matrix_int,gsl_matrix_complex>(f,x,y,z);
      case EXPR::MATRIX :
	return matrix::matrix_zipwith3<gsl_matrix_symbolic,gsl_matrix_int,gsl_matrix_symbolic>(f,x,y,z);
      default : return 0;
      }
    case EXPR::CMATRIX :
      switch (z->tag) {
      case EXPR::DMATRIX : 
	return matrix::matrix_zipwith3<gsl_matrix_symbolic,gsl_matrix_complex,gsl_matrix>(f,x,y,z);
      case EXPR::IMATRIX : 
	return matrix::matrix_zipwith3<gsl_matrix_symbolic,gsl_matrix_complex,gsl_matrix_int>(f,x,y,z);
      case EXPR::CMATRIX :
	return matrix::matrix_zipwith3<gsl_matrix_symbolic,gsl_matrix_complex,gsl_matrix_complex>(f,x,y,z);
      case EXPR::MATRIX :
	return matrix::matrix_zipwith3<gsl_matrix_symbolic,gsl_matrix_complex,gsl_matrix_symbolic>(f,x,y,z);
      default : return 0;
      }
    case EXPR::MATRIX :
      switch (z->tag) {
      case EXPR::DMATRIX : 
	return matrix::matrix_zipwith3<gsl_matrix_symbolic,gsl_matrix_symbolic,gsl_matrix>(f,x,y,z);
      case EXPR::IMATRIX : 
	return matrix::matrix_zipwith3<gsl_matrix_symbolic,gsl_matrix_symbolic,gsl_matrix_int>(f,x,y,z);
      case EXPR::CMATRIX :
	return matrix::matrix_zipwith3<gsl_matrix_symbolic,gsl_matrix_symbolic,gsl_matrix_complex>(f,x,y,z);
      case EXPR::MATRIX :
	return matrix::matrix_zipwith3<gsl_matrix_symbolic,gsl_matrix_symbolic,gsl_matrix_symbolic>(f,x,y,z);
      default : return 0;
      }
    default : return 0;
    }
  default : return 0;
  }
}

extern "C" 
pure_expr* matrix_scanl ( pure_expr *f, pure_expr *z, pure_expr *x )
{
  switch (x->tag) {
  case EXPR::DMATRIX :
    return matrix::matrix_scanl<gsl_matrix>(f,z,x);
  case EXPR::IMATRIX :
    return matrix::matrix_scanl<gsl_matrix_int>(f,z,x);
  case EXPR::CMATRIX :
    return matrix::matrix_scanl<gsl_matrix_complex>(f,z,x);
  case EXPR::MATRIX  :
    return matrix::matrix_scanl<gsl_matrix_symbolic>(f,z,x);
  default : return 0;
  }
}

extern "C" 
pure_expr* matrix_scanl1 ( pure_expr *f, pure_expr *x )
{
  switch (x->tag) {
  case EXPR::DMATRIX :
    return matrix::matrix_scanl1<gsl_matrix>(f,x);
  case EXPR::IMATRIX :
    return matrix::matrix_scanl1<gsl_matrix_int>(f,x);
  case EXPR::CMATRIX :
    return matrix::matrix_scanl1<gsl_matrix_complex>(f,x);
  case EXPR::MATRIX  :
    return matrix::matrix_scanl1<gsl_matrix_symbolic>(f,x);
  default : return 0;
  }
}

extern "C" 
pure_expr* matrix_scanr ( pure_expr *f, pure_expr *z, pure_expr *x )
{
  switch (x->tag) {
  case EXPR::DMATRIX :
    return matrix::matrix_scanr<gsl_matrix>(f,z,x);
  case EXPR::IMATRIX :
    return matrix::matrix_scanr<gsl_matrix_int>(f,z,x);
  case EXPR::CMATRIX :
    return matrix::matrix_scanr<gsl_matrix_complex>(f,z,x);
  case EXPR::MATRIX  :
    return matrix::matrix_scanr<gsl_matrix_symbolic>(f,z,x);
  default : return 0;
  }
}

extern "C" 
pure_expr* matrix_scanr1 ( pure_expr *f, pure_expr *x )
{
  switch (x->tag) {
  case EXPR::DMATRIX :
    return matrix::matrix_scanr1<gsl_matrix>(f,x);
  case EXPR::IMATRIX :
    return matrix::matrix_scanr1<gsl_matrix_int>(f,x);
  case EXPR::CMATRIX :
    return matrix::matrix_scanr1<gsl_matrix_complex>(f,x);
  case EXPR::MATRIX  :
    return matrix::matrix_scanr1<gsl_matrix_symbolic>(f,x);
  default : return 0;
  }
}

extern "C" 
pure_expr* matrix_foldl ( pure_expr *f, pure_expr *z, pure_expr *x )
{
  switch (x->tag) {
  case EXPR::DMATRIX :
    return matrix::matrix_foldl<gsl_matrix>(f,z,x);
  case EXPR::IMATRIX :
    return matrix::matrix_foldl<gsl_matrix_int>(f,z,x);
  case EXPR::CMATRIX :
    return matrix::matrix_foldl<gsl_matrix_complex>(f,z,x);
  case EXPR::MATRIX  :
    return matrix::matrix_foldl<gsl_matrix_symbolic>(f,z,x);
  default : return 0;
  }
}

extern "C" 
pure_expr* matrix_foldl1 ( pure_expr *f, pure_expr *x )
{
  switch (x->tag) {
  case EXPR::DMATRIX :
    return matrix::matrix_foldl1<gsl_matrix>(f,x);
  case EXPR::IMATRIX :
    return matrix::matrix_foldl1<gsl_matrix_int>(f,x);
  case EXPR::CMATRIX :
    return matrix::matrix_foldl1<gsl_matrix_complex>(f,x);
  case EXPR::MATRIX  :
    return matrix::matrix_foldl1<gsl_matrix_symbolic>(f,x);
  default : return 0;
  }
}

extern "C" 
pure_expr* matrix_foldr ( pure_expr *f, pure_expr *z, pure_expr *x )
{
  switch (x->tag) {
  case EXPR::DMATRIX :
    return matrix::matrix_foldr<gsl_matrix>(f,z,x);
  case EXPR::IMATRIX :
    return matrix::matrix_foldr<gsl_matrix_int>(f,z,x);
  case EXPR::CMATRIX :
    return matrix::matrix_foldr<gsl_matrix_complex>(f,z,x);
  case EXPR::MATRIX  :
    return matrix::matrix_foldr<gsl_matrix_symbolic>(f,z,x);
  default : return 0;
  }
}

extern "C" 
pure_expr* matrix_foldr1 ( pure_expr *f, pure_expr *x )
{
  switch (x->tag) {
  case EXPR::DMATRIX :
    return matrix::matrix_foldr1<gsl_matrix>(f,x);
  case EXPR::IMATRIX :
    return matrix::matrix_foldr1<gsl_matrix_int>(f,x);
  case EXPR::CMATRIX :
    return matrix::matrix_foldr1<gsl_matrix_complex>(f,x);
  case EXPR::MATRIX  :
    return matrix::matrix_foldr1<gsl_matrix_symbolic>(f,x);
  default : return 0;
  }
}

extern "C"
pure_expr* matrix_filter ( pure_expr *p, pure_expr *x )
{
  switch ( x->tag ) {
  case EXPR::DMATRIX : 
    return pure_double_matrix( matrix::matrix_filter<gsl_matrix>(p,x) ); 
  case EXPR::IMATRIX : 
    return pure_int_matrix( matrix::matrix_filter<gsl_matrix_int>(p,x) ); 
  case EXPR::CMATRIX : 
    return pure_complex_matrix
      ( matrix::matrix_filter<gsl_matrix_complex>(p,x) ); 
  case EXPR::MATRIX : 
    return pure_symbolic_matrix
      ( matrix::matrix_filter<gsl_matrix_symbolic>(p,x) ); 
  default : return 0;
  }
}

extern "C"
pure_expr* matrix_dropwhile ( pure_expr *p, pure_expr *x )
{
  switch ( x->tag ) {
  case EXPR::DMATRIX : 
    return pure_double_matrix( matrix::matrix_dropwhile<gsl_matrix>(p,x) ); 
  case EXPR::IMATRIX : 
    return pure_int_matrix( matrix::matrix_dropwhile<gsl_matrix_int>(p,x) ); 
  case EXPR::CMATRIX : 
    return pure_complex_matrix
      ( matrix::matrix_dropwhile<gsl_matrix_complex>(p,x) ); 
  case EXPR::MATRIX : 
    return pure_symbolic_matrix
      ( matrix::matrix_dropwhile<gsl_matrix_symbolic>(p,x) ); 
  default : return 0;
  }
}

extern "C"
pure_expr* matrix_takewhile ( pure_expr *p, pure_expr *x )
{
  switch ( x->tag ) {
  case EXPR::DMATRIX : 
    return pure_double_matrix( matrix::matrix_takewhile<gsl_matrix>(p,x) ); 
  case EXPR::IMATRIX : 
    return pure_int_matrix( matrix::matrix_takewhile<gsl_matrix_int>(p,x) ); 
  case EXPR::CMATRIX : 
    return pure_complex_matrix
      ( matrix::matrix_takewhile<gsl_matrix_complex>(p,x) ); 
  case EXPR::MATRIX : 
    return pure_symbolic_matrix
      ( matrix::matrix_takewhile<gsl_matrix_symbolic>(p,x) ); 
  default : return 0;
  }
}

extern "C"
pure_expr* matrix_all ( pure_expr *p, pure_expr *x )
{
  switch ( x->tag ) {
  case EXPR::DMATRIX : 
    return pure_int( matrix::matrix_all<gsl_matrix>(p,x) ); 
  case EXPR::IMATRIX : 
    return pure_int( matrix::matrix_all<gsl_matrix_int>(p,x) ); 
  case EXPR::CMATRIX : 
    return pure_int( matrix::matrix_all<gsl_matrix_complex>(p,x) ); 
  case EXPR::MATRIX : 
    return pure_int( matrix::matrix_all<gsl_matrix_symbolic>(p,x) ); 
  default : return 0;
  }
}

extern "C"
pure_expr* matrix_any ( pure_expr *p, pure_expr *x )
{
  switch ( x->tag ) {
  case EXPR::DMATRIX : 
    return pure_int( matrix::matrix_any<gsl_matrix>(p,x) ); 
  case EXPR::IMATRIX : 
    return pure_int( matrix::matrix_any<gsl_matrix_int>(p,x) ); 
  case EXPR::CMATRIX : 
    return pure_int( matrix::matrix_any<gsl_matrix_complex>(p,x) ); 
  case EXPR::MATRIX : 
    return pure_int( matrix::matrix_any<gsl_matrix_symbolic>(p,x) ); 
  default : return 0;
  }
}
