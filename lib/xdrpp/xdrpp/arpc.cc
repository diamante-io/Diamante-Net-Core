
#include <xdrpp/arpc.h>

namespace xdr {

void
arpc_server::receive(rpc_sock *ms, msg_ptr buf)
{
  dispatch(nullptr, std::move(buf), rpc_sock_reply_t{ms});
}

}
