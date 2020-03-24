% xdrc(1)
% David Mazieres
% 

# NAME

xdrc - RFC4506 XDR compiler for libxdrpp

# SYNOPSIS

xdrc {-hh|-serverhh|-servercc} [-o _outfile_] [-DMACRO=val...] _input_.x

# DESCRIPTION

`xdrc` compiles an RFC4506 XDR (external data representation) file
into a C++11 header file, creating a new C++ type for each type
defined in XDR.

## Native representations

`xdrc` uses the following representations for XDR types in C++:

* XDR structs are translated field-for-field into C++ structures.

* XDR unions are translated into a structure in which each field is
  actually a method that either returns a reference to the underlying
  value, or throws an exception if that field is not currently
  selected.  The discriminant method with no arguments returns the
  value, and with a value sets the discriminant (also constructing the
  appropriate union field).  For example, if the XDR file contains:

        union MyType switch (unsigned discriminant) {
        case 0:
          int zero;
        case 1:
          string one<>;
        default:
          void;
        }

    The C++ type can be accessed as follows:

        MyType mt;
        mt.discriminant(1);
        mt.one() = "hello";
        std::cout << mt.one() << std::endl;
        mt.discriminant(0);
        mt.zero() = 5;

        std::cout << mt.one() << std::endl; // throws xdr_wrong_union

* XDR `bool` is represented as a C++ `bool` (with XDR's `TRUE`
  translated to `true` and `FALSE` to `false`).

* XDR [unsigned] `int` and `hyper` types are represented as the
  cstdint types `std::int32_t`, `std::uint32_t`, `std::int64_t`, and
  `std::uint64_t`.  As per RFC4506, no types narrower than 32 bits
  exist.

* XDR enums are translated into simple (non-class) enums with
  underlying type `std::int32_t`.

* XDR pointers (`*`) are translated into C++ `xdr:pointer`, a subtype
  of `std::unique_ptr`.  `xdr::pointer` adds an `activate()` method
  that allocates an object of the appropriate type (if the pointer is
  null) and returns a reference to the current object.

* XDR fixed-length arrays are translated into C++ `xdr::xarray`, a
  subtype of `std::array`.

* XDR variable-length arrays (`type field<N>`) are translated into C++
  `xdr::xvector<T,N>`, a subtype of `std::vector<T>`, where `N`
  represents the maximum size.  Static constexpr method `max_size()`
  returns the maximum size.

* XDR opaque is translated into C++ `std::uint8_t`, but as per
  RFC4506, opaque may only appear as part of a fixed- or
  variable-length array declaration.

* XDR `string<N>` is translated into `xdr::xstring<N>`, a subtype of
  string encoding the maximum size.  Static constexpr method
  `max_size()` returns the maximum size.

## Extensions to RFC4506

`xdrc` supports the following extensions to the syntax defined in
RFC4506:

* The source file is run through the C preprocessor.  Macro XDRC is
  pre-defined to 1, permitting the use of #ifdef to take advantage of
  other xdrc-specific features.  Also, with the `-hh` option,
  `XDRC_HH=1` is pre-defined, and with the `-serverhh` and `-servercc`
  options, `XDRC_SERVER=1` is predefined.

* Lines beginning with a `%` sign are copied verbatim into the output
  file.

* Portions of the input file may be bracketed by `namespace myns {`
  ... `}`.  The corresponding C++ will be embedded in the same
  namespace.

* Type names may include a namespace scope `::`, so as to be able to
  make use of types defined in a different namespace.  (Alternatively,
  a literal can be used, such as `%using namespace otherns;`.)

The namespace-related extensions should be used sparingly if
compatibility with other languages and XDR compilers is desirable.
While it may be useful to enclose an entire source file in a
`namespace` for consistency, it is a good idea to wrap such directives
inside `#if XDRC` / `#endif` conditionals.  Heavy use of namespaces
for internal structuring purposes (e.g., wrapping every `enum` in its
own namespace) is a bad idea as it will make protocols incompatible
with RFC4506.

## Serialization and traversing data structures

A template class `xdr::xdr_traits<T>` is used to hold metadata for
each native C++ type `T` representing an XDR type.  For XDR structs,
unions, arrays and pointers, this traits class contains two important
static methods:

    template<class Archive> void save(Archive &archive, const T &);
    template<class Archive> void load(Archive &archive, T &);

These methods use `archive` as a function object and call it on every
field in the data structure.  Hence, the type `Archive` can have an
overloaded `operator()` that does different things for different
types.  To implement an archive, you will need to support the
following types:

> * `bool`, `std::int32_t`, `std::uint32_t`, `std::int64_t`,
>   `std::uint64_t`, `float`, `double`, `xdr::xstring`,
>   `xdr::opaque_array` and `xdr::opaque_vec` (the latter two are not
>   considered containers, despite being implemented in terms of
>   `xarray` and `xvector`).

> * The `xdr::xarray`, `xdr:xvector`, and `xdr::pointer` containers of
>   the above types (or their supertypes `std::array`, `std::vector`,
>   and `std::unique_ptr`).

