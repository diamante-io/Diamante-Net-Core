
%{
#include <algorithm>
#include "xdrc/xdrc_internal.h"

string xdr_unbounded = "";
static string getnewid(string, bool repeats_bad);

template<typename T> inline bool
compare_val(const T &a, const T &b)
{
  return a.val < b.val;
}
%}

%token <str> T_ID
%token <str> T_QID
%token <str> T_NUM

%token T_CONST
%token T_STRUCT
%token T_UNION
%token T_ENUM
%token T_TYPEDEF
%token T_PROGRAM
%token T_NAMESPACE

%token T_BOOL
%token T_UNSIGNED
%token T_INT
%token T_HYPER
%token T_FLOAT
%token T_DOUBLE
%token T_QUADRUPLE
%token T_VOID

/* Older bisons don't seem to accept %precedence
%precedence T_UNSIGNED
%precedence T_INT T_HYPER
*/

%token T_VERSION
%token T_SWITCH
%token T_CASE
%token T_DEFAULT

%token <str> T_OPAQUE
%token <str> T_STRING

%type <str> qid newid type_or_void type base_type value union_case
%type <str> vec_len
%type <decl> declaration union_decl type_specifier
%type <cnst> enum_tag
%type <num> number
%type <decl_list> struct_body declaration_list
%type <const_list> enum_body enum_tag_list
%type <ufield> union_case_list union_case_spec
%type <ubody> union_case_spec_list union_body
%type <str_list> void_or_arg_list arg_list

%%
file: /* empty */ { checkliterals(); }
	| file { checkliterals(); } definition { checkliterals(); }
	;

definition: def_const
	| def_enum
	| def_struct
	| def_type
	| def_union
        | def_program
	| def_namespace
	;

def_type: T_TYPEDEF declaration
	{
	  rpc_sym *s = &symlist.push_back();
	  s->settype(rpc_sym::TYPEDEF);
	  *s->stypedef = $2;
	  s->stypedef->id = getnewid(s->stypedef->id, true);
	}
	| T_TYPEDEF T_STRUCT declaration
	{
	  rpc_sym *s = &symlist.push_back();
	  s->settype(rpc_sym::TYPEDEF);
	  *s->stypedef = $3;
	  s->stypedef->type = string("struct ") + $3.type;
	  s->stypedef->id = getnewid(s->stypedef->id, true);
	}
	;

def_const: T_CONST newid '=' value ';'
	{
	  rpc_sym *s = &symlist.push_back();
	  s->settype(rpc_sym::CONST);
	  s->sconst->id = $2;
	  s->sconst->val = $4;
	}
	;

enum_tag: T_ID '=' value { $$.id = $1; $$.val = $3; }
	| T_ID
	{
	  $$.id = $1;
	  yywarn("RFC4506 requires a value for each enum tag");
	}

enum_tag_list: enum_tag
	{ $$.select(); assert($$->empty()); $$->push_back(std::move($1)); }
	| enum_tag_list ',' enum_tag
	{ $$ = std::move($1); $$->push_back(std::move($3)); }

enum_body: '{' enum_tag_list comma_warn '}' { $$ = std::move($2); }
	;

def_enum: T_ENUM newid enum_body ';'
	{
	  rpc_sym *s = &symlist.push_back();
	  s->settype(rpc_sym::ENUM);
	  s->senum->id = $2;
	  s->senum->tags = std::move(*$3);
	}
	;

comma_warn: /* empty */
	| ',' { yywarn("RFC4506 disallows comma after last enum tag"); }
	;

declaration_list: declaration
	{
	  $$.select();
	  assert($$->empty());
	  $$->push_back(std::move($1));
	}
	| declaration_list declaration
	{
	  $$ = std::move($1);
	  $$->push_back(std::move($2));
	}
	;

struct_body: '{' declaration_list '}'
	{
	  $$ = std::move($2);
	}
	;

def_struct: T_STRUCT newid struct_body ';'
	{
	  rpc_sym *s = &symlist.push_back();
	  s->settype(rpc_sym::STRUCT);
	  s->sstruct->id = $2;
	  s->sstruct->decls = std::move(*$3);
	}
	;

