
#include <iomanip>
#include <iostream>
#include <xdrpp/printer.h>
#include "tests/xdrtest.hh"

using namespace std;
using namespace xdr;

static string
xdr_printer(const fix_4 &f4)
{
  ostringstream os;
  os << "FIX4(" << f4.i << ")";
  return os.str();
}

void
withshow()
{
  using xdr::operator<<;

  bool b = true;
  
  u_4_12 u(12);
  u.f12().i = 99;
  u.f12().d = 99.99;

  cout << boolalpha << b << " " << u << endl;

  u.which(4);
  cout << u << endl;

  string tmp = xdr_to_string(fix_4{0});
  assert(tmp == "FIX4(0)\n");

  testns::numerics n;

  cout << noboolalpha << n << endl;

  // Should print "REDDEST"
  cout << REDDEST << endl;
}

int
main()
{
  withshow();

  testns::uniontest ut;
  ut.ip.activate()++;
  ut.key.arbitrary(REDDEST);
  ut.key.big().resize(4);
  cout << xdr_to_string(ut) << endl;

  // Should print "2"
  cout << REDDEST << endl;

  return 0;
}
