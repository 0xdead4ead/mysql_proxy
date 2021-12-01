#ifndef MYSQLPROXY_SYSTEM_LOGGER_SERVICE_HPP
#define MYSQLPROXY_SYSTEM_LOGGER_SERVICE_HPP

#include "config.hpp"

#include <boost/asio/execution_context.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/ref.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/type_traits/decay.hpp>

#include <fstream>
#include <sstream> // for CXXLOG_

// _ReturnAddress (MSVC)
#if defined(_MSC_VER)
#  include <intrin.h>
#endif // defined(_MSC_VER)

namespace mysqlproxy_system
{

namespace detail
{
class logger_service_impl;
MYSQLPROXY_SYSTEM_API void destructor (logger_service_impl *) BOOST_NOEXCEPT;
} // namespace detail

} // namespace mysqlproxy_system

namespace boost
{

// boost::scoped_ptr
template <>
inline void
checked_delete (mysqlproxy_system::detail::logger_service_impl *x)
    BOOST_NOEXCEPT
{
  destructor (x);
}

} // namespace boost

namespace mysqlproxy_system
{
namespace detail
{
class logger_service : public boost::asio::execution_context::service
{
public:
  struct logger_impl;

private:
  static std::allocator<logger_impl> memory_;

public:
  enum severity_level
  {
    LEVEL_DEBUG,
    LEVEL_INFO,
    LEVEL_WARNING,
    LEVEL_ERROR,
    LEVEL_CRITICAL
  };

  typedef std::allocator<logger_impl>::pointer impl_type;

  struct logger_impl
  {
    explicit logger_impl (const std::string &ident) : identifier (ident) {}

    std::string identifier;
  };

  typedef logger_service key_type;

  /// Constructor creates a thread to run a private io_context.
  MYSQLPROXY_SYSTEM_API
  logger_service (boost::asio::execution_context &context);

  MYSQLPROXY_SYSTEM_API ~logger_service ();

  MYSQLPROXY_SYSTEM_API /*virtual*/ void shutdown ();

  impl_type
  null () const
  {
    return 0;
  }

  /// Create a new logger implementation.
  MYSQLPROXY_SYSTEM_API void create (impl_type &impl,
                                     const std::string &identifier);

  /// Destroy a logger implementation.
  MYSQLPROXY_SYSTEM_API void destroy (impl_type &impl);

  /// Set the output file for the logger.
  MYSQLPROXY_SYSTEM_API void use_file (impl_type & /*impl*/,
                                       const std::string &file);

  /// Log a message.
  MYSQLPROXY_SYSTEM_API void log (impl_type &impl, const std::string &message,
                                  severity_level lv);

  /// Output a message to stdout.
  MYSQLPROXY_SYSTEM_API void cout (impl_type &impl,
                                   const std::string &message);

  /// Output a message to stderr.
  MYSQLPROXY_SYSTEM_API void cerr (impl_type &impl,
                                   const std::string &message);

  /// Return true if log file is opened
  MYSQLPROXY_SYSTEM_API bool is_open_file (impl_type &impl);

  MYSQLPROXY_SYSTEM_API boost::asio::io_context &get_io_context ();

private:
  /// Helper function used to open the output file from within the private
  /// io_context's thread.
  void use_file_impl (const std::string &file);

  /// Helper function used to log a message from within the private
  /// io_context's thread.
  void log_impl (impl_type &impl, const std::string &text);
  void cout_impl (impl_type &impl, const std::string &text);
  void cerr_impl (impl_type &impl, const std::string &text);

  boost::scoped_ptr<detail::logger_service_impl> private_;
};

template <typename Logger>
void
__do_log (Logger &l, const std::string &message,
          logger_service::severity_level lv)
{
  typedef typename boost::decay<Logger>::type logger_type;
  boost::bind (&logger_type::log, boost::ref (l), _1, _2) (message, lv);
}

template <typename Logger>
void
__do_cout (Logger &l, const std::string &message)
{
  typedef typename boost::decay<Logger>::type logger_type;
  boost::bind (&logger_type::cout, boost::ref (l), _1) (message);
}

template <typename Logger>
void
__do_cerr (Logger &l, const std::string &message)
{
  typedef typename boost::decay<Logger>::type logger_type;
  boost::bind (&logger_type::cerr, boost::ref (l), _1) (message);
}

template <typename Logger>
void
__do_log (boost::scoped_ptr<Logger> &l, const std::string &message,
          logger_service::severity_level lv)
{
  typedef Logger logger_type;
  boost::bind (&logger_type::log, boost::ref (l), _1, _2) (message, lv);
}

template <typename Logger>
void
__do_cout (boost::scoped_ptr<Logger> &l, const std::string &message)
{
  typedef Logger logger_type;
  boost::bind (&logger_type::cout, boost::ref (l), _1) (message);
}

template <typename Logger>
void
__do_cerr (boost::scoped_ptr<Logger> &l, const std::string &message)
{
  typedef Logger logger_type;
  boost::bind (&logger_type::cerr, boost::ref (l), _1) (message);
}

} // namespace detail

/// Provides thread-safe output to file descriptor
class basic_logger : public boost::noncopyable
{
public:
  typedef detail::logger_service service_type;

