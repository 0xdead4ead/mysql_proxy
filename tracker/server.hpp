#ifndef MYSQLPROXY_TRACKER_SERVER_HPP
#define MYSQLPROXY_TRACKER_SERVER_HPP

#include <boost/asio/ip/tcp.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

#include "connection.hpp"
#include "system/logger_service.hpp"

namespace mysqlproxy_tracker
{
namespace server
{

extern boost::scoped_ptr<mysqlproxy_system::basic_logger> writer;

class listener : private boost::noncopyable
{
public:
  explicit listener (boost::asio::io_context &ioc);

  void run (const std::string &address, const std::string &port);

private:
  void start_accept ();
  void handle_accept (const boost::system::error_code &e);

  boost::asio::io_context &io_context_;
  boost::asio::ip::tcp::acceptor acceptor_;

  connection_ptr new_connection_;
};
} // namespace server
} // namespace mysqlproxy_tracker

#endif // MYSQLPROXY_TRACKER_SERVER_HPP