
#include <iomanip>
#include <xdrpp/printer.h>

namespace xdr {

std::string
escape_string(const std::string &s)
{
  std::ostringstream os;
  os << '\"';
  for (char c : s) {
    if (c >= 0x20 && c < 0x7f) {
      if (c == '\"' || c == '\\')
	os << '\\';
      os << c;
    }
    else
      switch (c) {
      case '\t':
	os << "\\t";
	break;
      case '\r':
	os << "\\r";
	break;
      case '\n':
	os << "\\n";
	break;
      case '\"':
	os << "\\\"";
	break;
      default:
	os << "\\x" << std::setw(2) << std::setfill('0')
	   << std::oct << (unsigned(c) & 0xff);
	break;
      }
  }
  os << '\"';
  return os.str();
}

std::string
hexdump(const void *_data, size_t len)
{
  const std::uint8_t *data = static_cast<const std::uint8_t *>(_data);
  std::ostringstream os;
  os.fill('0');
  os.setf(std::ios::hex, std::ios::basefield);
  for (; len > 0; ++data, --len)
    os << std::setw(2) << unsigned(*data);
  return os.str();
}

namespace detail {

std::ostream &
Printer::bol(const char *name)
{
  if (skipnl_)
    skipnl_ = false;
  else {
    if (comma_)
      buf_ << ",";
    buf_ << std::endl << std::string(indent_, ' ');
  }
  comma_ = true;
  if (name)
    buf_ << name << " = ";
  return buf_;
}

}

}
