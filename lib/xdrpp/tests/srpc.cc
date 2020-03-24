
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <thread>
#include <xdrpp/srpc.h>
#include <xdrpp/rpcb_prot.hh>
#include <xdrpp/socket.h>
#include <xdrpp/clear.h>
#include "tests/xdrtest.hh"

using namespace std;
using namespace xdr;

using namespace testns;

class xdrtest2_server {
public:
  using rpc_interface_type = testns::xdrtest2;

  void null2();
  std::unique_ptr<ContainsEnum> nonnull2(std::unique_ptr<u_4_12> arg);
  void ut(std::unique_ptr<uniontest> arg);
  std::unique_ptr<bigstr>
  three(const bool &arg1, const int &arg2, const bigstr &arg3);
};

std::unique_ptr<bigstr>
xdrtest2_server::three(const bool &arg1, const int &arg2, const bigstr &arg3)
{
  std::unique_ptr<bigstr> res(new bigstr);
  
  // Fill in function body here
  
  return res;
}

void
xdrtest2_server::null2()
{
  cerr << "I got a null request" << endl;
}

std::unique_ptr<testns::ContainsEnum>
xdrtest2_server::nonnull2(std::unique_ptr<u_4_12> arg)
{
  using namespace testns;
  std::unique_ptr<ContainsEnum> res(new ContainsEnum);
  
  cerr << "I got a nonnull request" << endl
       << xdr_to_string(*arg, "arg");
  res->c(::REDDER).num() = ContainsEnum::TWO;

  xdr_clear(*arg);
  
  return res;
}

void
xdrtest2_server::ut(std::unique_ptr<uniontest> arg)
{
  
  // Fill in function body here
  
}

void
getmsg(sock_t fd)
{
#if 0
  msg_ptr p;
  try { p = read_message(fd); }
  catch (const std::exception &e) {
    cerr << "caught " << e.what() << endl;
    close(fd);
    return;
  }

  cerr << "got " << p->size() << "-byte message" << endl;

  xdr_get g(p);
  rpc_msg hdr;
  archive(g, hdr);
  
  cerr << xdr_to_string(hdr);

  close(fd);
#endif

  // XXX got rid of srpc_server
  xdrtest2_server s;
  srpc_server sfd(fd);
  sfd.register_service(s);
  sfd.run();
  
  close(fd);
}

void
sendreq(sock_t fd)
{
  srpc_client<testns::xdrtest2> sc (fd);

  u_4_12 arg(12);
  arg.f12().i = 77;
  arg.f12().d = 3.141592654;
  auto cep = sc.nonnull2(arg);

  cout << xdr_to_string(*cep, "The response");
}


void
test_rpcb()
{
  auto fd = tcp_connect(nullptr, "sunrpc");
  srpc_client<xdr::RPCBVERS4> rpcb(fd.get());

  xdr::rpcb arg;
  arg.r_prog = xdrtest2::program;
  arg.r_vers = xdrtest2::version;
  arg.r_netid = "tcp";
  arg.r_addr = "0.0.0.0.32.32";
  arg.r_owner = "dm";

  auto res = rpcb.RPCBPROC_SET(arg);
  std::cout << "RPCBPROC3_SET: " << *res << endl;

  res = rpcb.RPCBPROC_UNSET(arg);
  std::cout << "RPCBPROC_UNSET: " << *res << endl;
}


int
main(int argc, char **argv)
{
#if 0
  auto ai = get_rpcaddr(nullptr, 100000, 3);
  string host, port;
  get_numinfo(ai->ai_addr, ai->ai_addrlen, &host, &port);
  cout << host << ":" << port << endl;
#endif

  test_rpcb();

  int fds[2];
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == -1) {
    perror("socketpair");
    exit(1);
  }

  thread t1(sendreq, sock_t(fds[0]));

  getmsg(sock_t(fds[1]));
  
  t1.join();

  return 0;
}
