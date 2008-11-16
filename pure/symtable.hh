
#ifndef SYMTABLE_HH
#define SYMTABLE_HH

#include <string>
#include <set>
#include <map>
#include <vector>
#include <stdint.h>
#include "expr.hh"
#include "printer.hh"

using namespace std;

/* Symbol table entries. */

class symbol {
public:
  expr x; // cached expression node
  int32_t f; // symbol tag
  string s; // print name
  prec_t prec; // precedence level
  fix_t fix; // fixity
  bool priv; // private attribute
  symbol() : // constructor for dummy entries
    f(0), s(""), prec(10), fix(infix), priv(false) { }
  symbol(const string& _s, int _f, bool _priv = false) :
    f(_f), s(_s), prec(10), fix(infix), priv(_priv) { x = expr(f); }
  symbol(const string& _s, int _f, prec_t _prec, fix_t _fix,
	 bool _priv = false) :
    f(_f), s(_s), prec(_prec), fix(_fix), priv(_priv) { x = expr(f); }
};

/* Symbol table. */

class symtable {
  int32_t fno;
  map<string, symbol> tab;
  vector<symbol*> rtab;
  symbol* lookup_p(const char *s);
public:
  symtable();
  ~symtable();
  // the current namespace
  string *current_namespace;
  // additional namespaces to be searched for unqualified symbols
  set<string> *search_namespaces;
  // add default declarations for the builtin constants and operators (to be
  // invoked *after* possibly reading the prelude)
  void init_builtins();
  // get current number of symbols in table (symbols are always numbered
  // consecutively from 1 to nsyms())
  int32_t nsyms() { return fno; }
  /* The following operations look up a symbol in the table and possibly
     create it if necessary. Unqualified symbols are first searched for in the
     current namespace and all search namespaces (if any), and finally also in
     the default namespace. If the symbol is found (or could be created), a
     pointer to the symbol table entry is returned. Otherwise a null pointer
     is returned, in which case you can inspect the 'count' variable to
     determine the cause of the error. If 'count' is 0, then the symbol wasn't
     found; this can only happen with lookup(), as the sym() methods always
     try to create the symbol if it doesn't exist already. If 'count' is 1,
     then the symbol was found but is not visible in the current namespace
     because it is a 'private' symbol in another namespace. If 'count' is >1,
     then an unqualified symbol couldn't be resolved because it exists in
     multiple (current or search) namespaces. */
  int count;
  // look up an existing symbol
  symbol* lookup(const char *s);
  symbol* lookup(const string& s) { return lookup(s.c_str()); }
  // get a symbol by its name, create if necessary
  symbol* sym(const char *s, bool priv = false);
  symbol* sym(const char *s, prec_t prec, fix_t fix, bool priv = false);
  symbol* sym(const string& s, bool priv = false)
  { return sym(s.c_str(), priv); }
  symbol* sym(const string& s, prec_t prec, fix_t fix, bool priv = false)
  { return sym(s.c_str(), prec, fix, priv); }
  // get/create a symbol, throw an appropriate error if this fails
  symbol& checksym(const char *s, bool priv = false);
  symbol& checksym(const string& s, bool priv = false)
  { return checksym(s.c_str(), priv); }
  // get a symbol by its number
  symbol& sym(int32_t f);
  // check whether a symbol is currently visible
  bool visible(const symbol& sym);
  bool visible(int32_t f)
  { assert(f > 0 && (uint32_t)f < rtab.size());
    return visible(*rtab[f]); }
  // retrieve various builtin symbols (create when necessary)
  int32_t __show__sym; // This is cached here to improve performance.
  symbol& nil_sym();
  symbol& cons_sym();
  symbol& void_sym();
  symbol& pair_sym();
  symbol& seq_sym();
  symbol& neg_sym() { return *sym("neg"); }
  symbol& not_sym();
  symbol& bitnot_sym();
  symbol& or_sym();
  symbol& and_sym();
  symbol& bitor_sym();
  symbol& bitand_sym();
  symbol& shl_sym();
  symbol& shr_sym();
  symbol& less_sym();
  symbol& greater_sym();
  symbol& lesseq_sym();
  symbol& greatereq_sym();
  symbol& equal_sym();
  symbol& notequal_sym();
  symbol& plus_sym();
  symbol& minus_sym();
  symbol& mult_sym();
  symbol& fdiv_sym();
  symbol& div_sym();
  symbol& mod_sym();
  symbol& quote_sym() { return *sym("quote"); }
  symbol& catch_sym() { return *sym("catch"); }
  symbol& catmap_sym() { return *sym("catmap"); }
  symbol& rowcatmap_sym() { return *sym("rowcatmap"); }
  symbol& colcatmap_sym() { return *sym("colcatmap"); }
  symbol& failed_match_sym() { return *sym("failed_match"); }
  symbol& failed_cond_sym() { return *sym("failed_cond"); }
  symbol& signal_sym() { return *sym("signal"); }
  symbol& segfault_sym() { return *sym("stack_fault"); }
  symbol& bad_matrix_sym() { return *sym("bad_matrix_value"); }
  symbol& amp_sym();
  symbol& complex_rect_sym();
  symbol& complex_polar_sym();
};

#endif // ! SYMTABLE_HH
