
#include "symtable.hh"
#include "util.hh"
#include <assert.h>

symtable::symtable()
  : fno(0), rtab(1024),	current_namespace(new string),
    search_namespaces(new set<string>), __show__sym(0)
{
  // enter any additional predefined symbols here, e.g.:
  //sym_p("-", 6, infixl);
  sym_p("_"); // anonymous variable
}

symtable::~symtable()
{
  delete current_namespace;
  delete search_namespaces;
}

void symtable::init_builtins()
{
  nil_sym();
  cons_sym();
  void_sym();
  pair_sym();
  not_sym();
  bitnot_sym();
  or_sym();
  and_sym();
  bitor_sym();
  bitand_sym();
  shl_sym();
  shr_sym();
  less_sym();
  greater_sym();
  lesseq_sym();
  greatereq_sym();
  equal_sym();
  notequal_sym();
  plus_sym();
  minus_sym();
  mult_sym();
  fdiv_sym();
  div_sym();
  mod_sym();
  quote_sym();
  catch_sym();
  catmap_sym();
  rowcatmap_sym();
  colcatmap_sym();
  failed_match_sym();
  failed_cond_sym();
  signal_sym();
  segfault_sym();
  bad_matrix_sym();
  amp_sym();
  complex_rect_sym();
  complex_polar_sym();
}

/* These operations are used internally to look up and create symbols exactly
   as specified (no namespace search). */

inline symbol* symtable::lookup_p(const char *s, int& count)
{
  map<string, symbol>::iterator it = tab.find(s);
  count = it != tab.end();
  if (!count || !visible(it->second))
    return 0;
  else
    return &it->second;
}

inline symbol* symtable::lookup_p(const char *s)
{
  int count;
  return lookup_p(s, count);
}

symbol* symtable::sym_p(const char *s, bool priv)
{
  symbol *_sym = lookup_p(s);
  if (_sym)
    return _sym;
  string id = s;
  _sym = &tab[id];
  if (_sym->f == 0) {
    if ((uint32_t)++fno > rtab.capacity())
      rtab.reserve(rtab.capacity()+1024);
    *_sym = symbol(id, fno, priv);
    //cout << "new symbol " << _sym->f << ": " << _sym->s << endl;
    rtab[fno] = _sym;
    if (__show__sym == 0 && strcmp(s, "__show__") == 0) __show__sym = fno;
    return _sym;
  } else
    return 0;
}

symbol* symtable::sym_p(const char *s, prec_t prec, fix_t fix, bool priv)
{
  assert(prec <= 10);
  symbol *_sym = lookup_p(s);
  if (_sym)
    return _sym;
  string id = s;
  _sym = &tab[id];
  if (_sym->f == 0) {
    if ((uint32_t)++fno > rtab.capacity())
      rtab.reserve(rtab.capacity()+1024);
    *_sym = symbol(id, fno, prec, fix, priv);
    //cout << "new symbol " << _sym->f << ": " << _sym->s << endl;
    rtab[fno] = _sym;
    if (__show__sym == 0 && strcmp(s, "__show__") == 0) __show__sym = fno;
    return _sym;
  } else
    return 0;
}

symbol* symtable::lookup(const char *s)
{
  const char *t = strstr(s, "::");
  if (t) {
    // normalize symbols in the default namespace
    if (t == s) s+=2;
    symbol *sym = lookup_p(s, count);
    return sym;
  }
  symbol *default_sym = lookup_p(s), *search_sym = 0;
  count = 0;
  if (strcmp(s, "_") == 0) {
    // anonymous variable is always taken as is
    count = default_sym!=0;
    return default_sym;
  }
  // first look for a symbol in the current namespace
  if (!current_namespace->empty()) {
    string id = (*current_namespace)+"::"+s;
    symbol *sym = lookup_p(id.c_str());
    if (sym) {
      count = 1;
      return sym;
    }
  }
  // next scan the search namespaces; if the symbol is ambiguous, bail out
  // with an error here
  for (set<string>::iterator it = search_namespaces->begin(),
	 end = search_namespaces->end(); it != end; it++) {
    string id = (*it)+"::"+s;
    symbol *sym = lookup_p(id.c_str());
    if (sym && ++count > 1) return 0;
    if (!search_sym) search_sym = sym;
  }
  if (search_sym) return search_sym;
  // fall back to a symbol in the default namespace, if any
  count = default_sym!=0;
  return default_sym;
}

