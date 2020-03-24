// -*- C++ -*-

#ifndef _XDRPP_POLLSET_H_INCLUDED_
#define _XDRPP_POLLSET_H_INCLUDED_ 1

/** \file pollset.h Asynchronous I/O and event harness. */

#include <csignal>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include <poll.h>
#include <xdrpp/socket.h>

namespace xdr {

//! Structure to poll for a set of file descriptors and timeouts.
class pollset {
protected:
  static constexpr int kReadFlag = 0x1;
  static constexpr int kWriteFlag = 0x2;
  static constexpr int kOnceFlag = 0x4;
  //! Number of registered file decriptor and timeout callbacks.
  std::size_t num_cbs() const;

public:
  enum op_t {
    //! Specify interest in read-ready condition
    Read = kReadFlag,
    //! Specify interest in write-ready condition
    Write = kWriteFlag,		
    //! Valid only when removing callbacks.
    ReadWrite = kReadFlag | kWriteFlag,
    //! Like \c Read, but the callback only executes once.
    ReadOnce = kReadFlag | kOnceFlag,
    //! Like \c Write, but the callback only executes once.
    WriteOnce = kWriteFlag | kOnceFlag
  };

  using cb_t = std::function<void()>;

private:
  // File descriptor callback information
  struct fd_state {
    cb_t rcb;
    cb_t wcb;
    int idx {-1};		// Index in pollfds_
    bool roneshot;
    bool woneshot;
    ~fd_state();		// Sanity check no active callbacks
  };

  // File descriptor callback state
  std::vector<pollfd> pollfds_;
  std::unordered_map<sock_t, fd_state> state_;

  // Timeout callback state
  std::multimap<std::int64_t, cb_t> time_cbs_;

  cb_t &fd_cb_helper(sock_t s, op_t op);
  void consolidate();
  int next_timeout(int ms);
  void run_timeouts();

  // Hook for subtypes
  virtual void run_subtype_handlers() {}

public:
  pollset() = default;
  pollset(const pollset &) = delete;

  //! Go through one round of checking all file descriptors.  \arg \c
  //! timeout is a timeout in milliseconds (or -1 to wait forever).
  //! 
  //! Typical usage:  \code
  //!   PollSet ps;
  //!   // ... register some callbacks or asynchronous events ...
  //!   while(ps.pending())
  //!     ps.poll();
  //! \endcode
  void poll(int timeout = -1);

  //! Returns \c false if no file descriptor callbacks are registered
  //! and no timeouts or asynchronous events are pending.  If it
  //! returns \c false, then PollSet::poll will pause forever in the
  //! absence of a signal or a call to PollSet::inject_cb in a
  //! different thread.
  virtual bool pending() const { return num_cbs(); }

  //! Continously poll and only return on exception or when there is
  //! no more work to do.
  void run() { while (pending()) poll(); }

  //! Set a read or write callback on a particular file descriptor.
  //! \arg \c fd is the file descriptor.  \arg \c op specifies the
  //! condition on which to invoke the callback.  Only one \c Read and
  //! one \c Write callback are permitted per file descriptor.  E.g.,
  //! calling \c set_cb with \c ReadOnce overwrites a previous \c Read
  //! callback on the same file descriptor.  The value \c ReadWrite is
  //! illegal when adding a callback (you must set \c Read and \c
  //! Write callbacks separately).  \arg \c cb is the callback, which
  //! must be convertible to PollSet::cb_t.
  template<typename CB> void fd_cb(sock_t s, op_t op, CB &&cb) {
    if (!(fd_cb_helper(s, op) = std::forward<CB>(cb)))
      fd_cb(s, op);
  }

  //! Remove a callback on a file descriptor.  If \c op is \c
  //! ReadWrite, removes both read and write callbacks on the
  //! descriptor.
  void fd_cb(sock_t s, op_t op, std::nullptr_t = nullptr);

  //! Number of milliseconds since an arbitrary but fixed time, used
  //! as the basis of all timeouts.  Time zero is
  //! std::chrono::steady_clock's epoch, which in some implementations
  //! is the time a machine was booted.
  static std::int64_t now_ms();

  //! Abstract class used to represent a pending timeout.
  class Timeout {
    using iterator = decltype(time_cbs_)::iterator;
    iterator i_;
    explicit Timeout(iterator i) : i_(i) {}
    Timeout &operator=(iterator i) { i_ = i; return *this; }
    friend class pollset;
  public:
    //! A null timeout.  Relies on static initalization.
    static const Timeout null_;
    //! After static initialization, Timeouts are null by default.
    //! However, since \c null_ relies on static initialization,
    //! static/global Timeouts are not guaranteed to be null and
    //! should be explicitly initialized from \c
    //! pollset::timeout_null().
    Timeout() : i_(null_.i_) {}
    explicit operator bool() const { return i_ != null_.i_; }
  };

  //! Set a callback to run a certain number of milliseconds from now.
  //! \arg \c ms is the delay in milliseconds before running the
  //! callback.  \arg \c cb must be convertible to PollSet::cb_t.
  //! \returns an object on which you can call the method
  //! PollSet::timeout_cancel to cancel the timeout.
  template<typename CB> Timeout timeout(std::int64_t ms, CB &&cb) {
    return timeout_at(now_ms() + ms, std::forward<CB>(cb));
  }
  //! Set a callback to run at a specific time (as returned by
  //! PollSet::now_ms()).
  template<typename CB> Timeout timeout_at(std::int64_t ms, CB &&cb) {
    return Timeout(time_cbs_.emplace(ms, std::forward<CB>(cb)));
  }

