
#include <xdrpp/socket.h>

namespace xdr {

bool
sock_eagain()
{
  int err = WSAGetLastError();
  return err == WSAEINTR || err == WSAEWOULDBLOCK;
}

const char *
sock_errmsg()
{
  return "some windows error";
}

void
throw_sockerr(const char *msg)
{
  throw system_error(WSAGetLastError, msg);
}

bool
sock_eof()
{
  int err = WSAGetLastError();
  return err == WSAECONNRESET || err == WSAECONNABORTED || err == WSAESHUTDOWN;
}

ssize_t
read(sock_t s, void *buf, std::size_t count)
{
  int n = ::recv(s.fd(), buf, count, 0);
  if (n == SOCKET_ERROR)
    return sock_eof() ? 0 : -1;
  return n;
}

ssize_t
write(sock_t s, const void *buf, std::size_t count)
{
  int n = ::send(s.fd(), buf, count, 0);
  return n == SOCKET_ERROR ? -1 : n;
}

ssize_t
readv(sock_t s, const struct iovec *iov, int iovcnt)
{
  int tot;
  for(tot = 0; iovcnt > 0; ++iov, --iovcnt) {
    int n = read(s, iov->iov_base, iov->iov_len);
    if (n == -1) {
      if (!sock_eagain())
	return -1;
      if (tot > 0)
	return tot;
      WSASetLastError(WSAEWOULDBLOCK);
      return -1;
    }
    tot += n;
    if (n != iov->iov_len)
      break;
  }
  return tot;
}

ssize_t writev(sock_t s, const struct iovec *iov, int iovcnt)
{
  int tot;
  for(tot = 0; iovcnt > 0; ++iov, --iovcnt) {
    int n = write(s, iov->iov_base, iov->iov_len);
    if (n == -1)
      return -1;
    tot += n;
  }
  return tot;
}

void
close(sock_t s)
{
  while (closesocket(s.fd()) == SOCKET_ERROR)
    if (!sock_eagain())
      std::cerr << "closesocket: " << sock_errmsg() << std::endl;
      return;
    }
}

[[noreturn]] void
unimpl(const char *f)
{
  std::cerr << f << " unimplemented" << endl;
  std::terminate();
}
#define UNIMPL() unimpl(__func__)

void
set_nonblock(sock_t s)
{
  UNIMPL();
}

void
set_close_on_exec(sock_t s)
{
  // Does windows even have exec?
}

void
create_selfpipe(sock_t ss[2])
{
  UNIMPL();
}


}
