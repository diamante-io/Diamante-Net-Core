
#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstring>
#include <iostream>
#include <limits>
#include <system_error>
#include <signal.h>
#include <unistd.h>
#include <xdrpp/pollset.h>

namespace xdr {

std::mutex pollset_plus::signal_owners_lock;
pollset_plus *pollset_plus::signal_owners[num_sig];
volatile std::sig_atomic_t pollset_plus::signal_flags[num_sig];

pollset::Timeout
pollset::timeout_null()
{
  static decltype(pollset::time_cbs_) dummy_time_cbs_;
  return Timeout{dummy_time_cbs_.end()};
}
const pollset::Timeout pollset::Timeout::null_(pollset::timeout_null());

void
pollset_plus::signal_handler(int sig)
{
  assert(sig > 0 && sig < num_sig);
  if (signal_flags[sig])
    return;
  signal_flags[sig] = 1;
  std::atomic_thread_fence(std::memory_order_seq_cst);
  if (pollset_plus *ps = signal_owners[sig])
    // It would be pretty unpleasant if ps got deleted in another
    // thread right here.  To prevent this, we set signal_flags[sig]
    // to the value 1 to signal "wake in progress", then spin on the 1
    // when deleting signal callbacks.
    ps->wake(wake_type::Signal);
  std::atomic_thread_fence(std::memory_order_seq_cst);
  signal_flags[sig] = 2;
}

pollset_plus::pollset_plus()
{
  create_selfpipe(selfpipe_);
  set_close_on_exec(selfpipe_[0]);
  set_close_on_exec(selfpipe_[1]);
  set_nonblock(selfpipe_[0]);
  set_nonblock(selfpipe_[1]);
  this->fd_cb(selfpipe_[0], Read, [this](){ this->run_pending_asyncs(); });
}

pollset_plus::~pollset_plus()
{
  {
    std::lock_guard<std::mutex> lk {signal_owners_lock};
    while (signal_cbs_.begin() != signal_cbs_.end())
      erase_signal_cb(signal_cbs_.begin()->first);
  }

  fd_cb(selfpipe_[0], Read);
  close(selfpipe_[0]);
  close(selfpipe_[1]);
}

pollset::fd_state::~fd_state()
{
  // XXX - eventually remove
  assert (!rcb && !wcb);
}

void
pollset_plus::wake(wake_type wt)
{
  static_assert(sizeof wt == 1, "uint8_t enum has wrong size");
  write(selfpipe_[1], &wt, 1);
}

void
pollset_plus::run_pending_asyncs()
{
  std::vector<cb_t> cbs;
  std::vector<cb_t>::iterator i;

  // Catching and re-throwing exceptions ruins the stack trace from
  // uncaught exceptions, which hurts debugability, particularly in a
  // core routine that calls a bunch of callbacks.  Hence, we abuse
  // RAII where catch would be more approriate.
  struct cleanup {
    bool active;
    cb_t action;
    ~cleanup() { if (active) action(); }
  } c { false, [&](){inject_cb_vec(i+1, cbs.end());} };

  {
    wake_type buf[128];
    int n;
    while ((n = read(selfpipe_[0], buf, sizeof buf)) > 0)
      for (int i = 0; i < n && !signal_pending_; i++)
	if (buf[i] == wake_type::Signal)
	  signal_pending_ = true;
  }
  {
    std::lock_guard<std::mutex> lk {async_cbs_lock_};
    async_pending_ = false;
    swap(cbs, async_cbs_);
  }

  for (i = cbs.begin(), c.active = true; i != cbs.end(); i++)
    (*i)();
  c.active = false;
}

void
pollset_plus::inject_cb_vec(std::vector<cb_t>::iterator b,
			    std::vector<cb_t>::iterator e)
{
  if (b != e) {
    std::lock_guard<std::mutex> lk {async_cbs_lock_};
    std::move(b, e, async_cbs_.end());
    if (!async_pending_) {
      async_pending_ = true;
      wake();
    }
  }
}

pollset::cb_t &
pollset::fd_cb_helper(sock_t s, op_t op)
{
  fd_state &fs = state_[s];
  pollfd *pfdp;
  if (fs.idx < 0) {
    fs.idx = pollfds_.size();
    pollfds_.resize(fs.idx + 1);
    pfdp = &pollfds_.back();
    pfdp->fd = s.fd_;		// XXX
  }
  else {
    pfdp = &pollfds_.at(fs.idx);
    assert (pfdp->fd == s.fd_);	// XXX
  }
  if (op & kReadFlag) {
    if (op & kWriteFlag) {
      std::cerr << "Illegal call to pollset::fd_cb with ReadWrite"
		<< std::endl;
      std::terminate();
    }
    fs.roneshot = op & kOnceFlag;
    pfdp->events |= POLLIN;
    return fs.rcb;
  }
  else if (op & kWriteFlag) {
    fs.woneshot = op & kOnceFlag;
    pfdp->events |= POLLOUT;
    return fs.wcb;
  }
  else {
    std::cerr << "Illegal call to pollset::fd_cb with"
                 " neither Read nor Write"
	      << std::endl;
    std::terminate();
  }
}

void
pollset::fd_cb(sock_t s, op_t op, std::nullptr_t)
{
  auto fi = state_.find(s);
  if (fi == state_.end())
    return;
  pollfd &pfd = pollfds_.at(fi->second.idx);
  
  if (op & kReadFlag) {
    pfd.events &= ~POLLIN;
    fi->second.rcb = nullptr;
  }
  if (op & kWriteFlag) {
    pfd.events &= ~POLLOUT;
    fi->second.wcb = nullptr;
  }
}

std::size_t
pollset::num_cbs() const
{
  return pollfds_.size() + time_cbs_.size();
}

bool
pollset_plus::pending() const
{
  return nasync_ || num_cbs();
}

int
pollset::next_timeout(int ms)
{
  auto next = time_cbs_.begin();
  if (next == time_cbs_.end())
    return ms;
  int64_t now = now_ms();
  if (now >= next->first)
    return 0;
  int64_t wait = next->first - now;
  if (wait > std::numeric_limits<int>::max())
    wait = std::numeric_limits<int>::max();
  if (ms >= 0 && ms <= wait)
    return ms;
  return wait;
}

void
pollset::poll(int timeout)
{
  int r = ::poll(pollfds_.data(), pollfds_.size(), next_timeout(timeout));
  if (r < 0) {
    if (errno == EINTR)
      return;
    std::cerr << "poll: " << sock_errmsg() << std::endl;
    std::terminate();
  }
  size_t maxpoll = pollfds_.size();
  for (size_t i = 0; r > 0 && i < maxpoll; i++) {
    pollfd *pfp = &pollfds_.at(i);
    fd_state &fi = state_.at(sock_t(pfp->fd)); // XXX
    assert (!(pfp->revents & POLLNVAL));
    if (pfp->revents)
      --r;
    if (pfp->revents & (POLLIN|POLLHUP|POLLERR) && fi.rcb) {
      if (fi.roneshot) {
	cb_t cb {std::move(fi.rcb)};
	fi.rcb = nullptr;
	pfp->events &= ~POLLIN;
	cb();
      }
      else
	fi.rcb();
    }
    pfp = &pollfds_.at(i); // callback might have resized vector
    if (pfp->revents & (POLLOUT|POLLHUP|POLLERR) && fi.wcb) {
      if (fi.woneshot) {
	cb_t cb {std::move(fi.wcb)};
	fi.wcb = nullptr;
	pfp->events &= ~POLLOUT;
	cb();
      }
      else
	fi.wcb();
    }
  }

  run_timeouts();
  run_subtype_handlers();
  consolidate();
}

void
pollset::run_timeouts()
{
  auto i = time_cbs_.begin();
  if (i != time_cbs_.end()) {
    int64_t now = now_ms();
    while (i != time_cbs_.end() && now >= i->first) {
      struct cleanup {
	cb_t cb_;
	~cleanup() { cb_(); }
      } c {[&]() { time_cbs_.erase(i++); }};
      i->second();
    }
  }
}

void
pollset_plus::run_subtype_handlers()
{
  if (!signal_pending_)
    return;

  // Slightly convoluted logic because A) a callback might try to
  // change signal callbacks (so we have to release signal_owners_lock
  // for the duration of the callback), B) callbacks can throw
  // exceptions (and we don't want to lose other signals just because
  // one callback throws an exception), and C) other threads could
  // steal our signal callbacks (invalidating iterators to
  // signal_cbs_) whenever we release signal_owners_lock.
  std::vector<int> pending;
  std::unique_lock<std::mutex> lk {signal_owners_lock};
  for (auto i : signal_cbs_)
    if (signal_flags[i.first])
      pending.push_back(i.first);

  for (auto i : pending) {
    auto cbi = signal_cbs_.find(i);
    if (cbi == signal_cbs_.end())
      continue;
    while(signal_flags[i] & 1)
      std::this_thread::yield();
    signal_flags[i] = 0;
    cb_t cb {cbi->second};
    lk.unlock();
    cb();
    lk.lock();
  }
  signal_pending_ = false;
}

void
pollset::consolidate()
{
  while (!pollfds_.empty() && !pollfds_.back().events) {
    auto fi = state_.find(sock_t(pollfds_.back().fd)); // XXX
    if (fi != state_.end())
      state_.erase(fi);
    pollfds_.pop_back();
  }

  int i = pollfds_.size();
  if (i < 2)
    return;
  for (i -= 2; i >= 0; --i) {
    {
      pollfd &pfd1 = pollfds_.at(i);
      if (pfd1.events)
	continue;
      auto fi = state_.find(sock_t(pfd1.fd)); // XXX
      if (fi != state_.end())
	state_.erase(fi);
    }
    const pollfd &pfd = pollfds_[i] = pollfds_.back();
    state_.at(sock_t(pfd.fd)).idx = i; // XXX
    pollfds_.pop_back();
  }
}

void
pollset_plus::signal_cb(int sig, cb_t cb)
{
  if (!cb) {
    signal_cb(sig);
    return;
  }
  assert(sig > 0 && sig < num_sig);

  std::lock_guard<std::mutex> lk {signal_owners_lock};
  signal_cbs_[sig] = std::move(cb);
  if (signal_owners[sig] == this)
    return;
  if (pollset_plus *ps = signal_owners[sig]) {
    signal_owners[sig] = this;
    ps->signal_cbs_.erase(sig);
  }
  else {
    signal_owners[sig] = this;
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(sig, &sa, nullptr) == -1) {
      signal_owners[sig] = nullptr;
      throw std::system_error(errno, std::system_category(), "sigaction");
    }
  }
  std::atomic_thread_fence(std::memory_order_seq_cst);
  if (signal_flags[sig])
    wake(wake_type::Signal);
}

