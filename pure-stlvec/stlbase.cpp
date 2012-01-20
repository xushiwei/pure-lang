/* stl.cpp -- Support for stl containers of pure_expr*
    
Copyright (c) 2011 by Peter Summerland <p.summerland@gmail.com>.

All rights reserved.

This software is is part of pure-stlvec, an addon to the Pure Programming
Language (http://code.google.com/p/pure-lang/).

This software is distributed under a BSD-style license in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the COPYING file
included with the pure-stlvec distribution package for details.

*/

#include "stlbase.hpp"
#include <iostream>
#include <stdlib.h>

using namespace std;

/**** Helpers for pure_expr *******************************************/

ostream& operator<<(ostream& os, px* x){
  char *s = str(x);
  os << s;
  free(s);
  return os;
}

static bool px_trace_enabled = false;

void stl_set_px_trace(bool enable)
{
  px_trace_enabled = enable;
}

bool stl_px_trace_enabled()
{
  return px_trace_enabled;
}

#ifdef STL_DEBUG
px* px_new(px* x)
{
  void* ptr = x;
  if (px_trace_enabled)
    cerr << "[px_new(" << x << ") --> ";
  pure_new(x);
  if (px_trace_enabled)
    cerr << x->refc << "]"  << endl;
  return x;
}

void px_free(px* x)
{
  void* ptr = x;
  if (x->refc <= 0) {
    cerr << "[tried to free px with refc le 0\n";
    cerr << ptr << "]\n";
    bad_argument();
  }
  if (px_trace_enabled)
    cerr << "[px_free(" << x << ") --> ";
  if (x->refc > 1) {
    pure_free(x);
    if (px_trace_enabled)
      cerr << x->refc << "]" << endl;
  }
  else {
    if (px_trace_enabled)
      cerr << 0 << "]" << endl;
    pure_free(x);    
  }
}

void px_freenew(px* x)
{
  void* ptr = x;
  if (px_trace_enabled) {
    cerr << "[px_freenew(" << x << ") with refc = ";
    cerr << x->refc <<  "]" << endl;
  }
  pure_freenew(x);
}

void px_ref(px* x)
{
  void* ptr = x;
  if (px_trace_enabled)
    cerr << "[px_ref(" << x << ") --> ";
  pure_ref(x);
  if (px_trace_enabled)
    cerr << x->refc << "]"  << endl;
}

void px_unref(px* x)
{
  void* ptr = x;
  if (x->refc <= 0) {
    cerr << "[tried to unref px with refc le 0\n";
    cerr << ptr << "]\n";
    bad_argument();
  }
  if (px_trace_enabled)
    cerr << "[px_unref(" << x << ") --> ";
  if (x->refc > 1) {
    pure_unref(x);
    if (px_trace_enabled)
      cerr << x->refc << "]" << endl;
  }
  else {
    if (px_trace_enabled)
      cerr << 0 << "]" << endl;
    pure_unref(x);    
  }
}

#endif


/**** Handles to hold pure_expr* ****************************************/

px_handle::px_handle() : pxp_(px_new(pure_pointer(0))){}

px_handle& px_handle::operator=(const px_handle& rhs)
{
  if (&rhs!=this){
    px_new(rhs.pxp_);
    px_free(pxp_);
    pxp_ = rhs.pxp_;
  }
  return *this;
}

ostream& operator<<(ostream& os, const px_handle& pxh)
{
  os << pxh.pxp();
  return os;
}

/*** Function objects to lift px* functions to pxh functions ***/

pxh_fun& pxh_fun::operator=(const pxh_fun& rhs)
{
  if (&rhs!=this){
    px_new(rhs.fun_);
    px_free(fun_);
    fun_ = rhs.fun_;
  }
  return *this;
}

pxh pxh_fun1::operator()(const pxh& arg) const
{
  px* exception = 0;
  px* ret =  pure_appxl(fun_, &exception, 1, arg.pxp());
  if (exception) throw exception;
  if (!ret) bad_function();
  return pxh(ret);
}

size_t pxh_hash::operator()(const pxh& arg) const
{
  px* exception = 0;
  px* pxres =  pure_appxl(fun_, &exception, 1, arg.pxp());
  if (exception) throw exception;
  if (!pxres) bad_function();
  int hv;
  if ( !pure_is_int(pxres, &hv) ) bad_function();
  px_freenew(pxres);
  return static_cast<size_t>(hv);
}

pxh pxh_fun2::operator()(const pxh& arg1, const pxh& arg2) const
{
  px* exception = 0;
  px* ret =  pure_appxl(fun_, &exception, 2, arg1.pxp(), arg2.pxp());
  if (exception) throw exception;
  if (!ret) bad_function();
  return pxh(ret);
}

bool pxh_pred1::operator()(const pxh& arg) const
{
  px* exception = 0;
  int32_t ret;
  px* pxres = pure_appxl(fun_, &exception, 1, arg.pxp());
  if (exception) throw exception;
  if (!pxres) bad_function();
  int ok = pure_is_int(pxres, &ret);
  px_freenew(pxres);
  if (!ok) failed_cond();
  return ok && ret;
}

bool pxh_pred2::operator()(const pxh& left, const pxh& right) const
{
  int32_t ret;
  px* exception = 0;
  px* pxres = pure_appxl(fun_, &exception, 2, left.pxp(), right.pxp());
  if (exception) throw exception;
  if (!pxres) bad_function();
  int ok = pure_is_int(pxres, &ret);
  px_freenew(pxres);
  if (!ok) failed_cond();
  return ok && ret;
}

bool pxhpair_less::operator()(const pxhpair& left,
                              const pxhpair& right) const
{
  try {
    const pxh& lf = left.first;
    const pxh& rf = right.first;
    if (first_less(lf,rf)) return 1;
    if (first_less(rf,lf)) return 0;
    return (second_less(left.second, right.second));
  }
  catch (px* e) {
    pure_throw(e);
  }
}

