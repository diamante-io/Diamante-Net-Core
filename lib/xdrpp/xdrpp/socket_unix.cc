
#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <xdrpp/socket.h>
#include <xdrpp/srpc.h>
#include <xdrpp/rpcb_prot.hh>

namespace xdr {

using std::string;

bool
sock_eagain()
{
  int err = errno;
  return err == EAGAIN || err == EWOULDBLOCK || err == EINTR;
}

const char *
sock_errmsg()
{
  return std::strerror(errno);
}

void
throw_sockerr(const char *msg)
{
  throw std::system_error(errno, std::system_category(), msg);
}

ssize_t
read(sock_t s, void *buf, std::size_t count)
{
  return ::read(s.fd(), buf, count);
}

ssize_t
write(sock_t s, const void *buf, std::size_t count)
{
  return ::write(s.fd(), buf, count);
}

ssize_t
readv(sock_t s, const struct iovec *iov, int iovcnt)
{
  return ::readv(s.fd(), iov, iovcnt);
}

ssize_t writev(sock_t s, const struct iovec *iov, int iovcnt)
{
  return ::writev(s.fd(), iov, iovcnt);
}

void
close(sock_t s)
{
  while (::close(s.fd()) == -1)
    if (!sock_eagain()) { // EINTR possible
      std::cerr << "close: " << sock_errmsg() << std::endl;
      return;
    }
}

void
set_nonblock(sock_t s)
{
  int n;
  if ((n = fcntl (s.fd_, F_GETFL)) == -1
      || fcntl (s.fd_, F_SETFL, n | O_NONBLOCK) == -1)
    throw_sockerr("O_NONBLOCK");
}

void
set_close_on_exec(sock_t s)
{
  int n;
  if ((n = fcntl(s.fd_, F_GETFD)) == -1
      || fcntl(s.fd_, F_SETFD, n | FD_CLOEXEC) == -1)
    throw_sockerr("F_SETFD");
}


void
create_selfpipe(sock_t ss[2])
{
  int fds[2];
  if (pipe(fds) == -1)
    throw std::system_error(errno, std::system_category(), "pipe");
  ss[0] = sock_t(fds[0]);
  ss[1] = sock_t(fds[1]);
}

}
