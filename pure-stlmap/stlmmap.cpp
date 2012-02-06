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

#include <cassert>
#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <numeric>
#include "stlmmap.hpp"

using namespace std;

static int stlmmap_tag() 
{
  static ILS<int> _t = 0; int &t = _t();
  if (!t) t = pure_pointer_tag("stlmmap*");
  return t;
}

static int stlmmap_iter_tag() 
{
  static ILS<int> _t = 0; int &t = _t();
  if (!t) t = pure_pointer_tag("stlmmap_iter*");
  return t;
}

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

enum {gi_find, gi_lower, gi_upper};

static int range_size(smm* smmp, pmmi b, pmmi e)
{
  size_t sz = 0;
  pxhmmap& mmp = smmp->mmp;
  if (b == mmp.begin() && e == mmp.end())
    sz = mmp.size();
  else if (b == e)
    sz = 0;
  else
    while(b++ != e) sz++;
  return sz;
}

static bool extract_kv(smm* smmp, px* kv, px*& k, px*& v)
{
  bool ok = true;
  if (smmp->keys_only) {
    k = kv;
    v = NULL;
  } 
  else {
    if ( pxrocket_to_pxlhs_pxrhs(kv, &k, &v) )
      ;
    else if (smmp->has_dflt) {
      k = kv;
      v = smmp->dflt;
    }
    else {
      k = kv;
      v = NULL;
      ok = false;
    }
  }
  return ok;
}

static bool insert_aux(smm* smmp, px* kv, pmmi& pos, int& inserted)
{
  px *k, *v;
  bool ok = extract_kv(smmp,kv,k,v);
  if (ok) {
    inserted++;
    pos = smmp->mmp.insert(pxhpair(k,v));
  }
  return ok;
}

static px* px_pointer(smm* smmp)
{
  static ILS<px*> _sym = NULL; px*& sym = _sym();
  if (!sym) sym = pure_new(pure_symbol(pure_sym("stl::smm_delete")));
  px* ptr = pure_tag(stlmmap_tag(), pure_pointer(smmp));
  return pure_sentry(sym,ptr);
}

static px* px_pointer(smm_iter* smmip)
{
  static ILS<px*> _sym = NULL; px*& sym = _sym();
  if (!sym) sym = pure_new(pure_symbol(pure_sym("stl::smm_iter_delete")));
  px* ptr = pure_tag(stlmmap_iter_tag(), pure_pointer(smmip));
  return pure_sentry(sym,ptr);
}

static bool get_smmp(px* pxsmmp, smm** smmpp)
{
  void* ptr;
  bool ok = false;
  if ( pure_is_pointer(pxsmmp, &ptr) ) {
    ok = pure_get_tag(pxsmmp) == stlmmap_tag();
  }
  *smmpp = ok ? (smm*)ptr : NULL; 
  return ok;
}

static bool get_smmip(px* pxsmmip, int& tag, smm_iter** itr)
{
  void* ptr;
  smm_iter* smmip;
  bool ok = pure_is_pointer(pxsmmip,&ptr);
  if (ok) { 
    smmip = (smm_iter*)ptr;
    tag = pure_get_tag(pxsmmip);
    ok = tag ==stlmmap_iter_tag();
    if (ok) *itr = smmip;
  }
  return ok;
}

static pmmi get_iter(smm* smmp , px* key, int mode)
{
  pxhmmap& mmp = smmp->mmp;
  pmmi iter;
  if (key == smbeg()) {
    iter = mmp.begin();
  }
  else if (key == smend()) {
    iter = mmp.end();
  }
  else {
    if (mode==gi_upper) 
      iter = mmp.upper_bound(key);
    else if (mode==gi_lower)
      iter = mmp.lower_bound(key);
    else 
      iter = mmp.find(key);
  }
  return iter;  
}

static px* iter_to_key(const pxhmmap& mmp, const pmmi& it)
{
  if (it == mmp.end()) return smend();
  if (it == mmp.begin()) return smbeg();
  return it->first;
}

static px* get_elm_aux(smm* smmp, pmmi i, int what) 
{
  px* ret;
  pxhmmap &mmp = smmp->mmp; 
  if (i != mmp.end()) {
    switch (what) {
    case stl_smm_key:
      ret = i->first;
      break;
    case stl_smm_val:
      ret = smmp->keys_only ? i->first : i->second;
      break;
    case stl_smm_elm:
      px* key = i->first;     
      ret = smmp->keys_only ? key : pxhpair_to_pxrocket(*i);
      break;
    }
  }
  else {
    switch (what) {
    case stl_smm_key:
      ret = smend();
      break;
    case stl_smm_val:
      if (smmp->keys_only) 
        ret = pure_int(0);
      else
        index_error();
      break;
    case stl_smm_elm:
      if (smmp->keys_only) 
        ret = pure_int(0);
      else
        index_error();
      break;
      px* key = i->first;
      return smmp->keys_only ? key : pxhpair_to_pxrocket(*i);
    }
  }
  return ret;
}

static px* smm_foldl_rng(px* fun, px* val, const smm_range rng, pmmi i,  int mode)
{ 
  pmmi end = rng.end();
  smm* smmp = rng.smmp();
  pmmi sac_end = smmp->mmp.end(); 
  px* res = pure_new(val); 
  px* exception = 0;
  while (i != end  && i != sac_end){
    pmmi trg_i = i++;
    px* trg = get_elm_aux(smmp, trg_i, mode);
    px* fxy = pure_appxl(fun, &exception, 2, res, trg);
    if (exception) {
      pure_freenew(res);
      throw exception;
    }
    pure_new(fxy);
    pure_free(res);
    res = fxy;
  }
  pure_unref(res);
  if (i!=end) {
    pure_freenew(res);
    bad_argument();    
  }
  return res;
}

