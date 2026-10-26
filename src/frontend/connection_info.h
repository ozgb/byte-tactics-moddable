#ifndef CONNECTION_INFO_H
#define CONNECTION_INFO_H

#include "connection_blob.h"
#include "../../include/v4i.h"

#pragma pack(push, 1)
struct ConnInfo_443ff0 {
    V4i guid;
    Conn_443ff0 conn;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(ConnInfo_443ff0) == 24, "ConnInfo_443ff0");
#endif

#endif
