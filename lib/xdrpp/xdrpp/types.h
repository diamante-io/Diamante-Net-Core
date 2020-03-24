// -*- C++ -*-

/** \file types.h Type definitions for xdrc compiler output. */

#ifndef _XDRC_TYPES_H_HEADER_INCLUDED_
#define _XDRC_TYPES_H_HEADER_INCLUDED_ 1

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include <limits>

#include <xdrpp/endian.h>

//! Most of the xdrpp library is encapsulated in the xdr namespace.
namespace xdr {

using std::uint32_t;

inline uint32_t
size32(std::size_t s)
{
  uint32_t r {uint32_t(s)};
  assert(s == r);
  return r;
}


////////////////////////////////////////////////////////////////
// Exception types
////////////////////////////////////////////////////////////////

//! Generic class of XDR unmarshaling errors.
struct xdr_runtime_error : std::runtime_error {
  using std::runtime_error::runtime_error;
};

//! Attempt to exceed the bounds of a variable-length array or string.
struct xdr_overflow : xdr_runtime_error {
  using xdr_runtime_error::xdr_runtime_error;
};

//! Attempt to exceed recursion limits.
struct xdr_stack_overflow : xdr_runtime_error {
  using xdr_runtime_error::xdr_runtime_error;
};

//! Message not multiple of 4 bytes, or cannot fully be parsed.
struct xdr_bad_message_size : xdr_runtime_error {
  using xdr_runtime_error::xdr_runtime_error;
};

//! Attempt to set invalid value for a union discriminant.
struct xdr_bad_discriminant : xdr_runtime_error {
  using xdr_runtime_error::xdr_runtime_error;
};

//! Padding bytes that should have contained zero don't.
struct xdr_should_be_zero : xdr_runtime_error {
  using xdr_runtime_error::xdr_runtime_error;
};

//! Exception for use by \c xdr::xdr_validate.
struct xdr_invariant_failed : xdr_runtime_error {
  using xdr_runtime_error::xdr_runtime_error;
};

//! Attempt to access wrong field of a union.  Note that this is not
//! an \c xdr_runtime_error, because it cannot result from
//! unmarshalling garbage arguments.  Rather it is a logic error in
//! application code that neglected to check the union discriminant
//! before accessing the wrong field.
struct xdr_wrong_union : std::logic_error {
  using std::logic_error::logic_error;
};


////////////////////////////////////////////////////////////////
// Templates for XDR traversal and processing
////////////////////////////////////////////////////////////////

namespace detail {
// If a class actually contains a method called validate, then the
// validate function might as well call it.  So we use the standard
// gross overload resolution hack that argument 0 will match a
// pointer-to-method argument before it will match a ... argument.
template<typename T> inline void
call_validate(const T &t, decltype(&T::validate))
{
  t.validate();
}

// Conventional wisdom holds that varargs don't inline, but, perhpas
// because we don't actually call va_start, both gcc and clang seem
// able to make this compile to nothing (with optimization).
template<typename T> inline void
call_validate(const T &, ...)
{
}
}

//! If this function template is specialized, it provides a means of
//! placing extra restrictions on XDR data structures (beyond those of
//! the XDR specification).  When a specialized \c xdr::validate
//! function detects a bad data argument, it should throw an exception
//! of type \c xdr::xdr_invariant_failed.  Note this mechanism only
//! works for user-defined XDR structs and unions.  It does not work
//! for enums, typedef aliases, or built-in types (int, hyper, string,
//! vectors, etc.).
template<typename T> inline void
validate(const T &t)
{
  detail::call_validate(t, 0);
}

//! This is used to apply an archive to a field.  It is designed as a
//! template class that can be specialized to various archive formats,
//! since some formats may want the fied name and others not.  Other
//! uses include translating types to supertypes, e.g., so an archive
//! can handle \c std::string instead of \c xdr::xstring.  Never
//! invoke \c archive_adapter::apply directly.  Instead, call \c
//! xdr::archive, as the latter may be specialized for certain types.
template<typename Archive> struct archive_adapter {
  template<typename T> static void apply(Archive &ar, T &&t, const char *) {
    ar(std::forward<T>(t));
  }
};

//! By default, this function simply applies \c ar (which must be a
//! function object) to \c t.  However, it does so via the \c
//! xdr::archive_adapter template class, which can be specialized to
//! capture the field name as well.  Never specialize or overload this
//! function on \c Archive (specialize \c xdr::archive_adapter
//! instead).  However, in special cases (such as \c
//! xdr::transparent_ptr) it is reasonable to specialize this function
//! template on \c T.
template<typename Archive, typename T> inline void
archive(Archive &ar, T &&t, const char *name = nullptr)
{
  archive_adapter<Archive>::apply(ar, std::forward<T>(t), name);
}

//! Metadata for all marshalable XDR types.
template<typename T> struct xdr_traits {
  //! \c T is a valid XDR type that can be serialized.
  static Constexpr const bool valid = false;
  //! \c T is defined by xdrpp/xdrc (as opposed to native or std types).
  static Constexpr const bool xdr_defined = false;
  //! \c T is an \c xstring, \c opaque_array, or \c opaque_vec.
  static Constexpr const bool is_bytes = false;
  //! \c T is an XDR struct.
  static Constexpr const bool is_struct = false;
  //! \c T is an XDR union.
  static Constexpr const bool is_union = false;
  //! \c T is an XDR struct or union.
  static Constexpr const bool is_class = false;
  //! \c T is an XDR enum or bool (traits have enum_name).
  static Constexpr const bool is_enum = false;
  //! \c T is an xdr::pointer, xdr::xarray, or xdr::xvector (with load/save).
  static Constexpr const bool is_container = false;
  //! \c T is one of [u]int{32,64}_t, float, or double.
  static Constexpr const bool is_numeric = false;
  //! \c T has a fixed size.
  static Constexpr const bool has_fixed_size = false;
};

namespace detail {
// When a type T includes itself recursively (for instance because it
// contains a vector of itself), xdr_traits<T> will be incomplete at
// some points where one needs to know if the structure has a fixed
// size.  However, such recursive structures don't have a fixed size,
// anyway, so it is okay to short-circuit and return a false
// has_fixed_size.  The point of has_fixed_size_t is to allow
// specializations (notably for xvector<T>) that short-cirtuit to
// false.
template<typename T> struct has_fixed_size_t
  : std::integral_constant<bool, xdr_traits<T>::has_fixed_size> {};
}

//! Return the marshaled size of an XDR data type.
template<typename T> std::size_t
xdr_size(const T&t)
{
  return xdr_traits<T>::serial_size(t);
}

//! Default xdr_traits values for actual XDR types, used as a
//! supertype for most xdr::xdr_traits specializations.
struct xdr_traits_base {
  static Constexpr const bool valid = true;
  static Constexpr const bool xdr_defined = true;
  static Constexpr const bool is_bytes = false;
  static Constexpr const bool is_class = false;
  static Constexpr const bool is_enum = false;
  static Constexpr const bool is_container = false;
  static Constexpr const bool is_numeric = false;
  static Constexpr const bool is_struct = false;
  static Constexpr const bool is_union = false;
  static Constexpr const bool has_fixed_size = false;
};


////////////////////////////////////////////////////////////////
// Support for numeric types and bool
////////////////////////////////////////////////////////////////

//! A reinterpret-cast like function that works between types such as
//! floating-point and integers of the same size.  Used in marshaling,
//! so that a single set of byteswap routines can be used on all
//! numeric types including floating point.  Uses a union to avoid
//! strict pointer aliasing problems.
template<typename To, typename From> inline To
xdr_reinterpret(From f)
{
  static_assert(sizeof(To) == sizeof(From),
		"xdr_reinterpret with different sized objects");
  union {
    From from;
    To to;
  };
  from = f;
  return to;
}

//! Default traits for use as supertype of specializations of \c
//! xdr_traits for integral types.
template<typename T, typename U> struct xdr_integral_base : xdr_traits_base {
  using type = T;
  using uint_type = U;
  static Constexpr const bool xdr_defined = false;
  static Constexpr const bool is_numeric = true;
  static Constexpr const bool has_fixed_size = true;
  static Constexpr const std::size_t fixed_size = sizeof(uint_type);
  static Constexpr std::size_t serial_size(type) { return fixed_size; }
  static uint_type to_uint(type t) { return t; }
  static type from_uint(uint_type u) {
    return xdr_reinterpret<type>(u);
  }
};
template<> struct xdr_traits<std::int32_t>
  : xdr_integral_base<std::int32_t, std::uint32_t> {
  // Numeric type for case labels in switch statements
  using case_type = std::int32_t;
};
template<> struct xdr_traits<std::uint32_t>
  : xdr_integral_base<std::uint32_t, std::uint32_t> {
  using case_type = std::uint32_t;
};
template<> struct xdr_traits<std::int64_t>
  : xdr_integral_base<std::int64_t, std::uint64_t> {};
template<> struct xdr_traits<std::uint64_t>
  : xdr_integral_base<std::uint64_t, std::uint64_t> {};

//! Default traits for use as supertype of specializations of \c
//! xdr_traits for floating-point types.
template<typename T, typename U> struct xdr_fp_base : xdr_traits_base {
  using type = T;
  using uint_type = U;
  static_assert(sizeof(type) == sizeof(uint_type),
		"Cannot reinterpret between float and int of different size");
  static Constexpr const bool xdr_defined = false;
  static Constexpr const bool is_numeric = true;
  static Constexpr const bool has_fixed_size = true;
  static Constexpr const std::size_t fixed_size = sizeof(uint_type);
  static Constexpr std::size_t serial_size(type) { return fixed_size; }

  static uint_type to_uint(type t) { return xdr_reinterpret<uint_type>(t); }
  static type from_uint(uint_type u) { return xdr_reinterpret<type>(u); }
};
template<> struct xdr_traits<float>
  : xdr_fp_base<float, std::uint32_t> {};
template<> struct xdr_traits<double>
  : xdr_fp_base<double, std::uint64_t> {};


template<> struct xdr_traits<bool>
  : xdr_integral_base<bool, std::uint32_t> {
  using case_type = std::int32_t;
  static Constexpr const bool xdr_defined = false;
  static Constexpr const bool is_enum = true;
  static Constexpr const bool is_numeric = false;
  static type from_uint(uint_type u) { return u != 0; }
  static Constexpr const char *enum_name(uint32_t b) {
    return b == 0 ? "FALSE" : b == 1 ? "TRUE" : nullptr;
  }
  static const std::vector<int32_t> &enum_values() {
    static const std::vector<int32_t> v = { 0, 1 };
    return v;
  }
};


////////////////////////////////////////////////////////////////
// XDR containers (xvector, xarrray, pointer) and bytes (xstring,
// opaque_vec, opaque_array)
////////////////////////////////////////////////////////////////

//! Maximum length of vectors.  (The RFC says 0xffffffff, but out of
//! paranoia for integer overflows we chose something that still fits
//! in 32 bits when rounded up to a multiple of four.)
static Constexpr const uint32_t XDR_MAX_LEN = 0xfffffffc;

namespace detail {
//! Convenience supertype for traits of the three container types
//! (xarray, xvectors, and pointer).
template<typename T, bool variable,
	 bool VFixed = detail::has_fixed_size_t<typename T::value_type>::value>
struct xdr_container_base : xdr_traits_base {
  using value_type = typename T::value_type;
  static Constexpr const bool is_container = true;
  //! Container has variable number of elements
  static Constexpr const bool variable_nelem = variable;
  static Constexpr const bool has_fixed_size = false;