static px* smm_foldr_rng(px* fun, px* val, const smm_range& rng, pmmi i, int mode)
{ 
  pmmi beg = rng.beg();
  smm* smmp = rng.smmp();
  pmmi sac_beg = smmp->mmp.begin();  
  px* res = pure_new(val);
  px* exception = 0;
  while (i != beg && i != sac_beg) {
    pmmi trg_i = --i;
    px* trg = get_elm_aux(smmp, trg_i, mode);
    px* fxy = pure_appxl(fun, &exception, 2, trg, res);
    if (exception) {
      pure_freenew(res);
      throw exception;
    }
    pure_new(fxy);
    pure_free(res);
    res = fxy;
  }
  pure_unref(res);
  if (i!=beg) {
    pure_freenew(res);
    bad_argument();    
  }
  return res;
}

/*** smm_iter members  ***********************************************/

smm_iter::smm_iter(px* pxsmmp, pmmi i) : pxhsmmp(pxsmmp), iter(i), is_valid(1)
{
  if (iter != smmp()->mmp.end())
    smmp()->smis.push_back(this);
}

smm_iter::~smm_iter() 
{
  smmp()->remove_smm_iter(this);
}

smm* smm_iter::smmp () const
{
  void* ret;
  pure_is_pointer( pxhsmmp, &ret);
  return static_cast<smm*>(ret);
}

ostream& operator<<(ostream& os, const smm_iter* smmip)
{
  if (smmip->is_valid) {
    if (smmip->iter == smmip->smmp()->mmp.end())
      os << "pastend iterator";
    else
      os << smmip->iter->first;
  }
  else {
    os << "invalid iterator";
  }
  return os;
}

/*** stlmmap members  ***********************************************/

stlmmap::stlmmap(px* cmp, px* val_cmp, px* val_eql, bool keyonly):
  mmp(pxh_pred2(cmp)), keys_only(keyonly),
  px_comp(cmp), px_val_comp(val_cmp), px_val_equal(val_eql),
  has_recent_pmmi(0), latest_pmmi_pos(0),
  has_dflt(0), dflt(NULL)
{
  //PR(stlmmap,this);
}

stlmmap::stlmmap(px* cmp, px* val_cmp, px* val_eql, bool keyonly, px *d):
  mmp(pxh_pred2(cmp)), keys_only(keyonly),
  px_comp(cmp), px_val_comp(val_cmp), px_val_equal(val_eql),
  has_recent_pmmi(0), latest_pmmi_pos(0), 
  has_dflt(1), dflt(d)
{
  //PR(stlmmap,this);  
}

stlmmap::~stlmmap()
{
  //PR(~stlmmap,this);
  assert(smis.size()==0);
  //clear_all_iters();
}

px* stlmmap::parameter_tuple()
{
  px* ret = 0;
  px* kc = px_comp;
  if (keys_only) {
    px* nl = pure_listl(0);
    ret = pure_tuplel(5,pure_int(1),kc,nl,nl,nl);
  }
  else {
    px* df = dflt;
    px* vc = px_val_comp;
    px* ve = px_val_equal;
    ret = pure_tuplel(5, pure_int(0),kc, df, vc, ve);
  }
  return ret;
}

pmmi stlmmap::find(px* key)
{
  pmmi iter;
  if (key == smbeg())
    iter = mmp.begin();
  else if (key == smend())
    iter = mmp.end();
  else
    iter = mmp.find(key);
  return iter;  
}

void stlmmap::cache_pmmi(const pmmi& i)
{
  if ( i != mmp.end() ) {
    if (!has_recent_pmmi) {
      recent_pmmi.clear();
      has_recent_pmmi = true;
      latest_pmmi_pos = 0;
    }
    size_t num_cached = recent_pmmi.size();
    if (num_cached<SM_CACHE_SZ) {
      recent_pmmi.push_back(i);
      latest_pmmi_pos = num_cached;
    }
    else {
      latest_pmmi_pos = (latest_pmmi_pos + 1) % SM_CACHE_SZ;
      recent_pmmi[latest_pmmi_pos] = i;
    }
  }
}

bool stlmmap::get_cached_pmmi(px* k, pmmi& i)
{
  bool ret = false;
  if ( k != smend() && has_recent_pmmi ) {
    size_t num_cached = recent_pmmi.size();
    size_t pos = 0; 
    for (; pos < num_cached; pos++) {
      if ( same(recent_pmmi[pos]->first, k) ) {
        i = recent_pmmi[pos];
        ret = true;
        break;
      }
    }
  }
  // PR2(get_cached_pmmi,k,ret)
  return ret;
}

struct has_pmmi {
  has_pmmi(pmmi _iter) : iter(_iter) {}
  bool operator()(smm_iter* smmip){return smmip->iter == iter;} 
  pmmi iter;
};

void stlmmap::remove_smm_iter(smm_iter* smmip)
{
  smis.erase( remove(smis.begin(), smis.end(), smmip), smis.end() );
}

void stlmmap::clear_iter(pmmi pos)
{
  if ( pos == mmp.end() ) return;
  if (has_recent_pmmi) {
    vector<pmmi>::iterator i = 
      ::find(recent_pmmi.begin(),recent_pmmi.end(),pos);
    if (i != recent_pmmi.end()) recent_pmmi.erase(i);
  }
  vector<smm_iter*>::iterator i = smis.begin(), pe;
  has_pmmi is_trg(pos);
  while( i!=smis.end() ) {
    if ( is_trg(*i) ) (*i)->is_valid = 0;
    i++;
  }
  pe = remove_if( smis.begin(), smis.end(), is_trg );
  smis.erase( pe, smis.end() );
}

