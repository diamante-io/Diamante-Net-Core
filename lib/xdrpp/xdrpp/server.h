// -*- C++ -*-

//! \file server.h Classes for implementing RPC servers.  We use the
//! following terminology:  The term \e service denotes a class that
//! responds to RPCs for a particular program/version combination.  A
//! \e server is a collection of services.  A \e listener attaches
//! incoming connections from clients to a particular server.  And a
//! \e session is state (if needed) associated with a particular
//! client connection.

#ifndef _XDRPP_SERVER_H_HEADER_INCLUDED_
#define _XDRPP_SERVER_H_HEADER_INCLUDED_ 1

#include <iostream>
#include <xdrpp/marshal.h>
#include <xdrpp/printer.h>
#include <xdrpp/msgsock.h>
#include <xdrpp/rpcbind.h>
#include <xdrpp/rpc_msg.hh>
#include <map>

namespace xdr {

extern bool xdr_trace_server;

//! Structure that gets marshalled as an RPC success header.
struct rpc_success_hdr {
  uint32_t xid;
  explicit constexpr rpc_success_hdr(uint32_t x) : xid(x) {}
};
template<> struct xdr_traits<rpc_success_hdr> : xdr_traits_base {
  static constexpr bool valid = true;
  static constexpr bool is_class = true;
  static constexpr bool is_struct = true;
  static constexpr bool has_fixed_size = true;
  static constexpr std::size_t fixed_size = 24;
  static constexpr std::size_t serial_size(const rpc_success_hdr &) {
    return fixed_size;
  }
  template<typename Archive> static void save(Archive &a,
					      const rpc_success_hdr &t) {
    archive(a, t.xid, "xid");
    archive(a, REPLY, "mtype");
    archive(a, MSG_ACCEPTED, "stat");
    archive(a, AUTH_NONE, "flavor");
    archive(a, uint32_t(0), "body");
    archive(a, SUCCESS, "stat");
  }
};

// The following produce various pre-formatted error responses.
msg_ptr rpc_accepted_error_msg(uint32_t xid, accept_stat stat);
msg_ptr rpc_prog_mismatch_msg(uint32_t xid, uint32_t low, uint32_t high);
msg_ptr rpc_auth_error_msg(uint32_t xid, auth_stat stat);
msg_ptr rpc_rpc_mismatch_msg(uint32_t xid);


//! A pointer, but that gets marshalled as the underlying object and
//! can convert to the underlying type.  If \c p is a \c
//! transparent_ptr<T>, then \c std::move(p) can be passed as a \c
//! std::unique_ptr<T>, a \c T, or a <tt>const T&</tt>.  This is what
//! allows some flexibility in the signatures of server object methods.
template<typename T> struct transparent_ptr : std::unique_ptr<T> {
  using std::unique_ptr<T>::unique_ptr;
  transparent_ptr() : std::unique_ptr<T>(new T{}) {}
  operator T &() const { return *this->get(); }
  operator T &&() { return std::move(*this->get()); }
};

namespace detail {
template<typename T, bool fs = xdr_traits<T>::has_fixed_size>
struct transparent_ptr_base : xdr_traits_base {
  static constexpr bool has_fixed_size = false;
};
template<typename T> struct transparent_ptr_base<T, true> : xdr_traits_base {
  static constexpr bool has_fixed_size = true;
  static constexpr std::size_t fixed_size = xdr_traits<T>::fixed_size;
};
}

template<typename T> struct xdr_traits<transparent_ptr<T>>
  : detail::transparent_ptr_base<T> {
  using t_traits = xdr_traits<T>;
  using ptr_type = std::unique_ptr<T>;

  static constexpr bool is_class = true;

  template<typename Archive> static void save(Archive &a, const ptr_type &p) {
    archive(a, *p);
  }
  template<typename Archive> static void load(Archive &a, ptr_type &p) {
    archive(a, *p);
  }
  static size_t serial_size(const ptr_type &p) {
    return t_traits::serial_size(*p);
  }
};

// This is what makes the pointer transparent during marshaling.
template<typename Archive, typename T> inline void
archive(Archive &ar, const transparent_ptr<T> &t, const char *name = nullptr)
{
  archive(ar, *t, name);
}

namespace detail {
template<typename T> struct wrap_transparent_ptr_helper;

template<typename...T>
struct wrap_transparent_ptr_helper<std::tuple<T...>> {
  using type = std::tuple<transparent_ptr<T>...>;
};
}

//! Wrap xdr::transparent_ptr around each type in a tuple to generate
//! a new tuple type.
template<typename T> using wrap_transparent_ptr =
  typename detail::wrap_transparent_ptr_helper<T>::type;


namespace detail {
template<typename P, typename C, typename T, typename I = all_indices_of<T>>
struct dispatch_session_helper;

template<typename P, typename C, typename T, std::size_t...I>
struct dispatch_session_helper<P, C, T, indices<I...>> {
  template<typename S, typename...Rest> static auto
    dispatch(C &&c, S *s, T &&t, Rest &&...rest) ->
    typename std::enable_if<!std::is_same<S, void>::value,
      decltype(P::dispatch(std::forward<C>(c), s,
			   std::get<I>(std::forward<T>(t))...,
			   std::forward<Rest>(rest)...))>::type
  {
    return P::dispatch(std::forward<C>(c), s,
		       std::get<I>(std::forward<T>(t))...,
		       std::forward<Rest>(rest)...);
  }