symbol* symtable::sym(const char *s, bool priv)
{
  symbol *_sym = lookup(s);
  if (_sym)
    return _sym;
  else if (count > 1)
    return 0;
  if (s[0] == ':' && s[1] == ':') s+=2;
  string id = s;
  _sym = &tab[id];
  if (_sym->f == 0) {
    if ((uint32_t)++fno > rtab.capacity())
      rtab.reserve(rtab.capacity()+1024);
    *_sym = symbol(id, fno, priv);
    //cout << "new symbol " << _sym->f << ": " << _sym->s << endl;
    rtab[fno] = _sym;
    if (__show__sym == 0 && strcmp(s, "__show__") == 0) __show__sym = fno;
    count = 1;
    return _sym;
  } else
    // the symbol already exists, but isn't visible in the current namespace
    return 0;
}

symbol* symtable::sym(const char *s, prec_t prec, fix_t fix, bool priv)
{
  assert(prec <= 10);
  symbol *_sym = lookup(s);
  if (_sym)
    return _sym;
  else if (count > 1)
    return 0;
  if (s[0] == ':' && s[1] == ':') s+=2;
  string id = s;
  _sym = &tab[id];
  if (_sym->f == 0) {
    if ((uint32_t)++fno > rtab.capacity())
      rtab.reserve(rtab.capacity()+1024);
    *_sym = symbol(id, fno, prec, fix, priv);
    //cout << "new symbol " << _sym->f << ": " << _sym->s << endl;
    rtab[fno] = _sym;
    if (__show__sym == 0 && strcmp(s, "__show__") == 0) __show__sym = fno;
    count = 1;
    return _sym;
  } else
    // the symbol already exists, but isn't visible in the current namespace
    return 0;
}

symbol& symtable::checksym(const char *s, bool priv)
{
  symbol *_sym = sym(s, priv);
  if (_sym)
    return *_sym;
  else if (count > 1)
    throw err("symbol '"+string(s)+"' is ambiguous here");
  else
    throw err("symbol '"+string(s)+"' is private here");
}

symbol& symtable::sym(int32_t f)
{
  assert(f > 0 && (uint32_t)f < rtab.size());
  return *rtab[f];
}

bool symtable::visible(const symbol& sym)
{
  if (sym.priv) {
    size_t k = sym.s.find("::");
    if (k == string::npos) k = 0;
    string qual = sym.s.substr(0, k);
    return qual.empty() || qual == *current_namespace;
  } else
    return true;
}

symbol& symtable::nil_sym()
{
  symbol *_sym = lookup_p("[]");
  if (_sym)
    return *_sym;
  else
    return *sym_p("[]", 10, nullary);
}

symbol& symtable::cons_sym()
{
  symbol *_sym = lookup_p(":");
  if (_sym)
    return *_sym;
  else
    return *sym_p(":", 4, infixr);
}

symbol& symtable::void_sym()
{
  symbol *_sym = lookup_p("()");
  if (_sym)
    return *_sym;
  else
    return *sym_p("()", 10, nullary);
}

symbol& symtable::pair_sym()
{
  symbol *_sym = lookup_p(",");
  if (_sym)
    return *_sym;
  else
    return *sym_p(",", 1, infixr);
}

symbol& symtable::seq_sym()
{
  symbol *_sym = lookup_p("$$");
  if (_sym)
    return *_sym;
  else
    return *sym_p("$$", 0, infixl);
}

symbol& symtable::not_sym()
{
  symbol *_sym = lookup_p("~");
  if (_sym)
    return *_sym;
  else
    return *sym_p("~", 3, prefix);
}

symbol& symtable::bitnot_sym()
{
  symbol *_sym = lookup_p("not");
  if (_sym)
    return *_sym;
  else
    return *sym_p("not", 7, prefix);
}

