
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <thread>
#include <xdrpp/srpc.h>
#include <xdrpp/rpcb_prot.hh>
#include <xdrpp/rpcbind.h>
#include "tests/xdrtest.hh"

using namespace std;
using namespace xdr;

using namespace testns;

namespace {
pollset ps;
}


class xdrtest2_server {
public:
  using rpc_interface_type = xdrtest2;

  void null2();
  std::unique_ptr<ContainsEnum> nonnull2(std::unique_ptr<u_4_12> arg);
  void ut(std::unique_ptr<uniontest> arg);
  std::unique_ptr<bigstr> three(bool &, int &, const bigstr &);
};

std::unique_ptr<bigstr>
xdrtest2_server::three(bool &b, int &i, const bigstr &s)
{
  std::unique_ptr <bigstr> res {new bigstr{"three reply string"}};
  return res;
}

void
xdrtest2_server::null2()
{
  
  cout << "null2" << endl;
  
}

std::unique_ptr<ContainsEnum>
xdrtest2_server::nonnull2(std::unique_ptr<u_4_12> arg)
{
  std::unique_ptr<ContainsEnum> res(new ContainsEnum);
  
  cout << xdr_to_string(*arg, "nonnull2 arg");
  res->c(::RED).foo() = "Hello, world\n";
  
  return res;
}

void
xdrtest2_server::ut(std::unique_ptr<uniontest> arg)
{
  
  cout << xdr_to_string(*arg, "ut arg");
  
}

int
main(int argc, char **argv)
{
  if (argc > 1 && !strcmp(argv[1], "-s")) {
    xdrtest2_server s;
    srpc_tcp_listener<> rl(ps);
    rl.register_service(s);
    ps.run();
  }
  else if (argc > 1 && !strcmp(argv[1], "-c")) {
    auto fd = tcp_connect_rpc(argc > 2 ? argv[2] : nullptr,
			      xdrtest2::program, xdrtest2::version);
    srpc_client<xdrtest2> c{fd.get()};

    c.null2();

    u_4_12 u(12);
    u.f12().i = 1977;
    auto r = c.nonnull2(u);
    cout << xdr_to_string(*r, "nonnull2 reply");

    auto s = c.three(true, 8, "this is the third argument");
    cout << *s << endl;
  }
  else
    cerr << "need -s or -c option" << endl;
  return 0;
}
