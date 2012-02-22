/* stlhmap.cpp -- C++ support for stlhmap.pure

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
#include "stlhmap.hpp"

using namespace std;

static int stlhmap_tag() 
{
  static ILS<int> _t = 0; int &t = _t();
  if (!t) t = pure_pointer_tag("stlhmap*");
  return t;
}

static px* pxpair_to_pxrocket(const pxp_pair& pair)
{
  return pxlhs_pxrhs_to_pxrocket(pair.first, pair.second);
}

static px* pxpair_to_pxlhs(const pxp_pair& pair)
{
  return pair.first;
}

/*** stlhmap member functions ********************************************/

stlhmap::stlhmap(bool ko) : keys_only(ko)
{
  // PR(stlhmap,ko);
}; 

stlhmap::~stlhmap()
{
  free_elms();
}

void stlhmap::free_elms()
{
  for (pxhmapi i = hm.begin(); i != hm.end(); i++){
    pure_free(i->first);
    if (i->second) pure_free(i->second);
  }
}

void stlhmap::refc_elms()
{  
  for (pxhmapi i = hm.begin(); i != hm.end(); i++){
    pure_new(i->first);
    if (i->second) pure_new(i->second);
  }
}

/*** Helpers *************************************************************/

static bool extract_kv(sh* shp, px* kv, px*& k, px*& v)
{
  bool ok = true;
  if (shp->keys_only) {
    k = kv;
    v = NULL;
  } 
  else {
    ok = pxrocket_to_pxlhs_pxrhs(kv, &k, &v);
  }
  return ok;
}

// increases inserted by 1 iff inserted a (k=>val) elm or changed existing val
static bool insert_aux(sh* shp,px* kv, pxhmapi& pos,int& inserted,bool replace)
{
  px *k, *v;
  bool ok = extract_kv(shp,kv,k,v);
  if (ok) {
    pair<pxhmapi,bool> i_ok = shp->hm.insert(make_pair(k,v));
    pos = i_ok.first;
    if (i_ok.second) {
      pure_new(k);
      if (v) pure_new(v);
      inserted++;
    } 
    else if (replace) {
      pos->second = v;
      if (v) pure_new(v);
      inserted++;
    }
  }
  return ok;
}

static px* px_pointer(sh* shp)
{
  static ILS<px*> _sym = NULL; px*& sym = _sym();
  if (!sym) sym = pure_new(pure_symbol(pure_sym("stl::sh_delete")));
  px* ptr = pure_tag( stlhmap_tag(), pure_pointer(shp));
  return pure_sentry(sym,ptr);
}

static bool get_shp(px* pxshp, sh** shpp)
{
  void* ptr;
  bool ok = false;
  if ( pure_is_pointer(pxshp, &ptr) ) {
    int tag = pure_get_tag(pxshp);
    ok = tag == stlhmap_tag();
  }
  *shpp = ok ? (sh*)ptr : NULL; 
  return ok;
}

static px* get_elm_aux(sh* shp, pxhmapi i, int what) 
{
  px* ret;
  pxhmap &hm = shp->hm; 
  if (i != hm.end()) {
    switch (what) {
    case stl_sh_key:
      ret = i->first;
      break;
    case stl_sh_val: 
      if (shp->keys_only)
        ret = i->first;
      else
        ret = i->second; 
      break;
    case stl_sh_elm:
       if (shp->keys_only)
        ret = i->first;
      else 
        ret = pure_appl(px_rocket_sym(),2,i->first,i->second);
       break;
    }
  }
  else {
    index_error();
  }
  return ret;
}

static px* sh_foldl_aux(px* fun, px* val, sh* shp, int is_foldl1)
{ 
  int mode =  shp->keys_only ? stl_sh_key : stl_sh_elm;
  pxhmapi e = shp->hm.end();
  pxhmapi i = shp->hm.begin();
  px* res = pure_new(val); 
  px* exception = 0;
  if (is_foldl1) i++;
  while (i != e) {
    pxhmapi trg_i = i++;
    px* trg = get_elm_aux(shp, trg_i, mode);
    px* fxy = pure_appxl(fun, &exception, 2, res, trg);
    if (exception) {
      pure_freenew(res);
      pure_throw(exception);
    }
    pure_new(fxy);
    pure_free(res);
    res = fxy;
  }
  pure_unref(res);
  return res;
}