// Assumes signal_owners_lock already held when called.
void
pollset_plus::erase_signal_cb(int sig)
{
  pollset_plus *ps = signal_owners[sig];
  if (!ps)
    return;

  struct sigaction sa;
  sa.sa_handler = SIG_DFL;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  if (sigaction(sig, &sa, nullptr) == -1)
    throw std::system_error(errno, std::system_category(), "sigaction");

  signal_owners[sig] = nullptr;
  std::atomic_thread_fence(std::memory_order_seq_cst);
  ps->signal_cbs_.erase(sig);

  while(signal_flags[sig] & 1)
    std::this_thread::yield();

  if (signal_flags[sig]) {
    signal_flags[sig] = 0;
    std::raise(sig);
  }
}

void
pollset_plus::signal_cb(int sig, std::nullptr_t)
{
  assert(sig > 0 && sig < num_sig);
  std::lock_guard<std::mutex> lk {signal_owners_lock};
  erase_signal_cb(sig);
}

std::int64_t
pollset::now_ms()
{
  using namespace std::chrono;
  return duration_cast<milliseconds>(steady_clock::now().time_since_epoch())
    .count();
}

void
pollset::timeout_cancel(Timeout &t)
{
  if (t) {
    assert(time_cbs_.find(t.i_->first) != time_cbs_.end());
    time_cbs_.erase(t.i_);
    t = timeout_null();
  }
}

void
pollset::timeout_reschedule_at(Timeout &t, std::int64_t ms)
{
  auto i = t.i_;
  t.i_ = time_cbs_.emplace(ms, std::move(i->second));
  time_cbs_.erase(i);
}

}
