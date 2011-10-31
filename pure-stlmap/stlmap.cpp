/* stlmap.cpp -- C++ support for stlmap.pure

--- DRAFT - FOR DISCUSSON PURPOSES ONLY ---
    
Copyright (c) 2011 by Peter Summerland <p.summerland@gmail.com>.

All rights reserved.

This software is is part of pure-stlmap, an addon to the Pure Programming
Language (http://code.google.com/p/pure-lang/).

This software is distributed under a BSD-style license in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the COPYING file
included with the pure-stlmap distribution package for details.

*/

#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <numeric>
#include "stlmap.hpp"

using namespace std;

#define STL_INSERT_SEMANTICS

/*** Helpers for debugging only ************************************/

static bool sm_trace_enabled = false;

void stl_set_sm_trace(bool enable) 
{
  sm_trace_enabled = enable;
}

bool stl_sm_trace_enabled()
{
  return sm_trace_enabled;
}

/*** Helpers for stlmap.cpp only ************************************/

static void set_kv(pmi i, px** k, px** v)
{
  const pxh h_key = i->first;
  pxh h_val = i->second;
  *k = h_key.pxp();
  *v = h_val.pxp();
}

static int is_less(px* cmp, px* x, px* y) 
{
  int32_t lt;
  px* exception = 0;
  px* pxres = pure_appxl(cmp, &exception, 2, x, y);
  if (exception) pure_throw(exception);
  if (!pxres) bad_function();
  int ok = pure_is_int(pxres, &lt);
  px_freenew(pxres);
  if (!ok) failed_cond();
  return lt;
}

static pxh_pair sm_rocket_to_pair(pxh rp)
{
  px *k, *v;
  bool ok = rocket_to_pair(rp.pxp(), &k, &v);
  return pxh_pair(k,v);
}

static px* sm_pair_to_rocket(const pxh_pair& kv)
{
  return pair_to_rocket(kv.first.pxp(), kv.second.pxp());
}

static px* sm_pair_to_key(const pxh_pair& kv)
{
  return kv.first.pxp();
}

static int sm_get_size(sm* smp, pmi b, pmi e)
{
 size_t sz = 0;
 pxhmap& mp = smp->mp;
 if (b == mp.begin() && e == mp.end())
    sz = mp.size();
 else if (b == e)
   sz = 0;
 else
   while(b++ != e) sz++;
 return sz;
}

static void update_aux(sm* smp, px* k, px* v)
{
  pxhmap& mp = smp->mp;
  pmi i;
  if ( smp->get_cached_pmi(k, i) ) {
    i->second = v;
  }
  else {
    pair<pmi,bool> i_ok = mp.insert(pxh_pair(k,v));
    if (!i_ok.second)
      i_ok.first->second = v;
    smp->cache_pmi(i_ok.first);
  }
}

static bool insert_aux(sm* smp, px* kv)
{
  px *k, *v;
  bool ok = true;
  if (smp->keys_only) {
    k = kv;
    v = NULL;
  } 
  else {
    if ( rocket_to_pair(kv, &k, &v) )
      ;
    else if (smp->has_dflt) {
      k = kv;
      v = smp->dflt.pxp();
    }
    else {
      k = kv;
      v = NULL;
      ok = false;
    }
  }
  if (ok) {
#ifdef STL_INSERT_SEMANTICS
    // Do NOT override existing values
    pair<pmi,bool> i_ok = smp->mp.insert(pxh_pair(k,v));
    smp->cache_pmi(i_ok.first);
#else
    // Always use new values
    update_aux(smp, k, v); 
#endif
  }
  return ok;
}

/*** stlmap members  ***********************************************/

stlmap::stlmap(px* cmp, bool keyonly) : 
  mp(pxh_pred2(cmp)), px_comp(cmp), dflt(NULL),
  has_dflt(0), has_recent_pmi(0), keys_only(keyonly), 
  recent_pmi(mp.end()) {}