void stlmmap::clear_all_iters()
{
  for (vector<smm_iter*>::iterator i = smis.begin(); i!= smis.end(); i++) {
    (*i)->is_valid = 0;
  }
  smis.clear();
}

int stlmmap::erase(pmmi pos)
{
  clear_iter(pos);
  mmp.erase(pos);
  return 1;
}

int stlmmap::erase(px* k)
{
  int ret = 0;
  if ( !mmp.empty() ) {
    ret = 1;
    pmmi i;
    if ( get_cached_pmmi(k, i) )
      erase(i);
    else {
      if (k == smbeg())
        erase(mmp.begin());
      else if (k != smend()) {
        pair<pmmi,pmmi> er = mmp.equal_range(k);
        ret = erase(er.first, er.second);
       }
      else
        ret = 0;
    }
  }
  return ret;
}

int stlmmap::erase(pmmi first, pmmi last)
{
  int ret = 0;
  for (pmmi pos = first; pos != last; pos++) {
    clear_iter(pos); ret++;
  }
  mmp.erase(first, last);
  return ret;
}

void stlmmap::clear()
{
  has_recent_pmmi = false;
  clear_all_iters();
  mmp.clear();
}

/*** smm_range functions *********************************************/

bool smm_range::init_from_iters(px** pxsmmip, int sz)
{
  is_valid = false;
  if (sz==0 || sz>2) return false;
  num_iters = sz;
  int tag;
  smm_iter* smmip;
  if ( !get_smmip(pxsmmip[0],tag,&smmip) || !smmip->is_valid ) return false;

  smm* smmp = smmip->smmp();
  pxhsmmp = smmip->pxhsmmp;
  begin_it = smmip->iter;
  if (num_iters == 2) {
    pxhmmap& mmp = smmp->mmp;
    pxhmmap::key_compare k_cmp = mmp.key_comp();
    if (get_smmip(pxsmmip[1],tag,&smmip) && 
        smmip->is_valid && smmip->smmp()==smmp) 
      end_it = smmip->iter;
    else
      goto done;
    if ( begin_it == mmp.end() && end_it != mmp.end() )
      goto done;
    if ( end_it != mmp.end() && k_cmp(end_it->first,begin_it->first) )
      goto done;
  } 
  else {
    end_it = end_it++;
  }
  is_valid = true;
 done:
  return is_valid;
}

bool smm_range::init_from_keys(px** smmp_keys, int tpl_sz)
{
  is_valid = false;
  pxhsmmp = smmp_keys[0];
  smm* smmp;
  if ( !get_smmp(pxhsmmp, &smmp) ) return false;
  pxhmmap &mmp = smmp->mmp;
  pxhmmap::key_compare k_cmp = mmp.key_comp();
  num_iters = tpl_sz-1;
  pmmi et;
  if (num_iters > 2) return false;

  is_valid = true;
  if (num_iters == 0 || mmp.size() == 0) {
    begin_it = mmp.begin();
    end_it = mmp.end();
    return true;
  }
  
  px* b = smmp_keys[1];
  px* e = num_iters == 2 ? smmp_keys[2] : b;
  begin_it = get_iter(smmp, b, gi_lower);
  if (b == smbeg()) b = begin_it->first;
  if (num_iters == 1) {
    if ( begin_it == mmp.end() ||
         k_cmp(b, begin_it->first) || 
         k_cmp(begin_it->first,b)) {
      begin_it = end_it = mmp.end(); 
      goto done;
    }  
    end_it = begin_it;
    while (++end_it != mmp.end())
      if ( k_cmp(b, end_it->first) ) break;
    goto done;
  }
  else if ( begin_it == mmp.end() || e == smend()) {
    end_it = mmp.end();
    goto done;
  }
  else {
    pmmi i = get_iter(smmp, e, gi_upper); // upper too high
    while (i != mmp.begin() ) {
      pmmi prev = i; prev--;
      if ( k_cmp(prev->first,e) ) break;
      i = prev;
    }
    if (i == mmp.begin())
      end_it = begin_it;
    else
      end_it = i;
  }

 done:
  if (end_it != mmp.end() && k_cmp(end_it->first, begin_it->first) )
    end_it = begin_it;
  return is_valid;
}

smm_range::smm_range(px* pmmi_tuple)
{
  size_t tpl_sz;
  px** elems;
  pure_is_tuplev(pmmi_tuple, &tpl_sz, &elems);
  try {
    bool ok = init_from_iters(elems, tpl_sz) || init_from_keys(elems, tpl_sz);
  }
  catch (px* e) {
    free(elems);
    pure_throw(e);
  }
  free(elems);
}

smm* smm_range::smmp() const
{
  void* ptr;
  pure_is_pointer(pxhsmmp, &ptr);
  return static_cast<smm*>(ptr);
}



/*** Pure interface support functions  *************************************/

px* smm_type_tags()
{
  return pure_tuplel(2, pure_int(stlmmap_tag()), pure_int(stlmmap_iter_tag())); 
}

px* smm_make_empty(px* comp, px* v_comp, px* v_eql, px* dflt, int keys_only)
{
  return px_pointer( new smm(comp, v_comp, v_eql, keys_only, dflt) );
}

void smm_delete(smm* smmp){
  delete(smmp);
}

void smm_iter_delete(smm_iter* smmip){
  delete(smmip);
}

px* smm_container_info(px* tpl) 
{
  smm_range rng(tpl);
  if (!rng.is_valid) bad_argument();  
  return rng.smmp()->parameter_tuple();
} 

int smm_size(px* tpl)
{
  smm_range rng(tpl);
  if (!rng.is_valid) bad_argument();
  return range_size(rng.smmp(), rng.beg(), rng.end());
}

bool smm_empty(px* tpl)
{
  smm_range rng(tpl);
  if (!rng.is_valid) bad_argument();
  return rng.num_iters == 0 ? rng.smmp()->mmp.empty() : smm_size(tpl) == 0;
}

