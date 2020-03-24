// -*- C++ -*-

#ifndef _XDRPP_MESSAGE_H_HEADER_INCLUDED_
#define _XDRPP_MESSAGE_H_HEADER_INCLUDED_ 1

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <xdrpp/endian.h>
#include <xdrpp/socket.h>

struct sockaddr;

//! \file message.h Message buffer with space for marshaled length.

namespace xdr {

class message_t;
namespace detail {
struct free_message_t {
  void operator()(message_t *p);
};
} // namespace detail
using msg_ptr = std::unique_ptr<message_t, detail::free_message_t>;

//! Message buffer, with room at beginning for 4-byte length.  Note
//! the constructor is private, so you must create one with \c
//! message_t::alloc, which allocates more space than the size of the
//! \c message_t structure.  Hence \c message_t is just a data
//! structure at the beginning of the buffer.
class message_t {
  std::unique_ptr<sockaddr> peer_;
  std::size_t size_;
  alignas(std::uint32_t) char buf_[4];
  message_t(std::size_t size) : size_(size) {}
public:
  std::size_t size() const { return size_; }
  void shrink(std::size_t newsize);
  char *data() { return buf_ + 4; }
  const char *data() const { return buf_ + 4; }
  uint32_t word(std::ptrdiff_t i) const {
    return reinterpret_cast<const uint32_t *>(data())[i];
  }
  //const void *offset(std::ptrdiff_t i) const { return buf_ + i; }
  //! End of the buffer (one past last byte).
  char *end() { return buf_ + 4 + size_; }
  const char *end() const { return buf_ + 4 + size_; }

  //! 4-byte buffer to store size in network byte order, followed by data.
  char *raw_data() { return buf_; }
  const char *raw_data() const { return buf_; }
  //! Size of 4-byte length plus data.
  std::size_t raw_size() const { return size_ + 4; }

  //! Return socket address of peer (or nullptr if none).
  const sockaddr *peer() const { return peer_.get(); }
  //! Returns unique_ptr to peer address so it can be set/moved.
  std::unique_ptr<sockaddr> &&unique_peer() { return std::move(peer_); }

  //! Allocate a new buffer.
  static msg_ptr alloc(std::size_t size);
};

static_assert(std::is_standard_layout<message_t>::value,
	      "message_t should be standard layout");

}

#endif // !_XDRPP_MESSAGE_H_HEADER_INCLUDED_
