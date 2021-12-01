#include "processor.hpp"

#include <boost/thread/thread.hpp>

namespace mysqlproxy_common
{
uint32_t pool_size = 0;

// Thread function
void (*thread_func) (std::size_t) = 0;

// Signal handler
void (*signal_handler) (boost::system::error_code const &, int) = 0;

boost::once_flag processor::once = BOOST_ONCE_INIT;

void
processor::exec ()
{
  BOOST_ASSERT (thread_func);
  BOOST_ASSERT (signal_handler);

  instance ().sig_set_.async_wait (signal_handler);

  boost::thread_group threads;

  for (std::size_t i = 0; i < pool_size; ++i)
    threads.create_thread (boost::bind (thread_func, i));

  threads.join_all ();
}

processor *processor::p_instance_ = 0;
bool processor::destroyed_ = false;

} // namespace mysqlproxy_common