int  smm_count(px* pxsmmp, px* key)
{
  smm* smmp;
  if ( !get_smmp(pxsmmp,&smmp) ) bad_argument();
  return smmp->mmp.count(key);
}

bool smm_is_set(px* tpl)
{
  smm_range rng(tpl);
  if (!rng.is_valid) bad_argument();
  return rng.smmp()->keys_only;
}

px* smm_find(px* pxsmmp, px* key, int what)
{
  smm* smmp;
  if ( !get_smmp(pxsmmp,&smmp) ) bad_argument();
  px* ret = 0;
  pxhmmap& mmp  = smmp->mmp;
  int num_inserted;
  pmmi i = get_iter(smmp, key, gi_find);
  if (what==stl_smm_iter_dflt && i == mmp.end() && smmp->has_dflt){
    px* dflt = smmp->dflt;
    pmmi pos = smmp->mmp.insert(pxhpair(key,dflt));
    return px_pointer(new smm_iter(pxsmmp, pos));
  }
  else if (what==stl_smm_iter || what == stl_smm_iter_dflt)
    return px_pointer(new smm_iter(pxsmmp, i)); 
  else
    return get_elm_aux(smmp, i, what);
}

px* smm_copy_iter(px* pxsmmip)
{
  smm_iter* smmip;
  int tag;
  if ( !get_smmip(pxsmmip,tag,&smmip) || !smmip->is_valid ) bad_argument();
  return px_pointer( new smm_iter(smmip->pxhsmmp, pmmi(smmip->iter)) );
}

px* smm_begin(px* pxsmmp)
{
  smm* smmp;
  if ( !get_smmp(pxsmmp, &smmp) ) failed_cond();
  return px_pointer(  new smm_iter(pxsmmp, smmp->mmp.begin()) );
}

px* smm_end(px* pxsmmp)
{
  smm* smmp;
  if ( !get_smmp(pxsmmp, &smmp) ) failed_cond();
  return px_pointer(  new smm_iter(pxsmmp, smmp->mmp.end()) );
}

px* smm_bounds(px* pxsmmp, px* key, int what)
{
  smm* smmp;
  smm_iter* smmip;
  if ( !get_smmp(pxsmmp, &smmp) ) failed_cond();
  pxhmmap& mmp = smmp->mmp;
  switch (what) {
  case stl_smm_lower_bound:
    return px_pointer( new smm_iter(pxsmmp, mmp.lower_bound(key)) );
  case stl_smm_upper_bound:
    return px_pointer( new smm_iter(pxsmmp, mmp.upper_bound(key)) );
  case stl_smm_equal_range:
    pair<pmmi,pmmi> l_u = mmp.equal_range(key);
    px* pxsmmip_l = px_pointer( new smm_iter(pxsmmp, l_u.first) );
    px* pxsmmip_r = px_pointer( new smm_iter(pxsmmp, l_u.second) );
    return pure_tuplel(2, pxsmmip_l, pxsmmip_r);
  }
  bad_argument();
}

// valid, container, i1, i2,
px* smm_range_info(px* tpl)
{
  px* ret;
  smm_range rng(tpl);
  bool ok = rng.is_valid;
  px* valid = pure_int(ok);
  if (rng.is_valid) {
    px* container = rng.pxhsmmp;
    px* beg = px_pointer( new smm_iter(rng.pxhsmmp, rng.beg()) );
    px* end = px_pointer( new smm_iter(rng.pxhsmmp, rng.end()) );
    ret = pure_tuplel(4, valid, container, beg, end);
  }
  else {
    px* null_ptr = pure_pointer(0);
    ret = pure_tuplel(4, valid, null_ptr, null_ptr, null_ptr);
  }
  return ret;
}

px* smm_move_iter(px* pxsmmip, int count)
{
  smm_iter* smmip;
  int tag;
  if ( !get_smmip(pxsmmip,tag,&smmip) ) return 0;
  if ( !smmip->is_valid ) bad_argument();
  pmmi& i = smmip->iter;
  pmmi beg = smmip->smmp()->mmp.begin();
  pmmi end = smmip->smmp()->mmp.end();
  while (count > 0) {
    if (i == end) return pxsmmip;
    count--; i++;
  }
  while (count++ < 0) {
    if (i == beg) index_error();
    i--;
  }
  return pxsmmip;
}

px* smm_iter_is_at(px* pxsmmip, int where)
{
  px* ret;
  smm_iter* smmip;
  int tag;
  if ( !get_smmip(pxsmmip,tag,&smmip) || !smmip->is_valid ) return 0;
  switch (where) {
  case stl_smm_at_beginning:
    return pure_int( smmip->smmp()->mmp.begin() == smmip->iter );
  case stl_smm_at_pastend:
    return pure_int( smmip->smmp()->mmp.end() == smmip->iter );
  default:
    bad_argument();
  }
}

// fix return is_valid, container, key, val 
px* smm_iter_info(px* pxsmmip)
{
  smm_iter* smmip;
  int tag;
  if ( !get_smmip(pxsmmip,tag,&smmip) ) return 0;
  px* valid = pure_int( smmip->is_valid );
  px* cont = smmip->pxhsmmp;
  pmmi i = smmip->iter;
  smm* smmp = smmip->smmp();
  px* key; px* val;
  if (smmip->is_valid && i != smmp->mmp.end() ) {
    key = iter_to_key(smmp->mmp, i);    
    if (smmp->keys_only) 
      val = i->first;
    else
      val = i->second;
  } 
  else {
    key = smend();
    val = pure_listl(0);
  }
  return pure_tuplel(4,valid, cont, key, val);
}

