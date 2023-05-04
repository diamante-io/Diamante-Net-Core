
#include <cassert>
#include <iostream>
#include <sstream>
#include <xdrpp/printer.h>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include "tests/xdrtest.hh"

void
cereal_override(cereal::JSONOutputArchive &ar,
                const testns::inner &t,
                const char* field)
{
    xdr::archive(ar, 9999, "bort");
}

#include <xdrpp/cereal.h>

using namespace std;

int
main()
{
  testns::numerics n1, n2;
  n1.b = true;
  n2.b = false;
  n1.i1 = 0x7eeeeeee;
  n1.i2 = 0xffffffff;
  n1.i3 = UINT64_C(0x7ddddddddddddddd);
  n1.i4 = UINT64_C(0xfccccccccccccccc);
  n1.f1 = 3.141592654;
  n1.f2 = 2.71828182846;
  n1.e1 = testns::REDDER;
  n2.e1 = testns::REDDEST;

  cout << xdr::xdr_to_string(n1);

  ostringstream obuf;
  {
    //cereal::BinaryOutputArchive archive(obuf);
    cereal::JSONOutputArchive archive(obuf);
    archive(n1);
  }

  cout << obuf.str() << endl;

  {
    istringstream ibuf(obuf.str());
    //cereal::BinaryInputArchive archive(ibuf);
    cereal::JSONInputArchive archive(ibuf);
    archive(n2);
  }

  cout << xdr::xdr_to_string(n2);

  testns::nested_cereal_adapter_calls nc;
  nc.strptr.activate() = "hello";
  nc.strvec.push_back("goodbye");
  nc.strarr[0] = "friends";
  {
    ostringstream obuf2;
    {
      cereal::JSONOutputArchive archive(obuf2);
      archive(nc);
    }
    cout << obuf2.str();
  }

  {
    testns::outer tu;
    ostringstream obuf2;
    {
      cereal::JSONOutputArchive archive(obuf2);
      archive(tu);
    }
    cout << obuf2.str();
    assert(obuf2.str().find("\"bort\": 9999") != string::npos);
  }

  return 0;
}