union_case: T_CASE value ':' { $$ = $2; }
	| T_DEFAULT ':' { $$ = ""; }
	;

union_case_list: union_case
	{
	  $$.select();
	  assert($$->cases.empty());
	  if ($1.empty()) {
	    if ($$->hasdefault) {
	      yyerror("duplicate default statement");
	      YYERROR;
	    }
	    $$->hasdefault = true;
	  }
	  $$->cases.push_back($1);
	}
	| union_case_list union_case
	{
	  $$ = std::move($1);
	  if ($2.empty()) {
	    if ($$->hasdefault) {
	      yyerror("duplicate default statement");
	      YYERROR;
	    }
	    $$->hasdefault = true;
	  }
	  $$->cases.push_back($2);
	}
	;

union_decl: declaration { $$ = std::move($1); }
	| T_VOID ';'
	{
	  $$.qual = rpc_decl::SCALAR;
	  $$.ts_which = rpc_decl::TS_ID;
	  $$.type = "void";
	}
	;

union_case_spec: union_case_list union_decl
	{
	  $$ = std::move($1);
	  $$->decl = std::move($2);
	}

union_case_spec_list: union_case_spec
	{
	  $$.select();
	  assert($$->fields.empty());
	  if ($1->hasdefault)
	    $$->hasdefault = true;
	  $$->fields.push_back(std::move(*$1));
	}
	| union_case_spec_list union_case_spec
	{
	  if ($1->hasdefault && $2->hasdefault) {
	    yyerror("duplicate default statement");
	    YYERROR;
	  }
	  $$ = std::move($1);
	  if ($2->hasdefault)
	    $$->hasdefault = true;
	  $$->fields.push_back(std::move(*$2));
	}

union_body: T_SWITCH '(' type T_ID ')' '{' union_case_spec_list '}'
	{
	  $$ = std::move($7);
	  $$->tagtype = $3;
	  $$->tagid = $4;
	  int next = 0;
	  for (rpc_ufield &uf : $$->fields) {
	    if (uf.decl.type == "void")
	      uf.fieldno = 0;
	    else
	      uf.fieldno = ++next;
	  }
	}
	;

def_union: T_UNION newid union_body ';'
	{
          symlist.push_back();
	  symlist.back().settype(rpc_sym::UNION);
	  rpc_union &u = *symlist.back().sunion;
	  u = std::move(*$3);
	  u.id = $2;
	}
	;

def_program: T_PROGRAM newid '{'
	{
	  rpc_sym *s = &symlist.push_back();
	  s->settype(rpc_sym::PROGRAM);
	  s->sprogram->id = $2;
	}
	version_list '}' '=' number ';'
	{
	  rpc_sym *s = &symlist.back();
	  s->sprogram->val = $8;
	  std::sort(s->sprogram->vers.begin(), s->sprogram->vers.end(),
	  	    compare_val<rpc_vers>);
	}
	;

version_list: version_decl | version_list version_decl
	;

version_decl: T_VERSION newid '{'
	{
	  rpc_sym *s = &symlist.back();
	  rpc_vers *rv = &s->sprogram->vers.push_back();
	  rv->id = $2;
	}
	proc_list '}' '=' number ';'
	{
	  rpc_sym *s = &symlist.back();
	  rpc_vers *rv = &s->sprogram->vers.back();
	  rv->val = $8;
	  std::sort(rv->procs.begin(), rv->procs.end(), compare_val<rpc_proc>);
	}
	;

proc_list: proc_decl | proc_list proc_decl
	;

proc_decl: type_or_void newid '(' void_or_arg_list ')' '=' number ';'
	{
	  rpc_sym *s = &symlist.back();
	  rpc_vers *rv = &s->sprogram->vers.back();
	  rpc_proc *rp = &rv->procs.push_back();
	  rp->id = $2;
	  rp->val = $7;
	  rp->arg = std::move(*$4);
	  rp->res = $1;
	}
	;

def_namespace: T_NAMESPACE newid '{'
        {
	  rpc_sym *s = &symlist.push_back ();
	  s->settype (rpc_sym::NAMESPACE);
	  s->sliteral = $2;
        }
	file '}'
	{
	  rpc_sym *s = &symlist.push_back ();
	  s->settype (rpc_sym::CLOSEBRACE);
	}
	;