stlmap::stlmap(px* cmp, bool ko, px* d) :
    mp(pxh_pred2(cmp)), px_comp(cmp), dflt(d), 
    has_dflt(1), has_recent_pmi(0), keys_only(ko),
    recent_pmi(mp.end()) {}

pmi stlmap::find(px* key)
{
  pmi iter;
  if (key == smbeg())
    iter = mp.begin();
  else if (key == smend())
    iter = mp.end();
  else
    iter = mp.find(key);
  return iter;  
}

bool stlmap::get_cached_pmi(px* k, pmi& i)
{
  bool found = k != smbeg() && k != smend() &&
    has_recent_pmi && same(recent_pmi->first.pxp(), k);
  if (found) i = recent_pmi;
#ifdef STL_DEBUG
  if (found && stl_sm_trace_enabled())
    cerr <<"get_cached_pmi, found "<< k <<" with refc:"<< k->refc << endl;
#endif
  return found;
}

void stlmap::cache_pmi(pmi& i)
{
  if ( i != mp.end() ) {
    has_recent_pmi = true;
    recent_pmi = i;
  }
}

void stlmap::clear()
{
  has_recent_pmi = 0;
  mp.clear();
}

void stlmap::erase(pmi pos)
{
  if (has_recent_pmi && recent_pmi == pos)
    has_recent_pmi = 0;
  mp.erase(pos);
}

int stlmap::erase(px* k)
{
  int ret = 1;
  if ( !mp.empty() ) {
    pmi i;
    if ( get_cached_pmi(k, i) )
      erase(i);
    else {
      if (k == smbeg())
        erase(mp.begin());
      else if (k != smend()) {
        mp.erase(k);
      }
      else
        ret = 0;
    }
  }
  return ret;
}

void stlmap::erase(pmi first, pmi last)
{
  has_recent_pmi = false;
  mp.erase(first, last);
}

/*** sm_iters functions *********************************************/

enum {gi_find, gi_lower, gi_upper};

static pmi get_iter(pxhmap& mp , px* key, int mode)
{
  pmi iter;
  if (key == smbeg())
    iter = mp.begin();
  else if (key == smend())
    iter = mp.end();
  else {
    if (mode==gi_upper) 
      iter = mp.upper_bound(key);
    else if (mode=gi_lower)
      iter = mp.lower_bound(key);
    else 
      iter = mp.find(key);
  }
  return iter;  
}

static px* iter_to_key(const pxhmap& mp, const pmi& it)
{
  if (it == mp.end()) return smend();
  if (it == mp.begin()) return smbeg();
  return it->first.pxp();
}

static sm* get_sm_from_app(px* app)
{
  void* ret = 0;
  px* fun;
  size_t argc;
  px** args;
  pure_is_appv(app, &fun, &argc, &args);
  if (argc == 1 && !pure_is_pointer(args[0], &ret))
    ret = 0;
  free(args);
  return (sm*)ret; 
}

// sets two iterators [beg, end)
// if first >= last then use  mp.end(), mp.end()
sm_iters::sm_iters(px* pmi_tuple)
{
  size_t tpl_sz;
  px** elems;
  
  is_valid = true;
  pure_is_tuplev(pmi_tuple, &tpl_sz, &elems);
  smp = get_sm_from_app(elems[0]);
  pxhmap &mp = smp->mp;
  int num_iters = tpl_sz-1;
  if (num_iters != 0 && num_iters != 2) {
      is_valid = false;
      return;
  }
  if (num_iters == 0 || mp.size() == 0) {
    begin_it = mp.begin();
    end_it = mp.end();
    free(elems);
  }
  else {
    px* b = elems[1];
    px* e = elems[2];
    px* comp = smp->px_comp.pxp();    
    free(elems);
    begin_it = get_iter(mp, b, gi_lower);
    end_it = get_iter(mp,e,gi_upper);
    if (begin_it == mp.end()) {
      end_it = begin_it;
      return;
    }
    pmi prec_it = end_it;
    if ( prec_it != mp.begin() && e != smend() ) {
      prec_it--;
      px* pkey = prec_it->first.pxp();
      if ( !is_less(comp, pkey, e) ) end_it = prec_it;
    }    
    if (end_it != mp.end()) { 
      px* bkey = begin_it->first.pxp();
      px* ekey = end_it->first.pxp();
      if ( !is_less(comp, bkey, ekey) )
        begin_it = end_it = mp.end();
    }
  }
}

