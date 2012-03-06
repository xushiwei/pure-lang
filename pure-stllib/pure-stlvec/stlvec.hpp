/* stlvec.hpp -- C++ support for stlvec.pure
    
Copyright (c) 2011-2012 by Peter Summerland <p.summerland@gmail.com>.

All rights reserved.

This software is is part of pure-stlvec, an addon to the Pure Programming
Language (http://code.google.com/p/pure-lang/).

This software is distributed under a BSD-style license in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the COPYING file
included with the pure-stlvec distribution package for details.

*/

#ifndef STLVEC_H
#define STLVEC_H

#include <iostream>
#include <vector>
#include "stlbase.hpp"

/* sv, svi and friends - A sv is a vector of pxh's. It is the data structure
   pointed to by a stlvec on the Pure side of the interface. A svi is an
   iterator on a sv.*/

typedef std::vector<pxh> sv; 
typedef sv::iterator svi;
typedef sv::const_iterator const_svi;
typedef sv::reverse_iterator reverse_svi;
typedef sv::const_reverse_iterator const_reverse_svi;

const int svbeg  =  0;
const int svend  = -1; 
const int svback = -2;  // back_insert_iterator
const int svrev  = -3;  // request reversal of iterators

const int sv_max_num_iters = 3;

struct sv_range {
  sv* vec;
  svi iters[sv_max_num_iters];
  int num_iters;
  int sz;
  bool is_reversed;
  bool is_valid;

  sv_range(px* tpl);
  svi beg(){return iters[0];}
  svi mid(){return iters[1];}
  svi end(){return num_iters > 2 ? iters[2] : iters[1];}
  int size();
  bool contains(sv*, const svi&);
  bool overlaps(sv_range&);
};

struct sv_back_iter{
  sv* vec;
  bool is_valid;
  sv_back_iter(px* tpl);
};

int iter_pos(sv* vec, svi iter);

/*** C interface for vector of PX Handles ***/

extern "C" {
  sv*  stl_sv_make_from_xs(px* xs);
  sv*  stl_sv_make_n(px* xs, int n);
  sv*  stl_sv_dup(px* tpl);
  px*  stl_sv_vector(px* it);
  void stl_sv_reserve(sv* vec, int n);
  int  stl_sv_size(sv* vec);
  int  stl_sv_iter_size(px* tpl);
  bool stl_sv_empty(sv* vec);
  int  stl_sv_capacity(sv* vec);

  px*  stl_sv_get(sv* vec, int pos);
  void stl_sv_put(sv* vec, int pos, px* val);
  px*  stl_sv_front(sv* vec);
  px*  stl_sv_back(sv* vec);
  px*  stl_sv_list(px* tpl);

  void stl_sv_push_back(sv* vec, px* val);
  void stl_sv_splice(sv* vec, int b, px* xs_or_tpl);
  void stl_sv_pop_back(sv* vec);
  void stl_sv_erase(px* tpl);
  void stl_sv_clear(sv* vec);
  bool stl_sv_allpairs(px* fun, px* tpl1, px* tpl2);

  px*  stl_sv_listmap(px* fun, px* tpl);
  px*  stl_sv_listcatmap(px* fun, px* tpl);
  px*  stl_sv_foldl(px* fun, px* val, px* tpl);
  px*  stl_sv_foldl1(px* fun, px* tpl);
  px*  stl_sv_foldr(px* fun, px* val, px* tpl);
  px*  stl_sv_foldr1(px* fun, px* tpl);

}

#endif // STLVEC_H
