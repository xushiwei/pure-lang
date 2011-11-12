/* sv_minmax.cpp -- C++ support for sv_minmax.pure
    
Copyright (c) 2011 by Peter Summerland <p.summerland@gmail.com>.

All rights reserved.

This software is is part of pure-stlvec, an addon to the Pure Programming
Language (http://code.google.com/p/pure-lang/).

This software is distributed under a BSD-style license in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the COPYING file
included with the pure-stlvec distribution package for details.

*/


#include "stlvec.hpp"
#include "sv_algorithm.hpp"
#include <algorithm>

using namespace std;

int  sva_min_element(px* tpl, px* cmp)
{
  sv_range rng(tpl);
  if (!rng.is_valid || !rng.num_iters == 2) bad_argument();
  try {
    pxh_pred2 fun(cmp);
    svi p = min_element(rng.beg(), rng.end(), fun);
    return iter_pos(rng.vec,p);
  } catch (px* e) {
    pure_throw(e);
  }
}

int  sva_max_element(px* tpl, px* cmp)
{
  sv_range rng(tpl);
  if (!rng.is_valid || !rng.num_iters == 2) bad_argument();
  try {
    pxh_pred2 fun(cmp);
    svi p = max_element(rng.beg(), rng.end(), fun);
    return iter_pos(rng.vec,p);
  } catch (px* e) {
    pure_throw(e);
  }
}

bool sva_lexicographical_compare(px* tpl1, px* tpl2, px* cmp)
{
  pxh_pred2 fun(cmp);
  sv_range rng1(tpl1);
  sv_range rng2(tpl2);
  if (!rng1.is_valid || rng1.num_iters != 2) bad_argument();
  if (!rng2.is_valid || rng2.num_iters != 2) bad_argument();
  try {
    return lexicographical_compare(rng1.beg(), rng1.end(),
                                   rng2.beg(), rng2.end(), fun);
  } catch (px* e) {
    pure_throw(e);
  }
}

bool sva_next_permutation(px* tpl, px* cmp)
{
  pxh_pred2 fun(cmp);
  sv_range rng(tpl);
  if (!rng.is_valid || rng.num_iters != 2) bad_argument();
  try {
    return next_permutation(rng.beg(), rng.end(), fun);
  } catch (px* e) {
    pure_throw(e);
  }
}

bool sva_prev_permutation(px* tpl, px* cmp)
{
  pxh_pred2 fun(cmp);
  sv_range rng(tpl);
  if (!rng.is_valid || rng.num_iters != 2) bad_argument();
  try {
    return prev_permutation(rng.beg(), rng.end(), fun);
  } catch (px* e) {
    pure_throw(e);
  }
}