/*** Pure interface support functions  *************************************/

px* sh_type_tags()
{
  return pure_int(stlhmap_tag());
}

px*  sh_make_empty(int keys_only)
{
  return px_pointer( new sh(keys_only) );
}

px* sh_copy(px* pxshp)
{
  sh* shp;
  if (!get_shp(pxshp,&shp) ) bad_argument();
  pxhmap& hm = shp->hm;
  sh* cpy = new sh(*shp);
  cpy->refc_elms();
  return px_pointer( cpy );
}

void sh_delete(sh* shp){
  delete(shp);
}

void sh_reserve(px* pxshp, double max_load, int count)
{
  sh* shp;
  if (!get_shp(pxshp,&shp) ) bad_argument();
  pxhmap& hm = shp->hm;  
  if (max_load > 0.0) 
    hm.max_load_factor(max_load);
  if (count > 0)
    hm.reserve(count);
}

px* sh_info(px* pxshp)
{
  sh* shp;
  if (!get_shp(pxshp,&shp) ) bad_argument();
  pxhmap& hm = shp->hm;
  px* ko = pure_int(shp->keys_only);
  px* bc = pure_int( hm.bucket_count() );
  px* lf = pure_double( hm.load_factor() );
  px* mlf = pure_double( hm.max_load_factor() );
  return pure_tuplel(4,ko,bc,lf,mlf);
}

int sh_bucket_size(px* pxshp, int i)
{
  sh* shp;
  if (!get_shp(pxshp,&shp) ) bad_argument();
  pxhmap& hm = shp->hm;
  if (i<0 || i>=hm.bucket_count()) bad_argument();
  return  hm.bucket_size(i);
}

int sh_size(px* pxshp)
{
  sh* shp;
  if (!get_shp(pxshp,&shp) ) bad_argument();
  return shp->hm.size();
}

bool sh_empty(px* pxshp)
{
  sh* shp;
  if (!get_shp(pxshp,&shp) ) bad_argument();
  return shp->hm.empty();
}

int  sh_count(px* pxshp, px* key)
{
  sh* shp;
  if ( !get_shp(pxshp,&shp) ) bad_argument();
  return shp->hm.count(key);
}

bool sh_is_set(px* pxshp)
{
  sh* shp;
  if (!get_shp(pxshp,&shp) ) bad_argument();
  return shp->keys_only;
}

px* sh_get(sh* shp, px* key)
{
  pxhmap &hm = shp->hm;
  pxhmapi i = hm.find(key);
  if (i==hm.end()) index_error();
  return (shp->keys_only) ? i->first : i->second;
}

px* sh_find(px* pxshp, px* key, int what)
{
  sh* shp;
  if ( !get_shp(pxshp,&shp) ) bad_argument();
  pxhmapi i = shp->hm.find(key);
  return get_elm_aux(shp, i, what);
}

int sh_insert(px* pxshp, px* src, bool replace)
{
  sh* shp; pxhmapi pos;
  if (!get_shp(pxshp,&shp) ) bad_argument();
  size_t sz = 0;
  px** elms = NULL;
  bool ok;
  int num_inserted = 0;
  try {
    if (pure_is_listv(src, &sz, &elms)) {
      for (int i = 0; i<sz; i++) {
        if ( !insert_aux(shp, elms[i], pos, num_inserted, replace) )
          bad_argument();
      }
      free(elms);
    } else if (matrix_type(src) == 0) {
      sz = matrix_size(src); 
      px** hmelms = (pure_expr**) pure_get_matrix_data(src);
      for (int i = 0; i<sz; i++) {
        if ( !insert_aux(shp, hmelms[i], pos, num_inserted, replace) ) 
          bad_argument();
      }
    } else
      if ( !insert_aux(shp, src, pos, num_inserted, replace) ) 
        bad_argument();
  }
  catch (px* e){
    free(elms);
    pure_throw(e);
  }
  return num_inserted;
}