> * Any field types that are themselves XDR structures.

For debugging purposes and formats (such as JSON) that need access to
field names, it is also possible for the `Archive` type to receive the
field names of fields that are traversed.  The following template (in
the `xdr::` namespace) can be specialized to prepare arguments by
bundling them with their names:

    template<typename Archive> struct archive_adapter {
      template<typename T> static void
      apply(Archive &ar, T &&t, const char *) {
        ar(std::forward<T>(t));
      }
    };

## Program and version representations

For each `version` block (declared inside a `program` block, as
documented in Section 12.2 of RFC5531), `xdrc` generates a C++ struct
of the same name.  As a consequence, versions should use unique names.
The version struct generated by `xdrc` contains no data fields, but
rather is used to encode information about all procedures in the
interface for use by templates.  Specifically, the version struct
contains the following fields:

* `program` - A `uint32_t` encoding the program number specified in
  the XDR input file.

* `version` - A `uint32_t` encoding the program number specified in
  the XDR input file.

* `static constexpr const char *program_name()` - A static method
  returning the textual name of the program.

* `static constexpr const char *version_name()` - A static method
  returning the textual name of the version.

* For each procedure `myproc`, an inner struct `myproc_t` encoding the
  following metadata about the procedure:

    * `interface_type` - a typedef of the version struct containing
      this proc struct.

    * `proc` - a `uint32_t` containing the procedure number.

    * `proc_name()` - a static constexpr function returning the name
      of the procedure.

    * `arg_type` - exists only if the procedure takes 0 or 1 argument.
      If it takes 0 arguments, then a typedef of void.  If it takes
      one argument, then a typedef of the argument type.

    * `arg_tuple_type` - a `std::tuple` parameterized by the types of
      the argument list of this procedure.  E.g., for a declaration

            void myproc(void) = 1;

        `arg_tuple_type` is `std::tuple<>`.  For a declaration

            void myproc(int, bool) = 1;

        `arg_tuple_type` is `std::tuple<std::int32_t, bool>`.

    * `res_type` - the type returned by the procedure, including
      `void` for procedures declared to return void.

    * `res_wire_type` - the type returned by the procedure, except for
      procedures returning `void`, for which `res_wire_type` is
      typedefed to `xdr_void` (a.k.a., `std::tuple<>`), an empty data
      structure whose wire representation is 0 bytes.

    * `dispatch(c, a1, a2, ...)` - a static method that calls
      `c.myproc(a1, a2, ...)` (where `myproc` is the name of this
      procedure) and returns the result.  Hence, given a procedure
      metadata structure `P`, a template can use `P::dispatch` to call
      a method called `myproc` on an arbitrary class without needing
      to know that the name of the procedure is `myproc`.

* `call_dispatch(t, procno, a1, a2, ...)` - calls the template method
  `dispatch` on object `t`, passing as a template type argument the
  procedure metadata type corresponding to procedure number `procno`
  (a `std::uint32_t`).  Returns true if `procno` was valid for this
  program/version.  `call_dispatch` is perhaps best illustrated by
  example.  Given the source:

        program myprog {
          version myprog1 {
             void null(void) = 0;
             void non_null(int) = 1;
          } = 1;
        } = 0x40000000;
  
  You will get:

        struct myprog1 {
          // ...
          struct null_t { /* ... */ };
          struct non_null_t { /* ... */ };

          template<typename T, typename...A> static bool
          call_dispatch(T &&t, std::uint32_t proc, A &&...a) {
            switch(proc) {
            case 0:
              t.template dispatch<null_t>(std::forward<A>(a)...);
              return true;
            case 1:
              t.template dispatch<non_null_t>(std::forward<A>(a)...);
              return true;
            }
            return false;
          }
        };

* `_xdr_client` - a template struct, `template<typename T> struct
  _xdr_client`, containing a `T` (a pointer-like type), and whose
  constructor arguments are passed to `T`.  In addition, this
  structure contains one method for each procedure, that calls
  template method `invoke` on the object pointed to by `T` with the
  following template typename arguments:  The first argument is the
  procedure metadata type for this procedure.  The remaining
  (variadic) arguments are the types of all the arguments passed to
  this procedure.  Continuing the example above, `myprog1` would
  contain the following structure:

        template<typename T> struct _xdr_client {
          T t_;
          template<typename...ARGS> _xdr_client(ARGS &&...args)
            : t_(std::forward<ARGS>(args)...) {}

          template<typename...ARGS> auto
          null(ARGS &&...args) ->
          decltype(t_->template invoke<null_t>(
                   std::forward<ARGS>(args)...)) {
            return t_->template invoke<null_t>(
                   std::forward<ARGS>(args)...);
          }

          template<typename...ARGS> auto
          non_null(ARGS &&...args) ->
          decltype(t_->template invoke<non_null_t, int>(
                   std::forward<ARGS>(args)...)) {
            return t_->template invoke<non_null_t, int>(
                   std::forward<ARGS>(args)...);
          }
        };

# OPTIONS

\-hh
:   Selects C++ header file output.  This is the main output format,
    and its output is required for use with libxdrpp.

