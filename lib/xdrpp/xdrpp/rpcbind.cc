
#include <vector>
#include <xdrpp/rpcb_prot.hh>
#include <xdrpp/rpcbind.h>
#include <xdrpp/srpc.h>

namespace xdr {

using std::string;

namespace {

std::vector<rpcb> registered_services;

void
run_cleanup()
{
  try {
    auto fd = tcp_connect(nullptr, "sunrpc");
    srpc_client<xdr::RPCBVERS4> c{fd.get()};
    for (const auto &arg : registered_services)
      c.RPCBPROC_UNSET(arg);
  }
  catch (...) {}
}

void
set_cleanup()
{
  static struct once {
    once() { atexit(run_cleanup); }
  } o;
}

} // namespace

std::unique_ptr<sockaddr>
lookup_rpc(const char *host, std::uint32_t prog, std::uint32_t vers,
	   socklen_t *, int family, int sotype)
{
  unique_addrinfo ail = get_addrinfo(host, SOCK_STREAM, "sunrpc", family);

  for (const addrinfo *ai = ail.get(); ai; ai = ai->ai_next) {
    try {
      auto s = tcp_connect1(ai);
      srpc_client<xdr::RPCBVERS4> c{s.get()};

      rpcb arg;
      arg.r_prog = prog;
      arg.r_vers = vers;
      arg.r_netid = sotype == SOCK_DGRAM ? "udp" : "tcp";
      if (ai->ai_family == AF_INET6)
	arg.r_netid += '6';
      arg.r_addr = make_uaddr(s.get());
      auto res = c.RPCBPROC_GETADDR(arg);

      int port = parse_uaddr_port(*res);
      if (port == -1)
	continue;

      switch (ai->ai_family) {
      case AF_INET:
	((sockaddr_in *) ai->ai_addr)->sin_port = htons(port);
	break;
      case AF_INET6:
	((sockaddr_in6 *) ai->ai_addr)->sin6_port = htons(port);
	break;
      }

      std::unique_ptr<sockaddr> ret
	{ static_cast<sockaddr *>(::operator new(ai->ai_addrlen)) };
      memcpy(ret.get(), ai->ai_addr, ai->ai_addrlen);
      return ret;
    }
    catch(const std::system_error &) {}
  }
  throw std::system_error(std::make_error_code(std::errc::connection_refused),
			  "Could not obtain port from rpcbind");
}

unique_sock
tcp_connect_rpc(const char *host, std::uint32_t prog, std::uint32_t vers,
		int family)
{
  unique_addrinfo ail = get_addrinfo(host, SOCK_STREAM, "sunrpc", family);

  for (const addrinfo *ai = ail.get(); ai; ai = ai->ai_next) {
    try {
      auto s = tcp_connect1(ai);
      srpc_client<xdr::RPCBVERS4> c{s.get()};

      rpcb arg;
      arg.r_prog = prog;
      arg.r_vers = vers;
      arg.r_netid = ai->ai_family == AF_INET6 ? "tcp6" : "tcp";
      arg.r_addr = make_uaddr(s.get());
      auto res = c.RPCBPROC_GETADDR(arg);

      int port = parse_uaddr_port(*res);
      if (port == -1)
	continue;

      switch (ai->ai_family) {
      case AF_INET:
	((sockaddr_in *) ai->ai_addr)->sin_port = htons(port);
	break;
      case AF_INET6:
	((sockaddr_in6 *) ai->ai_addr)->sin6_port = htons(port);
	break;
      }
      s.clear();
      if ((s = tcp_connect1(ai)))
	return s;
    }
    catch(const std::system_error &) {}
  }
  throw std::system_error(std::make_error_code(std::errc::connection_refused),
			  "Could not obtain port from rpcbind");
}

int
parse_uaddr_port(const string &uaddr)
{
  std::size_t low = uaddr.rfind('.');
  if (low == string::npos || low == 0)
    return -1;
  std::size_t high = uaddr.rfind('.', low - 1);
  if (high == string::npos)
    return -1;

  try {
    int hb = std::stoi(uaddr.substr(high+1));
    int lb = std::stoi(uaddr.substr(low+1));
    if (hb < 0 || hb > 255 || lb < 0 || lb > 255)
      return -1;
    return hb << 8 | lb;
  }
  catch (std::invalid_argument &) {
    return -1;
  }
  catch (std::out_of_range &) {
    return -1;
  }
}

string
make_uaddr(const sockaddr *sa, socklen_t salen)
{
  string host, portstr;
  get_numinfo(sa, salen, &host, &portstr);
  unsigned port = std::stoul(portstr);
  if (port == 0 || port >= 65536)
    throw std::system_error(std::make_error_code(std::errc::invalid_argument),
			    "bad port number");
  host += "." + std::to_string(port >> 8) + "." + std::to_string(port & 0xff);
  return host;
}

string
make_uaddr(sock_t s)
{
  union {
    struct sockaddr sa;
    struct sockaddr_storage ss;
  };
  socklen_t salen{sizeof ss};
  std::memset(&ss, 0, salen);
  if (getsockname(s.fd_, &sa, &salen) == -1)
    throw_sockerr("getsockname");
  return make_uaddr(&sa, salen);
}

void
rpcbind_register(const sockaddr *sa, socklen_t salen, int so_type,
		 std::uint32_t prog, std::uint32_t vers)
{
  set_cleanup();

  auto fd = tcp_connect(nullptr, "sunrpc", sa->sa_family);
  srpc_client<xdr::RPCBVERS4> c{fd.get()};

  rpcb arg;
  arg.r_prog = prog;
  arg.r_vers = vers;
  arg.r_netid = so_type == SOCK_DGRAM ? "udp" : "tcp";
  if (sa->sa_family == AF_INET6)
    arg.r_netid += '6';
  arg.r_addr = make_uaddr(sa, salen);
  arg.r_owner = std::to_string(geteuid());
  c.RPCBPROC_UNSET(arg);
  auto res = c.RPCBPROC_SET(arg);
  if (!*res)
    throw std::system_error(std::make_error_code(std::errc::address_in_use),
			    "RPCBPROC_SET");
  registered_services.push_back(arg);
}

void
rpcbind_register(sock_t s, std::uint32_t prog, std::uint32_t vers)
{
  union {
    struct sockaddr sa;
    struct sockaddr_storage ss;
  };
  socklen_t salen{sizeof ss};
  std::memset(&ss, 0, salen);
  if (getsockname(s.fd_, &sa, &salen) == -1)
    throw_sockerr("getsockname");
  rpcbind_register(&sa, salen, socket_type(s.fd()), prog, vers);
}

} // namespace xdr