int sh_insert_stlhmap(px* pxshp1, px* pxshp2, bool replace)
{
  sh *shp1, *shp2;
  if (!get_shp(pxshp1,&shp1) ) bad_argument();
  if (!get_shp(pxshp2,&shp2) ) bad_argument();
  pxhmap& hmp1 = shp1->hm;
  pxhmap& hmp2 = shp2->hm;
  size_t oldsz = hmp1.size();
  try {
    if (replace) {
      int num_inserted = 0; 
      for (pxhmapi i = hmp2.begin(); i!=hmp2.end(); i++) {
        pair<pxhmapi,bool> i_ok = hmp1.insert(*i);
        if (!i_ok.second)
          (i_ok.first)->second = i->second;
        pure_new(i->second);
        num_inserted++;
      }
      return num_inserted;
    }
    else {
      int oldsz = hmp1.size();
      hmp1.insert(hmp2.begin(),hmp2.end());
      return hmp1.size() - oldsz;
    } 
  }
  catch (px* e) {
    pure_throw(e);
  }
  return hmp1.size() - oldsz;
}

int sh_insert_stlvec(px* pxshp, sv* svp, bool replace)
{
  int num_inserted = 0;
  sh* shp; 
  pxhmapi pos;
  if (!get_shp(pxshp,&shp) ) bad_argument();
  try {
    for (sv::iterator i = svp->begin(); i!=svp->end(); i++) {
      if ( !insert_aux(shp, *i, pos, num_inserted, replace) )
        bad_argument();
    }
  }
  catch (px* e) {
    pure_throw(e);
  }
  return num_inserted;
}

px*  sh_insert_stlhmap(px* pxshp, bool replace)
{
  sh* shp;
  if (!get_shp(pxshp,&shp) ) bad_argument();
  pxhmap& hm = shp->hm;
  sh* cpy = new sh(*shp);
  cpy->refc_elms();
  return px_pointer( cpy );
}

px*  sh_swap(px* pxshp1, px* pxshp2)
{
  sh* shp1; sh* shp2;
  if ( !get_shp(pxshp1, &shp1) ) failed_cond();
  if ( !get_shp(pxshp2, &shp2) ) failed_cond();
  shp1->hm.swap(shp2->hm);
}

int sh_clear(px* pxshp)
{
  sh* shp;
  if (!get_shp(pxshp,&shp) ) bad_argument();
  size_t sz = shp->hm.size();
  shp->free_elms();
  shp->hm.clear();
  return sz;
}

int sh_erase(px* pxshp, px* key)
{
  sh* shp;
  if (!get_shp(pxshp,&shp) ) bad_argument();
  pxhmap& hm = shp->hm;
  int ret = 0;
  pxhmapi i = hm.find(key);
  if ( i != hm.end() ) {
    try {
      pure_free(i->first); 
      if (i->second) pure_free(i->second);
      hm.erase(i);
      ret = 1;
    } 
    catch (px* e) {
      pure_throw(e);
    }
  }
  return ret;
}

bool sh_equal(px* pxshp1, px* pxshp2)
{
  sh *shp1, *shp2;
  if ( !get_shp(pxshp1,&shp1) || !get_shp(pxshp2,&shp2) ) bad_argument();
  pxhmap& hmp1 = shp1->hm;
  pxhmap& hmp2 = shp2->hm;
  px_pair_same eql;
  try {
    return equal(hmp1.begin(), hmp1.end(), hmp2.begin(),eql);
  }
  catch (px* e) {
    pure_throw(e);
  }
  return true;
}