\-serverhh
:   Generates a C++ header file containing declarations of objects you
    can use to implement a server for each interface, using
    `srpc_tcp_listener` or `arpc_tcp_listener`.  See the EXAMPLES
    section.

\-servercc
:   Generates a .cc file containing empty method definitions
    corresponding to the object files created with `-serverhh`.

\-a, -async
:   With `-serverhh` or `-servercc`, says to generate scaffolding for
    an event-driven interface to be used with `arpc_tcp_listener`, as
    opposed to the default `srpc_tcp_listener`.

\-p, -ptr
:   With `-serverhh` or `-servercc`, says to generate methods that take
    arguments and return values as `unique_ptr<T>`.  The default is to
    pass arguments by C++ reference.  Note that the library works with
    both references and `unique_ptr` arguments, so this argument only
    says what one would like to start out with, and one can later edit
    individual prototypes to change pointers to references.

\-o _outfile_
:   Specifies the output file into which to write the generated code.
    The default, for `-hh`, is to replace `.x` with `.hh` at the end
    of the input file name.  `-serverhh` and `-servercc` append
    `.server.hh` and `.server.cc`, respectively.  The special
    _outfile_ `-` sends output to standard output.

\-DMACRO=val
:   The input file is run through the C preprocessor, and this option
    adds additional defines.  (Note that the symbol `XDRC` is always
    defined to 1, if you wish to test for xdrc vs. other RPC
    compilers.)

# EXAMPLES

Consider the following XDR program definition in a file myprog.x:

    typedef string big_string<>;

    program MyProg {
      version MyProg1 {
        void null(void) = 0;
        big_string hello(int) = 1;
        big_string goodbye(big_string) = 2;
      } = 1;
    } = 0x2dee1645;

The `-serverhh` option will generate a header with the following
class:

    class MyProg1_server {
    public:
      using rpc_interface_type = MyProg1;

      void null();

      unique_ptr<big_string>
      hello(unique_ptr<int> arg);

      unique_ptr<big_string>
      goodbye(unique_ptr<big_string> arg);
    };

You have to add any fields you need to this structure, then
implement the three methods corresponding to the interface.  (Note
the very important type `rpc_interface_type` tells the library
which interface this object implements.)  Given such an object,
you can then implement a TCP RPC server (that registers its TCP
port with rpcbind) as follows:

    #include <xdrpp/server.h>
    #include "xdrpp/myprog.server.h"

    using namespace xdr;

    int
    main(int argc, char **argv)
    {
      MyProg1_server s;
      srpc_tcp_listener rl;
      rl.register_service(s);
      rl.run();
      return 1;
    }

To implement a simple client that talks to this server, you can use
code like the following:

    #include <iostream>
    #include <xdrpp/srpc.h>
    #include "myprog.h"

    using namespace std;
    using namespace xdr;

    int
    main(int argc, char **argv)
    {
      unique_fd fd = tcp_connect_rpc(argc > 2 ? argv[2] : nullptr,
                                     MyProg1::program,
                                     MyProg1::version);
      srpc_client<MyProg1> c{fd.get()};
      unique_ptr<big_string> result = c.hello(5);
      cout << "The result of hello(5) is " << *result << endl;
      return 0;
    }

## Compilation

The generated `xdrc` output files must be compiled with a compiler
supporting C++11 (or later, such as C++14) and expect to be used with
libxdrpp.  This typically requires extra compiler flags (e.g.,
`-std=c++11`).  You can use pkgconfig to find the location of the
headers and libraries.  With CXX set to both g++ and clang++, the
following Makefile variables work:

    CXXFLAGS = -std=c++11 `pkg-config --cflags xdrpp`
    LIBS = `pkg-config --libs xdrpp`

# FILES

PREFIX/include/xdrc/types.h
:   Types used in generated C++ code.

PREFIX/include/xdrc/cereal.h
:   Integration with the [cereal](http://uscilab.github.io/cereal/)
    serialization library.

# SEE ALSO

<http://tools.ietf.org/html/rfc4506>

<http://tools.ietf.org/html/rfc5531>

# BUGS

Certain names that are legal in XDR cannot be used as type or field
names.  For example, C++ keywords are not allowed (`namespace`,
`template`, etc.).  In addition, `xdrc` uses a number of names
beginning with underscores (especially names beginning with prefix
`_xdr_`).  Hence you should avoid starting your field names with
underscore.  Union types use private fields that have the names of the
XDR fields with underscore appended.  Hence, in a union you cannot use
two field names one of which is the other with an underscore appended.

`xdrc` translates an XDR `quadruple` to C++ type called `quadruple`,
but most compilers do not have such a type.  Moreover, libxdrpp does
nothing to support such a type.

IEEE 754 floating point allows for many different NaN (not a number)
values.  During serialization, xdrpp simply passes the bytes of a
floating point value through as-is (byteswapping on little-endian
machines).  Different C++ compilers and libraries could conceivably
produce different NaN values from the same code.  Hence, in the
presence of floating point, the serialization output of seemingly
deterministic code may depend on the compiler.
