// -*- C++ -*-

/** \file iniparse.h
 * \brief Parser for .ini style files.
 *
 * Accepts the following escape sequences in property values: \c '\\r'
 * \c '\\n' \c '\\s' (for space) and \c '\\\\'.  An example of how to
 * use the parser is this:
 * \code
 *   string foo, bar;
 *   bool baz;
 *
 *   IniActions a;
 *   a["group1"].add("foo", &foo, "baz", &baz);
 *   a["group2"].add("bar", &bar);
 *
 *   if (!ini_parse(a, "config.ini"))
 *     exit (1);
 * \endcode
 *
 * This will parse an ini file like this:
 *
 * \code{.txt}
 * [group1]
 * foo = some string
 * baz = true
 * [group2]
 * bar = \s  a string starting with three spaces
 * \endcode
 */

#ifndef _INIPARSE_H_
#define _INIPARSE_H_ 1

#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace xdr {

using std::string;

//! \cond
#define FROM_STRING(T) void from_string(const string &, T *);
FROM_STRING(string)
FROM_STRING(bool)
FROM_STRING(int)
FROM_STRING(long)
FROM_STRING(long long)
FROM_STRING(unsigned char)
FROM_STRING(unsigned short)
FROM_STRING(unsigned int)
FROM_STRING(unsigned long)
FROM_STRING(unsigned long long)
FROM_STRING(float)
FROM_STRING(double)
FROM_STRING(long double)
#undef FROM_STRING
//! \endcond

//! Contents of a specific property.
class IniLine {
  mutable bool error_ {false};
public:
  int lineno_ {0};
  string file_;

  string group_;		//!< Name of the group containing property
  string key_;			//!< Key of the property.
  string value_;		//!< Value of the property.
  //! Raw value of the property (with escape sequences unexpanded).
  string rawvalue_;

  //! Print a message to `cerr` with the file ane line number prefixed.
  std::ostream &warn() const {
    return std::cerr << file_ << ':' << lineno_ << ": ";
  }
  //! Like IniLine::warn, but also sets the error flag to indicate failure.
  std::ostream &fail() const { error_ = true; return warn(); }
  //! Returns `true` if there has been a parsing error in the ini file.
  bool error() const { return error_; }

  //! Transform the line into a vector of words.
  std::vector<string> argv() const;
  /** \brief Convert the line into a value of a type for which a
   *`from_string` function has been defined. */
  template<typename T> void convert(T *rp) const { from_string(value_, rp); }
};

//! Set of callbacks to run when parsing properties within a particular group.
struct IniGroup {
  using cb_t = std::function<void(const IniLine &li)>;
  std::unordered_map<string, cb_t> cbs_;

  IniGroup &operator=(const IniGroup &&) = delete;
  void parse(const IniLine &li);

  //! Add an explicit callback for a particular key.
  IniGroup &add(const string &key, cb_t &&cb) {
    if (!cbs_.emplace(key, std::move(cb)).second)
      throw std::runtime_error("IniGroup::add: duplicate key " + key);
    return *this;
  }
  IniGroup &add(const string &key, const cb_t &cb) {
    if (!cbs_.emplace(key, cb).second)
      throw std::runtime_error("IniGroup::add: duplicate key " + key);
    return *this;
  }
  //! Construct a callback that places the parced value in `valp`.
  template<typename T> IniGroup &add(const string &key, T *valp) {
    add(key, [valp](const IniLine &li) { li.convert(valp); });
    return *this;
  }
  //! A convenience allowing `add(field, callback, field, callback...)`.
  template<typename T, typename ...Rest> IniGroup &
  add(const string &key, T &&valp, const string &key2, Rest...rest) {
    add(key, std::forward<T>(valp));
    return add(key2, rest...);
  }
};

//! Holds the actions to execute on various properties in the ini file.
using IniActions = std::unordered_map<string, IniGroup>;

/** \brief Run the ini parser.
 *
 * Afterwards ``a.error()`` will tell you if there was an error.
 */
void ini_runparse(IniActions &a, IniLine &st, std::istream &s);
/** \brief Run the parser on a file.
 *
 * A simple wrapper around fba::ini_runparse that opens the file.
 */
bool ini_parse(IniActions &a, string file);

}

#endif /* !_INIPARSE_H_ */