px* sh_make_vector(px* pxshp) 
{
  sh* shp;
  if (!get_shp(pxshp,&shp) ) bad_argument();
  pxhmap& hm = shp->hm;
  pxhmapi b = hm.begin();
  pxhmapi e = hm.end();
  int sz = hm.size();
  if (!sz)
    return pure_matrix_columnsv(0,NULL);
  px** bfr = (px**)malloc(sizeof(px*)*sz);
  if (shp->keys_only) 
    transform(b, e, bfr, pxpair_to_pxlhs);
  else
    transform(b, e, bfr, pxpair_to_pxrocket);
  px* ret = pure_matrix_columnsv(sz, bfr);
  free(bfr);
  return ret;
}

void sh_fill_stlvec(px* pxshp, sv* svp) 
{
  sh* shp;
  if ( !get_shp(pxshp,&shp) ) bad_argument();
  pxhmapi b = shp->hm.begin();
  pxhmapi e = shp->hm.end();
  if (shp->keys_only) 
    transform(b, e, back_inserter(*svp), pxpair_to_pxlhs);
  else
    transform(b, e, back_inserter(*svp), pxpair_to_pxrocket);
}

/*** Mapping and folding ***********************************************/

px* sh_listmap(px* fun, px* pxshp, int what)
{
  sh* shp;
  if (!get_shp(pxshp,&shp) ) bad_argument();
  if (shp->keys_only) what = stl_sh_key;
  pxhmap& hm = shp->hm;
  pxhmapi b = hm.begin();
  pxhmapi e = hm.end();
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
  pxhmapi i = b;
  for (;i != e; i++){
    px* trg = get_elm_aux(shp, i, what);
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
  return res;
}

px* sh_listcatmap(px* fun, px* pxshp, int what)
{
  sh* shp;
  if (!get_shp(pxshp,&shp) ) bad_argument();
  if (shp->keys_only) what = stl_sh_key;
  pxhmap& hm = shp->hm;
  pxhmapi i = hm.begin();
  pxhmapi e = hm.end();
  px* cons = px_cons_sym();
  px* nl = pure_listl(0);
  px* res = nl;
  px* y = 0;
  px* exception;
  px* *elms;
  size_t sz;
  for (;i != e; i++){
    px* trg = get_elm_aux(shp, i, what);
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
  return res;  
}

px* sh_foldl(px* fun, px* val, px* pxshp)
{
  sh* shp;
  if ( !get_shp(pxshp,&shp) ) bad_argument();
  return sh_foldl_aux(fun, val, shp, 0);
}

px* sh_foldl1(px* fun, px* pxshp)
{
  sh* shp;
  if ( !get_shp(pxshp,&shp) ) bad_argument();
  pxhmapi b = shp->hm.begin();
  pxhmapi e = shp->hm.end();
  if ( b==e )  bad_argument();
  int mode =  shp->keys_only ? stl_sh_key : stl_sh_elm;
  px* val = get_elm_aux(shp, b, mode);
  return sh_foldl_aux(fun, val, shp, 1);
}

void sh_do(px* fun, px* pxshp)
{ 
  sh* shp;
  if ( !get_shp(pxshp,&shp) ) bad_argument();
  int mode =  shp->keys_only ? stl_sh_key : stl_sh_elm;
  pxhmapi e = shp->hm.end();
  pxhmapi i = shp->hm.begin();
  px* exception = 0;
  while (i != e) {
    px* trg = get_elm_aux(shp, i++, mode);
    px* fx = pure_appxl(fun, &exception, 1, trg);
    if (exception)
       pure_throw(exception);
    pure_freenew(fx);
  }
}

/*** Key oriented interface support ***************************************/

int sh_member(sh* shp, px* key)
{
  pxhmap &hm = shp->hm;
  return hm.find(key) != hm.end();
}

void sh_put(sh* shp, px* key, px* val)
{
  if (shp->keys_only) bad_argument();
  pxhmap& hm = shp->hm;
  pxhmapi pos = hm.find(key);
  if ( pos == hm.end() ) index_error();
  pos->second = val;
  if (val) pure_new(val);
}