px* smm_equal_iter(px* pxsmmip1, px* pxsmmip2)
{
  smm_iter* smmip1;
  int tag1;
  if ( !get_smmip(pxsmmip1,tag1,&smmip1) || !smmip1->is_valid ) bad_argument();
  smm* smmp1 = smmip1->smmp();
  smm_iter* smmip2;
  int tag2;
  if ( !get_smmip(pxsmmip2,tag2,&smmip2) || !smmip2->is_valid ) bad_argument();
  smm* smmp2 = smmip2->smmp();
  bool compatible = true;
  if (smmp1->keys_only) 
    compatible = smmp2->keys_only;
  else
    compatible = !smmp2->keys_only;
  if (!compatible) bad_argument();
  return pure_int( smmip1->iter == smmip2->iter ); 
}

px* smm_get_at(px* pxsmmip, int what)
{
  smm_iter* smmip;
  int tag;
  if ( !get_smmip(pxsmmip,tag,&smmip) || !smmip->is_valid ) bad_argument();
  smm* smmp = smmip->smmp();
  if (smmip->iter == smmp->mmp.end()) index_error();
  if ( what==smmp->keys_only ) what = stl_smm_key; 
  return get_elm_aux(smmp, smmip->iter, what);
}

px* smm_get_elm_at_inc(px* pxsmmip)
{
  smm_iter* smmip; 
  int tag;
  if ( !get_smmip(pxsmmip,tag,&smmip) || !smmip->is_valid ) bad_argument();
  smm* smmp = smmip->smmp();
  pmmi& i = smmip->iter;
  if ( i == smmp->mmp.end() ) index_error();
  int what = smmp->keys_only ? stl_smm_key : stl_smm_elm; 
  px* ret = get_elm_aux(smmp, i, what);
  i++;
  return ret;
}

px* smm_put_at(px* pxsmmip, px* val)
{
  smm_iter* smmip;
  int tag;
  if ( !get_smmip(pxsmmip,tag,&smmip) || !smmip->is_valid ) bad_argument();
  pmmi itr = smmip->iter;
  smm* smmp = smmip->smmp();
  if (smmp->keys_only) bad_argument();
  if (itr == smmp->mmp.end()) index_error();
  smmip->iter->second = val;
  return val;
}

px* smm_insert_hinted(px* pxsmmp, px* pxsmmip, px* kv)
{
  smm* smmp; pmmi pos;
  if (!get_smmp(pxsmmp,&smmp) ) bad_argument();
  smm_iter* smmip;
  int tag;
  if ( !get_smmip(pxsmmip,tag,&smmip) || !smmip->is_valid ) bad_argument();
  px *k, *v;
  if ( !extract_kv(smmp,kv,k,v) ) bad_argument();
  if ( !same(smmip->pxhsmmp,pxsmmp) ) bad_argument();
  try {
    pos = smmp->mmp.insert(smmip->iter, pxhpair(k,v));
  }
  catch (px* e) {
    pure_throw(e);
  }
  return px_pointer( new smm_iter(pxsmmp, pos) );
}

px* smm_insert_elm(px* pxsmmp, px* kv)
{
  smm* smmp;  pmmi pos;
  if (!get_smmp(pxsmmp,&smmp) ) bad_argument();
  int num_inserted = 0;
  try {
    if ( !insert_aux(smmp, kv, pos, num_inserted) ) bad_argument();
  }
  catch (px* e) {
    pure_throw(e);
  }
  smmp->cache_pmmi(pos);
  return px_pointer(new smm_iter(pxsmmp, pos));
}

int smm_insert_elms_xs(px* pxsmmp, px* src)
{
  smm* smmp; pmmi pos;
  if (!get_smmp(pxsmmp,&smmp) ) bad_argument();
  size_t sz = 0;
  px** elems = NULL;
  bool ok;
  int num_inserted = 0;
  try {
    if (pure_is_listv(src, &sz, &elems)) {
      for (int i = 0; i<sz; i++)
        if ( !insert_aux(smmp, elems[i], pos, num_inserted) ) bad_argument();
      free(elems);
    } else if (matrix_type(src) == 0) {
      sz = matrix_size(src); 
      elems = (pure_expr**) pure_get_matrix_data(src);
      for (int i = 0; i<sz; i++) 
        if ( !insert_aux(smmp, elems[i], pos, num_inserted) ) bad_argument();
    }
  }
  catch (px* e){
    free(elems);
    pure_throw(e);
  }
  return num_inserted;
}

int smm_insert_elms_stlmmap(px* pxsmmp, px* tpl)
{
  smm* smmp;
  if (!get_smmp(pxsmmp,&smmp) ) bad_argument();
  smm_range rng(tpl);
  if (!rng.is_valid) bad_argument();
  if (smmp == rng.smmp()) bad_argument();
  pxhmmap& mmp = smmp->mmp;
  size_t oldsz = mmp.size();
  mmp.insert(rng.beg(),rng.end());
  return mmp.size() - oldsz;
}

int smm_insert_elms_stlvec(px* pxsmmp, sv* sv_p)
{
  smm* smmp; pmmi pos;
  if (!get_smmp(pxsmmp,&smmp) ) bad_argument();
  int num_inserted = 0;
  pxhmmap& mmp = smmp->mmp;
  try {
    for (sv::iterator i = sv_p->begin(); i!=sv_p->end(); i++)
      if ( !insert_aux(smmp, *i, pos, num_inserted) ) bad_argument();
  }
  catch (px* e) {
    pure_throw(e);
  }
  return num_inserted;
}

px*  smm_swap(px* pxsmmp1, px* pxsmmp2)
{
  smm* smmp1; smm* smmp2;
  if ( !get_smmp(pxsmmp1, &smmp1) ) failed_cond();
  if ( !get_smmp(pxsmmp2, &smmp2) ) failed_cond();
  smmp1->has_recent_pmmi = false;
  smmp2->has_recent_pmmi = false;
  smmp1->mmp.swap(smmp2->mmp);
}

