
// This uses unordered_map, so a recent STL is required for now.
// TODO: hash policy operations, multimap interface.

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <pure/runtime.h>
#include <unordered_map>

using namespace std;

// Hashing and comparing Pure expressions. The required functionality is in
// the Pure runtime (hash() and same() functions, see pure/runtime.h).

namespace std {
  template<>
  struct hash<pure_expr*>
  {
    size_t operator()(pure_expr* x) const
    { return ::hash(x); };
  };
  template<>
  struct equal_to<pure_expr*> {
    bool operator()(pure_expr* x, pure_expr* y) const
    { return same(x, y); }
  };
}

typedef unordered_map<pure_expr*,pure_expr*> myhashmap;

// Pretty-printing support.

#include <sstream>

static const char *hashmap_str(myhashmap *m)
{
  ostringstream os;
  static char *buf = 0;
  size_t i = 0, n = m->size();
  int32_t fno = pure_getsym("=>"), gno = pure_getsym(",");
  assert(fno > 0 && gno > 0);
  pure_expr *f = pure_new(pure_symbol(fno));
  pure_expr *g = pure_new(pure_symbol(gno));
  int32_t nprec = fixity(g);
  bool init = true;
  pure_free(g);
  if (buf) free(buf);
  os << "{$";
  for (myhashmap::iterator it = m->begin(); it != m->end(); ++it)
    if (it->second) {
      pure_expr *x = pure_appl(f, 2, it->first, it->second);
      char *s = str(x);
      pure_freenew(x);
      if (init)
	init = false;
      else
	os << ",";
      os << s;
      free(s);
    } else {
      pure_expr *x = it->first, *y;
      int32_t nprec2 = NPREC_MAX;
      char *s = str(x);
      if (init)
	init = false;
      else
	os << ",";
      if (pure_is_app(x, &g, &y))
	if (pure_is_symbol(g, &gno))
	  nprec2 = fixity(g);
	else if (pure_is_app(g, &g, &y))
	  nprec2 = fixity(g);
      if (nprec2 <= nprec)
	os << "(" << s << ")";
      else
	os << s;
      free(s);
    }
  os << "$}";
  pure_free(f);
  buf = strdup(os.str().c_str());
  return buf;
}

// Pointer type tag.

extern "C" int hashmap_tag(void)
{
  static int t = 0;
  if (!t) {
    t = pure_pointer_tag("hashmap*");
    pure_pointer_add_printer(t, (pure_printer_fun)hashmap_str, 0);
  }
  return t;
}

// Basic interface functions.

extern "C" void hashmap_free(myhashmap *m)
{
  for (myhashmap::iterator it = m->begin(); it != m->end(); ++it) {
    pure_free(it->first);
    if (it->second) pure_free(it->second);
  }
  delete m;
}

static pure_expr *make_hashmap(myhashmap *m)
{
  return pure_sentry(pure_symbol(pure_sym("hashmap_free")),
                     pure_tag(hashmap_tag(), pure_pointer(m)));
}

extern "C" void hashmap_add(myhashmap *m, pure_expr *key);
extern "C" void hashmap_add2(myhashmap *m, pure_expr *key, pure_expr *val);

extern "C" pure_expr *hashmap(pure_expr *xs)
{
  size_t n;
  pure_expr **xv;
  if (!pure_is_listv(xs, &n, &xv)) return 0;
  int32_t fno = pure_getsym("=>"), gno;
  assert(fno > 0);
  myhashmap *m = new myhashmap;
  for (size_t i = 0; i < n; i++) {
    pure_expr *f, *g, *key, *val;
    if (pure_is_app(xv[i], &f, &val) && pure_is_app(f, &g, &key) &&
	pure_is_symbol(g, &gno) && gno == fno)
      hashmap_add2(m, key, val);
    else
      hashmap_add(m, xv[i]);
  }
  if (xv) free(xv);
  return make_hashmap(m);
}

extern "C" void hashmap_add(myhashmap *m, pure_expr *key)
{
  myhashmap::iterator it = m->find(key);
  if (it != m->end()) {
    if (it->second) pure_free(it->second);
  } else
    pure_new(key);
  (*m)[key] = 0;
}

extern "C" void hashmap_add2(myhashmap *m, pure_expr *key, pure_expr *val)
{
  myhashmap::iterator it = m->find(key);
  if (it != m->end()) {
    if (it->second) pure_free(it->second);
  } else
    pure_new(key);
  (*m)[key] = pure_new(val);
}

extern "C" void hashmap_del(myhashmap *m, pure_expr *key)
{
  myhashmap::iterator it = m->find(key);
  if (it != m->end()) {
    pure_free(it->first);
    if (it->second) pure_free(it->second);
    m->erase(it);
  }
}

extern "C" pure_expr *hashmap_get(myhashmap *m, pure_expr *key)
{
  myhashmap::iterator it = m->find(key);
  return (it != m->end())?(it->second?it->second:it->first):0;
}

extern "C" bool hashmap_member(myhashmap *m, pure_expr *key)
{
  myhashmap::iterator it = m->find(key);
  return it != m->end();
}

extern "C" bool hashmap_empty(myhashmap *m)
{
  return m->empty();
}

extern "C" int hashmap_size(myhashmap *m)
{
  return m->size();
}

extern "C" pure_expr *hashmap_keys(myhashmap *m)
{
  size_t i = 0, n = m->size();
  pure_expr **xs = new pure_expr*[n];
  for (myhashmap::iterator it = m->begin(); it != m->end(); ++it)
    xs[i++] = it->first;
  pure_expr *x = pure_listv(n, xs);
  delete[] xs;
  return x;
}

extern "C" pure_expr *hashmap_vals(myhashmap *m)
{
  size_t i = 0, n = m->size();
  pure_expr **xs = new pure_expr*[n];
  for (myhashmap::iterator it = m->begin(); it != m->end(); ++it)
    xs[i++] = it->second?it->second:it->first;
  pure_expr *x = pure_listv(n, xs);
  delete[] xs;
  return x;
}

extern "C" pure_expr *hashmap_list(myhashmap *m)
{
  size_t i = 0, n = m->size();
  int32_t fno = pure_getsym("=>");
  assert(fno > 0);
  pure_expr **xs = new pure_expr*[n], *f = pure_new(pure_symbol(fno));
  for (myhashmap::iterator it = m->begin(); it != m->end(); ++it)
    xs[i++] = it->second?pure_appl(f, 2, it->first, it->second):it->first;
  pure_expr *x = pure_listv(n, xs);
  delete[] xs;
  pure_free(f);
  return x;
}

extern "C" myhashmap *hashmap_copy(myhashmap *m)
{
  myhashmap *m2 = new myhashmap(*m);
  for (myhashmap::iterator it = m2->begin(); it != m2->end(); ++it) {
    pure_new(it->first); if (it->second) pure_new(it->second);
  }
  return m2;
}

extern "C" void hashmap_clear(myhashmap *m)
{
  for (myhashmap::iterator it = m->begin(); it != m->end(); ++it) {
    pure_free(it->first);
    if (it->second) pure_free(it->second);
  }
  m->clear();
}
