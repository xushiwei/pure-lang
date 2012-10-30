
/* Copyright (c) 1996-2009 by John W. Eaton.
   Copyright (c) 2003 by Paul Kienzle.
   Copyright (c) 2010-2012 by Albert Graef.

   This file is part of pure-octave.

   pure-octave is free software: you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the Free
   Software Foundation, either version 3 of the License, or (at your option)
   any later version.

   pure-octave is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.

   Please see the accompanying COPYING file for the precise license terms. The
   GPL are also be read online at http://www.gnu.org/licenses/. */

#include "octave/config.h"

#include <iostream>
#include "octave.h"
#include "symtab.h"
#include "parse.h"
#if 0
#include "unwind-prot.h"
#endif
#include "toplev.h"
#include "error.h" 
#include "quit.h"
#include "variables.h"
#include "sighandlers.h"
#include "sysdep.h"
#include "str-vec.h"

#ifdef _WIN32
#include <malloc.h>
#endif

#include "embed.h"

/* Basic Octave interface. This is a heavily hacked version of octave_embed by
   Paul Kienzle (http://wiki.octave.org/wiki.pl?OctaveEmbedded) which in turn
   is based on Octave's toplevel. */

static bool init = false, first_init = false;

static void install_builtins();

void octave_init(int argc, char *argv[])
{
  if (!init) {
    octave_main(argc,argv,1);
    init = true;
    if (!first_init) {
      atexit(octave_fini);
      first_init = true;
      install_builtins();
    }
  }
}

void octave_fini(void)
{
  if (init) {
    do_octave_atexit();
    init = false;
  }
}

static void recover(void)
{
#if 0
  // This isn't supported in the latest Octave versions. We simply leave this
  // disabled for now, which means that you'll have to use 'unwind_protect'
  // explicitly in your Octave code in order to handle Octave exceptions.
  // XXXFIXME: This might leak memory in some cases??
  unwind_protect::run_all ();
#endif
  can_interrupt = true;
  octave_interrupt_immediately = 0;
  octave_interrupt_state = 0;
  octave_signal_caught = 0;
  octave_exception_state = octave_no_exception;
  octave_restore_signal_mask ();
  octave_catch_interrupts ();
}

int octave_eval(const char *cmd)
{
  int parse_status;

  octave_save_signal_mask ();
  if (octave_set_current_context)
    {
#if defined (USE_EXCEPTIONS_FOR_INTERRUPTS)
      panic_impossible ();
#else
#if 0
      unwind_protect::run_all ();
#endif
      raw_mode (0);
      std::cout << "\n";
      octave_restore_signal_mask ();
#endif
    }

  can_interrupt = true;
  octave_catch_interrupts ();
  octave_initialized = true;

  // XXX FIXME XXX need to give caller an opaque pointer
  // so that they can define and use separate namespaces
  // when calling octave, with a shared global namespace.
  // Something like:
  //   int call_octave (const char *string, void *psymtab = NULL) {
  //     ...
  //     curr_sym_tab = psymtab == NULL ? top_level_sym_tab : symbol_table;
  // I suppose to be safe from callbacks (surely we have to
  // provide some way to call back from embedded octave into
  // the user's application), we should push and pop the current
  // symbol table.

  // Note that I'm trying to distinguish exception from 
  // failure in the return codes. I believe failure is 
  // indicated by -1.  I have execution exception (including
  // user interrupt and more dramatic failures) returning -2
  // and memory failure returning -3.  We should formalize
  // this with error codes defined in embed_octave.h.  Maybe
  // a more fine-grained approach could be used within octave
  // proper.
  try 
    {
#if 0
      curr_sym_tab = top_level_sym_tab;
#endif
      reset_error_handler ();
      eval_string(cmd, false, parse_status, 0);
    }
  catch (octave_interrupt_exception)
    {
      recover ();
      std::cout << "\n"; 
      error_state = -2; 
    }
  catch (std::bad_alloc)
    {
      recover ();
      std::cout << "\n"; 
      error_state = -3;
    }

  octave_restore_signal_mask();
  octave_initialized = false;

  // XXX FIXME XXX callbacks calling embed_octave
  // may or may not want error_state reset.
  return error_state;
}

/* Basic GSL-compatible matrix interface from the Pure runtime. This is used
   to marshall Pure matrix values from/to Octave matrices. */

#include "gsl_structs.h"