int smm_clear(px* pxsmmp)
{
  smm* smmp;
  if (!get_smmp(pxsmmp,&smmp) ) bad_argument();
  size_t sz = smmp->mmp.size();
  smmp->clear();
  return sz;
}

int smm_erase(px* pxsmmp, px* trg)
{
  smm* smmp;
  if (!get_smmp(pxsmmp,&smmp) ) bad_argument();
  int res = 0;
  size_t trg_sz;
  px** elems;
  pure_is_tuplev(trg, &trg_sz, &elems);
  if (trg_sz == 1) {
    smm_iter* smmip;
    int tag;
    if ( !get_smmip(trg,tag,&smmip) || !smmip->is_valid ) bad_argument();
    if ( !same(pxsmmp, smmip->pxhsmmp) ) bad_argument();
    smmip->smmp()->erase(smmip->iter);
    res = 1;
  }
  else {
    smm_range rng(trg);
    if (!rng.is_valid) bad_argument();
    if ( !same(pxsmmp, rng.pxhsmmp) ) bad_argument();
    res = rng.smmp()->erase(rng.beg(), rng.end());
  }
  return res;
}

bool smm_equal(px* tpl1, px* tpl2)
{
  smm_range rng1(tpl1);
  smm_range rng2(tpl2);
  if (!rng1.is_valid || !rng2.is_valid) bad_argument;
  if (smm_size(tpl1) != smm_size(tpl2)) return 0;
  smm* smmp = rng1.smmp();
  try {
    if (smmp->keys_only) {
      pxhpair_first_equivalent comp(smmp->px_comp);   
      return equal(rng1.beg(), rng1.end(), rng2.beg(), comp);
    }
    else {
      pxhpair_equivalent comp(smmp->px_comp,smmp->px_val_equal);   
      return equal(rng1.beg(), rng1.end(), rng2.beg(), comp);
    }
  }
  catch (px* e) {
    pure_throw(e);
  }
}

int smm_less(px* tpl1, px* tpl2)
{
  smm_range rng1(tpl1);
  smm_range rng2(tpl2);
  if (!rng1.is_valid || !rng2.is_valid) bad_argument;
  smm* smmp = rng1.smmp();
  try {
    if (smmp->keys_only) {
      return lexicographical_compare(rng1.beg(), rng1.end(),
                                     rng2.beg(), rng2.end(), 
                                     smmp->mmp.value_comp());
    }
    else {
      pxhpair_less comp(smmp->px_comp.pxp(),smmp->px_val_comp.pxp());   
      return lexicographical_compare(rng1.beg(), rng1.end(),
                                     rng2.beg(), rng2.end(), 
                                     comp);
    }
  }
  catch (px* e) {
    pure_throw(e);
  }
}

bool smm_includes(px* tpl1, px* tpl2)
{
  smm_range rng1(tpl1);
  smm_range rng2(tpl2);
  if (!rng1.is_valid || !rng2.is_valid) bad_argument;
  smm* smmp = rng1.smmp();
  try {
    if (smmp->keys_only) {
      return includes(rng1.beg(), rng1.end(),
                      rng2.beg(), rng2.end(), smmp->mmp.value_comp());
    }
    else {
      pxhpair_less comp(smmp->px_comp.pxp(),smmp->px_val_comp.pxp());   
      return includes(rng1.beg(), rng1.end(),
                      rng2.beg(), rng2.end(), comp);
    }
  }
  catch (px* e) {
    pure_throw(e);
  }
}

px* smm_setop(int op, px* tpl1, px* tpl2)
{
  smm_range rng1(tpl1);
  smm_range rng2(tpl2);
  if (!rng1.is_valid || !rng2.is_valid) bad_argument;
  smm* smmp = rng1.smmp();
  smm* trg = new smm(smmp->px_comp.pxp(),
                     smmp->px_val_comp.pxp(),
                     smmp->px_val_equal.pxp(),
                     smmp->keys_only, smmp->dflt.pxp());
  pxhmmap& mmp = trg->mmp;
  try {
    if (trg->keys_only) {
      pxhmmap::value_compare comp = mmp.value_comp();
      switch (op) {
      case stl_smm_merge:
        merge(rng1.beg(), rng1.end(), 
              rng2.beg(), rng2.end(),
              inserter(mmp, mmp.end()), comp);
        break;
      case stl_smm_union:
        set_union(rng1.beg(), rng1.end(), 
                  rng2.beg(), rng2.end(),
                  inserter(mmp, mmp.end()), comp);
        break;
      case stl_smm_difference:
        set_difference(rng1.beg(), rng1.end(),
                       rng2.beg(), rng2.end(),
                       inserter(mmp, mmp.end()), comp);
        break;
      case stl_smm_intersection:
        set_intersection(rng1.beg(), rng1.end(),
                         rng2.beg(), rng2.end(),
                         inserter(mmp, mmp.end()), comp);
        break;
      case stl_smm_symmetric_difference:
        set_symmetric_difference(rng1.beg(), rng1.end(),
                                 rng2.beg(), rng2.end(),
                                 inserter(mmp, mmp.end()), comp);
        break;
      default:
        bad_argument();
      }
    }
    else {
      pxhpair_less comp(trg->px_comp.pxp(),trg->px_val_comp.pxp());
      switch (op) {
      case stl_smm_merge:
        merge(rng1.beg(), rng1.end(), 
              rng2.beg(), rng2.end(),
              inserter(mmp, mmp.end()), comp);
        break;
      case stl_smm_union:
        set_union(rng1.beg(), rng1.end(), 
                  rng2.beg(), rng2.end(),
                  inserter(mmp, mmp.end()), comp);
        break;
      case stl_smm_difference:
        set_difference(rng1.beg(), rng1.end(),
                       rng2.beg(), rng2.end(),
                       inserter(mmp, mmp.end()), comp);
        break;
      case stl_smm_intersection:
        set_intersection(rng1.beg(), rng1.end(),
                         rng2.beg(), rng2.end(),
                         inserter(mmp, mmp.end()), comp);
        break;
      case stl_smm_symmetric_difference:
        set_symmetric_difference(rng1.beg(), rng1.end(),
                                 rng2.beg(), rng2.end(),
                                 inserter(mmp, mmp.end()), comp);
        break;
      default:
        bad_argument();
      }
    }
    return px_pointer(trg);
  } catch (px* e) {
    pure_throw(e);
  }
}

