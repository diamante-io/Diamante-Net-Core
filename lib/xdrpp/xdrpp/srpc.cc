
#include <cerrno>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <xdrpp/exception.h>
#include <xdrpp/srpc.h>

namespace xdr {

bool xdr_trace_client = std::getenv("XDR_TRACE_CLIENT");

msg_ptr
read_message(sock_t s)
{
  std::uint32_t len;
  ssize_t n = read(s, &len, 4);
  if (n == -1)
    throw xdr_system_error("xdr::read_message");
  if (n < 4)
    throw xdr_bad_message_size("read_message: premature EOF");
  if (n & 3)
    throw xdr_bad_message_size("read_message: received size not multiple of 4");
  if (n >= 0x80000000)
    throw xdr_bad_message_size("read_message: received size too big");

  len = swap32le(len);
  if (len & 0x80000000)
    len &= 0x7fffffff;
  else
    throw xdr_bad_message_size("read_message: message fragments unimplemented");

  msg_ptr m = message_t::alloc(len);
  n = read(s, m->data(), len);
  if (n == -1)
    throw xdr_system_error("xdr::read_message");
  if (n != len)
    throw xdr_bad_message_size("read_message: premature EOF");

  return m;
}

void
write_message(sock_t s, const msg_ptr &m)
{
  ssize_t n = write(s, m->raw_data(), m->raw_size());
  if (n == -1)
    throw xdr_system_error("xdr::write_message");
  // If this assertion fails, the file descriptor may have had
  // O_NONBLOCK set, which is not allowed for the synchronous
  // interface.
  assert(std::size_t(n) == m->raw_size());
}

uint32_t xid_counter;

void
prepare_call(uint32_t prog, uint32_t vers, uint32_t proc, rpc_msg &hdr)
{
  hdr.xid = ++xid_counter;
  hdr.body.mtype(CALL);
  hdr.body.cbody().rpcvers = 2;
  hdr.body.cbody().prog = prog;
  hdr.body.cbody().vers = vers;
  hdr.body.cbody().proc = proc;
}

void
srpc_server::run()
{
  for (;;)
    dispatch(nullptr, read_message(s_),
	     std::bind(write_message, s_, std::placeholders::_1));
}

}