bool pxhpair_equal::operator()(const pxhpair& lhs,
                               const pxhpair& rhs) const
{
  try {
    bool ok = first_equal(lhs.first, rhs.first)  && 
              second_equal(lhs.second, rhs.second);
    return ok;
  }
  catch (px* e) {
    pure_throw(e);
  }
}

bool pxhpair_first_equal::operator()(const pxhpair& lhs,
                                     const pxhpair& rhs) const
{
  try {
    return first_equal(lhs.first, rhs.first);
  }
  catch (px* e) {
    pure_throw(e);
  }
}

bool pxhpair_first_equivalent::operator()(const pxhpair& lhs,
                                          const pxhpair& rhs) const
{
  try {
    bool ok = !first_less(lhs.first, rhs.first) && 
      !first_less(rhs.first, lhs.first);
    return ok;
  }
  catch (px* e) {
    pure_throw(e);
  }
}

bool pxhpair_equivalent::operator()(const pxhpair& lhs,
                                    const pxhpair& rhs) const
{
  try {
    bool ok = !first_less(lhs.first, rhs.first) && 
              !first_less(rhs.first, lhs.first) && 
              second_equal(lhs.second, rhs.second);
    return ok;
  }
  catch (px* e) {
    pure_throw(e);
  }
}

pxh pxh_gen::operator()()
{
  px* exception = 0;
  px* ret =  pure_appxl(fun_, &exception, 1, pure_tuplel(0));
  if (exception) throw exception;
  if (!ret) bad_function();
  return pxh(ret);
}

extern "C"
int stl_refc(pure_expr *x){
  return x->refc - 1; // want refc before pass into here
}

/**** Interpreter Local Storage Global Symbols **************************/

static px* px_newsym(const char *name)
{
  return pure_new(pure_symbol(pure_sym(name)));
}

px* px_cons_sym()
{
  static ILS<px*> _sym = NULL; px* &sym = _sym();
  if (!sym) sym = px_newsym(":");
  return sym;
}

px* px_null_list_sym()
{
  static ILS<px*> _sym = NULL; px* &sym = _sym();
  if (!sym) sym = px_newsym("[]");
  return sym;
}

px* px_rocket_sym()
{
  static ILS<px*> _sym = NULL; px* &sym = _sym();
  if (!sym) sym = px_newsym("=>");
  return sym;
}

px* px_bad_function_sym()
{
  static ILS<px*> _sym = NULL; px* &sym = _sym();
  if (!sym) sym = px_newsym("bad_function");
  return sym;
}

px* px_out_of_bounds_sym()
{
  static ILS<px*> _sym = NULL; px* &sym = _sym();
  if (!sym) sym = px_newsym("out_of_bounds");
  return sym;
}

px* px_range_overflow_sym()
{
  static ILS<px*> _sym = NULL; px* &sym = _sym();
  if (!sym) sym = px_newsym(":");
  return sym;
}

px* px_range_overlap_sym()
{
  static ILS<px*> _sym = NULL; px* &sym = _sym();
  if (!sym) sym = px_newsym("range_overlap");
  return sym;
}

px* px_bad_argument_sym()
{
  static ILS<px*> _sym = NULL; px* &sym = _sym();
  if (!sym) sym = px_newsym("bad_argument");
  return sym;
}

px* px_failed_cond_sym()
{
  static ILS<px*> _sym = NULL; px* &sym = _sym();
  if (!sym) sym = px_newsym("failed_cond");
  return sym;
}

/**** Interpreter Local Storage STL Namespace Data *********************/

px* stlbegin_sym()
{
  static ILS<px*> _sym = NULL; px* &sym = _sym();
  if (!sym) sym = px_newsym("stl::smbeg");
  return sym;
}

px* stlend_sym()
{
  static ILS<px*> _sym = NULL; px* &sym = _sym();
  if (!sym) sym = px_newsym("stl::smend");
  return sym;
}

/*** Errors ********************************************************/

void bad_function() {pure_throw(px_bad_function_sym());}
void index_error() {pure_throw(px_out_of_bounds_sym());}
void range_overflow() {pure_throw(px_range_overflow_sym());}
void range_overlap() {pure_throw(px_range_overlap_sym());}
void bad_argument() {pure_throw(px_bad_argument_sym());}
void failed_cond() {pure_throw(px_failed_cond_sym());}

/*** Helpers ********************************************************/

bool pxrocket_to_pxlhs_pxrhs(px* pxr, px** lhs, px** rhs)
{
  px* app;
  size_t argc;
  px** args;
  bool ok = pure_is_appv(pxr, &app, &argc, &args) && argc == 2;
  if (ok) {
    *lhs = args[0];
    *rhs = args[1];
  }
  free(args);
  return ok;
}

px* pxlhs_pxrhs_to_pxrocket(const px* lhs, const px* rhs)
{
  px* rocket = px_rocket_sym();
  return pure_appl(rocket, 2, lhs, rhs); 
}

bool pxrocket_to_pxhpair(px* pxr, pxhpair& pair)
{
  px* lhs = 0;
  px* rhs = 0;
  bool ret = pxrocket_to_pxlhs_pxrhs(pxr, &lhs, &rhs);
  pair.first = lhs;
  pair.second = rhs;
  return ret;
}

px* pxhpair_to_pxrocket(const pxhpair& pair)
{
  return pxlhs_pxrhs_to_pxrocket(pair.first, pair.second);
}

px* pxhpair_to_pxlhs(const pxhpair& pair)
{
  return pair.first;
}

px* pxh_to_pxp(pxh h)
{
  return h.pxp();
}
