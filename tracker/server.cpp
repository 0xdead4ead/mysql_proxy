#include "server.hpp"

#include <boost/asio/placeholders.hpp>
#include <boost/bind/bind.hpp>

namespace mysqlproxy_tracker
{
namespace server
{

boost::scoped_ptr<mysqlproxy_system::basic_logger> writer;

listener::listener (boost::asio::io_context &ioc)
    : io_context_ (ioc), acceptor_ (io_context_), new_connection_ ()
{
}

void
listener::run (const std::string &address, const std::string &port)
{
  boost::asio::ip::tcp::resolver resolver (io_context_);
  boost::asio::ip::tcp::endpoint endpoint
      = *resolver.resolve (address, port).begin ();
  acceptor_.open (endpoint.protocol ());
  acceptor_.set_option (boost::asio::ip::tcp::acceptor::reuse_address (true));
  acceptor_.bind (endpoint);
  acceptor_.listen ();

  writer.reset (
      new mysqlproxy_system::basic_logger (io_context_, "Connection"));

  start_accept ();
}

void
listener::start_accept ()
{
  new_connection_.reset (new connection (io_context_, *writer));
  acceptor_.async_accept (new_connection_->client_socket (),
                          boost::bind (&listener::handle_accept, this,
                                       boost::asio::placeholders::error));
}

void
listener::handle_accept (const boost::system::error_code &e)
{
  if (!e)
    {
      new_connection_->start ();
    }

  start_accept ();
}

} // namespace server
} // namespace mysqlproxy_tracker