/*** Functions for map<pxh,pxh,pxh_pred2> *******************************/

sm* sm_make_empty(px* comp, int keys_only)
{
  sm* ret  = new sm(comp, keys_only); 
#ifdef STL_DEBUG
  if (stl_sm_trace_enabled())
    cerr << "TRACE SM:    new sm*: " << ret << endl;
#endif
  return ret;
}

void sm_delete(sm* p){
#ifdef STL_DEBUG
  if (stl_sm_trace_enabled())
    cerr << "TRACE SM: delete sm*: " << p << endl;
#endif
  delete(p);
}

bool sm_is_set(px* tpl)
{
  sm_iters itrs(tpl);
  if (!itrs.is_valid) bad_argument();
  return itrs.smp->keys_only;
}

px* sm_make_vector(px* tpl) 
{
  sm_iters itrs(tpl);
  if (!itrs.is_valid) bad_argument();
  pmi b = itrs.beg();
  pmi e = itrs.end();
  int sz = sm_get_size(itrs.smp, b, e);
  if (!sz)
    return pure_matrix_columnsv(0,NULL);
  px** bfr = (px**)malloc(sizeof(px*)*sz);
  if (itrs.smp->keys_only) 
    transform(b, e, bfr, sm_pair_to_key);
  else
    transform(b, e, bfr, sm_pair_to_rocket);
  px* ret = pure_matrix_columnsv(sz, bfr);
  free(bfr);
  return ret;
}

sv* sm_make_stlvec(px* tpl) 
{
  sm_iters itrs(tpl);
  if (!itrs.is_valid) bad_argument();
  pmi b = itrs.beg();
  pmi e = itrs.end();
  sv* ret = new sv;
  if (itrs.smp->keys_only) 
    transform(b, e, back_inserter(*ret), sm_pair_to_key);
  else
    transform(b, e, back_inserter(*ret), sm_pair_to_rocket);
  return ret;
}

px* sm_set_default(sm* smp, px* val)
{
  if (smp->keys_only) return 0; // fail
  smp->dflt = val;
  smp->has_dflt = 1;
  return val;
}

px* sm_get_default(sm* smp)
{
  if (smp->keys_only) return 0; // fail
  int ok = 1;
  px* val;
  if (smp->has_dflt) 
    val = smp->dflt.pxp();  
  else {
    val = NULL;
    ok = 0;
  }
  return pure_tuplel(2, pure_int(ok), val);
}

int sm_size(px* tpl)
{
  sm_iters itrs(tpl);
  if (!itrs.is_valid) bad_argument();
  return sm_get_size(itrs.smp, itrs.beg(), itrs.end());
}

px* sm_bounds(px* tpl)
{
  px** elems;
  px *ub, *lb;
  size_t tpl_sz;
  pure_is_tuplev(tpl, &tpl_sz, &elems);
  sm* smp = get_sm_from_app(elems[0]);
  pxhmap &mp = smp->mp;
  int num_iters = tpl_sz-1;
  switch (tpl_sz) {
  case 1:
    lb = smbeg();
    ub = smend();
    break;
  case 2:
    lb = iter_to_key( mp, get_iter(mp, elems[1], gi_lower) );
    ub = iter_to_key( mp, get_iter(mp, elems[1], gi_upper) );
    break;
  case 3:
    lb = iter_to_key( mp, get_iter(mp, elems[1], gi_lower) ); 
    ub = iter_to_key( mp, get_iter(mp, elems[2], gi_upper) );     
    break;
  }
  free(elems);
  return pure_tuplel(2,lb,ub);
}

