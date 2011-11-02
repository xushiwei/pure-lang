/* stlmmap.cpp -- C++ support for stlmmap.pure

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
#include "stlmmap.hpp"

using namespace std;

/*** Helpers for debugging only ************************************/

static bool smm_trace_enabled = false;

void stl_set_smm_trace(bool enable) 
{
  smm_trace_enabled = enable;
}

bool stl_smm_trace_enabled()
{
  return smm_trace_enabled;
}

/*** Helpers for stlmmap.cpp only ************************************/

static void set_kv(pmmi i, px** k, px** v)
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

static pxh_pair smm_rocket_to_pair(pxh rp)
{
  px *k, *v;
  bool ok = rocket_to_pair(rp.pxp(), &k, &v);
  return pxh_pair(k,v);
}

static px* smm_pair_to_rocket(const pxh_pair& kv)
{
  return pair_to_rocket(kv.first.pxp(), kv.second.pxp());
}

static px* smm_pair_to_key(const pxh_pair& kv)
{
  return kv.first.pxp();
}

static int smm_get_size(smm* smmp, pmmi b, pmmi e)
{
 size_t sz = 0;
 pxhmmap& mp = smmp->mp;
 if (b == mp.begin() && e == mp.end())
    sz = mp.size();
 else if (b == e)
   sz = 0;
 else
   while(b++ != e) sz++;
 return sz;
}

static void update_aux(smm* smmp, px* k, px* v)
{
  pxhmmap& mp = smmp->mp;
  pmmi i;
  if ( smmp->get_cached_pmmi(k, i) )
    i->second = v;
  else
    smmp->cache_pmmi( mp.insert(pxh_pair(k,v)) );
}

static px* apply_fun(px* fun, int what, pmmi i, px** exception) {
  px *key, *val, *pxi, *res = 0;
  int bfr;
  set_kv(i, &key, &val);
  if (what == stl_smm_both) {
    pxi = pair_to_rocket(key, val);  // bump k, v refc
    px_new(pxi);
  }
  else if (what == stl_smm_key)
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
  if (what==stl_smm_both)
    px_unref(pxi);  
  return res;
}

static bool insert_aux(smm* smmp, px* kv)
{
  px *k, *v;
  bool ok = true;
  if (smmp->keys_only) {
    k = kv;
    v = NULL;
  } 
  else {
    if ( rocket_to_pair(kv, &k, &v) )
      ;
    else if (smmp->has_dflt) {
      k = kv;
      v = smmp->dflt.pxp();
    }
    else {
      k = kv;
      v = NULL;
      ok = false;
    }
  }
  if (ok)
    smmp->cache_pmmi( smmp->mp.insert(pxh_pair(k,v)) );
  return ok;
}

