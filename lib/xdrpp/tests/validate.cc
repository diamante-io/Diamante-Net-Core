
#include <iomanip>
#include <iostream>
#include <xdrpp/marshal.h>
#include <xdrpp/printer.h>
#include "tests/xdrtest.hh"

using namespace std;
using namespace xdr;

namespace xdr {
template<> void
validate<fix_4>(const fix_4 &f4)
{
  if (f4.i == 0)
    throw xdr::xdr_invariant_failed("fix_4::i has value 0");
}
}

int
main()
{
  fix_4 f4;
  opaque_vec<> v;

  f4.i = 1;
  v = xdr_to_opaque(f4);
  xdr_from_opaque(v, f4);

  opaque_vec<> ss(v);
  fix_4 ff4;
  xdr_from_opaque(ss, ff4);
  assert (f4 == ff4);

  f4.i = 0;
  v = xdr_to_opaque(f4);
  bool ok = false;
  try {
    xdr_from_opaque(v, f4);
  } catch (const xdr::xdr_invariant_failed &) {
    ok = true;
  }
  assert(ok);

  xstring<2> s;
  static_cast<string &>(s) = "1234";
  ok = false;
  try {
    validate(s);
  } catch (const xdr::xdr_overflow &) {
    ok = true;
  }
  assert (ok);

  return 0;
}
