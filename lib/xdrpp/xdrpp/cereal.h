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
}

namespace xdr {

namespace detail {
template<typename Archive, typename T> typename
std::enable_if<xdr_traits<T>::is_class>::type
save(Archive &ar, const T &t)
{
  xdr_traits<T>::save(ar, t);
}

template<typename Archive, typename T> typename
std::enable_if<xdr_traits<T>::is_class>::type
load(Archive &ar, T &t)
{
  xdr_traits<T>::load(ar, t);
}

template<typename Archive, typename T> typename
std::enable_if<cereal::traits::is_output_serializable<
		 cereal::BinaryData<char *>,Archive>::value
               && xdr_traits<T>::is_bytes>::type
save(Archive &ar, const T &t)
{
  if (xdr_traits<T>::variable_length)
    ar(cereal::make_size_tag(static_cast<cereal::size_type>(t.size())));
  ar(cereal::binary_data(const_cast<char *>(
         reinterpret_cast<const char *>(t.data())), t.size()));
}

template<typename Archive, typename T> typename
std::enable_if<cereal::traits::is_input_serializable<
		 cereal::BinaryData<char *>,Archive>::value
               && xdr_traits<T>::is_bytes>::type
load(Archive &ar, T &t)
{
  cereal::size_type size;
  if (xdr_traits<T>::variable_length)
    ar(cereal::make_size_tag(size));
  else
    size = t.size();
  t.check_size(size);
  t.resize(static_cast<std::uint32_t>(size));
  ar(cereal::binary_data(t.data(), size));
}


template<typename Archive> struct nvp_adapter {
  template<typename T> static void
  apply(Archive &ar, T &&t, const char *field) {
    if (field)
      ar(cereal::make_nvp(field, std::forward<T>(t)));
    else
      ar(std::forward<T>(t));
  }

  template<uint32_t N> static void
  apply(Archive &ar, xstring<N> &s, const char *field) {
    apply(ar, field, static_cast<std::string &>(s));
  }
  template<uint32_t N> static void
  apply(Archive &ar, const xstring<N> &s, const char *field) {
    apply(ar, field, static_cast<const std::string &>(s));
  }
};
}

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


}

namespace cereal {
using xdr::detail::load;
using xdr::detail::save;
}

#endif // !_XDRPP_CEREAL_H_HEADER_INCLUDED_
