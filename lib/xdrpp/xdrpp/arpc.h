// -*- C++ -*-

//! \file arpc.h Asynchronous RPC interface.

#ifndef _XDRPP_ARPC_H_HEADER_INCLUDED_
#define _XDRPP_ARPC_H_HEADER_INCLUDED_ 1

#include <xdrpp/exception.h>
#include <xdrpp/server.h>
#include <xdrpp/srpc.h>	     // XXX xdr_trace_client

namespace xdr {

//! A \c unique_ptr to a call result, or NULL if the call failed (in
//! which case \c message returns an error message).
template<typename T> struct call_result : std::unique_ptr<T> {
  rpc_call_stat stat_;
  call_result(const rpc_msg &hdr) : stat_(hdr) {
    if (stat_)
      this->reset(new T{});
  }
  call_result(rpc_call_stat::stat_type type) : stat_(type) {}
  const char *message() const { return *this ? nullptr : stat_.message(); }
};
template<> struct call_result<void> {
  rpc_call_stat stat_;
  call_result(const rpc_msg &hdr) : stat_(hdr) {}
  call_result(rpc_call_stat::stat_type type) : stat_(type) {}
  const char *message() const { return stat_ ? nullptr : stat_.message(); }
  explicit operator bool() const { return bool(stat_); }
  xdr_void &operator*() { static xdr_void v; return v; }
};

class asynchronous_client_base {
  rpc_sock &s_;

public:
  asynchronous_client_base(rpc_sock &s) : s_(s) {}
  asynchronous_client_base(asynchronous_client_base &c) : s_(c.s_) {}

  template<typename P, typename...A>
  void invoke(const A &...a,
	      std::function<void(call_result<typename P::res_type>)> cb) {
    rpc_msg hdr { s_.get_xid(), CALL };
    hdr.body.cbody().rpcvers = 2;
    hdr.body.cbody().prog = P::interface_type::program;
    hdr.body.cbody().vers = P::interface_type::version;
    hdr.body.cbody().proc = P::proc;

    if (xdr_trace_client) {
      std::string s = "CALL ";
      s += P::proc_name();
      s += " -> [xid ";
      s += std::to_string(hdr.xid);
      s += "]";
      std::clog << xdr_to_string(std::tie(a...), s.c_str());
    }

    s_.send_call(xdr_to_msg(hdr, a...), [cb](msg_ptr m) {
	if (!m)
	  return cb(rpc_call_stat::NETWORK_ERROR);
	try {
	  xdr_get g(m);
	  rpc_msg hdr;
	  archive(g, hdr);
	  call_result<typename P::res_type> res(hdr);
	  if (res)
	    archive(g, *res);
	  g.done();

	  if (xdr_trace_client) {
	    std::string s = "REPLY ";
	    s += P::proc_name();
	    s += " <- [xid " + std::to_string(hdr.xid) + "]";
	    if (res)
	      std::clog << xdr_to_string(*res, s.c_str());
	    else {
	      s += ": ";
	      s += res.message();
	      s += "\n";
	      std::clog << s;
	    }
	  }

	  cb(std::move(res));
	}
	catch (const xdr_runtime_error &e) {
	  cb(rpc_call_stat::GARBAGE_RES);
	}
      });
  }

  asynchronous_client_base *operator->() { return this; }
};

template<typename T> using arpc_client =
  typename T::template _xdr_client<asynchronous_client_base>;


// And now for the server

template<typename T> class reply_cb;

namespace detail {
class reply_cb_impl {
  template<typename T> friend class xdr::reply_cb;
  using cb_t = service_base::cb_t;
  uint32_t xid_;
  cb_t cb_;
  const char *const proc_name_;

public:
  template<typename CB> reply_cb_impl(uint32_t xid, CB &&cb, const char *name)
    : xid_(xid), cb_(std::forward<CB>(cb)), proc_name_(name) {}
  reply_cb_impl(const reply_cb_impl &rcb) = delete;
  reply_cb_impl &operator=(const reply_cb_impl &rcb) = delete;
  ~reply_cb_impl() { if (cb_) reject(PROC_UNAVAIL); }

private:
  void send_reply_msg(msg_ptr &&b) {
    assert(cb_);		// If this fails you replied twice
    cb_(std::move(b));
    cb_ = nullptr;
  }

  template<typename T> void send_reply(const T &t) {
    if (xdr_trace_server) {
      std::string s = "REPLY ";
      s += proc_name_;
      s += " -> [xid " + std::to_string(xid_) + "]";
      std::clog << xdr_to_string(t, s.c_str());
    }
    send_reply_msg(xdr_to_msg(rpc_success_hdr(xid_), t));
  }

  void reject(accept_stat stat) {
    send_reply_msg(rpc_accepted_error_msg(xid_, stat));
  }
  void reject(auth_stat stat) {
    send_reply_msg(rpc_auth_error_msg(xid_, stat));
  }
};
} // namespace detail

// Prior to C++14, it's a pain to move objects into another thread.
// Hence we used shared_ptr to make reply_cb copyable as well as
// moveable.
template<typename T> class reply_cb {
  using impl_t = detail::reply_cb_impl;
public:
  using type = T;
  std::shared_ptr<impl_t> impl_;

  reply_cb() {}
  template<typename CB> reply_cb(uint32_t xid, CB &&cb, const char *name)
    : impl_(std::make_shared<impl_t>(xid, std::forward<CB>(cb), name)) {}

  void operator()(const type &t) const { impl_->send_reply(t); }
  void reject(accept_stat stat) const { impl_->reject(stat); }
  void reject(auth_stat stat) const { impl_->reject(stat); }
};
template<> class reply_cb<void> : public reply_cb<xdr_void> {
public:
  using type = void;
  using reply_cb<xdr_void>::reply_cb;
  using reply_cb<xdr_void>::operator();
  void operator()() const { this->operator()(xdr_void{}); }
};

template<typename T, typename Session, typename Interface>
class arpc_service : public service_base {
  T &server_;

public:
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

    dispatch_with_session<P>(server_, session, std::move(arg),
			     reply_cb<typename P::res_type>{
			       hdr.xid, std::move(reply), P::proc_name()});
  }

  arpc_service(T &server)
    : service_base(Interface::program, Interface::version),
      server_(server) {}
};

class arpc_server : public rpc_server_base {
public:
  template<typename T, typename Interface = typename T::rpc_interface_type>
  void register_service(T &t) {
    register_service_base(new arpc_service<T, void, Interface>(t));
  }
  void receive(rpc_sock *ms, msg_ptr buf);
};

template<typename Session = void,
	 typename SessionAllocator = session_allocator<Session>>
using arpc_tcp_listener =
  generic_rpc_tcp_listener<arpc_service, Session, SessionAllocator>;

} // namespace xdr

#endif // !_XDRPP_ARPC_H_HEADER_INCLUDED_
