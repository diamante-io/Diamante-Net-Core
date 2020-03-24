
#include <cassert>
#include <xdrpp/marshal.h>
#include "tests/xdrtest.hh"

using namespace xdr;

int
main()
{
  test_recursive tr;
  tr.next.activate().next.activate();

  xdr::marshaling_stack_limit = 8;

  auto bytes = xdr_to_opaque(tr);
  for (int i = 16; i; --i)
    xdr_from_opaque(bytes, tr);

  bool bad = true;
  xdr::marshaling_stack_limit = 5;
  try {
    xdr_from_opaque(bytes, tr);
  }
  catch (const xdr::xdr_stack_overflow &) {
    bad = false;
  }
  assert(!bad);

  return 0;
}