  //! An invalid timeout, useful for initializing PollSet::Timeout
  //! values before a timeout has been scheduled.
  static Timeout timeout_null();

  //! Cancel a pending timeout.  Sets the PollSet::Timeout argument \c
  //! t to PollSet::timeout_null(), but obviously does not not affect
  //! other copies of \c t that will now be invalid.
  void timeout_cancel(Timeout &t);

  //! Returns the absolute time (in milliseconds) at which a timeout
  //! will run.
  std::int64_t timeout_time(Timeout t) const { return t.i_->first; }

  //! Reschedule a timeout to run at a specific time.  Updates the
  //! argument \c t, but invalidates any other copies of \c t.
  void timeout_reschedule_at(Timeout &t, std::int64_t ms);
  //! Reschedule a timeout some number of milliseconds in the future.
  void timeout_reschedule(Timeout &t, std::int64_t ms) {
    timeout_reschedule_at(t, now_ms() + ms);
  }
};

//! Adds support for signal handlers, asynchonous events, and
//! callbacks injected from other threads to the basic functionality
//! in \c pollset_light.
class pollset_plus : public pollset {
  enum class wake_type : std::uint8_t {
    Normal = 0,
    Signal = 1
  };
  
  // State for asynchronous tasks
  template<typename R> struct async_task {
    pollset_plus *ps_;
    std::function<R()> work_;
    std::function<void(R)> cb_;
    std::unique_ptr<R> rp_;

    void start() {
      rp_.reset(new R { work_() });
      ps_->inject_cb(std::bind(&async_task::done, this));
    }
    void done() {
      std::unique_ptr<async_task> self {this};
      ps_->nasync_--;
      cb_(std::move(*rp_));
    }
  };

  // Self-pipe used to wake up poll from signal handlers and other threads
  sock_t selfpipe_[2];

  // Asynchronous events enqueued from other threads
  std::mutex async_cbs_lock_;
  std::vector<cb_t> async_cbs_;
  bool async_pending_{false};
  size_t nasync_{0};

  // Signal callback state
  static constexpr int num_sig = 32;
  static std::mutex signal_owners_lock;
  static pollset_plus *signal_owners[num_sig];
  static volatile std::sig_atomic_t signal_flags[num_sig];
  bool signal_pending_{false};
  std::map<int, cb_t> signal_cbs_;

  void wake(wake_type wt);
  void run_pending_asyncs();
  void inject_cb_vec(std::vector<cb_t>::iterator b,
		     std::vector<cb_t>::iterator e);
  void run_subtype_handlers() override;
  static void signal_handler(int);
  static void erase_signal_cb(int);

public:
  pollset_plus();
  ~pollset_plus();

  bool pending() const override;

  //! Cause PollSet::poll to return if it is sleeping.  Unlike most
  //! other methods, \c wake is safe to call from a signal handler or
  //! a different thread.
  void wake() { wake(wake_type::Normal); }

  //! Inject a callback to run immediately.  Unlike most methods, it
  //! is safe to call this function from another thread.  Being
  //! thread-safe adds extra overhead, so it does not make sense to
  //! call this function from the same thread as PollSet::poll.  Note
  //! that \c inject_cb acquires a lock and definitely must <i>not</i>
  //! be called from a signal handler (or deadlock could ensue).
  template<typename CB> void inject_cb(CB &&cb) {
    std::lock_guard<std::mutex> lk(async_cbs_lock_);
    async_cbs_.emplace_back(std::forward<CB>(cb));
    if (!async_pending_) {
      async_pending_ = true;
      wake();
    }
  }

  //! Execute a task asynchonously in another thread, then run
  //! callback on the task's result in the main thread.  \arg \c work
  //! is a task to perform asynchronously in another thread, and must
  //! be convertible to std::function<R()> for some type \c R.  \arg
  //! \c cb is the callback that processes the result in the main
  //! thread, and must be convertible to std::function<void(R)> for
  //! the same type \c R.
  template<typename Work, typename CB> void async(Work &&work, CB &&cb) {
    using R = decltype(work());
    async_task<R> *a = new async_task<R> {
      this, std::forward<Work>(work), std::forward<CB>(cb), nullptr
    };
    ++nasync_;
    std::thread(&async_task<R>::start, a).detach();
  }

  //! Add a callback for a particular signal.  Note that only one
  //! callback can be added for a particular signal across all
  //! `pollset_plus` instances in a single process.  Hence, calling
  //! this function may "steal" a signal from a different
  //! `pollset_plus` (erasing whatever callback the other
  //! `pollset_plus` had for the signal).  Such callback stealing is
  //! atomic, allowing one to steal a `pollset_plus`'s signals before
  //! deleting it with no risk of signals going uncaught.
  void signal_cb(int sig, cb_t cb);

  //! Remove any previously added callback for a particular signal.
  //! Because signal callbacks are process-wide, this static method
  //! will affect whatever `pollset_plus` currently owns the signal.
  static void signal_cb(int sig, std::nullptr_t = nullptr);
};

}

#endif // !_XDRPP_POLLSET_H_INCLUDED_