static gsl_matrix* 
gsl_matrix_alloc(const size_t n1, const size_t n2)
{
  gsl_block* block;
  gsl_matrix* m;
  if (n1 == 0 || n2 == 0)
    return 0;
  m = (gsl_matrix*)malloc(sizeof(gsl_matrix));
  if (m == 0)
    return 0;
  block = (gsl_block*)malloc(sizeof(gsl_block));
  if (block == 0) {
    free(m);
    return 0;
  }
  block->size = n1*n2;
  block->data = (double*)malloc(block->size*sizeof(double));
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

static gsl_matrix*
gsl_matrix_calloc(const size_t n1, const size_t n2)
{
  gsl_matrix* m = gsl_matrix_alloc(n1, n2);
  if (m == 0) return 0;
  memset(m->data, 0, m->block->size*sizeof(double));
  return m;
}

#if 0
static void gsl_matrix_free(gsl_matrix *m)
{
  if (m->owner) {
    if (m->block) free(m->block->data);
    free(m->block);
  }
  free(m);
}
#endif

static gsl_matrix_complex* 
gsl_matrix_complex_alloc(const size_t n1, const size_t n2)
{
  gsl_block_complex* block;
  gsl_matrix_complex* m;
  if (n1 == 0 || n2 == 0)
    return 0;
  m = (gsl_matrix_complex*)malloc(sizeof(gsl_matrix_complex));
  if (m == 0)
    return 0;
  block = (gsl_block_complex*)malloc(sizeof(gsl_block_complex));
  if (block == 0) {
    free(m);
    return 0;
  }
  block->size = n1*n2;
  block->data = (double*)malloc(2*block->size*sizeof(double));
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

static gsl_matrix_complex*
gsl_matrix_complex_calloc(const size_t n1, const size_t n2)
{
  gsl_matrix_complex* m = gsl_matrix_complex_alloc(n1, n2);
  if (m == 0) return 0;
  memset(m->data, 0, 2*m->block->size*sizeof(double));
  return m;
}

#if 0
static void gsl_matrix_complex_free(gsl_matrix_complex *m)
{
  if (m->owner) {
    if (m->block) free(m->block->data);
    free(m->block);
  }
  free(m);
}
#endif

static gsl_matrix_int* 
gsl_matrix_int_alloc(const size_t n1, const size_t n2)
{
  gsl_block_int* block;
  gsl_matrix_int* m;
  if (n1 == 0 || n2 == 0)
    return 0;
  m = (gsl_matrix_int*)malloc(sizeof(gsl_matrix_int));
  if (m == 0)
    return 0;
  block = (gsl_block_int*)malloc(sizeof(gsl_block_int));
  if (block == 0) {
    free(m);
    return 0;
  }
  block->size = n1*n2;
  block->data = (int*)malloc(block->size*sizeof(int));
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

static gsl_matrix_int*
gsl_matrix_int_calloc(const size_t n1, const size_t n2)
{
  gsl_matrix_int* m = gsl_matrix_int_alloc(n1, n2);
  if (m == 0) return 0;
  memset(m->data, 0, m->block->size*sizeof(int));
  return m;
}

#if 0
static void gsl_matrix_int_free(gsl_matrix_int *m)
{
  if (m->owner) {
    if (m->block) free(m->block->data);
    free(m->block);
  }
  free(m);
}
#endif

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

/* Marshalling between Pure and Octave values. We support all of Octave's
   basic data types, i.e., scalars and matrices of boolean, integer, double,
   complex and char data. Anything else is just passed through unchanged as a
   cooked octave_value pointer which frees itself automatically. (This is
   useful, in particular, for Octave function values which may also be used
   as the function argument of octave_call, see below.)

   Note that all types of Octave boolean and integer matrices are mapped to
   (machine) int matrices in Pure land; when converted back to Octave, these
   become Octave int32 matrices. Also note that in the Octave interface scalar
   values are indistinguishable from 1x1 matrices, so when converting from
   Octave to Pure, we always assume that a 1x1 matrix denotes a scalar.

   In the C++ interface, there's no marshalling of Octave's more advanced data
   types (n-dimensional matrices, cell arrays and structures), so these will
   show as opaque pointers in Pure land. However, converting these to Pure's
   symbolic matrices and records is supported through the appropriate Pure
   function hooks (__pure2oct__ and __oct2pure__), see octave.pure for
   details. */

static inline pure_expr *octave_pointer(const octave_value& val)
{
  octave_value *v = new octave_value(val);
  pure_expr *x = pure_pointer(v),
    *y = pure_symbol(pure_sym("octave_free"));
  return pure_sentry(y, x);
}

static inline bool is_octave_pointer(pure_expr *x, octave_value **v)
{
  pure_expr *f;
  return pure_is_pointer(x, (void**)v) && (f = pure_get_sentry(x)) &&
    f->tag > 0 && strcmp(pure_sym_pname(f->tag), "octave_free") == 0;
}

bool octave_valuep(pure_expr *x)
{
  return is_octave_pointer(x, 0);
}

static bool recursive = false, converters_enabled = true;

bool octave_converters(bool enable)
{
  bool save = converters_enabled;
  converters_enabled = enable;
  return save;
}

static pure_expr *try_octave_to_pure(const octave_value& val)
{
  pure_expr *x = octave_pointer(val);
  if (!x || !converters_enabled || recursive) return x;
  pure_new(x);
  int32_t fno = pure_sym("__oct2pure__");
  pure_expr *f = pure_symbol(fno);
  if (!f) return x;
  recursive = true;
  pure_expr *e = 0, *y = pure_appx(f, x, &e), *u, *v;
  recursive = false;
  if (!y) {
    // exception, return the pointer object as is
    if (e) pure_freenew(e); pure_unref(x);
    return x;
  } else if (pure_is_app(y, &u, &v) && u->tag == fno && v == x) {
    // conversion failed, return the pointer object as is
    pure_freenew(y); pure_unref(x);
    return x;
  } else {
    pure_new(y); pure_free(x);
    return y;
  }
}

static pure_expr *octave_to_pure(const octave_value& val)
{
  if (val.is_defined()) {
    int n = val.ndims();
    /* We only support Octave's basic scalar and matrix values here, and these
       are all represented as matrices with two dimensions. Anything else is
       handled through the appropriate hooks provided by Pure code, or becomes
       an Octave pointer object by default. */
    if (n != 2) return try_octave_to_pure(val);
    dim_vector dim = val.dims();
    size_t k = dim(0), l = dim(1);
    if (val.is_double_type()) {
      if (val.is_complex_type()) {
	if (k == 0 || l == 0) {
	  // Empty matrix.
	  return pure_complex_matrix(create_complex_matrix(k, l));
	}
	ComplexMatrix m = val.complex_matrix_value();
	std::complex<double> *v = m.fortran_vec();
	if (!v) return 0;
	if (k == 1 && l == 1) {
	  // Scalar. FIXME: There seems to be no way to distinguish these from
	  // 1x1 matrices in Octave!?
	  double c[2] = { v[0].real(), v[0].imag() };
	  return pure_complex(c);
	}
	gsl_matrix_complex *mat = create_complex_matrix(k, l);
	for (size_t i = 0; i < k; i++)
	  for (size_t j = 0; j < l; j++) {
	    size_t p = 2*(i*mat->tda+j);
	    mat->data[p]   = v[j*k+i].real();
	    mat->data[p+1] = v[j*k+i].imag();
	  }
	return pure_complex_matrix(mat);
      } else {
	if (k == 0 || l == 0) {
	  // Empty matrix.
	  return pure_double_matrix(create_double_matrix(k, l));
	}
	Matrix m = val.matrix_value();
	double *v = m.fortran_vec();
	if (!v) return 0;
	if (k == 1 && l == 1) {
	  // Scalar.
	  return pure_double(v[0]);
	}
	gsl_matrix *mat = create_double_matrix(k, l);
	for (size_t i = 0; i < k; i++)
	  for (size_t j = 0; j < l; j++)
	    mat->data[i*mat->tda+j] = v[j*k+i];
	return pure_double_matrix(mat);
      }
    } else if (val.is_bool_type()) {
      if (k == 0 || l == 0) {
	// Empty matrix.
	return pure_int_matrix(create_int_matrix(k, l));
      }
      boolMatrix m = val.bool_matrix_value();
      bool *v = m.fortran_vec();
      if (!v) return 0;
      if (k == 1 && l == 1) {
	// Scalar.
	return pure_int(v[0]);
      }
      gsl_matrix_int *mat = create_int_matrix(k, l);
      for (size_t i = 0; i < k; i++)
	for (size_t j = 0; j < l; j++)
	  mat->data[i*mat->tda+j] = v[j*k+i];
      return pure_int_matrix(mat);
    } else if (val.is_integer_type()) {
      if (val.is_int32_type()) {
	if (k == 0 || l == 0) {
	  // Empty matrix.
	  return pure_int_matrix(create_int_matrix(k, l));
	}
	int32NDArray m = val.int32_array_value();
	octave_int32 *v = m.fortran_vec();
	if (!v) return 0;
	if (k == 1 && l == 1) {
	  // Scalar.
	  return pure_int((int32_t)v[0]);
	}
	gsl_matrix_int *mat = create_int_matrix(k, l);
	for (size_t i = 0; i < k; i++)
	  for (size_t j = 0; j < l; j++)
	    mat->data[i*mat->tda+j] = (int32_t)v[j*k+i];
	return pure_int_matrix(mat);
      } else if (val.is_uint32_type()) {
	if (k == 0 || l == 0) {
	  // Empty matrix.
	  return pure_int_matrix(create_int_matrix(k, l));
	}
	uint32NDArray m = val.uint32_array_value();
	octave_uint32 *v = m.fortran_vec();
	if (!v) return 0;
	if (k == 1 && l == 1) {
	  // Scalar.
	  return pure_int((uint32_t)v[0]);
	}
	gsl_matrix_int *mat = create_int_matrix(k, l);
	for (size_t i = 0; i < k; i++)
	  for (size_t j = 0; j < l; j++)
	    mat->data[i*mat->tda+j] = (uint32_t)v[j*k+i];
	return pure_int_matrix(mat);
      } else if (val.is_int16_type()) {
	if (k == 0 || l == 0) {
	  // Empty matrix.
	  return pure_int_matrix(create_int_matrix(k, l));
	}
	int16NDArray m = val.int16_array_value();
	octave_int16 *v = m.fortran_vec();
	if (!v) return 0;
	if (k == 1 && l == 1) {
	  // Scalar.
	  return pure_int((int16_t)v[0]);
	}
	gsl_matrix_int *mat = create_int_matrix(k, l);
	for (size_t i = 0; i < k; i++)
	  for (size_t j = 0; j < l; j++)
	    mat->data[i*mat->tda+j] = (int16_t)v[j*k+i];
	return pure_int_matrix(mat);
      } else if (val.is_uint16_type()) {
	if (k == 0 || l == 0) {
	  // Empty matrix.
	  return pure_int_matrix(create_int_matrix(k, l));
	}
	uint16NDArray m = val.uint16_array_value();
	octave_uint16 *v = m.fortran_vec();
	if (!v) return 0;
	if (k == 1 && l == 1) {
	  // Scalar.
	  return pure_int((uint16_t)v[0]);
	}
	gsl_matrix_int *mat = create_int_matrix(k, l);
	for (size_t i = 0; i < k; i++)
	  for (size_t j = 0; j < l; j++)
	    mat->data[i*mat->tda+j] = (uint16_t)v[j*k+i];
	return pure_int_matrix(mat);
      } else if (val.is_int8_type()) {
	if (k == 0 || l == 0) {
	  // Empty matrix.
	  return pure_int_matrix(create_int_matrix(k, l));
	}
	int8NDArray m = val.int8_array_value();
	octave_int8 *v = m.fortran_vec();
	if (!v) return 0;
	if (k == 1 && l == 1) {
	  // Scalar.
	  return pure_int((int8_t)v[0]);
	}
	gsl_matrix_int *mat = create_int_matrix(k, l);
	for (size_t i = 0; i < k; i++)
	  for (size_t j = 0; j < l; j++)
	    mat->data[i*mat->tda+j] = (int8_t)v[j*k+i];
	return pure_int_matrix(mat);
      } else if (val.is_uint8_type()) {
	if (k == 0 || l == 0) {
	  // Empty matrix.
	  return pure_int_matrix(create_int_matrix(k, l));
	}
	uint8NDArray m = val.uint8_array_value();
	octave_uint8 *v = m.fortran_vec();
	if (!v) return 0;
	if (k == 1 && l == 1) {
	  // Scalar.
	  return pure_int((uint8_t)v[0]);
	}
	gsl_matrix_int *mat = create_int_matrix(k, l);
	for (size_t i = 0; i < k; i++)
	  for (size_t j = 0; j < l; j++)
	    mat->data[i*mat->tda+j] = (uint8_t)v[j*k+i];
	return pure_int_matrix(mat);
      } else if (val.is_int64_type()) {
	/* The 64 bit types are simply cast to 32 bit right now, we might want
	   to do something more clever in the future. */
	if (k == 0 || l == 0) {
	  // Empty matrix.
	  return pure_int_matrix(create_int_matrix(k, l));
	}
	int64NDArray m = val.int64_array_value();
	octave_int64 *v = m.fortran_vec();
	if (!v) return 0;
	if (k == 1 && l == 1) {
	  // Scalar.
	  return pure_int((int64_t)v[0]);
	}
	gsl_matrix_int *mat = create_int_matrix(k, l);
	for (size_t i = 0; i < k; i++)
	  for (size_t j = 0; j < l; j++)
	    mat->data[i*mat->tda+j] = (int64_t)v[j*k+i];
	return pure_int_matrix(mat);
      } else if (val.is_uint64_type()) {
	if (k == 0 || l == 0) {
	  // Empty matrix.
	  return pure_int_matrix(create_int_matrix(k, l));
	}
	uint64NDArray m = val.uint64_array_value();
	octave_uint64 *v = m.fortran_vec();
	if (!v) return 0;
	if (k == 1 && l == 1) {
	  // Scalar.
	  return pure_int((uint64_t)v[0]);
	}
	gsl_matrix_int *mat = create_int_matrix(k, l);
	for (size_t i = 0; i < k; i++)
	  for (size_t j = 0; j < l; j++)
	    mat->data[i*mat->tda+j] = (uint64_t)v[j*k+i];
	return pure_int_matrix(mat);
      } else
	return try_octave_to_pure(val);
    } else if (val.is_string()) {
      charMatrix s = val.char_matrix_value();
      int n = s.rows();
      if (n==0) return pure_matrix_rowsl(0);
      if (n==1) {
	std::string tmp = s.row_as_string(0);
	return pure_cstring_dup(tmp.c_str());
      }
      pure_expr **xv = (pure_expr**)malloc(n*sizeof(pure_expr*));
      if (!xv) return 0;
      for (int i = 0; i < n; i++) {
	std::string tmp = s.row_as_string(i);
	xv[i] = pure_cstring_dup(tmp.c_str());
      }
      pure_expr *ret = pure_matrix_rowsv(n, xv);
      free(xv);
      return ret;
    } else if (val.is_cs_list()) {
      // "Comma-separated" value (argument) lists, translated to Pure tuples.
      octave_value_list l = val.list_value();
      octave_idx_type n = l.length();
      if (n==0) return pure_tuplel(0);
      pure_expr **xv = (pure_expr**)malloc(n*sizeof(pure_expr*));
      if (!xv) return 0;
      for (octave_idx_type i = 0; i < n; i++) {
	xv[i] = octave_to_pure(l(i));
	if (!xv[i]) {
	  for (octave_idx_type j = 0; j < i; j++)
	    pure_freenew(xv[j]);
	  free(xv);
	  return 0;
	}
      }
      pure_expr *ret = pure_tuplev(n, xv);
      free(xv);
      return ret;
    } else {
      return try_octave_to_pure(val);
    }
  } else
    return 0;
}

static octave_value *pure_to_octave(pure_expr *x);
static octave_value *try_pure_to_octave(pure_expr *x)
{
  if (!x || !converters_enabled || recursive) return 0;
  int32_t fno = pure_sym("__pure2oct__");
  pure_expr *f = pure_symbol(fno);
  if (!f) return 0;
  recursive = true;
  pure_expr *e = 0, *y = pure_appx(f, x, &e), *a, *b;
  recursive = false;
  octave_value *v;
  if (!y) {
    // exception
    if (e) pure_freenew(e);
    return 0;
  } else if (is_octave_pointer(y, &v)) {
    // conversion to Octave pointer value, take as is
    octave_value *ret = new octave_value(*v);
    pure_freenew(y);
    return ret;
  } else if (pure_is_app(y, &a, &b) && a->tag == fno && b == x) {
    // conversion failed
    pure_freenew(y);
    return 0;
  } else {
    // we got another Pure value, recursively try the built-in conversions on
    // that
    recursive = true;
    octave_value *ret = pure_to_octave(y);
    recursive = false;
    pure_freenew(y);
    return ret;
  }
}

static octave_value *pure_to_octave(pure_expr *x)
{
  double d;
  int i;
  char *s;
  void *p;
  octave_value *v;
  if (pure_is_double(x, &d)) {
    return new octave_value(d);
  } else if (pure_is_int(x, &i)) {
    return new octave_value(octave_int32(i));
  } else if (pure_is_double_matrix(x, &p)) {
    gsl_matrix *mat = (gsl_matrix*)p;
    size_t k = mat->size1, l = mat->size2;
    Matrix m(k, l);
    double *v = m.fortran_vec();
    for (size_t i = 0; i < k; i++)
      for (size_t j = 0; j < l; j++)
	v[j*k+i] = mat->data[i*mat->tda+j];
    return new octave_value(m);
  } else if (pure_is_int_matrix(x, &p)) {
    gsl_matrix_int *mat = (gsl_matrix_int*)p;
    size_t k = mat->size1, l = mat->size2;
    dim_vector dv(k, l);
    int32NDArray m(dv);
    octave_int32 *v = m.fortran_vec();
    for (size_t i = 0; i < k; i++)
      for (size_t j = 0; j < l; j++)
	v[j*k+i] = octave_int32(mat->data[i*mat->tda+j]);
    return new octave_value(m);
  } else if (pure_is_complex_matrix(x, &p)) {
    gsl_matrix_complex *mat = (gsl_matrix_complex*)p;
    size_t k = mat->size1, l = mat->size2;
    ComplexMatrix m(k, l);
    std::complex<double> *v = m.fortran_vec();
    for (size_t i = 0; i < k; i++)
      for (size_t j = 0; j < l; j++) {
	size_t p = 2*(i*mat->tda+j);
	v[j*k+i].real() = mat->data[p];
	v[j*k+i].imag() = mat->data[p+1];
      }
    return new octave_value(m);
  } else if (pure_is_cstring_dup(x, &s)) {
    charMatrix m(s);
    free(s);
    return new octave_value(m);
  } else if (pure_is_symbolic_matrix(x, &p)) {
    gsl_matrix_symbolic *mat = (gsl_matrix_symbolic*)p;
    size_t k = mat->size1, l = mat->size2;
    if (k==0 || l!=1) return try_pure_to_octave(x);
    string_vector v(k);
    for (size_t i = 0; i < k; i++)
      if (pure_is_cstring_dup(mat->data[i*mat->tda], &s)) {
	v[i] = s; free(s);
      } else
	return try_pure_to_octave(x);
    charMatrix m(v);
    return new octave_value(m);
  } else if (is_octave_pointer(x, &v)) {
    return new octave_value(*v);
  } else
    // Try the appropriate hooks provided by Pure code.
    return try_pure_to_octave(x);
}

/* Get and set global Octave variables. Note that in order to access global
   variables in Octave code executed with octave_eval, the variables *must*
   first be declared 'global' in Octave, e.g.: octave_eval "global ans". */

pure_expr *octave_get(const char *id)
{
  octave_value val = get_global_value(id, true);
  return octave_to_pure(val);
}

pure_expr *octave_set(const char *id, pure_expr *x)
{
  octave_value *val = pure_to_octave(x);
  if (val) {
    set_global_value(id, *val);
    delete val;
    return x;
  } else
    return 0;
}

/* Create an Octave function value, either from its name or from its inline
   function description. In the former case the argument must be a valid
   Octave function name (as accepted by Octave's str2func function). In the
   latter case the argument must be an inline description (as accepted by
   Octave's inline function), and the parameter names may optionally be given
   along with the description as a tuple of strings. */

pure_expr *octave_func(pure_expr *fun)
{
  // First try to find an ordinary function handle.
  char *s;
  if (pure_is_cstring_dup(fun, &s)) {
    octave_value f = symbol_table::find_function(s);
    free(s);
    if (f.is_defined()) {
      pure_expr *f = pure_string_dup("str2func"),
	*ret = octave_call(f, 1, fun);
      pure_freenew(f);
      return ret;
    }
  }
  // Try an inline function.
  pure_expr *f = pure_string_dup("inline"),
    *ret = octave_call(f, 1, fun);
  pure_freenew(f);
  return ret;
}

/* Call an Octave function.

   - fun is the name of the function (as a string) or an Octave function value
     (as returned, e.g., by octave_func);

   - nargout denotes the desired number of return values;

   - args is a tuple with the argument values. */

pure_expr *octave_call(pure_expr *fun, int nargout, pure_expr *args)
{
  pure_expr **xs;
  size_t n;
  octave_value *v;
  octave_function *f = 0;
  char *s = 0;
  if (is_octave_pointer(fun, &v)) {
    if (v->is_function_handle() || v->is_inline_function())
      f = v->function_value();
    else
      return 0;
  } else if (!pure_is_cstring_dup(fun, &s))
    return 0;
  if (nargout < 0) return 0;
  if (pure_is_tuplev(args, &n, &xs)) {
    octave_value_list args, ret;
    for (size_t i = 0; i < n; i++) {
      octave_value *val = pure_to_octave(xs[i]);
      if (!val) {
	free(xs);
	return 0;
      }
      args(i) = *val;
      delete val;
    }
    free(xs);
    reset_error_handler ();
    if (f)
      ret = feval(f, args, nargout);
    else
      ret = feval(s, args, nargout);
    if (s) free(s);
    n = ret.length();
    if (n > (size_t)nargout) n = nargout;
    xs = (pure_expr**)alloca(n*sizeof(pure_expr*));
    for (size_t i = 0; i < n; i++) {
      xs[i] = octave_to_pure(ret(i));
      if (!xs[i]) {
	for (size_t j = 0; j<i; j++)
	  pure_freenew(xs[j]);
	return 0;
      }
    }
    return pure_tuplev(n, xs);
  } else
    return 0;
}

void octave_free(void *val)
{
  octave_value *v = (octave_value*)val;
  delete v;
}

/* Add a builtin to call Pure from Octave. */

#include "oct.h"

#define PURE_HELP "\
  RES = pure_call(NAME, ARG, ...)\n\
  [RES, ...] = pure_call(NAME, ARG, ...)\n\
\n\
  Execute the Pure function named NAME (a string) with the given arguments.\n\
  The Pure function may return multiple results as a tuple. Example:\n\
  pure_call('succ', 99) => 100.\n"

DEFUN_DLD(pure_call, args, nargout, PURE_HELP)
{
  int nargin = args.length();
  octave_value_list retval;
  if (nargin < 1)
    print_usage();
  else {
    charMatrix ch = args(0).char_matrix_value();
    octave_idx_type nr = ch.rows();
    if (nr != 1)
      print_usage();
    else if (!error_state) {
      std::string name = ch.row_as_string(0);
      int fno = pure_getsym(name.c_str());
      if (fno <= 0) {
	error("pure: unknown function name '%s'", name.c_str());
      } else {
	pure_expr *ret, *e = 0;
	ret = pure_symbolx(fno, &e);
	if (ret && nargin > 1) {
	  pure_expr **xs = (pure_expr**)alloca((nargin-1)*sizeof(pure_expr*));
	  for (int i = 0; i < nargin-1; i++) {
	    xs[i] = octave_to_pure(args(i+1));
	    if (!xs[i]) {
	      for (int j = 0; j<i; j++)
		pure_freenew(xs[j]);
	      error("pure: invalid argument #%d in call to function '%s'",
		    i+1, name.c_str());
	      return retval;
	    }
	  }
	  ret = pure_appxv(ret, nargin-1, xs, &e);
	}
	if (ret) {
	  size_t n;
	  pure_expr **xs;
	  pure_is_tuplev(ret, &n, &xs); // this never fails
	  if (nargout > 0 && n != (size_t)nargout) {
	    error("pure: wrong number of return values in call to function '%s' \n(expected %d, got %d)", name.c_str(), nargout, (int)n);
	  } else {
	    for (size_t i = 0; i < n; i++) {
	      octave_value *val = pure_to_octave(xs[i]);
	      if (!val) {
		free(xs);
		if (n != 1)
		  error("pure: invalid return value #%d in call to function '%s'",
			i+1, name.c_str());
		else
		  error("pure: invalid return value in call to function '%s'",
			name.c_str());
		return octave_value_list();
	      }
	      retval(i) = *val;
	      delete val;
	    }
	  }
	  free(xs);
	  pure_freenew(ret);
	} else {
	  if (e) pure_freenew(e);
	  error("pure: exception in call to function '%s'", name.c_str());
	}
      }
    }
  }
  return retval;
}

static void install_builtins()
{
  install_builtin_function(Fpure_call, "pure_call", PURE_HELP);
}
