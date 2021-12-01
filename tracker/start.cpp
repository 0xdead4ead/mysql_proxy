#include <boost/program_options.hpp>
#include <boost/thread.hpp>
#include <iostream>

#include "common/processor.hpp"
#include "server.hpp"

namespace mysqlproxy_tracker
{

std::string address, port;
std::string output_file;

} // namespace mysqlproxy_tracker

static void
run__thread_app (std::size_t index)
{
  mysqlproxy_common::processor::instance ().io_context ().run ();
}

static void
__signal_handler (boost::system::error_code const &ec, int sig)
{
  mysqlproxy_common::processor::instance ().io_context ().stop ();
}

int
main (int argc, char *argv[], char *envp[])
{
  boost::program_options::options_description desc ("All options");

  desc.add_options () (
      "threads,t",
      boost::program_options::value<uint32_t> (&mysqlproxy_common::pool_size)
          ->default_value (boost::thread::hardware_concurrency ()),
      "Thread pool's size") (
      "address,a",
      boost::program_options::value<std::string> (&mysqlproxy_tracker::address)
          ->required (),
      "Server address") (
      "port,p",
      boost::program_options::value<std::string> (&mysqlproxy_tracker::port)
          ->required (),
      "Server port") ("output-file",
                      boost::program_options::value<std::string> (
                          &mysqlproxy_tracker::output_file),
                      "Output log stream pathname") ("help", "This message");

  // Variable to store our command line arguments.
  boost::program_options::variables_map vm;

  // Parsing and storing arguments.
  boost::program_options::store (
      boost::program_options::parse_command_line (argc, argv, desc), vm);

  // Must be called after all the parsing and storing.
  boost::program_options::notify (vm);

  if (vm.count ("help"))
    {
      std::cout << desc << "\n";
      return 1;
    }

  mysqlproxy_tracker::server::writer.reset (
      new mysqlproxy_system::basic_logger (
          mysqlproxy_common::processor::instance ().io_context (), "Server"));

  if (!mysqlproxy_tracker::output_file.empty ())
    {
      mysqlproxy_tracker::server::writer->use_file (
          mysqlproxy_tracker::output_file);

      CXXLOG_INFO (mysqlproxy_tracker::server::writer,
                   "Begin logging file: \"" << mysqlproxy_tracker::output_file
                                            << '\"');
    }

  mysqlproxy_tracker::server::listener listen (
      mysqlproxy_common::processor::instance ().io_context ());
  listen.run (mysqlproxy_tracker::address, mysqlproxy_tracker::port);

  BOOST_ASSERT (mysqlproxy_common::pool_size > 0);

  mysqlproxy_common::thread_func = run__thread_app;
  mysqlproxy_common::signal_handler = __signal_handler;
  mysqlproxy_common::processor::exec ();

  return 0;
}