  template<typename Archive> static void save(Archive &a, const T &t) {
    if (variable)
      archive(a, size32(t.size()));
    for (const value_type &v : t)
      archive(a, v);
  }
  template<typename Archive> static void load(Archive &a, T &t) {
    uint32_t n;
    if (variable) {
      archive(a, n);
      t.check_size(n);
      if (t.size() > n)
	t.resize(n);
    }
    else
      n = size32(t.size());
    for (uint32_t i = 0; i < n; ++i)
      archive(a, t.extend_at(i));
  }
  static std::size_t serial_size(const T &t) {
    std::size_t s = variable ? 4 : 0;
    for (const value_type &v : t)
      s += xdr_traits<value_type>::serial_size(v);
    return s;
  }
};

template<typename T> struct xdr_container_base<T, true, true>
  : xdr_container_base<T, true, false> {
  static std::size_t serial_size(const T &t) {
    return 4 + t.size() * xdr_traits<typename T::value_type>::fixed_size;
  }
};

template<typename T> struct xdr_container_base<T, false, true>
  : xdr_container_base<T, false, false> {
  static Constexpr const bool has_fixed_size = true;
  static Constexpr const std::size_t fixed_size =
    T::container_fixed_nelem * xdr_traits<typename T::value_type>::fixed_size;
  static std::size_t serial_size(const T &) { return fixed_size; }
};

//! Placeholder type to avoid clearing array
struct no_clear_t {
  Constexpr no_clear_t() {}
};
Constexpr const no_clear_t no_clear;
} // namespace detail

//! XDR arrays are implemented using std::array as a supertype.
template<typename T, uint32_t N> struct xarray
  : std::array<T, size_t(N)> {
  using array = std::array<T, size_t(N)>;
  xarray() { array::fill(T{}); }
  xarray(detail::no_clear_t) {}
  xarray(const xarray &) = default;
  xarray &operator=(const xarray &) = default;

  static Constexpr const std::size_t container_fixed_nelem = N;
  static Constexpr std::size_t size() { return N; }
  static void validate() {}
  static void check_size(uint32_t i) {
    if (i != N)
      throw xdr_overflow("invalid size in xdr::xarray");
  }
  static void resize(uint32_t i) {
    if (i != N)
      throw xdr_overflow("invalid resize in xdr::xarray");
  }
  T &extend_at(uint32_t i) {
    if (i >= N)
      throw xdr_overflow("attempt to access invalid position in xdr::xarray");
    return (*this)[i];
  }
};

template<typename T, uint32_t N>
struct xdr_traits<xarray<T,N>>
  : detail::xdr_container_base<xarray<T,N>, false> {};

//! XDR \c opaque is represented as std::uint8_t;
template<uint32_t N = XDR_MAX_LEN> struct opaque_array
  : xarray<std::uint8_t,N> {
  using xarray = xdr::xarray<std::uint8_t,N>;
  using xarray::xarray;
  // Pay a little performance to avoid heartbleed-type errors...
  opaque_array() : xarray(detail::no_clear) { std::memset(this->data(), 0, N); }
  opaque_array(detail::no_clear_t) : xarray(detail::no_clear) {}
};
template<uint32_t N> struct xdr_traits<opaque_array<N>> : xdr_traits_base {
  static Constexpr const bool is_bytes = true;
  static Constexpr const std::size_t has_fixed_size = true;
  static Constexpr const std::size_t fixed_size =
    (std::size_t(N) + std::size_t(3)) & ~std::size_t(3);
  static std::size_t serial_size(const opaque_array<N> &) { return fixed_size; }
  static Constexpr const bool variable_nelem = false;
};


//! A vector with a maximum size (returned by xvector::max_size()).
//! Note that you can exceed the size, but an error will happen when
//! marshaling or unmarshaling the data structure.
template<typename T, uint32_t N = XDR_MAX_LEN>
struct xvector : std::vector<T> {
  using vector = std::vector<T>;
  using vector::vector;