symbol& symtable::or_sym()
{
  symbol *_sym = lookup_p("||");
  if (_sym)
    return *_sym;
  else
    return *sym_p("||", 2, infixr);
}

symbol& symtable::and_sym()
{
  symbol *_sym = lookup_p("&&");
  if (_sym)
    return *_sym;
  else
    return *sym_p("&&", 3, infixr);
}

symbol& symtable::bitor_sym()
{
  symbol *_sym = lookup_p("or");
  if (_sym)
    return *_sym;
  else
    return *sym_p("or", 6, infixl);
}

symbol& symtable::bitand_sym()
{
  symbol *_sym = lookup_p("and");
  if (_sym)
    return *_sym;
  else
    return *sym_p("and", 7, infixl);
}

symbol& symtable::shl_sym()
{
  symbol *_sym = lookup_p("<<");
  if (_sym)
    return *_sym;
  else
    return *sym_p("<<", 5, infixl);
}

symbol& symtable::shr_sym()
{
  symbol *_sym = lookup_p(">>");
  if (_sym)
    return *_sym;
  else
    return *sym_p(">>", 5, infixl);
}

symbol& symtable::less_sym()
{
  symbol *_sym = lookup_p("<");
  if (_sym)
    return *_sym;
  else
    return *sym_p("<", 4, infix);
}

symbol& symtable::greater_sym()
{
  symbol *_sym = lookup_p(">");
  if (_sym)
    return *_sym;
  else
    return *sym_p(">", 4, infix);
}

symbol& symtable::lesseq_sym()
{
  symbol *_sym = lookup_p("<=");
  if (_sym)
    return *_sym;
  else
    return *sym_p("<=", 4, infix);
}

symbol& symtable::greatereq_sym()
{
  symbol *_sym = lookup_p(">=");
  if (_sym)
    return *_sym;
  else
    return *sym_p(">=", 4, infix);
}

symbol& symtable::equal_sym()
{
  symbol *_sym = lookup_p("==");
  if (_sym)
    return *_sym;
  else
    return *sym_p("==", 4, infix);
}

symbol& symtable::notequal_sym()
{
  symbol *_sym = lookup_p("~=");
  if (_sym)
    return *_sym;
  else
    return *sym_p("~=", 4, infix);
}

symbol& symtable::plus_sym()
{
  symbol *_sym = lookup_p("+");
  if (_sym)
    return *_sym;
  else
    return *sym_p("+", 6, infixl);
}

symbol& symtable::minus_sym()
{
  symbol *_sym = lookup_p("-");
  if (_sym)
    return *_sym;
  else
    return *sym_p("-", 6, infixl);
}

symbol& symtable::mult_sym()
{
  symbol *_sym = lookup_p("*");
  if (_sym)
    return *_sym;
  else
    return *sym_p("*", 7, infixl);
}

symbol& symtable::fdiv_sym()
{
  symbol *_sym = lookup_p("/");
  if (_sym)
    return *_sym;
  else
    return *sym_p("/", 7, infixl);
}

symbol& symtable::div_sym()
{
  symbol *_sym = lookup_p("div");
  if (_sym)
    return *_sym;
  else
    return *sym_p("div", 7, infixl);
}

symbol& symtable::mod_sym()
{
  symbol *_sym = lookup_p("mod");
  if (_sym)
    return *_sym;
  else
    return *sym_p("mod", 7, infixl);
}

symbol& symtable::amp_sym()
{
  symbol *_sym = lookup_p("&");
  if (_sym)
    return *_sym;
  else
    return *sym_p("&", 9, postfix);
}

symbol& symtable::complex_rect_sym()
{
  symbol *_sym = lookup_p("+:");
  if (_sym)
    return *_sym;
  else
    return *sym_p("+:", 5, infix);
}

symbol& symtable::complex_polar_sym()
{
  symbol *_sym = lookup_p("<:");
  if (_sym)
    return *_sym;
  else
    return *sym_p("<:", 5, infix);
}
