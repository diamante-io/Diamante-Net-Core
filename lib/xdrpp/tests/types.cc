#include <iomanip>
#include <iostream>
#include <xdrpp/printer.h>
#include "tests/xdrtest.hh"

using namespace std;
using namespace xdr;

int
main()
{
  {
    uptr b(false);
    {
      uptr a(true);
      cout << "created" << endl;
      b = a;
      cout << "assigned" << endl;
    }
    cout << "destroyed a" << endl;
  }
  cout << "destroyed b" << endl;

  return 0;
}
