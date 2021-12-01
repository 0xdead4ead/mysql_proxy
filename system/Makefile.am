AUTOMAKE_OPTIONS = subdir-objects

lib_LTLIBRARIES = libmysqlproxy_system.la
libmysqlproxy_system_la_SOURCES = \
    asio.cpp \
    config.hpp \
    logger_service.hpp \
    logger_service.cpp

AM_CPPFLAGS = \
    -DBOOST_ASIO_SEPARATE_COMPILATION \
    -DBOOST_ASIO_DISABLE_VISIBILITY \
    -DBOOST_THREAD_VERSION=4 \
    -DBOOST_BIND_GLOBAL_PLACEHOLDERS \
    @BOOST_CPPFLAGS@ \
    @OPENSSL_INCLUDES@

AM_LDFLAGS = @OPENSSL_LDFLAGS@ @BOOST_LDFLAGS@
libmysqlproxy_system_la_LIBADD = \
    @OPENSSL_LIBS@ \
    -lboost_thread-mt \
    -lboost_system-mt \
    -lboost_date_time-mt