  // If the previous one fails SFINAE, try omitting the session
  // argument for methods that don't need it.
  template<typename...Rest> static auto
    dispatch(C &&c, void *, T &&t, Rest &&...rest) ->
    decltype(P::dispatch(std::forward<C>(c), std::get<I>(std::forward<T>(t))...,
			 std::forward<Rest>(rest)...))
  {
    return P::dispatch(std::forward<C>(c), std::get<I>(std::forward<T>(t))...,
		       std::forward<Rest>(rest)...);
  }
};
}

//! Call \c P::dispatch with a session pointer (unless the session
//! type \c S is void, in which case the argument is omitted) and with
//! a tuple that should be unpacked into multiple arguments.  For
//! example,
//! \code
//!   dispatch_with_session<P>(c, (void *) 0, make_tuple(1, 2), 3, 4);
//! \endcode
//! is equivalent to
//! \code
//!   P::dispatch(c, 1, 2, 3, 4);
//! \endcode
template<typename P, typename C, typename S, typename T,
	 typename...Rest> inline auto
dispatch_with_session(C &&c, S *s, T &&t, Rest &&...rest) ->
  decltype(detail::dispatch_session_helper<P, C, T>::dispatch(
               c, s, std::forward<T>(t), std::forward<Rest>(rest)...))
{
  return detail::dispatch_session_helper<P, C, T>::dispatch(
             std::forward<C>(c), s, std::forward<T>(t),
	     std::forward<Rest>(rest)...);
}


//! Trivial session allocator that just calls new and delete.
template<typename S> struct session_allocator {
  constexpr session_allocator() {}
  S *allocate(rpc_sock *s) { return new S{s}; }
  void deallocate(S *session) { delete session; }
};
template<> struct session_allocator<void> {
  constexpr session_allocator() {}
  void *allocate(rpc_sock *) { return nullptr; }
  void deallocate(void *) {}
};


struct service_base {
  using cb_t = std::function<void(msg_ptr)>;

  const uint32_t prog_;
  const uint32_t vers_;

  service_base(uint32_t prog, uint32_t vers) : prog_(prog), vers_(vers) {}
  virtual ~service_base() {}
  virtual void process(void *session, rpc_msg &hdr, xdr_get &g, cb_t reply) = 0;

  bool check_call(const rpc_msg &hdr) {
    return hdr.body.mtype() == CALL
      && hdr.body.cbody().rpcvers == 2
      && hdr.body.cbody().prog == prog_
      && hdr.body.cbody().vers == vers_;
  }

  template<typename T> static bool decode_arg(xdr_get &g, T &arg) {
    try {
      archive(g, arg);
      g.done();
      return true;
    }
    catch (const xdr_runtime_error &) {
      return false;
    }
  }
};

class rpc_server_base {
  std::map<uint32_t,
	   std::map<uint32_t, std::unique_ptr<service_base>>> servers_;
protected:
  void register_service_base(service_base *s);
public:
  void dispatch(void *session, msg_ptr m, service_base::cb_t reply);
};


//! Listens for connections on a TCP socket (optionally registering
//! the socket with \c rpcbind), and then serves one or more
//! program/version interfaces to accepted connections.
class rpc_tcp_listener_common : public rpc_server_base {
  void accept_cb();
  void receive_cb(rpc_sock *ms, void *session, msg_ptr mp);

protected:
  unique_sock listen_sock_;
  const bool use_rpcbind_;
  rpc_tcp_listener_common(pollset &ps, unique_sock &&s,
			  bool use_rpcbind = false);
  rpc_tcp_listener_common(pollset &ps)
    : rpc_tcp_listener_common(ps, unique_sock(invalid_sock), true) {}
  virtual ~rpc_tcp_listener_common();
  virtual void *session_alloc(rpc_sock *) = 0;
  virtual void session_free(void *session) = 0;

public:
  pollset &ps_;
};

template<template<typename, typename, typename> class ServiceType,
	 typename Session, typename SessionAllocator>
class generic_rpc_tcp_listener : public rpc_tcp_listener_common {
  SessionAllocator sa_;
protected:
  void *session_alloc(rpc_sock *s) override {
    return sa_.allocate(s);
  }
  void session_free(void *session) override { sa_.deallocate(session); }
public:
  //using rpc_tcp_listener_common::rpc_tcp_listener_common;
  generic_rpc_tcp_listener(pollset &ps)
    : rpc_tcp_listener_common(ps) {}
  generic_rpc_tcp_listener(pollset &ps, unique_sock &&s, bool use_rpcbind,
			   SessionAllocator sa)
    : rpc_tcp_listener_common(ps, std::move(s), use_rpcbind), sa_(sa) {}
  ~generic_rpc_tcp_listener() {}

  //! Add objects implementing RPC program interfaces to the server.
  template<typename T, typename Interface = typename T::rpc_interface_type>
  void register_service(T &t) {
    register_service_base(new ServiceType<T,Session,Interface>(t));
    if(use_rpcbind_)
      rpcbind_register(listen_sock_.get(), Interface::program,
		       Interface::version);
  }
};


} // namespace xdr

#endif // !_XDRPP_SERVER_H_HEADER_INCLUDED_
