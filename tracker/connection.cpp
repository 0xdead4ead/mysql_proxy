#include "connection.hpp"

//#include <boost/asio/connect.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/bind/bind.hpp>
//#include <boost/asio/ip/basic_endpoint.hpp>

// temporarety!
const char *mysql_address = "127.0.0.1";
const uint16_t mysql_port = 3306;

namespace mysqlproxy_tracker
{
namespace server
{
connection::connection (boost::asio::io_context &io_context,
                        mysqlproxy_system::basic_logger &writer)
    : strand_ (boost::asio::make_strand (io_context)),
      server_socket_ (strand_), client_socket_ (strand_), writer_ (writer)
{
}

boost::asio::ip::tcp::socket &
connection::server_socket ()
{
  return server_socket_;
}

boost::asio::ip::tcp::socket &
connection::client_socket ()
{
  return client_socket_;
}

void
connection::start ()
{
  server_socket_.async_connect (
      boost::asio::ip::tcp::endpoint (
          boost::asio::ip::make_address (mysql_address), mysql_port),
      boost::bind (&connection::handle_connect, shared_from_this (),
                   boost::asio::placeholders::error));
}

void
connection::handle_connect (const boost::system::error_code &err)
{
  if (!err)
    {
      do_read (server_socket_);
    }
  else
    {
      CXXLOG_ERROR (writer_,
                    "handle_connect: " << err.message () << std::endl);
    }
}

void
connection::do_read (boost::asio::ip::tcp::socket &sock)
{
  buffer &buf = &sock == &server_socket_ ? server_buffer_ : client_buffer_;

  boost::asio::async_read (
      sock, boost::asio::buffer (buf.header_),
      boost::bind (&connection::on_read_header, shared_from_this (),
                   boost::ref (sock), boost::asio::placeholders::error,
                   boost::asio::placeholders::bytes_transferred));
}

void
connection::do_write (boost::asio::ip::tcp::socket &sock)
{
  std::vector<boost::asio::const_buffer> buffers;

  for (std::size_t i = 0; i < for_write_.size (); i++)
    {
      buffers.push_back (boost::asio::buffer (for_write_[i].header_));
      buffers.push_back (boost::asio::buffer (for_write_[i].data_));
    }

  boost::asio::async_write (
      sock, buffers,
      boost::bind (&connection::on_write, shared_from_this (),
                   boost::ref (sock), boost::asio::placeholders::error,
                   boost::asio::placeholders::bytes_transferred));
}

void
connection::on_read_header (boost::asio::ip::tcp::socket &sock,
                            const boost::system::error_code &err,
                            std::size_t /*bytes_transferred*/)
{
  if (err)
    {
      CXXLOG_ERROR (writer_,
                    "on_read_header: " << err.message () << std::endl);
      return;
    }

  buffer &buf = &sock == &server_socket_ ? server_buffer_ : client_buffer_;

  mysqlproxy_common::protocol::prefix *header
      = reinterpret_cast<mysqlproxy_common::protocol::prefix *> (
          buf.header_.data ());

  buf.data_.resize (header->payload_length);

  boost::asio::async_read (
      sock, boost::asio::buffer (buf.data_),
      boost::bind (&connection::on_read_data, shared_from_this (),
                   boost::ref (sock), boost::asio::placeholders::error,
                   boost::asio::placeholders::bytes_transferred));
}

void
connection::on_read_data (boost::asio::ip::tcp::socket &sock,
                          const boost::system::error_code &err,
                          std::size_t bytes_transferred)
{
  if (err)
    {
      CXXLOG_ERROR (writer_, "on_read_data: " << err.message () << std::endl);
      return;
    }

  buffer &buf = &sock == &server_socket_ ? server_buffer_ : client_buffer_;

  mysqlproxy_common::protocol::command *com
      = reinterpret_cast<mysqlproxy_common::protocol::command *> (
          buf.data_.data ());

  if (&buf == &client_buffer_)
    {
      if (com->value == MYSQLPROXY_PROTOCOL_COM_QUERY)
        {
          mysqlproxy_common::protocol::query *query
              = reinterpret_cast<mysqlproxy_common::protocol::query *> (
                  com->data);

          CXXLOG_INFO (writer_, "on_read_data: "
                                    << "COM_QUERY \"" << query->text << "\""
                                    << std::endl);
        }

      client_command_ = com->value;
    }
  else
    {
      mysqlproxy_common::protocol::command *com_cl
          = reinterpret_cast<mysqlproxy_common::protocol::command *> (
              client_buffer_.data_.data ());

      if (com->value != MYSQLPROXY_PROTOCOL_OK_PACKET
          && com->value != MYSQLPROXY_PROTOCOL_ERR_PACKET
          && com->value != MYSQLPROXY_PROTOCOL_EOF_PACKET
          && client_command_ == MYSQLPROXY_PROTOCOL_COM_QUERY)
        {
          for_write_.push_back (buf);

          do_read (server_socket_);
          return;
        }
    }

  for_write_.push_back (buf);
  do_write (&sock == &server_socket_ ? client_socket_ : server_socket_);
}

void
connection::on_write (boost::asio::ip::tcp::socket &sock,
                      const boost::system::error_code &err,
                      std::size_t /*bytes_transferred*/)
{
  if (err)
    {
      CXXLOG_ERROR (writer_, "on_read_data: " << err.message () << std::endl);
      return;
    }

  for_write_.clear ();

  do_read (sock);
}
} // namespace server
} // namespace mysqlproxy_tracker