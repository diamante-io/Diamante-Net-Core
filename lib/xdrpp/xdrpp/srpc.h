// -*- C++ -*-

#ifndef _XDRPP_SRPC_H_HEADER_INCLUDED_
#define _XDRPP_SRPC_H_HEADER_INCLUDED_ 1

//! \file srpc.h Simple synchronous RPC functions.

#include <xdrpp/exception.h>
#include <xdrpp/server.h>

namespace xdr {

extern bool xdr_trace_client;

msg_ptr read_message(sock_t s);
void write_message(sock_t s, const msg_ptr &m);

void prepare_call(uint32_t prog, uint32_t vers, uint32_t proc, rpc_msg &hdr);
template<typename P> inline void
prepare_call(rpc_msg &hdr)
{
  prepare_call(P::interface_type::program, P::interface_type::version,
	       P::proc, hdr);
}


//! Synchronous file descriptor demultiplexer.
class synchronous_client_base {
  const sock_t s_;

  static void moveret(pointer<xdr_void> &) {}
  template<typename T> static T &&moveret(T &t) { return std::move(t); }

  //static xdr_void arg_tuple() { return xdr_void{}; }
  template<typename T> const T &arg_tuple(const T &t) { return t; }
  template<typename...T> std::tuple<const T &...> arg_tuple(const T &...t) {
    return std::make_tuple(std::cref(t)...);
  }

public:
  synchronous_client_base(sock_t s) : s_(s) {}
  synchronous_client_base(const synchronous_client_base &c) : s_(c.s_) {}

  template<typename P, typename...A> typename std::conditional<
    std::is_void<typename P::res_type>::value, void,
    std::unique_ptr<typename P::res_type>>::type
  invoke(const A &...a) {
    rpc_msg hdr;
    prepare_call<P>(hdr);
    uint32_t xid = hdr.xid;

    if (xdr_trace_client) {
      std::string s = "CALL ";
      s += P::proc_name();
      s += " -> [xid " + std::to_string(xid) + "]";
      std::clog << xdr_to_string(std::tie(a...), s.c_str());
    }
    write_message(s_, xdr_to_msg(hdr, a...));
    msg_ptr m = read_message(s_);

    xdr_get g(m);
    archive(g, hdr);
    check_call_hdr(hdr);
    if (hdr.xid != xid)
      throw xdr_runtime_error("synchronous_client: unexpected xid");

    pointer<typename P::res_wire_type> r;
    archive(g, r.activate());
    g.done();
    if (xdr_trace_client) {
      std::string s = "REPLY ";
      s += P::proc_name();
      s += " <- [xid " + std::to_string(xid) + "]";
      std::clog << xdr_to_string(*r, s.c_str());
    }
    return moveret(r);
  }

  // because _xdr_client expects a pointer type
  synchronous_client_base *operator->() { return this; }
};

//! Create an RPC client from an interface type and connected stream
//! socket.  Note that the file descriptor is not closed afterwards
//! (as you may wish to use different interfaces over the same file
//! descriptor).  A simple example looks like this:
//!
//! \code
//!    unique_sock fd = tcp_connect_rpc(argc > 2 ? argv[2] : nullptr,
//!                                     MyProg1::program, MyProg1::version);
//!    srpc_client<MyProg1> c{fd.get()};
//!    unique_ptr<big_string> result = c.hello(5);
//! \endcode
template<typename T> using srpc_client =
  typename T::template _xdr_client<synchronous_client_base>;


template<typename T, typename Session, typename Interface>
class srpc_service : public service_base {
  template<typename P, typename A> typename
  std::enable_if<std::is_same<void, typename P::res_type>::value,
		 const xdr_void *>::type
  dispatch1(Session *s, A &a) {
    static xdr_void v;
    dispatch_with_session<P>(server_, s, std::move(a));
    return &v;
  }
  template<typename P, typename A> typename
  std::enable_if<!std::is_same<void, typename P::res_type>::value,
		 std::unique_ptr<typename P::res_type>>::type
  dispatch1(Session *s, A &a) {
    return dispatch_with_session<P>(server_, s, std::move(a));
  }

public:
  T &server_;

  srpc_service(T &server)
    : service_base(Interface::program, Interface::version), server_(server) {}

  void process(void *session, rpc_msg &hdr, xdr_get &g, cb_t reply) override {
    if (!check_call(hdr))
      reply(nullptr);
    if (!Interface::call_dispatch(*this, hdr.body.cbody().proc,
				  static_cast<Session *>(session),
				  hdr, g, std::move(reply)))
      reply(rpc_accepted_error_msg(hdr.xid, PROC_UNAVAIL));
  }

  template<typename P>
  void dispatch(Session *session, rpc_msg &hdr, xdr_get &g, cb_t reply) {
    wrap_transparent_ptr<typename P::arg_tuple_type> arg;
    if (!decode_arg(g, arg))
      return reply(rpc_accepted_error_msg(hdr.xid, GARBAGE_ARGS));
    
    if (xdr_trace_server) {
      std::string s = "CALL ";
      s += P::proc_name();
      s += " <- [xid " + std::to_string(hdr.xid) + "]";
      std::clog << xdr_to_string(arg, s.c_str());
    }

    auto res = this->template dispatch1<P>(session, arg);

    if (xdr_trace_server) {
      std::string s = "REPLY ";
      s += P::proc_name();
      s += " -> [xid " + std::to_string(hdr.xid) + "]";
      std::clog << xdr_to_string(*res, s.c_str());
    }

    reply(xdr_to_msg(rpc_success_hdr(hdr.xid), *res));
  }
};

//! Attach RPC services to a single, connected stream socket.  No
//! procedures will be implemented by the RPC server until interface
//! objects are reigstered with \c register_server.
class srpc_server : public rpc_server_base {
  const sock_t s_;
  bool close_on_destruction_;

public:
  srpc_server(sock_t s, bool close_on_destruction = true)
    : s_(s), close_on_destruction_(close_on_destruction) {}
  ~srpc_server() { if (close_on_destruction_) close(s_); }

  //! Add objects implementing RPC program interfaces to the server.
  template<typename T, typename Interface = typename T::rpc_interface_type>
  void register_service(T &t) {
    register_service_base(new srpc_service<T, void, Interface>(t));
  }

  //! Start serving requests.  (Loops until an exception.)
  void run();
};

template<typename Session = void,
	 typename SessionAllocator = session_allocator<Session>>
using srpc_tcp_listener =
  generic_rpc_tcp_listener<srpc_service, Session, SessionAllocator>;

}

#endif // !_XDRPP_SRPC_H_HEADER_INCLUDED_
