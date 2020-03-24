// -*- C++ -*-

/** \file endian.h Low-level byteswap and miscellaneous OS
 *  compatibility routines. */

#ifndef _XDRC_ENDIAN_H_HEADER_INCLUDED_
#define _XDRC_ENDIAN_H_HEADER_INCLUDED_ 1

#include <cstdint>
#include <string>

namespace xdr {

#if defined(_MSC_VER) && !defined(MSVC)
#define MSVC 1
#endif // _MSC_VER && !MSVC

#if MSVC
inline std::string
errstr(int no)
{
  char buf[1024];
  if (strerror_s(buf, sizeof(buf), no) == 0) {
    return std::string(buf);
  }
  return std::string();
}
#define xdr_strerror xdr::errstr

#else // !MSVC
#define xdr_strerror std::strerror
#endif // !MSVC

#define Constexpr constexpr

#ifndef XDRPP_WORDS_BIGENDIAN
#if MSVC
#define XDRPP_WORDS_BIGENDIAN 0
#else // !MSVC
#include <xdrpp/build_endian.h>
#endif // !MSVC
#endif // !XDRPP_WORDS_BIGENDIAN

//! True on big endian machines, false on little endian machines.
#if XDRPP_WORDS_BIGENDIAN
Constexpr const bool is_big_endian = true;
#else // !XDRPP_WORDS_BIGENDIAN
Constexpr const bool is_big_endian = false;
#endif // !XDRPP_WORDS_BIGENDIAN

// Needed on OpenBSD
#undef swap32
#undef swap64

//! Byteswap 32-bit number.
Constexpr inline std::uint32_t
swap32(std::uint32_t v)
{
  return v << 24 | (v & 0xff00) << 8 | (v >> 8 & 0xff00) | v >> 24;
}

//! Byteswap 64-bit number.
Constexpr inline std::uint64_t
swap64(std::uint64_t v)
{
  return std::uint64_t(swap32(std::uint32_t(v)))<<32 |
         std::uint64_t(swap32(std::uint32_t(v>>32)));
}

//! Byteswap 32-bit value only on little-endian machines, identity
//! function on big-endian machines.
Constexpr inline std::uint32_t
swap32le(std::uint32_t v)
{
  return xdr::is_big_endian ? v : swap32(v);
}

//! Byteswap 32-bit value only on big-endian machines.
Constexpr inline std::uint32_t
swap32be(std::uint32_t v)
{
  return xdr::is_big_endian ? swap32(v) : v;
}

//! Byteswap 64-bit value only on little-endian machines.
Constexpr inline std::uint64_t
swap64le(std::uint64_t v)
{
  return xdr::is_big_endian ? v : swap64(v);
}

//! Byteswap 64-bit value only on big-endian machines.
Constexpr inline std::uint64_t
swap64be(std::uint64_t v)
{
  return xdr::is_big_endian ? swap64(v) : v;
}

}

#endif // !_XDRC_ENDIAN_H_HEADER_INCLUDED_