int sm_member(sm* smp, px* key)
{
  int ret = 0;
  pxhmap& mp = smp->mp;
  pmi i;
  if (!mp.empty()) {
    if (smp->get_cached_pmi(key, i) ) {
      ret = 1;
    }
    else {
      i = smp->find(key);
      if (i != mp.end()) {
        smp->cache_pmi(i);
        ret = 1;
      }
    }
  }
  return ret;
}

px* sm_prev(sm* smp, px* key)
{
  pxhmap& mp = smp->mp;
  if (mp.empty()) index_error();
  pmi i = mp.end();
  if ( !smp->get_cached_pmi(key,i) )
    i = smp->find(key);
  if ( i == mp.begin() || i==mp.end() && key != smend())
    index_error();
  else
    i--;
  smp->cache_pmi(i);
  return iter_to_key(mp, i);
}

px* sm_next(sm* smp, px* key)
{
  pxhmap& mp = smp->mp;
  pmi i = mp.end();
  if (mp.empty()) index_error();
  if ( !smp->get_cached_pmi(key,i) )
    i = smp->find(key);
  if ( i == mp.end() )
    index_error();
  else
    i++;
  smp->cache_pmi(i);
  return iter_to_key(mp, i);
}

px* sm_get(sm* smp, px* key)
{
  pxhmap &mp = smp->mp; 
  px* ret = 0;
  pmi i;
  if ( !smp->get_cached_pmi(key, i) ) 
    i = get_iter(mp, key, gi_find);  
  if (i != mp.end()) {
    smp->cache_pmi(i);
    ret = smp->keys_only ? pure_int(1) : i->second.pxp();
  }
  else if (smp->keys_only) {
    ret =  pure_int(0);
  }
  else if (smp->has_dflt) {
    px* dv = smp->dflt.pxp();
    update_aux(smp, key, dv);
    ret = dv;
  } 
  else {
    index_error();
  }
  return ret;
}

bool sm_includes(px* tpl1, px* tpl2)
{
  sm_iters itrs1(tpl1);
  sm_iters itrs2(tpl2);
  if (!itrs1.is_valid || !itrs2.is_valid) bad_argument;
  try {
    return includes(itrs1.beg(), itrs1.end(), itrs2.beg(), itrs2.end(), 
                    itrs1.smp->mp.value_comp());
  }
  catch (px* e) {
    pure_throw(e);
  }
}

sm* sm_setop(int op, px* tpl1, px* tpl2)
{
  sm_iters itrs1(tpl1);
  sm_iters itrs2(tpl2);
  if (!itrs1.is_valid || !itrs2.is_valid) bad_argument;
  sm* smp = itrs1.smp;
  px* px_comp = smp->px_comp.pxp();
  sm* res = new sm(px_comp, smp->keys_only, smp->dflt.pxp());
  pxhmap& mp = res->mp;
  try {
    switch (op) {
    case stl_sm_union:
      set_union(itrs1.beg(), itrs1.end(), 
                itrs2.beg(), itrs2.end(),
                inserter(mp, mp.end()), mp.value_comp());
      break;
    case stl_sm_difference:
      set_difference(itrs1.beg(), itrs1.end(),
                     itrs2.beg(), itrs2.end(),
                     inserter(mp, mp.end()), mp.value_comp());
      break;
    case stl_sm_intersection:
      set_intersection(itrs1.beg(), itrs1.end(),
                       itrs2.beg(), itrs2.end(),
                       inserter(mp, mp.end()), mp.value_comp());
      break;
    case stl_sm_symmetric_difference:
      set_symmetric_difference(itrs1.beg(), itrs1.end(),
                               itrs2.beg(), itrs2.end(),
                               inserter(mp, mp.end()), mp.value_comp());
      break;
    otherwise:
      bad_argument();
    }
    return res;
  } catch (px* e) {
    pure_throw(e);
  }
}

