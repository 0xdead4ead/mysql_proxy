AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = mysqlproxy_tracker

mysqlproxy_tracker_SOURCES = \
    start.cpp \
    connection.hpp \
    connection.cpp \
    server.hpp \
    server.cpp \
    ../common/mysql.hpp \
    ../common/mysql.cpp \
    ../common/processor.hpp \
    ../common/processor.cpp

AM_CPPFLAGS = \
    -DBOOST_ASIO_SEPARATE_COMPILATION \
    -DBOOST_ASIO_DISABLE_VISIBILITY \
    -DBOOST_THREAD_VERSION=4 \
    -DBOOST_BIND_GLOBAL_PLACEHOLDERS \
    @BOOST_CPPFLAGS@ \
    @OPENSSL_INCLUDES@ \
    -I$(srcdir)/..

LDADD = \
    ../system/libmysqlproxy_system.la \
    @OPENSSL_LIBS@ \
    -lboost_thread-mt \
    -lboost_system-mt \
    -lboost_program_options-mt \
    -lboost_date_time

AM_LDFLAGS = @OPENSSL_LDFLAGS@ @BOOST_LDFLAGS@