
#include "xdrc_internal.h"

using std::endl;

namespace {

indenter nl;

void
gen_args(std::ostream &os, const rpc_proc &p)
{
  if (!server_session.empty())
    os << server_session << " *session";
  for (std::size_t i = 0; i < p.arg.size(); ++i) {
    if (i || !server_session.empty())
      os << ", ";
    string arg = p.arg.size() == 1 ? "arg" : "arg" + std::to_string(i+1);
    if (server_ptr)
      os << "std::unique_ptr<" << p.arg[i] << "> " << arg;
    else
      os << "const " << p.arg[i] << " &" << arg;
  }
  if (server_async) {
    if (p.arg.size())
      os << ", ";
    os << "xdr::reply_cb<" << p.res << "> cb";
  }
}

void
gen_decl(std::ostream &os, const rpc_program &u, const rpc_vers &v)
{
  string name = v.id + "_server";

  os << endl
     << nl << "class " << name << " {"
     << nl << "public:";
  ++nl;
  os << nl << "using rpc_interface_type = " << v.id << ";" << endl;
  for (const rpc_proc &p : v.procs) {
    //    string arg = p.arg == "void" ? ""
    //  : (string("std::unique_ptr<") + p.arg + "> arg");
    string res = server_async || p.res == "void" ? "void"
      : (string("std::unique_ptr<") + p.res + ">");
    os << nl << res << " " << p.id << "(";
    gen_args(os, p);
    os << ");";
  }
  os << nl.close << "};";
}

void
gen_def(std::ostream &os, const rpc_program &u, const rpc_vers &v)
{
  string name = v.id + "_server";

  for (const rpc_proc &p : v.procs) {
    //string arg = p.arg == "void" ? ""
    //  : (string("std::unique_ptr<") + p.arg + "> arg");
    string res = p.res == "void" ? "void"
      : (string("std::unique_ptr<") + p.res + ">");
    os << endl
       << nl << res
       << nl << name << "::" << p.id
       << "(";
    gen_args(os, p);
    os << ")"
       << nl << "{";
    if (res != "void" && !server_async)
       os << nl.open << "std::unique_ptr<" << p.res << "> res(new "
	  << p.res << ");"
	  << nl
	  << nl << "// Fill in function body here"
	  << nl
	  << nl << "return res;"
	  << nl.close << "}";
    else
       os << nl.open
	  << nl << "// Fill in function body here"
	  << nl
	  << nl.close << "}";
  }
}

void
gen_server_internal(std::ostream &os, bool cc)
{
  if (!cc)
    os << "// -*- C++ -*-" << endl;
  os << "// Scaffolding originally generated from " << input_file << "."
     << nl << "// Edit to add functionality." << endl;

  string guard = guard_token(".server");

  if (cc) {
    string output_prefix = strip_suffix(output_file, ".cc");
    if (output_prefix != output_file)
      os << nl << "#include \"" << output_prefix << ".hh\"";
    else
      os << nl << "#include \"" << file_prefix << ".server.hh\"";
  }
  else {
    os << nl << "#ifndef " << guard
       << nl << "#define " << guard << " 1"
       << nl;
    if (server_async)
      os << nl << "#include <xdrc/arpc.h>";
    os << nl << "#include \"" << file_prefix << ".hh\"";
  }

  int last_type = -1;

  for (auto &s : symlist) {
    switch (s.type) {
    case rpc_sym::PROGRAM:
      for (const rpc_vers &v : s.sprogram->vers)
	(cc ? gen_def : gen_decl)(os, *s.sprogram, v);
      break;
    case rpc_sym::NAMESPACE:
      if (last_type != rpc_sym::NAMESPACE)
	os << endl;
      os << nl << "namespace " << *s.sliteral << " {";
      break;
    case rpc_sym::CLOSEBRACE:
      if (last_type != rpc_sym::CLOSEBRACE)
	os << endl;
      os << nl << "}";
      break;
    default:
      break;
    }
    last_type = s.type;
  }
  os << nl;

  if (!cc)
    os << nl << "#endif // !" << guard << nl;
}

}

void
gen_server(std::ostream &os)
{
  gen_server_internal(os, false);
}

void
gen_servercc(std::ostream &os)
{
  gen_server_internal(os, true);
}