px* sm_update(sm* smp, px* key, px* val)
{
  if (smp->keys_only) return 0; // fail for sets
  update_aux(smp, key, val);
  return val;
}

px* sm_update_with(sm* smp, px* key, px* unaryfun)
{
  if (smp->keys_only) return 0; // fail for sets
  if (!smp->has_dflt)
    failed_cond();
  px* old_val = sm_get(smp,key);
  px* exception = 0;
  px* new_val = pure_appxl(unaryfun, &exception, 1, old_val);
  if (exception) pure_throw(exception);
  if (!new_val) bad_function();
  update_aux(smp, key, new_val);
  return new_val;
}

px* sm_first(px* tpl)
{
  sm_iters itrs(tpl);
  if ( !itrs.is_valid ) bad_argument();
  pmi b = itrs.beg();
  if (b != itrs.end())
    return itrs.smp->keys_only ? sm_pair_to_key(*b) : sm_pair_to_rocket(*b);
  index_error();
}

px* sm_last(px* tpl)
{
  sm_iters itrs(tpl);
  if ( !itrs.is_valid ) bad_argument();
  pmi e = itrs.end();
  if (itrs.beg() != e--)
    return itrs.smp->keys_only ? sm_pair_to_key(*e) : sm_pair_to_rocket(*e);
  index_error();
}

void sm_rmfirst(px* tpl)
{
  sm_iters itrs(tpl);
  if (!itrs.is_valid) bad_argument();
  if ( itrs.beg() != itrs.end() )
    itrs.smp->erase(itrs.beg());
  else
    index_error();
}

void sm_rmlast(px* tpl)
{
  sm_iters itrs(tpl);
  if (!itrs.is_valid) bad_argument();
  if ( itrs.beg() != itrs.end() )
    itrs.smp->erase(--itrs.end());
  else
    index_error();
}

void sm_insert_elm(sm* smp, px* kv)
{
  if ( !insert_aux(smp, kv) ) bad_argument();
}

void sm_insert_elms_xs(sm* smp, px* src)
{
  pxhmap& mp = smp->mp;
  size_t sz = 0;
  px** elems = NULL;
  bool ok;
  if (pure_is_listv(src, &sz, &elems)) {
    for (int i = 0; i<sz; i++)
      if ( !insert_aux(smp, elems[i]) ) bad_argument();
    free(elems);
  } else if (matrix_type(src) == 0) {
    sz = matrix_size(src); 
    elems = (pure_expr**) pure_get_matrix_data(src);
    for (int i = 0; i<sz; i++) 
      if ( !insert_aux(smp, elems[i]) ) bad_argument();
  } 
}

void sm_insert_elms_stlmap(sm* smp, px* tpl)
{
  sm_iters itrs(tpl);
  if (itrs.beg() != itrs.end()) {
    pmi inserted = smp->mp.begin();
    if (!itrs.is_valid) bad_argument();
    for (pmi i = itrs.beg(); i!=itrs.end(); i++)
      inserted = smp->mp.insert(inserted, *i);
    smp->cache_pmi(inserted);
  } 
}

void sm_insert_elms_stlvec(sm* smp, px* tpl)
{
  sv_iters itrs(tpl);
  if (!itrs.is_valid || itrs.num_iters != 2) bad_argument();
  pxhmap& mp = smp->mp;
  for (svi i = itrs.beg(); i!=itrs.end(); i++)
    if ( !insert_aux(smp, i->pxp()) ) bad_argument();
}

void sm_erase(px* tpl)
{
  sm_iters itrs(tpl);
  if (!itrs.is_valid) bad_argument();
  itrs.smp->erase(itrs.beg(), itrs.end());
}