  /// The native implementation type of the timer.
  typedef service_type::impl_type impl_type;

  /// Constructor.
  /**
   * This constructor creates a logger.
   *
   * @param context The execution context used to locate the logger service.
   *
   * @param identifier An identifier for this logger.
   */
  explicit basic_logger (boost::asio::execution_context &context,
                         const std::string &identifier)
      : service_ (boost::asio::use_service<service_type> (context)),
        impl_ (service_.null ())
  {
    service_.create (impl_, identifier);
  }

  /// Destructor.
  ~basic_logger () { service_.destroy (impl_); }

  /// Get the io_context associated with the object.
  boost::asio::io_context &
  get_io_context ()
  {
    return service_.get_io_context ();
  }

  /// Set the output file.
  void
  use_file (const std::string &file)
  {
    service_.use_file (impl_, file);
  }

  /// Log a message.
  /// Note: if use_file is not call above, send message to stdout
  void
  log (const std::string &message, service_type::severity_level lv)
  {
    service_.log (impl_, message, lv);
  }

  /// Output a message to stdout.
  void
  cout (const std::string &message)
  {
    service_.cout (impl_, message);
  }

  /// Output a message to stderr.
  void
  cerr (const std::string &message)
  {
    service_.cerr (impl_, message);
  }

  bool
  is_open_file ()
  {
    return service_.is_open_file (impl_);
  }

private:
  /// The backend service implementation.
  service_type &service_;

  /// The underlying native implementation.
  impl_type impl_;
};

} // namespace mysqlproxy_system

#if !defined(_MSC_VER)
#  define CXXLOG_(x, str_1, lv)                                               \
    {                                                                         \
      std::ostringstream os;                                                  \
      os << '(' << __FUNCTION__ << '@' << '0' << 'x' << std::hex              \
         << reinterpret_cast<uintptr_t> (__builtin_return_address (0)) << ')' \
         << '\t' << str_1;                                                    \
      mysqlproxy_system::detail::__do_log (x, os.str (), lv);                 \
    }
#else // !defined(_MSC_VER)
#  define CXXLOG_(x, str_1, lv)                                               \
    {                                                                         \
      std::ostringstream os;                                                  \
      os << '(' << __FUNCTION__ << '@' << '0' << 'x' << std::hex              \
         << reinterpret_cast<uintptr_t> (_ReturnAddress ()) << ')' << '\t'    \
         << str_1;                                                            \
      mysqlproxy_system::detail::__do_log (x, os.str (), lv);                 \
    }
#endif // !defined(_MSC_VER)

// \r\n added after each string args
// Helper macro-definitions for i/o output
#define CXXLOG_INFO(x, str)                                                   \
  CXXLOG_ (x, str, mysqlproxy_system::detail::logger_service::LEVEL_INFO)

#define CXXLOG_DEBUG(x, str)                                                  \
  CXXLOG_ (x, str, mysqlproxy_system::detail::logger_service::LEVEL_DEBUG)

#define CXXLOG_WARNING(x, str)                                                \
  CXXLOG_ (x, str, mysqlproxy_system::detail::logger_service::LEVEL_WARNING)

#define CXXLOG_ERROR(x, str)                                                  \
  CXXLOG_ (x, str, mysqlproxy_system::detail::logger_service::LEVEL_ERROR)

#define CXXLOG_CRITICAL(x, str)                                               \
  CXXLOG_ (x, str, mysqlproxy_system::detail::logger_service::LEVEL_CRITICAL)

#define CXXCOUT(x, str_1)                                                     \
  {                                                                           \
    std::ostringstream os;                                                    \
    os << str_1;                                                              \
    mysqlproxy_system::detail::__do_cout (x, os.str ());                      \
  }
#define CXXCERR(x, str_1)                                                     \
  {                                                                           \
    std::ostringstream os;                                                    \
    os << str_1;                                                              \
    mysqlproxy_system::detail::__do_cerr (x, os.str ());                      \
  }

#endif // MYSQLPROXY_SYSTEM_LOGGER_SERVICE_HPP