#ifndef MYSQLPROXY_COMMON_PROCESSOR_HPP
#define MYSQLPROXY_COMMON_PROCESSOR_HPP

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/once.hpp>

namespace mysqlproxy_common
{

// Number threads for main loop
extern uint32_t pool_size;

// Thread function
extern void (*thread_func) (std::size_t);

// Signal handler
extern void (*signal_handler) (boost::system::error_code const &, int);

class processor : public boost::noncopyable
{
  static void
  init ()
  {
    static processor instance_;
    p_instance_ = &instance_;
  }

  static boost::once_flag once;

public:
  static const uint32_t exception_code_dead = 0x0000dead;

  // Concurrent , but non-reentrant code
  static processor &
  instance ()
  {
    if (!p_instance_)
      {
        // Check for dead reference
        if (destroyed_)
          {
            on_dead_reference ();
          }
        else
          {
            // First call—initialize
            boost::call_once (&init, once);
          }
      }
    return *p_instance_;
  }

  boost::asio::io_context &
  io_context ()
  {
    return io_context_;
  }

  // Begin to execution app
  static void exec ();

private:
  processor () : io_context_ (), sig_set_ (io_context_, SIGINT, SIGTERM) {}

  BOOST_NORETURN BOOST_NOINLINE static void
  on_dead_reference ()
  {
    throw exception_code_dead;
  }

  ~processor ()
  {
    BOOST_ASSERT (!destroyed_);
    p_instance_ = 0;
    destroyed_ = true;
  }

  static processor *p_instance_;
  static bool destroyed_;

  boost::asio::io_context io_context_;
  boost::asio::signal_set sig_set_;
};

} // namespace mysqlproxy_common

#endif // MYSQLPROXY_COMMON_PROCESSOR_HPP