void sm_clear(sm* smp)
{
  smp->clear();
}

int sm_remove(sm* smp, px* k)
{
  return smp->erase(k);
}

int  sm_remove_if(sm* smp, px* x, px* pred)
{
  return 0;
}

bool sm_allpairs(px* comp, px* tpl1, px* tpl2)
{
  bool all_ok;
  sm_iters itrs1(tpl1);
  sm_iters itrs2(tpl2);
  if (!itrs1.is_valid) bad_argument();
  if (!itrs2.is_valid) bad_argument();

  sm* map1 = itrs1.smp;
  sm* map2 = itrs1.smp;
  try {
    if (map1->keys_only) {
      if (!map2->keys_only) bad_argument();
      pxh_pred2 fun(comp);
      pmi i1 = itrs1.beg();
      pmi i2 = itrs2.beg();
      pmi end1 = itrs1.end();
      pmi end2  =itrs2.end();
      all_ok = 1;
      while(i1 != end1) {
        if (i2 == end2  || !fun(i1->first, i2->first) ) {
          all_ok = 0;
          break;
        }
        i1++; i2 ++;
      }
      if (all_ok && i2 != end2) 
        all_ok = 0;
    }
    else {
      if (map2->keys_only) bad_argument();
      if ( sm_size(tpl1) != sm_size(tpl2) ) {
        all_ok = 0;
      }
      else {
        pxh_pair_pred2 fun(comp);
        all_ok = equal(itrs1.beg(), itrs1.end(), itrs2.beg(), fun);
      }
    }
  } catch (px* e) {
    pure_throw(e);
  }
  return all_ok;
}

px* apply_fun(px* fun, int what, pmi i, px** exception) {
  px *key, *val, *pxi, *res = 0;
  int bfr;
  set_kv(i, &key, &val);
  if (what == stl_sm_both) {
    pxi = pair_to_rocket(key, val);  // bump k, v refc
    px_new(pxi);
   }
  else if (what == stl_sm_key)
    pxi = key; 
  else
    pxi = val;
  *exception = 0;
  if (!pure_is_int(fun, &bfr)) {
    res = pure_appxl(fun, exception, 1, pxi);
  }
  else {
    res = pxi;
  }
  if (what==stl_sm_both)
    px_unref(pxi);  
  return res;
}

// if fun is any int then it is treated as identity (i.e., ignored)
px* sm_listmap(px* fun, px* tpl, int what)
{
  sm_iters itrs(tpl);
  if (!itrs.is_valid) bad_argument();
  if (itrs.smp->keys_only) what = stl_sm_key;
  pmi b = itrs.beg(); 
  pmi e = itrs.end();
  bool xx = b == e;
  px* cons = pure_const(cons_tag());
  px* nl = pure_const(null_list_tag());
  px* res = nl;
  px* y = 0;
  px* exception;
  for (pmi i = b; i != e; i++){
    px* pxi = apply_fun(fun, what, i, &exception);
    if (exception) {
      if (res) px_freenew(res);
      if (pxi) px_freenew(pxi);
      pure_throw(exception);
    }
    px* last = pure_app(pure_app(cons,pxi),nl);
    if (res==nl)
      res = y = last;
    else {
      y->data.x[1] = px_new(last);
      y = last;
    }
  }
  return res;  
}

px* sm_listcatmap(px* fun, px* tpl, int what)
{
  sm_iters itrs(tpl);
  if (!itrs.is_valid) bad_argument();
  if (itrs.smp->keys_only) what = stl_sm_key;
  pmi b = itrs.beg(); 
  pmi e = itrs.end(); 
  px* cons = pure_const(cons_tag());
  px* nl = pure_const(null_list_tag());
  px* res = nl;
  px* y = 0;
  px* exception;
  px* *elms;
  size_t sz;
  for (pmi i = b; i != e; i++){
    px* pxi = apply_fun(fun, what, i, &exception);
    if (exception) {
      if (res) px_freenew(res);
      if (pxi) px_freenew(pxi);
      pure_throw(exception);
    }
    if ( !pure_is_listv(pxi, &sz, &elms) ){
      px_freenew(pxi);
      if (res) px_freenew(res);
      bad_argument();      
    }
    for (int j = 0; j < sz; j++) {
      px* last = pure_app(pure_app(cons,elms[j]),nl);
      if (res==nl)
        res = y = last;    
      else {
        y->data.x[1] = px_new(last);
        y = last;
      }
    }
    px_freenew(pxi);
    free(elms);
  }
  return res;  
}

