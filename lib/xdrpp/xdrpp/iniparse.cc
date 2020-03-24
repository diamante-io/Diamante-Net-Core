
#include <cstring>
#include <fstream>
#include <type_traits>
#include <xdrpp/iniparse.h>

namespace xdr {

using std::size_t;

bool ini_unescape(string::const_iterator, string::const_iterator, string *);

void
from_string(const string &s, string *sp)
{
  *sp = s;
}

void
from_string(const string &s, bool *bp)
{
  if (s == "true")
    *bp = true;
  else if (s == "false")
    *bp = false;
  else
    throw std::invalid_argument ("boolean must be \"true\" or \"false\"");
}

template<class T> inline T
add_base(T (&conv)(const string &, size_t *, int), const string &s, size_t *pos)
{
  return conv(s, pos, 0);
}
template<class T> inline T
add_base(T (&conv)(const string &, size_t *), const string &s, size_t *pos)
{
  return conv(s, pos);
}

template<typename T, typename U, typename ...Base> inline void
from_string_with(U (&conv)(const string &, size_t *, Base...),
		 const string &s, T *rp)
{
  std::size_t pos;
  U r (add_base(conv, s, &pos));
  T t = r;
  if (static_cast<U>(t) != r)
    throw std::out_of_range ("value out of range");
  else if (s.find_first_not_of("\t\n\r ", pos) != string::npos)
    throw std::invalid_argument ("trailing garbage: " + s.substr(pos));
  *rp = std::move(t);
}

#define FROM_STRING(T, conv)						\
  void from_string(const string &s, T *rp) { from_string_with(conv, s, rp); }
FROM_STRING(int, std::stoi)
FROM_STRING(long, std::stol)
FROM_STRING(long long, std::stoll)
FROM_STRING(unsigned char, std::stoul)
FROM_STRING(unsigned short, std::stoul)
FROM_STRING(unsigned int, std::stoul)
FROM_STRING(unsigned long, std::stoul)
FROM_STRING(unsigned long long, std::stoll)
FROM_STRING(float, std::stof)
FROM_STRING(double, std::stod)
FROM_STRING(long double, std::stold)

std::vector<string>
IniLine::argv() const
{
  std::vector<string> av;
  string a;
  size_t i = 0, e;
  while (i < rawvalue_.size()) {
    e = i;
    do {
      e = rawvalue_.find_first_of("\t\r ", e+1);
    } while (e != string::npos && e > 0 && rawvalue_[e-1] == '\\');
    if (e == string::npos)
      e = rawvalue_.size();
    if (!ini_unescape(rawvalue_.cbegin() + i, rawvalue_.cbegin() + e, &a))
      // This shouldn't happen, since ini_parse should already hit the error
      throw std::invalid_argument("stray backslash at end of line");
    av.emplace_back(a);
    i = rawvalue_.find_first_not_of("\t\r ", e);
  }
  return av;
}

static void
ignoreline(const IniLine &li)
{
}

void
IniGroup::parse(const IniLine &li)
{
  auto i (cbs_.find(li.key_));
  if (i == cbs_.end()) {
    li.warn() << "unknown property "
	      << li.group_ << '.' << li.key_ << std::endl;
    cbs_.emplace(li.key_, ignoreline);
  }
  else
    i->second(li);
}

bool
ini_unescape(string::const_iterator i, string::const_iterator e, string *out)
{
  bool escape {false};
  string v;
  for (; i != e; i++) {
    if (escape) {
      switch (*i) {
      case 'n':
	v.push_back('\n');
	break;
      case 'r':
	v.push_back('\r');
	break;
      case 's':
	v.push_back(' ');
	break;
      case 't':
	v.push_back('\t');
	break;
      default:
	v.push_back(*i);
	break;
      }
      escape = false;
    }
    else if (*i == '\\')
      escape = true;
    else
      v.push_back(*i);
  }
  if (escape)
    return false;
  out->assign(std::move(v));
  return true;
}

static bool
parsekv(const string &line, string *kout, string *vout, string *rvout)
{
  string k;
  auto li = (line.cbegin());

  while (li < line.cend() && isspace(*li))
    li++;
  while (li < line.cend() && !isspace(*li) && *li != '=')
    k.push_back(*li++);
  while (li < line.cend() && isspace(*li))
    li++;
  if (k.empty() || li >= line.cend() || *li++ != '=')
    return false;
  while (li < line.cend() && isspace(*li))
    li++;

  if (!ini_unescape(li, line.cend(), vout))
    return false;
  rvout->assign(string(li, line.cend()));
  kout->assign(std::move(k));
  return true;
}

void
ini_runparse(IniActions &a, IniLine &st, std::istream &s)
{
  auto group = a.end();
  string line;
  while (getline(s, line).good()) {
    st.lineno_++;
    line.erase(0, line.find_first_not_of("\t\r "));
    if (line.empty() || line[0] == '#')
      continue;

    if (line[0] == '[') {
      size_t e = line.rfind(']');
      if (e == 1 || e == string::npos
	  || line.find_first_not_of("\t\r ", e+1) != string::npos)
	st.fail() << "syntax error" << std::endl;
      else {
	st.group_ = line.substr(1, e-1);
	group = a.find(st.group_);
	if (group == a.end())
	  st.warn() << "unknown group " << st.group_ << "\n";
      }
    }
    else if (st.group_.empty())
      st.fail() << "key precedes group" << std::endl;
    else if (group == a.end())
      ;
    else if (parsekv(line, &st.key_, &st.value_, &st.rawvalue_)) {
      try { group->second.parse(st); }
      catch (std::logic_error &e) { st.fail() << e.what() << std::endl; }
    }
    else
      st.fail() << "syntax error" << std::endl;
  }
}

bool
ini_parse(IniActions &a, string file)
{
  IniLine st;
  st.file_ = file;
  std::ifstream s {st.file_};
  if (s.bad()) {
    std::cerr << file << ": " << std::strerror (errno) << std::endl;
    return false;
  }
  ini_runparse(a, st, s);
  return !st.error();
}

}