type_specifier: type { $$.type = $1; }
	| T_ENUM enum_body
	{
	  $$.ts_which = rpc_decl::TS_ENUM;
	  $$.ts_enum.reset(new rpc_enum {"", std::move(*$2)});
	}
	| T_STRUCT struct_body
	{
	  $$.ts_which = rpc_decl::TS_STRUCT;
	  $$.ts_struct.reset(new rpc_struct {"", std::move(*$2)});
	}
	| T_UNION union_body
	{
	  $$.ts_which = rpc_decl::TS_UNION;
	  $$.ts_union.reset(new rpc_union {std::move(*$2)});
	}
	;

vec_len: '<' '>' { $$ = xdr_unbounded; }
	| '<' value '>' { $$ = $2; }
	;

declaration: type_specifier T_ID ';'
	{ $$ = std::move($1); $$.set_id($2); }
	| type_specifier T_ID '[' value ']' ';'
	{ $$ = std::move($1); $$.set_id($2);
	  $$.qual = rpc_decl::ARRAY; $$.bound = $4; }
	| T_OPAQUE T_ID '[' value ']' ';'
	{ $$.type = std::move($1); $$.set_id($2);
	  $$.qual = rpc_decl::ARRAY; $$.bound = $4; }
	| type_specifier T_ID vec_len ';'
	{ $$ = std::move($1); $$.set_id($2);
	  $$.qual = rpc_decl::VEC; $$.bound = $3; }
	| T_OPAQUE T_ID vec_len ';'
	{ $$.type = std::move($1); $$.set_id($2);
	  $$.qual = rpc_decl::VEC; $$.bound = $3; }
	| T_STRING T_ID vec_len ';'
	{ $$.type = std::move($1); $$.set_id($2);
	  $$.qual = rpc_decl::VEC; $$.bound = $3; }
	| type_specifier '*' T_ID ';'
	{ $$ = std::move($1); $$.qual = rpc_decl::PTR; $$.set_id($3); }
	| T_STRING T_ID ';'
	{ $$.set_id($2); $$.type = $1; $$.qual = rpc_decl::VEC;
	  $$.bound = xdr_unbounded;
	  yywarn ("strings require variable-length array declarations (<>)");
	}
	;

type_or_void: type | T_VOID { $$ = "void"; }
	;

void_or_arg_list: T_VOID { $$.select(); }
	| arg_list
	{ $$.select() = std::move(*$1);
	  std::reverse($$->begin(), $$->end());
	}
	;

arg_list: type { $$.select().push_back($1); }
        | type ',' arg_list
	{ $$.select() = std::move(*$3);
	  $$->push_back($1);
        }
	;

type: base_type | qid
	;

base_type: T_INT { $$ = "int"; }
	| T_UNSIGNED T_INT { $$ = "unsigned"; }
	| T_HYPER { $$ = "hyper"; }
	| T_UNSIGNED T_HYPER { $$ = "unsigned hyper"; }
	| T_UNSIGNED
	{
	  $$ = "unsigned";
	  yywarn("RFC4506 requires \"int\" after \"unsigned\"");
	}
	| T_FLOAT { $$ = "float"; }
	| T_DOUBLE { $$ = "double"; }
	| T_QUADRUPLE { $$ = "quadruple"; }
	| T_BOOL { $$ = "bool"; }
	;

value: qid | T_NUM
	;

number: T_NUM { $$ = strtoul ($1.c_str(), NULL, 0); }
	;

newid: T_ID { $$ = getnewid ($1, true); }
	;

qid: T_ID | T_QID
	;

%%
symlist_t symlist;

void
checkliterals()
{
  for (size_t i = 0; i < litq.size (); i++) {
    rpc_sym *s = &symlist.push_back ();
    s->settype (rpc_sym::LITERAL);
    *s->sliteral = litq[i];
  }
  litq.clear ();
}

static string
getnewid(string id, bool repeats_bad)
{
  if (!repeats_bad) { /* noop */ }
  else if (ids.find(id) != ids.end()) {
    yywarn(string("redefinition of symbol ") + id);
  } else {
    ids.insert(id);
  }
  // Possible place to add namespace scope::
  return id;
}
