
#include <iostream>
#include <xdrpp/server.h>

namespace xdr {

bool xdr_trace_server = std::getenv("XDR_TRACE_SERVER");

msg_ptr
rpc_accepted_error_msg(uint32_t xid, accept_stat stat)
{
  assert(stat != SUCCESS && stat != PROG_MISMATCH);
  msg_ptr buf(message_t::alloc(24));
  xdr_put p(buf);
  p(xid);
  p(REPLY);
  p(MSG_ACCEPTED);
  p(AUTH_NONE);
  p(uint32_t(0));
  p(stat);
  assert(p.p_ == p.e_);
  return buf;
}

msg_ptr
rpc_prog_mismatch_msg(uint32_t xid, uint32_t low, uint32_t high)
{
  msg_ptr buf(message_t::alloc(32));
  xdr_put p(buf);
  p(xid);
  p(REPLY);
  p(MSG_ACCEPTED);
  p(AUTH_NONE);
  p(uint32_t(0));
  p(PROG_MISMATCH);
  p(low);
  p(high);
  assert(p.p_ == p.e_);
  return buf;
}

msg_ptr
rpc_auth_error_msg(uint32_t xid, auth_stat stat)
{
  msg_ptr buf(message_t::alloc(20));
  xdr_put p(buf);
  p(xid);
  p(REPLY);
  p(MSG_DENIED);
  p(AUTH_ERROR);
  p(stat);
  assert(p.p_ == p.e_);
  return buf;
}

msg_ptr
rpc_rpc_mismatch_msg(uint32_t xid)
{
  msg_ptr buf(message_t::alloc(24));
  xdr_put p(buf);
  p(xid);
  p(REPLY);
  p(MSG_DENIED);
  p(RPC_MISMATCH);
  p(uint32_t(2));
  p(uint32_t(2));
  assert(p.p_ == p.e_);
  return buf;
}


void
rpc_server_base::register_service_base(service_base *s)
{
  servers_[s->prog_][s->vers_].reset(s);
}

void
rpc_server_base::dispatch(void *session, msg_ptr m, service_base::cb_t reply)
{
  xdr_get g(m);
  rpc_msg hdr;

  try { archive(g, hdr); }
  catch (const xdr_runtime_error &e) {
    std::cerr << "rpc_server_base::dispatch: ignoring malformed header: "
	      << e.what() << std::endl;
    return;
  }
  if (hdr.body.mtype() != CALL) {
    std::cerr << "rpc_server_base::dispatch: ignoring non-CALL" << std::endl;
    return;
  }

  if (hdr.body.cbody().rpcvers != 2)
    return reply(rpc_rpc_mismatch_msg(hdr.xid));

  auto prog = servers_.find(hdr.body.cbody().prog);
  if (prog == servers_.end())
    return reply(rpc_accepted_error_msg(hdr.xid, PROG_UNAVAIL));

  auto vers = prog->second.find(hdr.body.cbody().vers);
  if (vers == prog->second.end()) {
    uint32_t low = prog->second.cbegin()->first;
    uint32_t high = prog->second.crbegin()->first;
    return reply(rpc_prog_mismatch_msg(hdr.xid, low, high));
  }

  try {
    vers->second->process(session, hdr, g, reply);
    return;
  }
  catch (const xdr_runtime_error &e) {
    std::cerr << "rpc_server_base::dispatch: " << e.what() << std::endl;
  }
  reply(rpc_accepted_error_msg(hdr.xid, GARBAGE_ARGS));
}


rpc_tcp_listener_common::rpc_tcp_listener_common(pollset &ps, unique_sock &&s,
						 bool reg)
  : listen_sock_(s ? std::move(s) : tcp_listen()), use_rpcbind_(reg),
    ps_(ps)
{
  set_close_on_exec(listen_sock_.get());
  ps_.fd_cb(listen_sock_.get(), pollset::Read,
	    std::bind(&rpc_tcp_listener_common::accept_cb, this));
}

rpc_tcp_listener_common::~rpc_tcp_listener_common()
{
  ps_.fd_cb(listen_sock_.get(), pollset::Read);
  // XXX should clean up if use_rpcbind_.
}

void
rpc_tcp_listener_common::accept_cb()
{
  sock_t s = accept(listen_sock_.get(), nullptr, 0);
  if (s == invalid_sock) {
    std::cerr << "rpc_tcp_listener_common: accept: " << sock_errmsg()
	      << std::endl;
    return;
  }
  set_close_on_exec(s);
  rpc_sock *ms = new rpc_sock(ps_, s);
  ms->set_servcb(std::bind(&rpc_tcp_listener_common::receive_cb, this, ms,
			   session_alloc(ms), std::placeholders::_1));
}

void
rpc_tcp_listener_common::receive_cb(rpc_sock *ms, void *session, msg_ptr mp)
{
  if (!mp) {
    session_free(session);
    delete ms;
    return;
  }
  try {
    dispatch(session, std::move(mp), rpc_sock_reply_t(ms));
  }
  catch (const xdr_runtime_error &e) {
    std::cerr << e.what() << std::endl;
    session_free(session);
    delete ms;
  }
}

}