  //! Return the maximum size allowed by the type.
  static Constexpr uint32_t max_size() { return N; }

  //! Check whether a size is in bounds
  static void check_size(size_t n) {
    if (n > max_size())
      throw xdr_overflow("xvector overflow");
  }

  void append(const T *elems, std::size_t n) {
    check_size(this->size() + n);
    this->insert(this->end(), elems, elems + n);
  }
  T &extend_at(uint32_t i) {
    if (i >= N)
      throw xdr_overflow("attempt to access invalid position in xdr::xvector");
    if (i == this->size())
      this->emplace_back();
    return (*this)[i];
  }
  void resize(uint32_t n) {
    check_size(n);
    vector::resize(n);
  }
};

namespace detail {
template<typename T> struct has_fixed_size_t<xvector<T>> : std::false_type {};
}

template<typename T, uint32_t N> struct xdr_traits<xvector<T,N>>
  : detail::xdr_container_base<xvector<T,N>, true> {};

//! Variable-length opaque data is just a vector of std::uint8_t.
template<uint32_t N = XDR_MAX_LEN> using opaque_vec = xvector<std::uint8_t, N>;
template<uint32_t N>
struct xdr_traits<xvector<std::uint8_t, N>> : xdr_traits_base {
  static Constexpr const bool is_bytes = true;
  static Constexpr const bool has_fixed_size = false;;
  static Constexpr std::size_t serial_size(const opaque_vec<N> &a) {
    return (std::size_t(a.size()) + std::size_t(7)) & ~std::size_t(3);
  }
  static Constexpr const bool variable_nelem = true;
};


//! A string with a maximum length (returned by xstring::max_size()).
//! Note that you can exceed the size, but an error will happen when
//! marshaling or unmarshaling the data structure.
template<uint32_t N = XDR_MAX_LEN> struct xstring : std::string {
  using string = std::string;

  //! Return the maximum size allowed by the type.
  static Constexpr uint32_t max_size() { return N; }

  //! Check whether a size is in bounds
  static void check_size(size_t n) {
    if (n > max_size())
      throw xdr_overflow("xstring overflow");
  }

  //! Check that the string length is not greater than the maximum
  //! size.  \throws std::out_of_range and clears the contents of the
  //! string if it is too long.
  void validate() const { check_size(size()); }

  xstring() = default;
  xstring(const xstring &) = default;
  xstring(xstring &&) = default;
  xstring &operator=(const xstring &) = default;
  xstring &operator=(xstring &&) = default;

  template<typename...Args> xstring(Args&&...args)
    : string(std::forward<Args>(args)...) { validate(); }

  using string::data;
  char *data() { return &(*this)[0]; } // protobufs does this, so probably ok

//! \hideinitializer
#define ASSIGN_LIKE(method)					\
  template<typename...Args> xstring &method(Args&&...args) {	\
    string::method(std::forward<Args>(args)...);		\
    validate();							\
    return *this;						\
  }
  ASSIGN_LIKE(operator=)
  ASSIGN_LIKE(operator+=)
  ASSIGN_LIKE(append)
  ASSIGN_LIKE(push_back)
  ASSIGN_LIKE(assign)
  ASSIGN_LIKE(insert)
  ASSIGN_LIKE(replace)
  ASSIGN_LIKE(swap)
#undef ASSIGN_LIKE

  void resize(size_type n) { check_size(n); string::resize(n); }
  void resize(size_type n, char ch) { check_size(n); string::resize(n, ch); }
};

template<uint32_t N> struct xdr_traits<xstring<N>> : xdr_traits_base {
  static Constexpr const bool is_bytes = true;
  static Constexpr const bool has_fixed_size = false;;
  static Constexpr std::size_t serial_size(const xstring<N> &a) {
    return (std::size_t(a.size()) + std::size_t(7)) & ~std::size_t(3);
  }
  static Constexpr const bool variable_nelem = true;
};


//! Optional data (represented with pointer notation in XDR source).
template<typename T> struct pointer : std::unique_ptr<T> {
  using value_type = T;
  using std::unique_ptr<T>::unique_ptr;
  using std::unique_ptr<T>::get;
  pointer() = default;
  pointer(const pointer &p) : std::unique_ptr<T>(p ? new T(*p) : nullptr) {}
  pointer(pointer &&p) = default;
  pointer &operator=(const pointer &up) {
    if (const T *tp = up.get()) {
      if (T *selfp = this->get())
	*selfp = *tp;
      else
	this->reset(new T(*tp));
    }
    else
      this->reset();
    return *this;
  }
  pointer &operator=(pointer &&) = default;

  static void check_size(uint32_t n) {
    if (n > 1)
      throw xdr_overflow("xdr::pointer size must be 0 or 1");
  }
  uint32_t size() const { return *this ? 1 : 0; }
  T *begin() { return get(); }
  const T *begin() const { return get(); }
  T *end() { return begin() + size(); }
  const T *end() const { return begin() + size(); }
  T &extend_at(uint32_t i) {
    if (i != 0)
      throw xdr_overflow("attempt to access position > 0 in xdr::pointer");
    if (!size())
      this->reset(new T);
    return **this;
  }
  void resize(uint32_t n) {
    if (n == size())
      return;
    switch(n) {
    case 0:
      this->reset();
      break;
    case 1:
      this->reset(new T);
      break;
    default:
      throw xdr_overflow("xdr::pointer::resize: valid sizes are 0 and 1");
    }
  }
  T &activate() {
    if (!*this)
      this->reset(new T{});
    return *this->get();
  }

  //! Compare by value, rather than looking at the value of the pointer.
  friend bool operator==(const pointer &a, const pointer &b) {
    return (!a && !b) || (a && b && *a == *b);
  }
  friend bool operator!=(const pointer &a, const pointer &b) {
    return !(a == b);
  }
  friend bool operator<(const pointer &a, const pointer &b) {
    return (!a && b) || (a && b && *a < *b);
  }
  friend bool operator>(const pointer &a, const pointer &b) {
    return b < a;
  }
  friend bool operator<=(const pointer &a, const pointer &b) {
    return !(b < a);
  }
  friend bool operator>=(const pointer &a, const pointer &b) {
    return !(a < b);
  }
};

// Note an explicit third template argument (VFixed = false) is
// required because pointers are used recursively, so we might not
// have xdr_traits<T> available at the time we instantiate
// xdr_traits<pointer<T>>.
template<typename T> struct xdr_traits<pointer<T>>
  : detail::xdr_container_base<pointer<T>, true, false> {};


////////////////////////////////////////////////////////////////
// Support for XDR struct types
////////////////////////////////////////////////////////////////

//! Type-level representation of a pointer-to-member value.  When used
//! as a function object, dereferences the field, and returns it as
//! the same reference type as its argument (lvalue rference, const
//! lvalue reference, or const rvalue reference).
template<typename T, typename F, F T::*Ptr> struct field_ptr {
  using class_type = T;
  using field_type = F;
  using value_type = F T::*;
  //static constexpr value_type value = Ptr;
  static Constexpr value_type value() { return Ptr; }
  F &operator()(T &t) const { return t.*Ptr; }
  const F &operator()(const T &t) const { return t.*Ptr; }
  F &operator()(T &&t) const { return std::move(t.*Ptr); }
};

template<typename ...Fields> struct xdr_struct_base;

namespace detail {
//! Default traits for fixed-size structures.
template<typename FP, typename ...Fields>
  struct xdr_struct_base_fs : xdr_struct_base<Fields...> {
  static Constexpr const bool has_fixed_size = true;
  static Constexpr const std::size_t fixed_size =
    (xdr_traits<typename FP::field_type>::fixed_size
     + xdr_struct_base<Fields...>::fixed_size);
  static Constexpr std::size_t serial_size(const typename FP::class_type &) {
    return fixed_size;
  }
};
//! Default traits for variable-size structures.
template<typename FP, typename ...Fields>
  struct xdr_struct_base_vs : xdr_struct_base<Fields...> {
  static Constexpr const bool has_fixed_size = false;
  static std::size_t serial_size(const typename FP::class_type &t) {
    return (xdr_size(t.*(FP::value()))
	    + xdr_struct_base<Fields...>::serial_size(t));
  }
};
}

//! Supertype to construct XDR traits of structure objects, used in
//! output of the \c xdrc compiler.
template<> struct xdr_struct_base<> : xdr_traits_base {
  static Constexpr const bool is_class = true;
  static Constexpr const bool is_struct = true;
  static Constexpr const bool has_fixed_size = true;
  static Constexpr const std::size_t fixed_size = 0;
  template<typename T> static Constexpr std::size_t serial_size(const T&) {
    return fixed_size;
  }
};
template<typename FP, typename ...Rest> struct xdr_struct_base<FP, Rest...>
  : std::conditional<(detail::has_fixed_size_t<typename FP::field_type>::value
		      && xdr_struct_base<Rest...>::has_fixed_size),
    detail::xdr_struct_base_fs<FP, Rest...>,
    detail::xdr_struct_base_vs<FP, Rest...>>::type {
  using field_info = FP;
  using next_field = xdr_struct_base<Rest...>;
};


////////////////////////////////////////////////////////////////
// XDR-compatible representations of std::tuple and xdr_void
////////////////////////////////////////////////////////////////

//! Placeholder type used to contain a parameter pack of tuple
//! indices, so as to unpack a tuple in function call arguments.
template<std::size_t...N> struct indices {
  Constexpr indices() {}
};

namespace detail {

template<typename T, typename U> struct cat_indices;
template<std::size_t...M, std::size_t...N>
struct cat_indices<indices<M...>, indices<N...>> {
  using type = indices<M..., N...>;
};

template<std::size_t N> struct all_indices_helper {
  using type = typename cat_indices<typename all_indices_helper<N-1>::type,
				    indices<N-1>>::type;
};
template<> struct all_indices_helper<0> {
  using type = indices<>;
};

//! A type representing all tuple indices from 0 to N-1.
template<std::size_t N> using all_indices =
  typename all_indices_helper<N>::type;

template<std::size_t N, typename T> struct tuple_base;

template<std::size_t N, typename T> struct tuple_base_fs;
template<std::size_t N, typename...T>
struct tuple_base_fs<N, std::tuple<T...>> : xdr_traits_base {
  using type = std::tuple<T...>;
  using elem_type = typename std::remove_cv<
    typename std::remove_reference<
      typename std::tuple_element<N-1, type>::type>::type>::type;
  using next = tuple_base<N-1, type>;
  static Constexpr const bool xdr_defined = false;
  static Constexpr const bool is_class = true;
  static Constexpr const bool is_struct = true;
  static Constexpr const bool has_fixed_size = true;
  static Constexpr const std::uint32_t fixed_size =
    xdr_traits<elem_type>::fixed_size + next::fixed_size;
  static Constexpr std::size_t serial_size(const type &) { return fixed_size; }
};

template<std::size_t N, typename T> struct tuple_base_vs;
template<std::size_t N, typename...T>
struct tuple_base_vs<N, std::tuple<T...>> : xdr_traits_base {
  using type = std::tuple<T...>;
  using elem_type = typename std::remove_cv<
    typename std::remove_reference<
      typename std::tuple_element<N-1, type>::type>::type>::type;
  using next = tuple_base<N-1, type>;
  static Constexpr const bool xdr_defined = false;
  static Constexpr const bool is_class = true;
  static Constexpr const bool is_struct = true;
  static Constexpr const bool has_fixed_size = false;
  static std::size_t serial_size(const type &t) {
    return (xdr_traits<elem_type>::serial_size(std::get<N-1>(t))
	    + next::serial_size(t));
  }
};

template<typename...T> struct tuple_base<0, std::tuple<T...>>
  : xdr_traits_base {
  using type = std::tuple<T...>;
  static Constexpr const bool xdr_defined = false;
  static Constexpr const bool is_class = true;
  static Constexpr const bool is_struct = true;
  static Constexpr const bool has_fixed_size = true;
  static Constexpr const std::size_t fixed_size = 0;
  static Constexpr std::size_t serial_size(const type &) { return fixed_size; }

  template<typename Archive> static void save(Archive &, const type &) {}
  template<typename Archive> static void load(Archive &, type &) {}
};

template<std::size_t N, typename...T> struct tuple_suffix_fixed_size {
  using type = std::tuple<T...>;
  using elem_type = typename std::tuple_element<N-1, type>::type;
  static Constexpr const bool value =
    xdr_traits<elem_type>::has_fixed_size
    && tuple_suffix_fixed_size<N-1, T...>::value;
};
template<typename...T> struct tuple_suffix_fixed_size<0, T...> {
  static Constexpr const bool value = true;
};

template<std::size_t N, typename...T> struct tuple_base<N, std::tuple<T...>>
  : std::conditional<tuple_suffix_fixed_size<N, T...>::value,
		     tuple_base_fs<N, std::tuple<T...>>,
		     tuple_base_vs<N, std::tuple<T...>>>::type {
  using type = std::tuple<T...>;

  static const char *name() {
    static std::string n = "<" + std::to_string(N-1) + ">";
    return n.c_str();
  }

  template<typename Archive> static void save(Archive &ar, const type &obj) {
    tuple_base<N-1, type>::save(ar, obj);
    archive(ar, std::get<N-1>(obj), name());
  }
  template<typename Archive> static void load(Archive &ar, type &obj) {
    tuple_base<N-1, type>::load(ar, obj);
    archive(ar, std::get<N-1>(obj), name());
  }
};
}

//! A type representing all the indices of a particuar tuple.
template<typename T> using all_indices_of =
  typename detail::all_indices<std::tuple_size<
                              typename std::remove_reference<T>::type>::value>;

template<typename...T> struct xdr_traits<std::tuple<T...>>
  : detail::tuple_base<sizeof...(T), std::tuple<T...>> {};

//! Placehoder type representing void values marshaled as 0 bytes.
using xdr_void = std::tuple<>;


////////////////////////////////////////////////////////////////
// XDR union types
////////////////////////////////////////////////////////////////

namespace detail {
//! Dereference a pointer to a member of type \c F of a class of type
//! \c T, preserving reference types.  Hence applying to an lvalue
//! reference \c T returns an lvalue reference \c F, while an rvalue
//! reference \c T produces an rvalue reference \c F.
template<typename T, typename F> inline F &
member(T &t, F T::*mp)
{
  return t.*mp;
}
template<typename T, typename F> inline const F &
member(const T &t, F T::*mp)
{
  return t.*mp;
}
template<typename T, typename F> inline F &&
member(T &&t, F T::*mp)
{
  return std::move(t.*mp);
}
} // namespace detail

struct field_constructor_t {
  Constexpr field_constructor_t() {}
  template<typename T, typename F> void operator()(F T::*mp, T &t) const {
    new (&(t.*mp)) F{};
  }
  template<typename T, typename F, typename TT> void
  operator()(F T::*mp, T &t, TT &&tt) const {
    new (&(t.*mp)) F (detail::member(std::forward<TT>(tt), mp));
  }
};
//! Passed to the auto-generated _xdr_with_mem_ptr static method to
//! construct the active union field (or at least the union field
//! corresponding to the second argument to _xdr_with_mem_ptr, which
//! should be the active union field).
Constexpr const field_constructor_t field_constructor {};

struct field_destructor_t {
  Constexpr field_destructor_t() {}
  template<typename T, typename F> void
  operator()(F T::*mp, T &t) const { detail::member(t, mp).~F(); }
};
//! Passed to _xdr_with_mem_ptr to destroy the active union field.
Constexpr const field_destructor_t field_destructor {};

struct field_assigner_t {
  Constexpr field_assigner_t() {}
  template<typename T, typename F, typename TT> void
  operator()(F T::*mp, T &t, TT &&tt) const {
    detail::member(t, mp) = detail::member(std::forward<TT>(tt), mp);
  }
};
//! Passed to _xdr_with_mem_ptr to assign to the active union field.
Constexpr const field_assigner_t field_assigner {};

struct field_archiver_t {
  Constexpr field_archiver_t() {}

