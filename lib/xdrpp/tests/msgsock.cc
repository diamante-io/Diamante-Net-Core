
#include <cassert>
#include <cstring>
#include <iostream>
#include <sstream>
#include <thread>
#include <sys/socket.h>
#include <xdrpp/msgsock.h>
#include <xdrpp/printer.h>

using namespace std;
using namespace xdr;

void
echoserver(sock_t s)
{
  pollset_plus ps;
  bool done {false};
  msg_sock ss(ps, s, nullptr);
  int i = 0;

  ss.setrcb([&done,&ss,&i](msg_ptr b) {
      if (b) {
	cout << "echoing #" << i++ << " (" << b->size() << " bytes)" << endl;
	ss.putmsg(b);
      }
      else
	done = true;
    });

  ps.signal_cb(SIGPIPE, [](){});
  while (!done && ps.pending())
    ps.poll();
}

void
echoclient(sock_t s)
{
  pollset_plus ps;
  msg_sock ss { ps, s };
  unsigned int i = 0;

  {
    msg_ptr b (message_t::alloc(i));
    ss.putmsg(b);
  }
  ss.setrcb([&i,&ss](msg_ptr b) {
      assert(b);
      assert(b->size() == i);
      cout << b->size() << ": " << hexdump(b->data(), b->size()) << std::endl;
      for (unsigned j = 0; j < b->size(); j++) {
	assert(unsigned(b->data()[j]) == b->size());
      }
      ++i;
      b = message_t::alloc(i);
      memset(b->data(), i, i);
      ss.putmsg(b);
    });

  while (i < 100 && ps.pending())
    ps.poll();
}

int
main(int argc, char **argv)
{
  int fds[2];
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == -1) {
    perror("socketpair");
    exit(1);
  }

  thread t1 (echoclient, sock_t(fds[0]));
  echoserver(sock_t(fds[1]));
  t1.join();

  return 0;
}