px* smm_make_vector(px* tpl) 
{
  smm_range rng(tpl);
  if (!rng.is_valid) bad_argument();
  pmmi b = rng.beg();
  pmmi e = rng.end();
  smm* smmp = rng.smmp();
  int sz = range_size(smmp, b, e);
  if (!sz)
    return pure_matrix_columnsv(0,NULL);
  px** bfr = (px**)malloc(sizeof(px*)*sz);
  if (smmp->keys_only) 
    transform(b, e, bfr, pxhpair_to_pxlhs);
  else
    transform(b, e, bfr, pxhpair_to_pxrocket);
  px* ret = pure_matrix_columnsv(sz, bfr);
  free(bfr);
  return ret;
}

void smm_fill_stlvec(px* tpl, sv* svp) 
{
  smm_range rng(tpl);
  if (!rng.is_valid) bad_argument();
  pmmi b = rng.beg();
  pmmi e = rng.end();
  if (rng.smmp()->keys_only) 
    transform(b, e, back_inserter(*svp), pxhpair_to_pxlhs);
  else
    transform(b, e, back_inserter(*svp), pxhpair_to_pxrocket);
}

/*** Mapping and folding ***********************************************/

px* smm_listmap(px* fun, px* tpl, int what)
{
  smm_range rng(tpl);
  if (!rng.is_valid) bad_argument();
  smm* smmp = rng.smmp();
  if (smmp->keys_only) what = stl_smm_key;
  pmmi sac_end = smmp->mmp.end();
  pmmi b = rng.beg();
  pmmi e = rng.end();
  px* cons = px_cons_sym();
  px* nl = pure_listl(0);
  px* res = nl;
  px* y = 0;
  px* exception;
  int use_function = 1;
  if (pure_is_int(fun,&use_function))
    use_function = false;
  else
    use_function = true;
  pmmi i = b;
  for (;i != e && i != sac_end; i++){
    px* trg = get_elm_aux(smmp, i, what);
    px* pxi = trg;
    if (use_function) {
      pxi = pure_appxl(fun, &exception, 1, trg);
      if (exception) {
        if (res) pure_freenew(res);
        if (pxi) pure_freenew(pxi);
        pure_throw(exception);
      }
    }
    px* last = pure_app(pure_app(cons,pxi),nl);
    if (res==nl)
      res = y = last;
    else {
      y->data.x[1] = pure_new(last);
      y = last;
    }
  }
  if (i!=e) {
    pure_freenew(res);
    bad_argument();    
  }
  return res;
}

px* smm_listcatmap(px* fun, px* tpl, int what)
{
  smm_range rng(tpl);
  if (!rng.is_valid) bad_argument();
  smm* smmp = rng.smmp();
  if (smmp->keys_only) what = stl_smm_key;
  pmmi i = rng.beg(); 
  pmmi e = rng.end(); 
  pmmi end = smmp->mmp.end();
  px* cons = px_cons_sym();
  px* nl = pure_listl(0);
  px* res = nl;
  px* y = 0;
  px* exception;
  px* *elms;
  size_t sz;
  for (;i != e && i != end; i++){
    px* trg = get_elm_aux(smmp, i, what);
    px* pxi = pure_appxl(fun, &exception, 1, trg);
    if (exception) {
      if (res) pure_freenew(res);
      if (pxi) pure_freenew(pxi);
      pure_throw(exception);
    }
    if ( !pure_is_listv(pxi, &sz, &elms) ){
      pure_freenew(pxi);
      if (res) pure_freenew(res);
      bad_argument();      
    }
    for (int j = 0; j < sz; j++) {
      px* last = pure_app(pure_app(cons,elms[j]),nl);
      if (res==nl)
        res = y = last;    
      else {
        y->data.x[1] = pure_new(last);
        y = last;
      }
    }
    pure_freenew(pxi);
    free(elms);
  }
  if (i!=e) {
    pure_freenew(res);
    bad_argument();    
  }
  return res;  
}

px* smm_foldl(px* fun, px* val, px* tpl)
{
  smm_range rng(tpl);
  if (!rng.is_valid) bad_argument();
  smm* smmp = rng.smmp();
  int mode =  smmp->keys_only ? stl_smm_key : stl_smm_elm;
  pmmi end = smmp->mmp.end();
  try {
    px* ret = smm_foldl_rng(fun, val, rng, rng.beg(), mode);
    return ret;
  } catch (px* e) {
    pure_throw(e);
  }
}

px* smm_foldl1(px* fun, px* tpl)
{
  smm_range rng(tpl);
  if ( !rng.is_valid ) bad_argument();
  smm* smmp = rng.smmp();
  int mode =  smmp->keys_only ? stl_smm_key : stl_smm_elm;
  pmmi end = smmp->mmp.end();
  pmmi b = rng.beg();
  pmmi e = rng.end();
  if ( b==e || b==end ) bad_argument();
  px* val;
  if (mode == stl_smm_key)
    val = b->first;
  else
    val = pxlhs_pxrhs_to_pxrocket(b->first, b->second);
  try {
    return smm_foldl_rng(fun, val, rng, ++b, mode);
  } catch (px* e) {
    pure_throw(e);
  }
}

