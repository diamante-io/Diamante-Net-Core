xdrpp
=====

This package consists of an
[RFC4506](http://tools.ietf.org/html/rfc4506) XDR compiler, `xdrc`,
and an [RFC5531](https://tools.ietf.org/html/rfc5531) RPC library
`libxdrpp` for C++11.  It takes advantage of C++ to hide most of the
complexity of setting up connections and cleaning up data, and
provides a type-safe interface that should help avoid many programming
errors common with other C/C++ XDR/RPC libraries.  The library
provides both synchronous (see [srpc.h](srpc_8h.html)) and
asynchronous/event-driven (see [arpc.h](arpc_8h.html)) interfaces to
RPC.  A type-safe event harness allows integration with other file
descriptor, timer, signal, and inter-thread callbacks (see
[pollset.h](pollset_8h.html)).  Deterministic marshaling to
byte-vectors makes it easy to hash or digitally sign XDR data
structures (see [marshal.h](marshal_8h.html)).  
Other features include [pretty printing](printer_8h.html), tracing
(set the `XDR_TRACE_CLIENT` or `XDR_TRACE_SERVER` environment variable
or the corresponding lower-case global `bool` values), and optional
integration with [autocheck](autocheck_8h.html) and
[cereal](cereal_8h.html) (the latter of which can, among other things,
translate XDR to and from JSON).

For the latest source code, run:

    git clone https://github.com/xdrpp/xdrpp.git

**See the [xdrc manual page][manpage] for more information.**

A high-level example gives a flavor for the library.  To use RPC you
first define a protocol by writing a `.x` file in
[XDR format](http://tools.ietf.org/html/rfc4506).  Then, use the
included XDR compiler, [`xdrc`](md_doc_xdrc_81.html), to translate
this file into C++ types.  Finally use XDRPP's facilities to translate
the XDR interface into a C++ interface on both ends:  The client gets
an object with methods corresponding to the RPC procedures, while the
server must implement an object with methods for each of the
procedures.

Here is an example XDR source file:

~~~~ {.c}
// myprog.x

typedef string big_string<>;

program MyProg {
  version MyProg1 {
    void null(void) = 0;
    big_string hello(int) = 1;
    big_string goodbye(big_string) = 2;
  } = 1;
} = 0x2dee1645;
~~~~

Assuming a server that has registered with rpcbind, the client can
invoke these RPCs as follows:

~~~~ {.cxx}
// client.cc
// compile as:  c++ -std=c++11 `pkg-config --cflags xdrpp` -c client.cc
//              c++ -std=c++11 -o client client.o `pkg-config --libs xdrpp`

#include <iostream>
#include <xdrpp/srpc.h>
#include "myprog.h"

using namespace std;
using namespace xdr;

int
main(int argc, char **argv)
{
  unique_fd fd = tcp_connect_rpc("myserver.example.com",
                                 MyProg1::program, MyProg1::version);
  srpc_client<MyProg1> c{fd.get()};
  unique_ptr<big_string> result = c.hello(5);
  cout << "The result of hello(5) is " << *result << endl;
  return 0;
}
~~~~

A server is not much more complicated, except that it must implement
each of the RPC methods as methods of a C++ class.  For example:

~~~~ {.cxx}
#include <xdrpp/server.h>
#include "xdrpp/myprog.h"

using namespace xdr;

class MyProg1_server {
public:
  using rpc_interface_type = MyProg1;

  void null();

  unique_ptr<big_string>
  hello(unique_ptr<int> arg);

  unique_ptr<big_string>
  goodbye(unique_ptr<big_string> arg);
};

// ... implement three methods of MyProg1 ...

int
main(int argc, char **argv)
{
  MyProg1_server s;
  srpc_tcp_listener rl;
  rl.register_service(s);
  rl.run(); // should never return
  return 1;
}
~~~~

The compiler has options (`-serverhh` and `-servercc`) to generate
scaffolding for `class MyProg` and its declaration.

Note that these examples assume the server is running `rpcbind`, so
that the client can find the TCP port number of the RPC program on the
server.  It is also possible to connect manually and pass file
descriptors for a connected socket to the constructor for
`srpc_client`, or to pass a `unique_fd` to an `srpc_tcp_listener` (in
which case the `srpc_tcp_listener` takes ownership of the file
descriptor).

[manpage]: md_doc_xdrc_81.html