  template<typename F, typename T, typename Archive> void
  operator()(F T::*mp, Archive &ar, T &t, const char *name) const {
    archive(ar, detail::member(t, mp), name);
  }
  template<typename F, typename T, typename Archive> void
  operator()(F T::*mp, Archive &ar, const T &t, const char *name) const {
    archive(ar, detail::member(t, mp), name);
  }
};
//! Passed to _xdr_with_mem_ptr to archive the active union field.
Constexpr const field_archiver_t field_archiver {};

struct field_size_t {
  Constexpr field_size_t() {}
  template<typename F, typename T> void
  operator()(F T::*mp, const T &t, std::size_t &size) const {
    size = xdr_traits<F>::serial_size(detail::member(t, mp));
  }
};
//! Passed to _xdr_with_mem_ptr to compute the size of the active
//! union field.
Constexpr const field_size_t field_size {};


////////////////////////////////////////////////////////////////
// Comparison operators
////////////////////////////////////////////////////////////////

namespace detail {
template<typename T, typename F> struct struct_equal_helper {
  static bool equal(const T &a, const T &b) {
    Constexpr const typename F::field_info fi {};
    if (!(fi(a) == fi(b)))
      return false;
    return struct_equal_helper<T, typename F::next_field>::equal(a, b);
  }
};
template<typename T> struct struct_equal_helper<T, xdr_struct_base<>> {
  static bool equal(const T &, const T &) { return true; }
};

template<typename T, typename F> struct struct_lt_helper;
} // namespace detail


//! Equality for XDR structures.  To use this operator, you will have
//! to include using declaration <tt>using xdr::operator==</tt> in the
//! namespace of your XDR file.  Note that a <tt>using namespace
//! xdr</tt> using \e directive (as opposed to \e declaration) is
//! insufficient, because the C++ standard explicitly prevents using
//! directives from impacting argument-dependent lookup.  A <tt>using
//! namespace xdr</tt> directive at global scope is also insufficient,
//! though more subtly, because standard library functions for
//! comparing vectors will fail.
template<typename T> inline typename
std::enable_if<xdr_traits<T>::is_struct && xdr_traits<T>::xdr_defined,
	       bool>::type
operator==(const T &a, const T &b)
{
  return detail::struct_equal_helper<T, xdr_traits<T>>::equal(a, b);
}

template<typename T> inline typename
std::enable_if<xdr_traits<T>::is_struct && xdr_traits<T>::xdr_defined,
	       bool>::type
operator!=(const T &a, const T &b)
{
  return !(a == b);
}

//! Ordering of XDR structures.  See note at \c xdr::operator==.
template<typename T> inline typename
std::enable_if<xdr_traits<T>::is_struct && xdr_traits<T>::xdr_defined,
	       bool>::type
operator<(const T &a, const T &b)
{
  return detail::struct_lt_helper<T, xdr_traits<T>>::lt(a, b);
}

template<typename T> inline typename
std::enable_if<xdr_traits<T>::is_struct && xdr_traits<T>::xdr_defined,
	       bool>::type
operator>(const T &a, const T &b)
{
  return b < a;
}

template<typename T> inline typename
std::enable_if<xdr_traits<T>::is_struct && xdr_traits<T>::xdr_defined,
	       bool>::type
operator<=(const T &a, const T &b)
{
  return !(b < a);
}

template<typename T> inline typename
std::enable_if<xdr_traits<T>::is_struct && xdr_traits<T>::xdr_defined,
	       bool>::type
operator>=(const T &a, const T &b)
{
  return !(a < b);
}

namespace detail {
struct union_field_equal_t {
  Constexpr union_field_equal_t() {}
  template<typename T, typename F>
  void operator()(F T::*mp, const T &a, const T &b, bool &out) const {
    out = a.*mp == b.*mp;
  }
};
Constexpr const union_field_equal_t union_field_equal {};

struct union_field_lt_t {
  Constexpr union_field_lt_t() {}
  template<typename T, typename F>
  void operator()(F T::*mp, const T &a, const T &b, bool &out) const {
    out = a.*mp < b.*mp;
  }
};
Constexpr const union_field_lt_t union_field_lt {};
} // namespace detail

//! Equality of XDR unions.  See note at \c xdr::operator== for XDR
//! structs.
template<typename T> inline typename
std::enable_if<xdr_traits<T>::is_union, bool>::type
operator==(const T &a, const T &b)
{
  if (a._xdr_discriminant() == b._xdr_discriminant()) {
    bool r{true};
    a._xdr_with_mem_ptr(detail::union_field_equal,
			a._xdr_discriminant(), a, b, r);
    return r;
  }
  return false;
}

//! Ordering of XDR unions.  See note at \c xdr::operator==.
template<typename T> inline typename
std::enable_if<xdr_traits<T>::is_union, bool>::type
operator<(const T &a, const T &b)
{
  if (a._xdr_discriminant() < b._xdr_discriminant())
    return true;
  if (b._xdr_discriminant() < a._xdr_discriminant())
    return false;

  bool r{false};
  a._xdr_with_mem_ptr(detail::union_field_lt,
		      a._xdr_discriminant(), a, b, r);
  return r;
}

namespace detail {
template<typename T, typename F> struct struct_lt_helper {
  static bool lt(const T &a, const T &b) {
    Constexpr const typename F::field_info fi {};
    if ((fi(a) < fi(b)))
      return true;
    if ((fi(b) < fi(a)))
      return false;
    return struct_lt_helper<T, typename F::next_field>::lt(a, b);
  }
};
template<typename T> struct struct_lt_helper<T, xdr_struct_base<>> {
  static bool lt(const T &, const T &) { return false; }
};
} // namespace detail

} // namespace xdr

#endif // !_XDRC_TYPES_H_HEADER_INCLUDED_
