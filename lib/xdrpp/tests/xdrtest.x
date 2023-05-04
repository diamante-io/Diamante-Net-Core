%using xdr::operator==;
%using xdr::operator<;

enum senum {
  SE_NEGATIVE = -1,
  SE_POSITIVE = 1
};

union sunion switch (senum d) {
  case SE_NEGATIVE:
    bool neg;
  case SE_POSITIVE:
    int pos;
  default:
    void;
};

union uunion switch (unsigned d) {
  case 1:
    bool one;
  case 2:
    int two;
  case 3:
    double three;
  case 4:
    string four<>;
};

struct test_recursive {
  string elem<>;
  test_recursive *next;
  test_recursive nextvec<>;
};

struct fix_4 {
  int i;
};

struct fix_12 {
  int i;
  double d;
};

union u_4_12 switch (int which) {
 case 4:
   fix_4 f4;
 case 12:
   fix_12 f12;
};

typedef fix_12 v12<>;

enum color {
  RED,
  REDDER,
  REDDEST
};

union uptr switch (bool b) {
  case FALSE:
    void;
  case TRUE:
    int *val;
};

namespace testns {
%using xdr::operator==;
%using xdr::operator<;

enum other_color {
  RED,
  REDDER,
  REDDEST
};

union other_union switch (other_color oc) {
  case RED:
    string red_string<>;
  case REDDER:
    string reder_string<>;
};

struct bytes {
  string s<16>;
  opaque fixed[16];
  opaque variable<16>;
};

struct hasbytes {
  bytes the_bytes<>;
};

struct numerics {
  bool b;
  int i1;
  unsigned i2;
  hyper i3;
  unsigned hyper i4;
  float f1;
  double f2;
  other_color e1;
};
  
struct uniontest {
  int *ip;
  union switch (color arbitrary) {
  case ::REDDEST:
    opaque big<>;
  case ::RED:
    hyper medium;
  } key;
};

union unionvoidtest switch (color arbitrary) {
case ::RED:
  opaque big<>;
default:
  void;
};

typedef string bigstr<>;
typedef opaque bigopaque<>;

struct containertest {
  u_4_12 uvec<>;
  bigstr sarr[2];
};

struct containertest1 {
  u_4_12 uvec<2>;
  bigstr sarr[2];
};

union ContainsEnum switch (color c) {
 case color::RED:
   string foo<>;
 case color::REDDER:
   enum { ONE, TWO } num;
};

program xdrtest_prog {
  version xdrtest {
    void null(void) = 1;
    ContainsEnum nonnull(u_4_12) = 2;
  } = 1;
  version xdrtest2 {
    void null2(void) = 1;
    ContainsEnum nonnull2(u_4_12) = 2;
    void ut(uniontest) = 3;
    bigstr three(bool, int, bigstr) = 4;
  } = 2;
} = 0x20000000;

program other_prog {
  version opv1 {
    void o_null(void) = 1;
    void multi_arg(u_4_12, ContainsEnum) = 2;
  } = 1;
}= 0x20000001;

union voidu switch (bool b) {
  case FALSE:
    void;
};

typedef string string32<32>;
struct nested_cereal_adapter_calls {
  string32* strptr;
  string32  strvec<>;
  string32  strarr[2];
};

struct inner {
  int f;
};
struct outer {
  inner in;
};

}
