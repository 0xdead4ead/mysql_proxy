#define MYSQLPROXY_SYSTEM_SOURCE
#define BOOST_ASIO_SOURCE

#include "logger_service.hpp"

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/date_time.hpp>
#include <boost/system/system_error.hpp>
#include <boost/thread.hpp>
#include <boost/throw_exception.hpp>

#include <iostream>
#include <locale>
#include <sstream>

namespace mysqlproxy_system
{
namespace detail
{
class logger_service_impl
{
  friend class mysqlproxy_system::detail::logger_service;

  logger_service_impl ()
      : work_io_context_ (),
        work_ (boost::asio::make_work_guard (work_io_context_)),
        work_thread_ (new boost::thread (
            boost::bind (&boost::asio::io_context::run, &work_io_context_))),
        start_time_ (boost::posix_time::microsec_clock::local_time ()),
        loc (std::locale::classic (),
             new ldt_facet (ldt_facet::iso_time_format_extended_specifier))
  {
  }

  /// Private io_context used for performing logging operations.
  boost::asio::io_context work_io_context_;

  /// Work for the private io_context to perform. If we do not give the
  /// io_context some work to do then the io_context::run() function will exit
  /// immediately.
  boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
      work_;

  /// Thread used for running the work io_context's run loop.
  boost::scoped_ptr<boost::thread> work_thread_;

  /// Saved time to starting service
  boost::posix_time::ptime start_time_;

  /// Boost.date_time facet
  typedef boost::date_time::time_facet<boost::local_time::local_date_time,
                                       char>
      ldt_facet;
  std::locale loc;

  /// The file to which log messages will be written.
  std::ofstream ofstream_;
};

void
destructor (logger_service_impl *x) BOOST_NOEXCEPT
{
  delete x;
}

std::allocator<logger_service::logger_impl> logger_service::memory_
    = std::allocator<logger_service::logger_impl> ();

logger_service::logger_service (boost::asio::execution_context &context)
    : boost::asio::execution_context::service (context),
      private_ (new detail::logger_service_impl ())
{
}

/// Destructor shuts down the private io_context.
logger_service::~logger_service ()
{
  /// Indicate that we have finished with the private io_context. Its
  /// io_context::run() function will exit once all other work has completed.
  private_->work_.reset ();
  if (private_->work_thread_)
    private_->work_thread_->join ();
}

void
logger_service::shutdown ()
{
  // ...
}

void
logger_service::create (impl_type &impl, const std::string &identifier)
{
  logger_impl *new_ = memory_.allocate (1);
  memory_.construct (new_, identifier);
  impl = new_;
}

void
logger_service::destroy (impl_type &impl)
{
  memory_.destroy (impl);
  memory_.deallocate (impl, 1);
  impl = null ();
}

void
logger_service::use_file (impl_type & /*impl*/, const std::string &file)
{
  // Pass the work of opening the file to the background thread.
  boost::asio::post (private_->work_io_context_,
                     boost::bind (&logger_service::use_file_impl, this, file));
}

void
logger_service::log (impl_type &impl, const std::string &message,
                     severity_level lv)
{
  using namespace boost::system::errc;

  const char *level = 0;
  switch (lv)
    {
    case LEVEL_DEBUG:
      level = "DEBUG";
      break;
    case LEVEL_INFO:
      level = "INFO";
      break;
    case LEVEL_WARNING:
      level = "WARNING";
      break;
    case LEVEL_ERROR:
      level = "ERROR";
      break;
    case LEVEL_CRITICAL:
      level = "CRITICAL";
      break;
    default:
      boost::throw_exception (
          boost::system::system_error (make_error_code (invalid_argument),
                                       "mysqlproxy_system.logger_service"));
    }

  // Format the text to be logged.
  std::ostringstream os;
  os.imbue (private_->loc); // set date/time format

  boost::posix_time::ptime pt
      = boost::posix_time::microsec_clock::local_time ();
  os << '[' << pt << ']'; // current local time
  os << '[' << pt - private_->start_time_ << ']';
  os << '[' << level << ']';      // severity level
  if (!impl->identifier.empty ()) // identifier
    os << '[' << impl->identifier << ']' << '\t';
  else
    os << '\t';

  os << message; // message

  // Pass the work of writing to the file to the background thread.
  boost::asio::post (private_->work_io_context_,
                     boost::bind (&logger_service::log_impl, this,
                                  boost::ref (impl), os.str ()));
}

void
logger_service::cout (impl_type &impl, const std::string &message)
{
  // Pass the work of writing to the file to the background thread.
  boost::asio::post (private_->work_io_context_,
                     boost::bind (&logger_service::cout_impl, this,
                                  boost::ref (impl), message));
}

void
logger_service::cerr (impl_type &impl, const std::string &message)
{
  // Pass the work of writing to the file to the background thread.
  boost::asio::post (private_->work_io_context_,
                     boost::bind (&logger_service::cerr_impl, this,
                                  boost::ref (impl), message));
}

bool
logger_service::is_open_file (impl_type & /*impl*/)
{
  return private_->ofstream_.is_open ();
}

boost::asio::io_context &
logger_service::get_io_context ()
{
  return private_->work_io_context_;
}

void
logger_service::use_file_impl (const std::string &file)
{
  if (private_->ofstream_.is_open ())
    return;

  // private_->ofstream_.rdbuf()->pubsetbuf(0, 0); // unbuffered

  private_->ofstream_.close ();
  private_->ofstream_.clear ();

  private_->ofstream_.open (file.c_str ()); // for writing only

  if (!private_->ofstream_)
    boost::throw_exception (
        std::runtime_error ("mysqlproxy_service: Failed to open log file!"));
}

void
logger_service::log_impl (impl_type & /*impl*/, const std::string &text)
{
  if (private_->ofstream_.is_open ())
    private_->ofstream_ << text << std::endl /*flush()*/;
  else
    std::cout << text << std::endl /*flush()*/;
}

void
logger_service::cout_impl (impl_type & /*impl*/, const std::string &text)
{
  std::cout << text;
  std::cout.flush ();
}

void
logger_service::cerr_impl (impl_type & /*impl*/, const std::string &text)
{
  std::cerr << text;
  std::cerr.flush (); // stderr is always unbuffered (ISO C)
}

} // namespace detail
} // namespace mysqlproxy_system