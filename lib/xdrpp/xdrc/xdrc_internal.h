// -*-c++-*-

#include <cassert>
#include <iosfwd>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <unordered_map>

#include <xdrpp/endian.h>
#include "union.h"

#ifdef _MSC_VER
#include <io.h>
#define popen _popen
#define pclose _pclose
#define access _access
#define isatty _isatty
#define fileno _fileno
#else
#include <unistd.h>
#endif

using std::string;

template<typename T> struct vec : std::vector<T> {
  using std::vector<T>::vector;
  using std::vector<T>::push_back;
  T &push_back() { this->emplace_back(); return this->back(); }
};

extern std::set<string> ids;

extern int lineno;
extern int printlit;
#undef yyerror
int yyerror(string);
int yywarn(string);

int yylex();
int yyparse();
void checkliterals();

struct rpc_enum;
struct rpc_struct;
struct rpc_union;

struct rpc_decl {
  string id;
  enum { SCALAR, PTR, ARRAY, VEC } qual {SCALAR};
  string bound;

  enum { TS_ID, TS_ENUM, TS_STRUCT, TS_UNION } ts_which {TS_ID};
  string type;
  union {
    union_entry_base _base;
    union_ptr<rpc_enum> ts_enum;
    union_ptr<rpc_struct> ts_struct;
    union_ptr<rpc_union> ts_union;
  };

  rpc_decl() : _base() {}
  ~rpc_decl() { _base.destroy(); }
  rpc_decl(const rpc_decl &d)
    : id(d.id), qual(d.qual), bound(d.bound), ts_which(d.ts_which),
      type(d.type), _base(d._base) {}
  rpc_decl(rpc_decl &&d)
    : id(std::move(d.id)), qual(d.qual), bound(std::move(d.bound)),
      ts_which(d.ts_which), type(d.type), _base(std::move(d._base)) {}
  rpc_decl &operator=(const rpc_decl &d) {
    id = d.id;
    qual = d.qual;
    bound = d.bound;
    ts_which = d.ts_which;
    type = d.type;
    _base = d._base;
    return *this;
  }
  rpc_decl &operator=(rpc_decl &&d) {
    id = std::move(d.id);
    qual = d.qual;
    bound = std::move(d.bound);
    ts_which = d.ts_which;
    _base = std::move(d._base);
    type = std::move(d.type);
    return *this;
  }

  void set_id(const string &nid);
};

struct rpc_const {
  string id;
  string val;
};

struct rpc_struct {
  string id;
  vec<rpc_decl> decls;
};

struct rpc_enum {
  string id;
  vec<rpc_const> tags;
};

struct rpc_ufield {
  vec<string> cases;
  rpc_decl decl;
  bool hasdefault{false};
  int fieldno{-1}; // 1, 2, 3, but 0 for void
};

struct rpc_union {
  string id;
  string tagtype;
  string tagid;
  vec<rpc_ufield> fields;
  bool hasdefault{false};
};

struct rpc_proc {
  string id;
  uint32_t val;
  vec<string> arg;
  string res;
};

struct rpc_vers {
  string id;
  uint32_t val;
  vec<rpc_proc> procs;
};

struct rpc_program {
  string id;
  uint32_t val;
  vec<rpc_vers> vers;
};

struct rpc_sym {
  union {
    union_entry_base _base;
    union_entry<rpc_const> sconst;
    union_entry<rpc_decl> stypedef;
    union_entry<rpc_struct> sstruct;
    union_entry<rpc_enum> senum;
    union_entry<rpc_union> sunion;
    union_entry<rpc_program> sprogram;
    union_entry<string> sliteral;
  };

  enum symtype { CONST, STRUCT, UNION, ENUM, TYPEDEF, PROGRAM, LITERAL,
		 NAMESPACE, CLOSEBRACE } type;

  rpc_sym () : _base() {}
  rpc_sym (rpc_sym &&s) : _base(std::move(s._base)), type (s.type) {}
  ~rpc_sym () { _base.destroy (); }

  symtype gettype () const { return type; }

  void settype (symtype t) {
    switch (type = t) {
    case NAMESPACE:
      sliteral.select ();
      break;
    case CONST:
      sconst.select ();
      break;
    case STRUCT:
      sstruct.select ();
      break;
    case UNION:
      sunion.select ();
      break;
    case ENUM:
      senum.select ();
      break;
    case TYPEDEF:
      stypedef.select ();
      break;
    case PROGRAM:
      sprogram.select ();
      break;
    case LITERAL:
      sliteral.select ();
      break;
    default:
      _base.deselect();
      break;
    }
  }
};

#define YYSTYPE YYSTYPE
struct YYSTYPE {
  uint32_t num;
  struct rpc_decl decl;
  struct rpc_const cnst;
  string str;
  union {
    union_entry_base _base;
    union_entry<vec<rpc_decl>> decl_list;
    union_entry<vec<rpc_const>> const_list;
    union_entry<rpc_ufield> ufield;
    union_entry<rpc_union> ubody;
    union_entry<vec<string>> str_list;
  };

  YYSTYPE() : _base() {}
  YYSTYPE(const YYSTYPE &st)
    : num(st.num), decl(st.decl), cnst(st.cnst), str(st.str), _base(st._base) {}
  YYSTYPE(YYSTYPE &&st)
    : num(st.num), decl(std::move(st.decl)), cnst(std::move(st.cnst)),
      str(std::move(st.str)), _base(std::move(st._base)) {}
  ~YYSTYPE() { _base.destroy(); }
  YYSTYPE &operator=(const YYSTYPE &st) {
    num = st.num;
    decl = st.decl;
    cnst = st.cnst;
    str = st.str;
    _base = st._base;
    return *this;
  }
  YYSTYPE &operator=(YYSTYPE &&st) {
    num = st.num;
    decl = std::move(st.decl);
    cnst = std::move(st.cnst);
    str = std::move(st.str);
    _base = std::move(st._base);
    return *this;
  }
};
extern YYSTYPE yylval;

using symlist_t = vec<rpc_sym>;
extern symlist_t symlist;

using strlist_t = vec<string>;
extern strlist_t litq;

string guard_token(const string &extra);
string strip_directory(string in);
string strip_suffix(string in, string suffix);
void gen_hh(std::ostream &os);
void gen_server(std::ostream &os);
void gen_servercc(std::ostream &os);

extern string input_file;
extern string output_file;
extern string file_prefix;
extern string server_session;
extern bool server_ptr;
extern bool server_async;

template <typename T>
struct omanip {
  using ostream = std::ostream;
  T *obj;
  void (T::*fn)(ostream &);
  omanip(T *t, void(T::*fn)(ostream &))
    : obj(t), fn(fn) {}
  friend ostream &operator<<(ostream &os, omanip<T> &m) {
    ((m.obj)->*(m.fn))(os);
    return os;
  }
};

struct indenter : omanip<indenter> {
  int level_{0};
  void do_indent(ostream &os) { os << std::endl << std::string(level_, ' '); }
  void do_open(ostream &os) { ++*this; do_indent(os); }
  void do_close(ostream &os) { --*this; do_indent(os); }
  void do_outdent(ostream &os) {
    os << std::endl << std::string(level_ > 2 ? level_ - 2 : 0, ' ');
  }

  indenter() : omanip(this, &indenter::do_indent) {}
  omanip<indenter> open = omanip(this, &indenter::do_open);
  omanip<indenter> close = omanip(this, &indenter::do_close);
  omanip<indenter> outdent = omanip(this, &indenter::do_outdent);
  void operator++() { level_ += 2; }
  void operator--() { level_ -= 2; assert (level_ >= 0); }
};
