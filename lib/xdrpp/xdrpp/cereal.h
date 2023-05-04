// -*- C++ -*-

/** \file cereal.h Interface for
 * [cereal](http://uscilab.github.io/cereal/) serealization back ends.
 * By including this file, you can archive any XDR data structure
 * using any format supported by [cereal].  Note that only the binary
 * structures actually sanity-check the lengths of strings.  You don't
 * need to do anything with this file other than include it. Simply
 * follow the [cereal] archive documentation to proceed, without the
 * need to implement your own `archive` (or `save`/`load`) methods.
 *
 * Note you still need to include the cereal archive headers you want
 * to use, e.g., <tt>#include &lt;cereal/archives/json.hpp&gt;</tt>.
 *
 * [cereal]: http://uscilab.github.io/cereal/
 */

#ifndef _XDRPP_CEREAL_H_HEADER_INCLUDED_
#define _XDRPP_CEREAL_H_HEADER_INCLUDED_ 1

#include <type_traits>
#include <cereal/cereal.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/details/traits.hpp>
#include <xdrpp/types.h>

namespace cereal {
class JSONInputArchive;
class JSONOutputArchive;
class XMLOutputArchive;
class XMLInputArchive;
} // namespace cereal

namespace xdr {

namespace detail {
template<typename Archive, typename T>
std::enable_if_t<xdr_traits<T>::is_class>
save(Archive &ar, const T &t)
{
  xdr_traits<T>::save(ar, t);
}

template<typename Archive, typename T>
std::enable_if_t<xdr_traits<T>::is_class>
load(Archive &ar, T &t)
{
  xdr_traits<T>::load(ar, t);
}

template<typename Archive> constexpr bool is_cereal_binary =
  cereal::traits::is_output_serializable<
  cereal::BinaryData<char *>,Archive>::value
  || cereal::traits::is_input_serializable<
  cereal::BinaryData<char *>,Archive>::value;

template<typename T> struct bytes_superclass {
  static constexpr bool is_bytes = false;
};
#define DECLARE_BYTES(TYPE, ...)				\
template<uint32_t N> struct bytes_superclass<TYPE> {		\
  static constexpr bool is_bytes = true;			\
  using type = TYPE;						\
  using super = __VA_ARGS__;					\
  static super &upcast(type &t) { return t; }			\
  static const super &upcast(const type &t) { return t; }	\
};
DECLARE_BYTES(opaque_array<N>, std::array<std::uint8_t, size_t(N)>)
DECLARE_BYTES(opaque_vec<N>, std::vector<std::uint8_t>)
DECLARE_BYTES(xstring<N>, std::string)
#undef DECLARE_BYTES

template<typename Archive, typename T>
std::enable_if_t<!is_cereal_binary<Archive> && bytes_superclass<T>::is_bytes>
save(Archive &ar, const T &t)
{
  ar(bytes_superclass<T>::upcast(t));
}

template<typename Archive, typename T>
std::enable_if_t<!is_cereal_binary<Archive> && bytes_superclass<T>::is_bytes>
load(Archive &ar, T &t)
{
  ar(bytes_superclass<T>::upcast(t));
}

template<typename Archive, typename T>
std::enable_if_t<is_cereal_binary<Archive> && bytes_superclass<T>::is_bytes>
save(Archive &ar, const T &t)
{
  if (xdr_traits<T>::variable_nelem)
    ar(cereal::make_size_tag(static_cast<cereal::size_type>(t.size())));
  ar(cereal::binary_data(const_cast<char *>(
         reinterpret_cast<const char *>(t.data())), t.size()));
}

template<typename Archive, typename T>
std::enable_if_t<is_cereal_binary<Archive> && bytes_superclass<T>::is_bytes>
load(Archive &ar, T &t)
{
  cereal::size_type size;
  if (xdr_traits<T>::variable_nelem)
    ar(cereal::make_size_tag(size));
  else
    size = t.size();
  t.check_size(size);
  t.resize(static_cast<std::uint32_t>(size));
  ar(cereal::binary_data(t.data(), size));
}

// value is true iff there exists a function
//
// cereal_override(Archive&, T, const char *)
//
// or at least callable with such arguments.
template<typename Archive, typename T> class has_cereal_override {
  template<typename U> static std::true_type
  test(decltype(cereal_override(std::declval<Archive&>(),
				std::declval<U>(),
				"")) *);

  template<typename U> static std::false_type test(...);

public:
  static constexpr bool value = decltype(test<T>(0))::value;
};

template<typename Archive> struct nvp_adapter {
  template<typename T> static
  std::enable_if_t<!has_cereal_override<Archive, T>::value>
  apply(Archive &ar, T &&t, const char *field) {
    if (field)
      ar(cereal::make_nvp(field, std::forward<T>(t)));
    else
      ar(std::forward<T>(t));
  }

  template<typename T> static
  std::enable_if_t<has_cereal_override<Archive, T>::value>
  apply(Archive &ar, T &&t, const char *field) {
    cereal_override(ar, std::forward<T>(t), field);
  }
};
} // namespace detail

//! \hideinitializer \cond
#define CEREAL_ARCHIVE_TAKES_NAME(archive)		\
template<> struct archive_adapter<cereal::archive>	\
 : detail::nvp_adapter<cereal::archive> {}
CEREAL_ARCHIVE_TAKES_NAME(JSONInputArchive);
CEREAL_ARCHIVE_TAKES_NAME(JSONOutputArchive);
CEREAL_ARCHIVE_TAKES_NAME(XMLOutputArchive);
CEREAL_ARCHIVE_TAKES_NAME(XMLInputArchive);
#undef CEREAL_ARCHIVE_TAKES_NAME
//! \endcond


} // namespace xdr

namespace cereal {
using xdr::detail::load;
using xdr::detail::save;
} // namespace cereal

#endif // !_XDRPP_CEREAL_H_HEADER_INCLUDED_