// if fun is any int then it is treated as identity (i.e., ignored)
static px* listmap_aux(px* fun, pmmi b, pmmi e, int what) 
{
  bool xx = b == e;
  px* cons = pure_const(cons_tag());
  px* nl = pure_const(null_list_tag());
  px* res = nl;
  px* y = 0;
  px* exception;
  for (pmmi i = b; i != e; i++){
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

/*** stlmmap members  ***********************************************/

stlmmap::stlmmap(px* cmp, bool keyonly) : 
  mp(pxh_pred2(cmp)), px_comp(cmp), dflt(NULL),
  has_dflt(0), has_recent_pmmi(0), keys_only(keyonly), 
  recent_pmmi(mp.end()) {}

stlmmap::stlmmap(px* cmp, bool ko, px* d) :
    mp(pxh_pred2(cmp)), px_comp(cmp), dflt(d), 
    has_dflt(1), has_recent_pmmi(0), keys_only(ko),
    recent_pmmi(mp.end()) {}

pmmi stlmmap::find(px* key)
{
  pmmi iter;
  if (key == smmbeg())
    iter = mp.begin();
  else if (key == smmend())
    iter = mp.end();
  else
    iter = mp.find(key);
  return iter;  
}

bool stlmmap::get_cached_pmmi(px* k, pmmi& i)
{
  bool found = k != smmbeg() && k != smmend() &&
    has_recent_pmmi && same(recent_pmmi->first.pxp(), k);
  if (found) i = recent_pmmi;
#ifdef STL_DEBUG
  if (fou123
nd && stl_smm_trace_enabled())
    cerr <<"get_cached_pmmi, found "<< k <<" with refc:"<< k->refc << endl;
#endif
  return found;
}

void stlmmap::cache_pmmi(pmmi i)
{
  if ( i != mp.end() ) {
    has_recent_pmmi = true;
    recent_pmmi = i;
  }
}

void stlmmap::clear()
{
  has_recent_pmmi = 0;
  mp.clear();
}

void stlmmap::erase(pmmi pos)
{
  if (has_recent_pmmi && recent_pmmi == pos)
    has_recent_pmmi = 0;
  mp.erase(pos);
}

int stlmmap::erase(px* k)
{
  int ret = 1;
  if ( !mp.empty() ) {
    pmmi i;
    if (k == smmbeg())
      erase(mp.begin());
    else if (k != smmend()) {
      ret = mp.erase(k);
    }
    else
      ret = 0;
  }
  has_recent_pmmi = false;
  return ret;
}

void stlmmap::erase(pmmi first, pmmi last)
{
  has_recent_pmmi = false;
  mp.erase(first, last);
}

/*** smm_iters functions *********************************************/

enum {gi_find, gi_lower, gi_upper};

static pmmi get_iter(pxhmmap& mp , px* key, int mode)
{
  pmmi iter;
  if (key == smmbeg())
    iter = mp.begin();
  else if (key == smmend())
    iter = mp.end();
  else {
    if (mode==gi_upper) 
      iter = mp.upper_bound(key);
    else if (mode==gi_lower)
      iter = mp.lower_bound(key);
    else 
      iter = mp.find(key);
  }
  return iter;  
}

static px* iter_to_key(const pxhmmap& mp, const pmmi& it)
{
  if (it == mp.end()) return smmend();
  if (it == mp.begin()) return smmbeg();
  return it->first.pxp();
}

static smm* get_smm_from_app(px* app)
{
  void* ret = 0;
  px* fun;
  size_t argc;
  px** args;
  pure_is_appv(app, &fun, &argc, &args);
  if (argc == 1 && !pure_is_pointer(args[0], &ret))
    ret = 0;
  free(args);
  return (smm*)ret; 
}

// sets two iterators [beg, end)
// if first >= last then use  mp.end(), mp.end()
smm_iters::smm_iters(px* pmmi_tuple)
{
  size_t tpl_sz;
  px** elems;
  
  is_valid = true;
  pure_is_tuplev(pmmi_tuple, &tpl_sz, &elems);
  smmp = get_smm_from_app(elems[0]);
  pxhmmap &mp = smmp->mp;
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
    px* comp = smmp->px_comp.pxp();    
    free(elems);
    begin_it = get_iter(mp, b, gi_lower);
    end_it = get_iter(mp,e,gi_upper);
    if (begin_it == mp.end()) {
      end_it = begin_it;
      return;
    }
    pmmi prec_it = end_it;
    if ( prec_it != mp.begin() && e != smmend() ) {
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

/*** Functions for multimap<pxh,pxh,pxh_pred2> *******************************/

smm* smm_make_empty(px* comp, int keys_only)
{
  smm* ret  = new smm(comp, keys_only); 
#ifdef STL_DEBUG
  if (stl_smm_trace_enabled())
    cerr << "TRACE SD:    new smm*: " << ret << endl;
#endif
  return ret;
}

void smm_delete(smm* p){
#ifdef STL_DEBUG
  if (stl_smm_trace_enabled())
    cerr << "TRACE SD: delete smm*: " << p << endl;
#endif
  delete(p);
}

bool smm_is_set(px* tpl)
{
  smm_iters itrs(tpl);
  if (!itrs.is_valid) bad_argument();
  return itrs.smmp->keys_only;
}

px* smm_make_vector(px* tpl) 
{
  smm_iters itrs(tpl);
  if (!itrs.is_valid) bad_argument();
  pmmi b = itrs.beg();
  pmmi e = itrs.end();
  int sz = smm_get_size(itrs.smmp, b, e);
  if (!sz)
    return pure_matrix_columnsv(0,NULL);
  px** bfr = (px**)malloc(sizeof(px*)*sz);
  if (itrs.smmp->keys_only) 
    transform(b, e, bfr, smm_pair_to_key);
  else
    transform(b, e, bfr, smm_pair_to_rocket);
  px* ret = pure_matrix_columnsv(sz, bfr);
  free(bfr);
  return ret;
}

sv* smm_make_stlvec(px* tpl) 
{
  smm_iters itrs(tpl);
  if (!itrs.is_valid) bad_argument();
  pmmi b = itrs.beg();
  pmmi e = itrs.end();
  sv* ret = new sv;
  if (itrs.smmp->keys_only) 
    transform(b, e, back_inserter(*ret), smm_pair_to_key);
  else
    transform(b, e, back_inserter(*ret), smm_pair_to_rocket);
  return ret;
}

px* smm_set_default(smm* smmp, px* val)
{
  if (smmp->keys_only) return 0; // fail
  smmp->dflt = val;
  smmp->has_dflt = 1;
  return val;
}

px* smm_get_default(smm* smmp)
{
  if (smmp->keys_only) return 0; // fail
  int ok = 1;
  px* val;
  if (smmp->has_dflt) 
    val = smmp->dflt.pxp();  
  else {
    val = NULL;
    ok = 0;
  }
  return pure_tuplel(2, pure_int(ok), val);
}

int smm_size(px* tpl)
{
  smm_iters itrs(tpl);
  if (!itrs.is_valid) bad_argument();
  return smm_get_size(itrs.smmp, itrs.beg(), itrs.end());
}

px* smm_bounds(px* tpl)
{
  px** elems;
  px *ub, *lb;
  size_t tpl_sz;
  pure_is_tuplev(tpl, &tpl_sz, &elems);
  smm* smmp = get_smm_from_app(elems[0]);
  pxhmmap &mp = smmp->mp;
  int num_iters = tpl_sz-1;
  switch (tpl_sz) {
  case 1:
    lb = smmbeg();
    ub = smmend();
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

int smm_member(smm* smmp, px* key)
{
  int ret = 0;
  pxhmmap& mp = smmp->mp;
  pmmi i;
  if (!mp.empty()) {
    if (smmp->get_cached_pmmi(key, i) ) {
      ret = 1;
    }
    else {
      i = smmp->find(key);
      if (i != mp.end()) {
        smmp->cache_pmmi(i);
        ret = 1;
      }
    }
  }
  return ret;
}

px* smm_prev(smm* smmp, px* key)
{
  pxhmmap& mp = smmp->mp;
  if (mp.empty()) index_error();
  pmmi i = mp.end();
  if ( !smmp->get_cached_pmmi(key,i) )
    i = smmp->find(key);
  if ( i == mp.begin() || i==mp.end() && key != smmend())
    index_error();
  else
    i--;
  smmp->cache_pmmi(i);
  return iter_to_key(mp, i);
}

px* smm_next(smm* smmp, px* key)
{
  pxhmmap& mp = smmp->mp;
  pmmi i = get_iter(
  if (mp.empty()) index_error();
  
  if ( i == mp.end() )
    index_error();
  else
    i++;
  smmp->cache_pmmi(i);
  return iter_to_key(mp, i);
}

px* smm_get(smm* smmp, px* key)
{
  pxhmmap &mp = smmp->mp; 
  px* ret = 0;
  pmmi beg;
  if ( !smmp->get_cached_pmmi(key, beg) ) 
    beg = get_iter(mp, key, gi_find);
  if (beg != mp.end()) {
    pair<pmmi,pmmi> lb_ub = mp.equal_range(beg->first);
    beg = lb_ub.first;
    pmmi end = lb_ub.second;
    smmp->cache_pmmi(beg);
    if (smmp->keys_only) {
      ret = pure_int( smm_get_size(smmp, beg, end) );
    } else {
      ret = listmap_aux(pure_int(0), beg, end, stl_smm_val);
    }
  }
  else if (smmp->keys_only)
    ret =  pure_int(0);
  else if (smmp->has_dflt)
    ret = smmp->dflt.pxp();
  else 
    index_error();
  return ret;
}

bool smm_includes(px* tpl1, px* tpl2)
{
  smm_iters itrs1(tpl1);
  smm_iters itrs2(tpl2);
  if (!itrs1.is_valid || !itrs2.is_valid) bad_argument;
  try {
    return includes(itrs1.beg(), itrs1.end(), itrs2.beg(), itrs2.end(), 
                    itrs1.smmp->mp.value_comp());
  }
  catch (px* e) {
    pure_throw(e);
  }
}

smm* smm_setop(int op, px* tpl1, px* tpl2)
{
  smm_iters itrs1(tpl1);
  smm_iters itrs2(tpl2);
  if (!itrs1.is_valid || !itrs2.is_valid) bad_argument;
  smm* smmp = itrs1.smmp;
  px* px_comp = smmp->px_comp.pxp();
  smm* res = new smm(px_comp, smmp->keys_only, smmp->dflt.pxp());
  pxhmmap& mp = res->mp;
  try {
    switch (op) {
    case stl_smm_union:
      set_union(itrs1.beg(), itrs1.end(), 
                itrs2.beg(), itrs2.end(),
                inserter(mp, mp.end()), mp.value_comp());
      break;
    case stl_smm_difference:
      set_difference(itrs1.beg(), itrs1.end(),
                     itrs2.beg(), itrs2.end(),
                     inserter(mp, mp.end()), mp.value_comp());
      break;
    case stl_smm_intersection:
      set_intersection(itrs1.beg(), itrs1.end(),
                       itrs2.beg(), itrs2.end(),
                       inserter(mp, mp.end()), mp.value_comp());
      break;
    case stl_smm_symmetric_difference:
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

// FIX ME
px* smm_update(smm* smmp, px* key, px* val)
{
  if (smmp->keys_only) return 0; // fail for sets
  update_aux(smmp, key, val);
  return val;
}

// FIX ME
px* smm_update_with(smm* smmp, px* key, px* unaryfun)
{
  if (smmp->keys_only) return 0; // fail for sets
  if (!smmp->has_dflt)
    failed_cond();
  px* old_val = smm_get(smmp,key);
  px* exception = 0;
  px* new_val = pure_appxl(unaryfun, &exception, 1, old_val);
  if (exception) pure_throw(exception);
  if (!new_val) bad_function();
  update_aux(smmp, key, new_val);
  return new_val;
}

px* smm_first(px* tpl)
{
  smm_iters itrs(tpl);
  if ( !itrs.is_valid ) bad_argument();
  pmmi b = itrs.beg();
  if (b != itrs.end())
    return itrs.smmp->keys_only ? smm_pair_to_key(*b) : smm_pair_to_rocket(*b);
  index_error();
}

px* smm_last(px* tpl)
{
  smm_iters itrs(tpl);
  if ( !itrs.is_valid ) bad_argument();
  pmmi e = itrs.end();
  if (itrs.beg() != e--)
    return itrs.smmp->keys_only ? smm_pair_to_key(*e) : smm_pair_to_rocket(*e);
  index_error();
}

void smm_rmfirst(px* tpl)
{
  smm_iters itrs(tpl);
  if (!itrs.is_valid) bad_argument();
  if ( itrs.beg() != itrs.end() )
    itrs.smmp->erase(itrs.beg());
  else
    index_error();
}

void smm_rmlast(px* tpl)
{
  smm_iters itrs(tpl);
  if (!itrs.is_valid) bad_argument();
  if ( itrs.beg() != itrs.end() )
    itrs.smmp->erase(--itrs.end());
  else
    index_error();
}

void smm_insert_elm(smm* smmp, px* kv)
{
  if ( !insert_aux(smmp, kv) ) bad_argument();
}

void smm_insert_elms_xs(smm* smmp, px* src)
{
  pxhmmap& mp = smmp->mp;
  size_t sz = 0;
  px** elems = NULL;
  bool ok;
  if (pure_is_listv(src, &sz, &elems)) {
    for (int i = 0; i<sz; i++)
      if ( !insert_aux(smmp, elems[i]) ) bad_argument();
    free(elems);
  } else if (matrix_type(src) == 0) {
    sz = matrix_size(src); 
    elems = (pure_expr**) pure_get_matrix_data(src);
    for (int i = 0; i<sz; i++) 
      if ( !insert_aux(smmp, elems[i]) ) bad_argument();
  } 
}

void smm_insert_elms_stlmmap(smm* smmp, px* tpl)
{
  smm_iters itrs(tpl);
  if (itrs.beg() != itrs.end()) {
    pmmi inserted = smmp->mp.begin();
    if (!itrs.is_valid) bad_argument();
    for (pmmi i = itrs.beg(); i!=itrs.end(); i++)
      inserted = smmp->mp.insert(inserted, *i);
    smmp->cache_pmmi(inserted);
  }
}

void smm_insert_elms_stlvec(smm* smmp, px* tpl)
{
  sv_iters itrs(tpl);
  if (!itrs.is_valid || itrs.num_iters != 2) bad_argument();
  pxhmmap& mp = smmp->mp;
  for (svi i = itrs.beg(); i!=itrs.end(); i++)
    if ( !insert_aux(smmp, i->pxp()) ) bad_argument();
}

void smm_erase(px* tpl)
{
  smm_iters itrs(tpl);
  if (!itrs.is_valid) bad_argument();
  itrs.smmp->erase(itrs.beg(), itrs.end());
}

void smm_clear(smm* smmp)
{
  smmp->clear();
}

int smm_remove(smm* smmp, px* k, int all)
{
  int ret = 1;
  if (all) {
    ret = smmp->erase(k);
  }
  else {
    pxhmmap &mp = smmp->mp;
    pmmi i;
    if ( !smmp->get_cached_pmmi(k, i) ) 
      i = get_iter(mp, k, gi_find);  
    if (i != mp.end())
      smmp->erase(i);
    else
      ret = 0;
  }
}

int smm_remove_if(smm* smmp, px* k, px* pred, int all)
{
  int ret = 0;
  pxhmmap &mp = smmp->mp;
  pxh_pred1 fun(pred);
  pair<pmmi,pmmi> lb_ub = mp.equal_range(k);
  pmmi i = lb_ub.first;
  pmmi end = lb_ub.second;
  try {
    while (i != end) {
      pmmi trg = i;
      if ( fun(i++->second) ) {
        smmp->erase(trg);
        ret++;
        if (!all) break;
      }
    }
  } catch (px* e) {
    pure_throw(e);
  }
  return ret;
}

bool smm_allpairs(px* comp, px* tpl1, px* tpl2)
{
  bool all_ok;
  smm_iters itrs1(tpl1);
  smm_iters itrs2(tpl2);
  if (!itrs1.is_valid) bad_argument();
  if (!itrs2.is_valid) bad_argument();

  smm* smmp1 = itrs1.smmp;
  smm* smmp2 = itrs1.smmp;
  try {
    if (smmp1->keys_only) {
      if (!smmp2->keys_only) bad_argument();
      pxh_pred2 fun(comp);
      pmmi i1 = itrs1.beg();
      pmmi i2 = itrs2.beg();
      pmmi end1 = itrs1.end();
      pmmi end2  =itrs2.end();
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
      if (smmp2->keys_only) bad_argument();
      if ( smm_size(tpl1) != smm_size(tpl2) ) {
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

px* smm_listmap(px* fun, px* tpl, int what)
{
  smm_iters itrs(tpl);
  if (!itrs.is_valid) bad_argument();
  if (itrs.smmp->keys_only) what = stl_smm_key;
  return listmap_aux(fun, itrs.beg(), itrs.end(), what);
}

px* smm_listcatmap(px* fun, px* tpl, int what)
{
  smm_iters itrs(tpl);
  if (!itrs.is_valid) bad_argument();
  if (itrs.smmp->keys_only) what = stl_smm_key;
  pmmi b = itrs.beg(); 
  pmmi e = itrs.end(); 
  px* cons = pure_const(cons_tag());
  px* nl = pure_const(null_list_tag());
  px* res = nl;
  px* y = 0;
  px* exception;
  px* *elms;
  size_t sz;
  for (pmmi i = b; i != e; i++){
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

static px* smm_foldl_itrs(px* fun, px* val, pmmi beg, pmmi end, int mode)
{ 
  px* res = px_new(val);
  px* exception = 0;
  for (pmmi i = beg; i != end; i++){
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

px* smm_foldl(px* fun, px* val, px* tpl)
{
  smm_iters itrs(tpl);
  if (!itrs.is_valid) bad_argument();
  int mode =  itrs.smmp->keys_only ? stl_smm_key : stl_smm_both;
  try {
    return smm_foldl_itrs(fun, val, itrs.beg(), itrs.end(), mode);
  } catch (px* e) {
    pure_throw(e);
  }
}

px* smm_foldl1(px* fun, px* tpl)
{
  smm_iters itrs(tpl);
  if ( !itrs.is_valid ) bad_argument();
  int mode =  itrs.smmp->keys_only ? stl_smm_key : stl_smm_both;
  if (itrs.beg() == itrs.end() ) return 0;
  pmmi b = itrs.beg();
  px* val;
  if (mode == stl_smm_key)
    val = b->first.pxp();
  else
    val = pair_to_rocket(b->first.pxp(), b->second.pxp());
  try {
    return smm_foldl_itrs(fun, val, ++b, itrs.end(), mode);
  } catch (px* e) {
    pure_throw(e);
  }
}

static px* smm_foldr_itrs(px* fun, px* val, pmmi beg, pmmi end, int mode)
{ 
  px* res = px_new(val);
  px* exception = 0;
  pmmi i = end;
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

px* smm_foldr(px* fun, px* val, px* tpl)
{
  smm_iters itrs(tpl);
  if (!itrs.is_valid) bad_argument();
  int mode = itrs.smmp->keys_only ? stl_smm_key : stl_smm_both;
  try {
    return smm_foldr_itrs(fun, val, itrs.beg(), itrs.end(), mode);
  } catch (px* x) {
    pure_throw(x);
  }
}

px* smm_foldr1(px* fun, px* tpl)
{
  smm_iters itrs(tpl);
  if ( !itrs.is_valid ) bad_argument();
  int mode =  itrs.smmp->keys_only ? stl_smm_key : stl_smm_both;
  if ( itrs.beg() == itrs.end() ) return 0;
  pmmi e = --itrs.end();
  px* val;
  if (mode == stl_smm_key)
    val = e->first.pxp();
  else
    val = pair_to_rocket(e->first.pxp(), e->second.pxp());
  try {
    return smm_foldr_itrs(fun, val, itrs.beg(), e, mode);
  } catch (px* x) {
    pure_throw(x);
  }
}
