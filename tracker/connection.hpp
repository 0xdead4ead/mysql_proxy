#ifndef MYSQLPROXY_TRACKER_CONNECTION_HPP
#define MYSQLPROXY_TRACKER_CONNECTION_HPP

#include <boost/array.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <vector>

#include "common/mysql.hpp"
#include "system/logger_service.hpp"

namespace mysqlproxy_tracker
{
namespace server
{

class connection : public boost::enable_shared_from_this<connection>,
                   private boost::noncopyable
{
public:
  explicit connection (boost::asio::io_context &io_context,
                       mysqlproxy_system::basic_logger &writer);

  boost::asio::ip::tcp::socket &server_socket ();
  boost::asio::ip::tcp::socket &client_socket ();

  void start ();

private:
  static const std::size_t header_lenght
      = sizeof (mysqlproxy_common::protocol::prefix);

  struct buffer
  {
    boost::array<uint8_t, header_lenght> header_;
    std::vector<uint8_t> data_;
  };

  void handle_connect (const boost::system::error_code &err);

  void do_read (boost::asio::ip::tcp::socket &sock);
  void do_write (boost::asio::ip::tcp::socket &sock);
  void on_read_header (boost::asio::ip::tcp::socket &sock,
                       const boost::system::error_code &err,
                       std::size_t bytes_transferred);
  void on_read_data (boost::asio::ip::tcp::socket &sock,
                     const boost::system::error_code &err,
                     std::size_t bytes_transferred);
  void on_write (boost::asio::ip::tcp::socket &sock,
                 const boost::system::error_code &err,
                 std::size_t bytes_transferred);

  boost::asio::strand<boost::asio::io_context::executor_type> strand_;

  boost::asio::ip::tcp::socket server_socket_;
  boost::asio::ip::tcp::socket client_socket_;

  mysqlproxy_system::basic_logger &writer_;

  buffer server_buffer_;
  buffer client_buffer_;
  std::vector<buffer> for_write_;
  uint8_t client_command_;
};

typedef boost::shared_ptr<connection> connection_ptr;

} // namespace server
} // namespace mysqlproxy_tracker

#endif // MYSQLPROXY_TRACKER_CONNECTION_HPP