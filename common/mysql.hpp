#ifndef MYSQLPROXY_COMMON_MYSQL_HPP
#define MYSQLPROXY_COMMON_MYSQL_HPP

#include <boost/cstdint.hpp>

namespace mysqlproxy_common
{
namespace protocol
{
#define MYSQLPROXY_PROTOCOL_COM_QUERY 0x3
#define MYSQLPROXY_PROTOCOL_OK_PACKET 0x00
#define MYSQLPROXY_PROTOCOL_ERR_PACKET 0xff
#define MYSQLPROXY_PROTOCOL_EOF_PACKET 0xfe

#pragma pack(push, 1)
struct prefix
{
  uint32_t payload_length : 24;
  uint32_t sequence_id : 8;
};

struct command
{
    uint8_t value;
    uint8_t data[];
};

struct query
{
  uint8_t statement; // ?
  char text[];
};
#pragma pack(pop)
} // namespace protocol
} // mysqlproxy_common

#endif // MYSQLPROXY_COMMON_MYSQL_HPP