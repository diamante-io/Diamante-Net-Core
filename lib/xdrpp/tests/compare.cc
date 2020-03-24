
#include <cassert>
#include "tests/xdrtest.hh"


int
main()
{

  fix_12 f1, f2;
  f1.i = 5;
  f1.d = 0.0;
  f2 = f1;

  assert(f2 == f1);
  assert(!(f1 < f2));
  assert(!(f2 < f1));
  f2.d = -0.1;
  assert(!(f1 == f2));
  assert(!(f1 < f2));
  assert(f2 < f1);
  f2.i++;
  assert(f1 < f2);
  assert(!(f2 < f1));

  testns::ContainsEnum ce1(REDDER);
  ce1.num() = testns::ContainsEnum::ONE;
  testns::ContainsEnum ce2 = ce1;

  assert(ce1 == ce2);
  ce2.num() = testns::ContainsEnum::TWO;
  assert(!(ce1 == ce2));
  assert(ce1 < ce2);
  assert(!(ce2 < ce1));
  ce1.c(RED);
  ce1.foo() = "hello world";
  assert(!(ce1 == ce2));
  assert(ce1 < ce2);
  assert(!(ce2 < ce1));

  testns::bytes b1, b2;
  assert(b1 == b2);
  //assert(testns::operator==(b1, b2));

  testns::hasbytes hb1, hb2;
  assert(hb1 == hb2);
  
  return 0;
}
