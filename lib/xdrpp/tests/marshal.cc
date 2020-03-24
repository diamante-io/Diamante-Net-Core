
#include <cassert>
#include <iostream>
#include <xdrpp/clear.h>
#include <xdrpp/marshal.h>
#include <xdrpp/printer.h>
#include "tests/xdrtest.hh"

using namespace std;

template<typename T>
typename std::enable_if<!xdr::xdr_traits<T>::has_fixed_size, std::size_t>::type
xdr_getsize(const T &t)
{
  return xdr::xdr_size(t);
}

template<typename T>
typename std::enable_if<xdr::xdr_traits<T>::has_fixed_size, std::size_t>::type
xdr_getsize(const T &t)
{
  assert(xdr::xdr_traits<T>::fixed_size == xdr::xdr_size(t));
  return xdr::xdr_traits<T>::fixed_size;
}

#define CHECK_SIZE(v, s)						\
do {									\
  size_t __s = xdr_getsize(v);						\
  if (__s != s) {							\
    cerr << #v << " has size " << __s << " should have " << s << endl;	\
    terminate();							\
  }									\
} while (0)


void
test_size()
{
  CHECK_SIZE(int32_t(), 4);
  CHECK_SIZE(fix_4(), 4);
  CHECK_SIZE(fix_12(), 12);
  CHECK_SIZE(xdr::opaque_array<5>(), 8);
  CHECK_SIZE(u_4_12(4), 8);
  CHECK_SIZE(u_4_12(12), 16);

  {
    bool ok = false;
    try { CHECK_SIZE(u_4_12(0), 9999); }
    catch (const xdr::xdr_bad_discriminant &) { ok = true; }
    assert(ok);
  }

  v12 v;
  CHECK_SIZE(v, 4);
  v.emplace_back();
  CHECK_SIZE(v, 16);
  v.emplace_back();
  CHECK_SIZE(v, 28);

  CHECK_SIZE(xdr::xstring<>(), 4);
  CHECK_SIZE(xdr::xstring<>("123"), 8);
  CHECK_SIZE((xdr::xvector<int32_t,5>()), 4);

  tuple<uint32_t, double> tn {99, 3.141592654};
  assert(xdr::xdr_traits<decltype(tn)>::has_fixed_size);
  CHECK_SIZE(tn, 12);

  tuple<xdr::xvector<int>, int, xdr::xstring<>> tv{{1, 2, 3}, 99, "hello"};
  assert(!xdr::xdr_traits<decltype(tv)>::has_fixed_size);
  CHECK_SIZE(tv, 32);
}

void
udsb(uint32_t, double, xdr::xstring<> &, bool, std::nullptr_t)
{
}

void
dump_indices(xdr::indices<>)
{
  cout << endl;
}
template<std::size_t N, std::size_t...Ns> void
dump_indices(xdr::indices<N, Ns...>)
{
  cout << " " << N;
  dump_indices(xdr::indices<Ns...>{});
}

//! Apply a function [object] to elements at a set of tuple indices,
//! with arbitrary arguments appended.
template<typename F, typename T, std::size_t...I, typename...A> inline auto
apply_indices(F &&f, T &&t, xdr::indices<I...>, A &&...a) ->
  decltype(f(std::get<I>(std::forward<T>(t))..., std::forward<A>(a)...))
{
  return f(std::get<I>(std::forward<T>(t))..., std::forward<A>(a)...);
}

void
test_tuple()
{
  using namespace xdr;
  using namespace detail;

  auto foo = make_tuple(uint32_t(6), 3.1415,
			xdr::xstring<>("Hello world"), true);
  decltype(foo) bar;
  xdr::xdr_from_msg(xdr_to_msg(foo), bar);
  apply_indices(udsb, foo, indices<0,1,2,3>{}, nullptr);
  apply_indices(udsb, foo, all_indices<4>{}, nullptr);
  assert (foo == bar);
}

