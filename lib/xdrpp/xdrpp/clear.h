// -*- C++ -*-

/** \file clear.h Support for clearing an XDR data structure. */

#ifndef _XDRPP_CLEAR_H_HEADER_INCLUDED_
#define _XDRPP_CLEAR_H_HEADER_INCLUDED_ 1

#include <cstring>
#include <type_traits>
#include <xdrpp/types.h>

namespace xdr {

namespace detail{
//! Helper type for xdr::xdr_clear function.
struct xdr_clear_t {
  Constexpr xdr_clear_t() {}

  template<typename T> typename
  std::enable_if<xdr_traits<T>::variable_nelem>::type
  operator()(T &t) const { t.resize(0); }

  template<std::uint32_t N> void operator()(opaque_array<N> &t) const {
    std::memset(t.data(), 0, t.size());
  }

  template<typename T> typename
  std::enable_if<xdr_traits<T>::is_struct>::type
  operator()(T &t) const { xdr_traits<T>::load(*this, t); }

  template<typename T> typename
  std::enable_if<!xdr_traits<T>::variable_nelem>::type
  operator()(T &t) const { xdr_traits<T>::load(*this, t); }

  template<typename T> typename std::enable_if<xdr_traits<T>::is_union>::type
  operator()(T &t) const {
    auto dt = typename xdr_traits<T>::discriminant_type{};
    if (t._xdr_field_number(dt) > 0)
      xdr_traits<T>::load(*this, t);
    else
      t._xdr_discriminant(dt, false);
  }

  template<typename T> typename
  std::enable_if<xdr_traits<T>::is_numeric || xdr_traits<T>::is_enum>::type
  operator()(T &t) const { t = T{}; }
};
}

//! Reset XDR data structure to its default contents.  All vectors and
//! strings are set to length 0, all fixed-size opaque arrays zeroed
//! out, and all numeric and enum types sent to their default values
//! (generally 0).
template<typename T> void
xdr_clear(T &t)
{
  static Constexpr detail::xdr_clear_t c;
  archive(c, t);
}

}

#endif // !_XDRPP_CLEAR_H_HEADER_INCLUDED
