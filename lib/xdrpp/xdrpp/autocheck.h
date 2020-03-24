// -*- C++ -*-

/** \file autocheck.h Support for using the [autocheck
  * framework](https://github.com/thejohnfreeman/autocheck/wiki) with
  * XDR data types.  If you include this file (which requires the
  * autocheck headers to be available), you can generate arbitrary
  * instances of XDR types for testing your application.  For example:
  * \code
  *   // g will generate arbitrary values of type my_xdr_type
  *   autocheck::generator<my_xdr_type> g;
  *   // For variable size objects (vectors, strings, linked lists),
  *   // size indicates how big to make them.  For numeric values,
  *   // size affects how big the number is likely to be.
  *   size_t size = 50;
  *   my_xdr_type arbitrary_value = g(object_size);
  *   fuzz_with(arbitrary_value);
  * \endcode
  */

#ifndef _XDRC_AUTOCHECK_H_HEADER_INCLUDED_
#define _XDRC_AUTOCHECK_H_HEADER_INCLUDED_ 1

#include <autocheck/autocheck.hpp>
#include <xdrpp/types.h>

namespace xdr {

#ifndef XDR_AUTOCHECK_FUZZY_STRINGS
// If defined to 0, changes the behavior of the library to generate
// strings with ASCII characters with small size parameters.  The
// default (1) generates strings with arbitrary bytes.
#define XDR_AUTOCHECK_FUZZY_STRINGS 1
#endif // !XDR_AUTOCHECK_FUZZY_STRINGS

namespace detail {

//! Convert an int to a uint8_t or char, extracting the low byte.
//! This is harder than it sounds when char is signed, because
//! converting to a smaller signed type yields an
//! implementation-defined value.  Worse, MSVC with certain options
//! goes even further and crashes the program when you simply cast an
//! int to a char.  Since there's no equivalent of reinterpret_cast
//! between signed and unsigned values, and since even the standard
//! union trick may be unsafe according to:
//!
//!     http://stackoverflow.com/questions/11373203/accessing-inactive-union-member-undefined
//!
//! we just gave up and did the brute force arithmetic thing, taking
//! advantage of the fact that all these one-byte values get promoted
//! to int for arithmetic.
template<typename T> constexpr T
to_int8(uint8_t u)
{
  return u <= std::numeric_limits<T>::max() ? u :
    u + (std::numeric_limits<T>::min() - std::numeric_limits<T>::max());
}
template<> constexpr uint8_t
to_int8<uint8_t>(uint8_t u)
{
  return u;
}

}

struct generator_t {
  std::size_t size_;
  std::size_t levels_;  // Only decrease size after this many levels
  Constexpr explicit generator_t(std::size_t size, std::size_t levels = 2)
    : size_(size), levels_(levels) {}

  // Handle char and uint8_t (which can legally be the same type for
  // some compilers), allowing variable_nelem cases to handle both
  // containers and bytes (string/opaque).
  template<typename T> typename
  std::enable_if<(std::is_same<T, char>::value
		  || std::is_same<T, std::uint8_t>::value)>::type
  operator()(T &t) const {
#if XDR_AUTOCHECK_FUZZY_STRINGS
    t = detail::to_int8<T>(uint8_t(autocheck::generator<int>{}(0x10000)));
#else // !XDR_AUTOCHECK_FUZZY_STRINGS
    t = detail::to_int8<T>(autocheck::generator<T>{}(size_));
#endif // !XDR_AUTOCHECK_FUZZY_STRINGS
  }

  template<typename T> typename
  std::enable_if<xdr_traits<T>::is_numeric>::type
  operator()(T &t) const {
    t = autocheck::generator<T>{}(std::numeric_limits<T>::max());
  }

  template<typename T> typename
  std::enable_if<xdr_traits<T>::is_enum>::type
  operator()(T &t) const {
    if(autocheck::generator<bool>{}(size_)) {
      typename xdr_traits<T>::case_type v;
      (*this)(v);
      t = T(v);
    }
    else {
      uint32_t n;
      (*this)(n);
      const auto &vals = xdr_traits<T>::enum_values();
      t = T(vals[n % vals.size()]);
    }
  }

  template<typename T> typename
  std::enable_if<xdr_traits<T>::is_struct>::type
  operator()(T &t) const { xdr_traits<T>::load(*this, t); }

  template<typename T> typename
  std::enable_if<xdr_traits<T>::is_union>::type
  operator()(T &t) const {
    const auto &vals = T::_xdr_case_values();
    typename xdr_traits<T>::discriminant_type v;
    if (!T::_xdr_has_default_case) {
      // Just pick a random case if there's no default
      uint32_t n;
      (*this)(n);
      v = vals[n % vals.size()];
    }
    else if (xdr_traits<decltype(v)>::is_enum)
      (*this)(v);
    else {
      if (autocheck::generator<bool>{}(size_))
	(*this)(v);
      else {
	uint32_t n;
	(*this)(n);
	v = vals[n % vals.size()];
      }
    }
    t._xdr_discriminant(v, false);
    t._xdr_with_mem_ptr(field_archiver, v, *this, t, nullptr);
  }

  // Generator with shrunken size for elements of a container
  generator_t elt_gen() const {
    return levels_ ? generator_t(size_, levels_-1) : generator_t(size_>>1, 0);
  }

  template<typename T> typename
  std::enable_if<xdr_traits<T>::variable_nelem == true>::type
  operator()(T &t) const {
    uint32_t n = autocheck::generator<uint32_t>{}(size_);
    if (n > t.max_size())
      n %= t.max_size() + 1;
    t.resize(n);
    generator_t g(elt_gen());
    for (auto &e : t)
      archive(g, e);
  }

  template<typename T> typename
  std::enable_if<xdr_traits<T>::variable_nelem == false>::type
  operator()(T &t) const {
    generator_t g(elt_gen());
    for (auto &e : t)
      archive(g, e);
  }

  template<typename T> void
  operator()(pointer<T> &t) const {
    if (autocheck::generator<std::uint32_t>{}(size_+1)) {
      generator_t g(elt_gen());
      archive(g, t.activate());
    }
    else
      t.reset();
  }
};

} // namespace xdr

namespace autocheck {

template<typename T> class generator<
  T, typename std::enable_if<xdr::xdr_traits<T>::valid
			     && !xdr::xdr_traits<T>::is_numeric>::type> {
public:
  using result_type = T;
  result_type operator()(size_t size) const {
    xdr::generator_t g(size);
    T t;
    xdr::archive(g, t);
    return t;
  }
};

} // namespace autocheck

#endif // !_XDRC_AUTOCHECK_H_HEADER_INCLUDED_