static px* sm_foldl_itrs(px* fun, px* val, pmi beg, pmi end, int mode)
{ 
  px* res = px_new(val);
  px* exception = 0;
  for (pmi i = beg; i != end; i++){
    px* fun_v = pure_appl(fun, 1, res);
    px* fxy = apply_fun(fun_v, mode, i, &exception);
    if (exception) {
      px_unref(res);
      throw exception;
    }
    px_new(fxy);
    px_free(res);
    res = fxy;
  }
  px_unref(res);
  return res;
}

px* sm_foldl(px* fun, px* val, px* tpl)
{
  sm_iters itrs(tpl);
  if (!itrs.is_valid) bad_argument();
  int mode =  itrs.smp->keys_only ? stl_sm_key : stl_sm_both;
  try {
    return sm_foldl_itrs(fun, val, itrs.beg(), itrs.end(), mode);
  } catch (px* e) {
    pure_throw(e);
  }
}

px* sm_foldl1(px* fun, px* tpl)
{
  sm_iters itrs(tpl);
  if ( !itrs.is_valid ) bad_argument();
  int mode =  itrs.smp->keys_only ? stl_sm_key : stl_sm_both;
  if (itrs.beg() == itrs.end() ) return 0;
  pmi b = itrs.beg();
  px* val;
  if (mode == stl_sm_key)
    val = b->first.pxp();
  else
    val = pair_to_rocket(b->first.pxp(), b->second.pxp());
  try {
    return sm_foldl_itrs(fun, val, ++b, itrs.end(), mode);
  } catch (px* e) {
    pure_throw(e);
  }
}

static px* sm_foldr_itrs(px* fun, px* val, pmi beg, pmi end, int mode)
{ 
  px* res = px_new(val);
  px* exception = 0;
  pmi i = end;
  while (i-- != beg) {
    px* fun_v = apply_fun(fun, mode, i, &exception);
    px_ref(fun_v);
    px* fxy = pure_appxl(fun_v, &exception, 1, res);
    if (exception) {
      px_unref(res);
      throw exception;
    }
    px_unref(fun_v);
    px_new(fxy);
    px_free(res);
    res = fxy;
  }
  px_unref(res);
  return res;
}

px* sm_foldr(px* fun, px* val, px* tpl)
{
  sm_iters itrs(tpl);
  if (!itrs.is_valid) bad_argument();
  int mode = itrs.smp->keys_only ? stl_sm_key : stl_sm_both;
  try {
    return sm_foldr_itrs(fun, val, itrs.beg(), itrs.end(), mode);
  } catch (px* x) {
    pure_throw(x);
  }
}

px* sm_foldr1(px* fun, px* tpl)
{
  sm_iters itrs(tpl);
  if ( !itrs.is_valid ) bad_argument();
  int mode =  itrs.smp->keys_only ? stl_sm_key : stl_sm_both;
  if ( itrs.beg() == itrs.end() ) return 0;
  pmi e = --itrs.end();
  px* val;
  if (mode == stl_sm_key)
    val = e->first.pxp();
  else
    val = pair_to_rocket(e->first.pxp(), e->second.pxp());
  try {
    return sm_foldr_itrs(fun, val, itrs.beg(), e, mode);
  } catch (px* x) {
    pure_throw(x);
  }
}