int
main()
{
  test_size();
  test_tuple();

  testns::bytes b1, b2;
  xdr::xdr_clear(b2);
  b1.s = "Hello world\n";
  b1.fixed.fill(0xc5);
  b1.variable = { 2, 4, 6, 8 };
  
  xdr::xdr_from_msg(xdr::xdr_to_msg(b1), b2);
  assert(b1.s == b2.s);
  assert(b1.fixed == b2.fixed);
  assert(b1.variable == b2.variable);

  xdr::xdr_from_opaque(xdr::xdr_to_opaque(b1), b2);
  assert(b1.s == b2.s);
  assert(b1.fixed == b2.fixed);
  assert(b1.variable == b2.variable);

  testns::numerics n1, n2;
  xdr::xdr_clear(n2);
  n1.b = false;
  n2.b = true;
  n1.i1 = 0x7eeeeeee;
  n1.i2 = 0xffffffff;
  n1.i3 = UINT64_C(0x7ddddddddddddddd);
  n1.i4 = UINT64_C(0xfccccccccccccccc);
  n1.f1 = 3.141592654;
  n1.f2 = 2.71828182846;
  n1.e1 = testns::REDDER;
  n2.e1 = testns::REDDEST;

  {
    unique_ptr<double> dp1 (new double (3.141592653));
    uint64_t x = xdr::xdr_reinterpret<uint64_t>(*dp1);
    assert (!memcmp(&x, dp1.get(), sizeof(x)));
    x = xdr::xdr_traits<double>::to_uint(*dp1);
    assert (memcmp(&x, dp1.get(), sizeof(x)) == 0);
    unique_ptr<double> dp2 (new double (1.23456789));
    *dp2 = xdr::xdr_traits<double>::from_uint(x);
    assert (!memcmp(&x, dp2.get(), sizeof(x)));
    assert (*dp1 == *dp2);
  }

  xdr::xdr_from_msg(xdr::xdr_to_msg(n1), n2);
  assert(xdr::xdr_to_string(n1) == xdr::xdr_to_string(n2));
  assert(n1.b == n2.b);
  assert(n1.i1 == n2.i1);
  assert(n1.i2 == n2.i2);
  assert(n1.i3 == n2.i3);
  assert(n1.i4 == n2.i4);
  assert(n1.f1 == n2.f1);
  assert(n1.f2 == n2.f2);
  assert(n1.e1 == n2.e1);

  testns::uniontest u1, u2;
  xdr::xdr_clear(u2);
  u1.ip.activate() = 0x12349876;
  u1.key.arbitrary(REDDEST);
  u1.key.big() = { 5, 4, 3, 2, 1, 0, 0, 0, 255 };

  {
    xdr::msg_ptr m (xdr::xdr_to_msg(u1));
    xdr::xdr_from_msg(m, u2);
    assert(xdr::xdr_to_string(u1) == xdr::xdr_to_string(u2));
  }

  assert(*u1.ip == *u2.ip);
  assert(u1.key.arbitrary() == u2.key.arbitrary());
  assert(u1.key.big() == u2.key.big());
  {
    bool ok = false;
    try { u2.key.medium() = 7777; }
    catch (const xdr::xdr_wrong_union &) { ok = true; }
    assert (ok);
  }

  {
    testns::unionvoidtest uv;
    uv.arbitrary(RED);
    assert(uv.arbitrary() == RED);
    uv.arbitrary(REDDER);
    assert(uv.arbitrary() == REDDER);
    uv.arbitrary(REDDEST);
    assert(uv.arbitrary() == REDDEST);
  }

  {
    testns::containertest ct1, ct2;
    xdr::xdr_clear(ct2);

    ct1.uvec = { u_4_12(4), u_4_12(12), u_4_12(4), u_4_12(4) };
    ct1.sarr[0] = "hello";
    ct1.sarr[1] = "world";

    xdr::msg_ptr m (xdr::xdr_to_msg(ct1));
    xdr::xdr_from_msg(m, ct2);
    assert(xdr::xdr_to_string(ct1) == xdr::xdr_to_string(ct2));

    assert(ct1.uvec.at(0).f4().i == ct2.uvec.at(0).f4().i);
    assert(ct1.uvec.at(1).f12().i == ct2.uvec.at(1).f12().i);
    assert(ct1.uvec.at(1).f12().d == ct2.uvec.at(1).f12().d);
    assert(ct1.uvec.at(2).f4().i == ct2.uvec.at(2).f4().i);
    assert(ct1.uvec.at(3).f4().i == ct2.uvec.at(3).f4().i);
    for (unsigned i = 0; i < 2; i++)
      assert(ct1.sarr.at(i) == ct2.sarr.at(i));

    testns::containertest1 ct3;
    bool ok = false;
    try { xdr::xdr_from_msg(m, ct3); }
    catch (const xdr::xdr_overflow &) { ok = true; }
    assert(ok);
  }

  return 0;
}