px* smm_foldr(px* fun, px* val, px* tpl)
{
  smm_range rng(tpl);
  if (!rng.is_valid) bad_argument();
  int mode = rng.smmp()->keys_only ? stl_smm_key : stl_smm_elm;
  try {
    px* ret = smm_foldr_rng(fun, val, rng, rng.end(), mode);
    return ret;
  } catch (px* x) {
    pure_throw(x);
  }
}

px* smm_foldr1(px* fun, px* tpl)
{
  smm_range rng(tpl);
  if ( !rng.is_valid ) bad_argument();
  smm* smmp = rng.smmp();
  int mode =  smmp->keys_only ? stl_smm_key : stl_smm_elm;
  pmmi beg = smmp->mmp.begin();
  pmmi end = smmp->mmp.end();
  pmmi b = rng.beg();
  pmmi e = rng.end();
  if ( b==e || b==end || e==beg) bad_argument();
  e--;
  px* val;
  if (mode == stl_smm_key)
    val = e->first;
  else
    val = pxlhs_pxrhs_to_pxrocket(e->first, e->second);
  try {
    return smm_foldr_rng(fun, val, rng, e, mode);
  } catch (px* x) {
    pure_throw(x);
  }
}

void smm_do(px* fun, px* tpl)
{ 
  smm_range rng(tpl);
  if ( !rng.is_valid ) bad_argument();
  smm* smmp = rng.smmp();
  int mode =  smmp->keys_only ? stl_smm_key : stl_smm_elm;
  pmmi i = rng.beg();
  pmmi e = rng.end();
  px* exception = 0;
  while (i != e) {
    px* trg = get_elm_aux(smmp, i++, mode);
    px* fx = pure_appxl(fun, &exception, 1, trg);
    pure_freenew(fx);
    if (exception) pure_throw(exception);
  }
}

/*** Key oriented interface support ***************************************/

int smm_member(px* pxsmmp, px* key)
{
  smm* smmp;
  if (!get_smmp(pxsmmp,&smmp) ) bad_argument();
  int ret = 0;
  pxhmmap& mmp = smmp->mmp;
  pmmi i;
  if (!mmp.empty()) {
    if (smmp->get_cached_pmmi(key, i) ) {
      ret = 1;
    }
    else {
      i = smmp->find(key);
      if (i != mmp.end()) {
        smmp->cache_pmmi(i);
        ret = 1;
      }
    }
  }
  return ret;
}

px* smm_bounding_keys(px* tpl)
{
  smm_range rng(tpl);
  if (!rng.is_valid) bad_argument;
  pxhmmap& mmp = rng.smmp()->mmp;
  return pure_tuplel(2,iter_to_key(mmp, rng.beg()),
                       iter_to_key(mmp, rng.end())); 
}

px* smm_prev_key(px* pxsmmp, px* key)
{
  smm* smmp;
  if (!get_smmp(pxsmmp,&smmp) ) bad_argument();
  pxhmmap& mmp = smmp->mmp;
  if (mmp.empty()) index_error();
  pxhmmap::key_compare k_cmp = mmp.key_comp();
  pmmi i = mmp.end();
  if ( !smmp->get_cached_pmmi(key,i) )
    i = smmp->find(key);
  for (;;) {
    if ( i == mmp.begin() || i==mmp.end() && key != smend() )
      index_error();
    else {
      i--;
      if ( key==smend() || k_cmp(i->first,key) ) break;
    }
  }
  smmp->cache_pmmi(i);
  return iter_to_key(mmp, i);
}

px* smm_next_key(px* pxsmmp, px* key)
{
  smm* smmp;
  if (!get_smmp(pxsmmp,&smmp) ) bad_argument();
  pxhmmap& mmp = smmp->mmp;
  pxhmmap::key_compare k_cmp = mmp.key_comp();
  pmmi i = mmp.end();
  if (mmp.empty()) index_error();
  if (key == smend()) return key;
  if ( !smmp->get_cached_pmmi(key,i) )
    i = smmp->find(key);
  if (i==mmp.end()) index_error();
  for (;;) {
    i++;
    if ( key==smbeg() || i==mmp.end() || k_cmp(key,i->first) ) break;
  }
  smmp->cache_pmmi(i);
  return iter_to_key(mmp, i);
}

px* smm_update(px* pxsmmp, px* k, px* src)
{
  smm* smmp;
  if (!get_smmp(pxsmmp,&smmp) ) bad_argument();  
  if (smmp->keys_only) return 0;
  pxhmmap& mmp = smmp->mmp;
  pmmi trgi = get_iter(smmp, k, gi_lower);
  pmmi ub = get_iter(smmp, k, gi_upper);  
  size_t src_sz = 0;
  px** elems = NULL;
  bool ok;
  int i;
  if (pure_is_listv(src, &src_sz, &elems)) {
    for (i = 0; i<src_sz && trgi != ub; i++, trgi++)
      trgi->second = elems[i];
    if (i <src_sz)
      for (; i<src_sz; i++)
        mmp.insert(pxhpair(k,elems[i]));
    else
      smmp->erase(trgi,ub);
    free(elems);
  } 
  else if (matrix_type(src) == 0) {
    src_sz = matrix_size(src); 
    elems = (pure_expr**) pure_get_matrix_data(src);
    for (i = 0; i<src_sz && trgi != ub; i++, trgi++)
      trgi->second = elems[i];
    if (i <src_sz)
      for (; i<src_sz; i++)
        mmp.insert(pxhpair(k,elems[i]));
    else
      smmp->erase(trgi,ub);
  }
  return pxsmmp;
}
