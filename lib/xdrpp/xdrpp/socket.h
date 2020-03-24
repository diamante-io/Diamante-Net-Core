// -*- C++ -*-

#ifndef _XDRPP_SOCKET_H_HEADER_INCLUDED_
#define _XDRPP_SOCKET_H_HEADER_INCLUDED_ 1

//! \file socket.h Simplified support for creating sockets.

#include <memory>
#include <system_error>
#if MSVC
#include <WinSock2.h>
#include <Ws2tcpip.h>
struct iovec {
  void *iov_base;
  size_t iov_len;
};
typedef int socklen_t;
typedef unsigned __int64 ssize_t;

#else // !MSVC
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>
#endif // !MSVC

namespace xdr {

//! Abstract away the type of a socket (for windows).
struct sock_t {
#if MSVC
  using type = SOCKET;
  static constexpr type invalid = INVALID_SOCKET;
#else // !MSVC
  using type = int;
  static constexpr type invalid = -1;
#endif // !MSVC

  type fd_;
  constexpr sock_t() : fd_(invalid) {}
  constexpr sock_t(type fd) : fd_(fd) {}

  bool operator==(sock_t s) const { return fd_ == s.fd_; }
  bool operator!=(sock_t s) const { return fd_ != s.fd_; }
  bool operator<(sock_t s) const { return fd_ < s.fd_; }
  explicit operator bool() const { return fd_ != invalid; }
  type fd() const { return fd_; }
};
} namespace std {
template<> struct hash<xdr::sock_t> {
  using argument_type = xdr::sock_t;
  using result_type = size_t;
  constexpr hash() {}
  size_t operator()(const xdr::sock_t s) const { return s.fd(); }
};
} namespace xdr {

constexpr sock_t invalid_sock{};

//! Returns the size of an AF_INET or AF_INET6 socket (based on sa_family).
socklen_t socksize(const sockaddr *sa);

//! Returns true if the most recent (socket) error is a temporary
//! error, such as EAGAIN, EWOULDBLOCK, or EINTR.
bool sock_eagain();

//! Last socket error message (\c strerror(errno) on POSIX).
const char *sock_errmsg();

//! Throw a \c system_error exception for the last socket error.
[[noreturn]] void throw_sockerr(const char *);

ssize_t read(sock_t s, void *buf, std::size_t count);
ssize_t write(sock_t s, const void *buf, std::size_t count);
ssize_t readv(sock_t s, const struct iovec *iov, int iovcnt);
ssize_t writev(sock_t s, const struct iovec *iov, int iovcnt);
void close(sock_t s);

//! Set the \c O_NONBLOCK flag on a socket.  \throws std::system_error
//! on failure.
void set_nonblock(sock_t s);

//! Set the close-on-exec flag of a file descriptor.  \throws
//! std::system_error on failure.
void set_close_on_exec(sock_t s);

//! Wrapper around accept for sock_t.
inline sock_t
accept(sock_t s, sockaddr *addr, socklen_t *addrlen)
{
  return ::accept(s.fd(), addr, addrlen);
}

//! Create a socket (or pipe on unix, where both are file descriptors)
//! that is connected to itself.
void create_selfpipe(sock_t ss[2]);

// The remaining functions should be the same on Unix/Windows

//! Category for system errors dealing with DNS (getaddrinfo, etc.).
const std::error_category &gai_category();

//! A deleter to use \c std::unique_ptr with \c addrinfo structures
//! (which must be freed recursively).
struct delete_addrinfo {
  constexpr delete_addrinfo() {}
  void operator()(addrinfo *ai) const { freeaddrinfo(ai); }
};
//! Automatically garbage-collected addrinfo pointer.
using unique_addrinfo = std::unique_ptr<addrinfo, delete_addrinfo>;

//! Wrapper around \c getaddrinfo that returns a garbage-collected
//! xdr::unique_addrinfo.  \throws std::system_error with
//! xdr::gai_category on failure.
unique_addrinfo get_addrinfo(const char *host,
			     int socktype = SOCK_STREAM,
			     const char *service = nullptr,
			     int family = AF_UNSPEC);

//! Return printable versions of numeric host and port number
void get_numinfo(const sockaddr *sa, socklen_t salen,
		 std::string *host, std::string *serv);

//! Self-closing socket.  Note that this socket will be closed as soon
//! as it goes out of scope, hence it is important to see whether
//! functions you pass it to take a "unique_sock &" or a "const
//! unique_sock &&".  In the latter case, you are expected to keep the
//! file descriptor around.
class unique_sock {
  sock_t s_;
public:
  unique_sock() : s_(invalid_sock) {}
  explicit unique_sock(sock_t s) : s_(s) {}
  unique_sock(unique_sock &&uf) : s_(uf.release()) {}
  ~unique_sock() { clear(); }
  unique_sock &operator=(unique_sock &&uf) {
    clear();
    s_ = uf.release();
    return *this;
  }

  //! Return the file descriptor number, but maintain ownership.
  sock_t get() const { return s_; }
  //! True if the file descriptor is not -1.
  explicit operator bool() const { return s_ != invalid_sock; }
  //! Return the file descriptor number, relinquishing ownership of
  //! it.  The \c unique_sock will have file descriptor -1 after this
  //! method returns.
  sock_t release() {
    sock_t ret = s_;
    s_ = invalid_sock;
    return ret;
  }
  void clear() {
    if (s_ != invalid_sock) {
      close(s_);
      s_ = invalid_sock;
    }
  }
  void reset(sock_t s) { clear(); s_ = s; }
  //! Return underlying file descriptor.
  sock_t::type fd() const { return s_.fd(); }
};

//! Try connecting to the first \b addrinfo in a linked list.
unique_sock tcp_connect1(const addrinfo *ai, bool ndelay = false);

//! Try connecting to every \b addrinfo in a list until one succeeds.
unique_sock tcp_connect(const addrinfo *ai);
inline unique_sock
tcp_connect(const unique_addrinfo &ai)
{
  return tcp_connect(ai.get());
}
unique_sock tcp_connect(const char *host, const char *service,
			int family = AF_UNSPEC);

//! Create bind a listening TCP socket.
unique_sock tcp_listen(const char *service = nullptr,
		       int family = AF_UNSPEC,
		       int backlog = 5);

//! Create and bind a UDP socket.
unique_sock udp_listen(const char *service = nullptr,
		       int family = AF_UNSPEC);

//! Returns SOCK_STREAM or SOCK_DGRAM.
int socket_type(int fd);
}

#endif // !_XDRPP_SOCKET_H_HEADER_INCLUDED_
