
#include <cassert>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <sys/uio.h>

#include <xdrpp/msgsock.h>
#include <xdrpp/rpc_msg.hh>
#include <xdrpp/server.h>

namespace xdr {

msg_sock::~msg_sock()
{
  ps_.fd_cb(s_, pollset::ReadWrite);
  close(s_);
  *destroyed_ = true;
}

void
msg_sock::init()
{
  set_nonblock(s_);
  initcb();
}

void
msg_sock::initcb()
{
  if (rcb_)
    ps_.fd_cb(s_, pollset::Read, [this](){ input(); });
  else
    ps_.fd_cb(s_, pollset::Read);
}

void
msg_sock::input()
{
  std::shared_ptr<bool> destroyed{destroyed_};
  for (int i = 0; i < 3 && !*destroyed; i++) {
    if (rdmsg_) {
      iovec iov[2];
      iov[0].iov_base = rdmsg_->data() + rdpos_;
      iov[0].iov_len = rdmsg_->size() - rdpos_;
      iov[1].iov_base = nextlenp();
      iov[1].iov_len = sizeof nextlen_;
      ssize_t n = readv(s_, iov, 2);
      if (n <= 0) {
	if (n < 0 && eagain(errno))
	  return;
	if (n == 0)
	  errno = ECONNRESET;
	else
	  std::cerr << "msg_sock::input: " << sock_errmsg() << std::endl;
	rcb_(nullptr);
	return;
      }
      rdpos_ += n;
      if (rdpos_ >= rdmsg_->size()) {
	rdpos_ -= rdmsg_->size();
	rcb_(std::move(rdmsg_));
	if (*destroyed)
	  return;
      }
    }
    else if (rdpos_ < sizeof nextlen_) {
      ssize_t n = read(s_, nextlenp() + rdpos_, sizeof nextlen_ - rdpos_);
      if (n <= 0) {
	if (n < 0 && eagain(errno))
	  return;
	if (n == 0)
	  errno = rdpos_ ? ECONNRESET : 0;
	else
	  std::cerr << "msg_sock::input: " << sock_errmsg() << std::endl;
	rcb_(nullptr);
	return;
      }
      rdpos_ += n;
    }

    if (rdmsg_ || rdpos_ < sizeof nextlen_)
      return;
    size_t len = nextlen();
    if (!(len & 0x80000000)) {
      std::cerr << "msgsock: message fragments unimplemented" << std::endl;
      errno = ECONNRESET;
      rcb_(nullptr);
      return;
    }
    len &= 0x7fffffff;
    if (!len) {
      rdpos_ = 0;
      rcb_(message_t::alloc(0));
      continue;
    }

    if (len <= maxmsglen_) {
      // Length comes from untrusted source; don't crash if can't alloc
      try { rdmsg_ = message_t::alloc(len); }
      catch (const std::bad_alloc &) {
	std::cerr << "msg_sock: allocation of " << len << "-byte message failed"
		  << std::endl;
      }
    }
    else {
      std::cerr << "msg_sock: rejecting " << len << "-byte message (too long)"
		<< std::endl;
      ps_.fd_cb(s_, pollset::Read);
    }
    if (rdmsg_)
      rdpos_ = 0;
    else {
      errno = E2BIG;
      rcb_(nullptr);
    }
  }
}

void
msg_sock::putmsg(msg_ptr &mb)
{
  if (wfail_) {
    mb.reset();
    return;
  }

  bool was_empty = !wsize_;
  wsize_ += mb->raw_size();
  wqueue_.emplace_back(mb.release());
  if (was_empty)
    output(false);
}

void
msg_sock::pop_wbytes(size_t n)
{
  if (n == 0)
    return;
  assert (n <= wsize_);
  wsize_ -= n;
  size_t frontbytes = wqueue_.front()->raw_size() - wstart_;
  if (n < frontbytes) {
    wstart_ += n;
    return;
  }
  n -= frontbytes;
  wqueue_.pop_front();
  while (n > 0 && n >= (frontbytes = wqueue_.front()->raw_size())) {
    n -= frontbytes;
    wqueue_.pop_front();
  }
  wstart_ = n;
}

void
msg_sock::output(bool cbset)
{
  static constexpr size_t maxiov = 8;
  size_t i = 0;
  iovec v[maxiov];
  for (auto b = wqueue_.begin(); i < maxiov && b != wqueue_.end(); ++b, ++i) {
    if (i) {
      v[i].iov_len = (*b)->raw_size();
      v[i].iov_base = const_cast<char *> ((*b)->raw_data());
    }
    else {
      v[i].iov_len = (*b)->raw_size() - wstart_;
      v[i].iov_base = const_cast<char *> ((*b)->raw_data()) + wstart_;
    }
  }
  ssize_t n = writev(s_, v, i);
  if (n <= 0) {
    if (n != -1 || !eagain(errno)) {
      wfail_ = true;
      wsize_ = wstart_ = 0;
      wqueue_.clear();
    }
    return;
  }
  pop_wbytes(n);

  if (wsize_ && !cbset)
    ps_.fd_cb(s_, pollset::Write, [this](){ output(true); });
  else if (!wsize_ && cbset)
    ps_.fd_cb(s_, pollset::Write);
}

void
rpc_sock::abort_all_calls()
{
  decltype(calls_) calls(std::move(calls_));
  calls_.clear();
  for (auto &call : calls)
    try { call.second(nullptr); }
    catch (const std::exception &e) {
      std::cerr << e.what() << std::endl;
    }
}

void
rpc_sock::recv_msg(msg_ptr b)
{
  if (!b || b->size() < 8) {
    abort_all_calls();
    recv_call(nullptr);
  }
  else if (b->word(1) == swap32le(CALL))
    recv_call(std::move(b));
  else if (b->word(1) == swap32le(REPLY)) {
    auto calli = calls_.find(b->word(0));
    if (calli == calls_.end()) {
      std::cerr << "ignoring reply to unknown call" << std::endl;
      return;
    }
    auto cb (std::move(calli->second));
    calls_.erase(calli);
    cb(std::move(b));
  }
  else {
    abort_all_calls();
    recv_call(nullptr);
  }
}

void
rpc_sock::send_call(msg_ptr &b, msg_sock::rcb_t cb)
{
  calls_.emplace(b->word(0), std::move(cb));
  ms_->putmsg(b);
}

void
rpc_sock::recv_call(msg_ptr b)
{
  if (servcb_)
    servcb_(std::move(b));
  else {
    std::cerr << "rpc_sock::recv_call: incoming call but no server"
	      << std::endl;
    send_reply(rpc_accepted_error_msg(b->word(0), PROG_UNAVAIL));
  }
}

}
