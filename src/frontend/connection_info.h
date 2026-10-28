#ifndef CONNECTION_INFO_H
#define CONNECTION_INFO_H

#include "connection_blob.h"
#include "../../include/v4i.h"

#pragma pack(push, 1)
struct ConnectionInfo {
    V4i guid;
    ConnectionBlob conn;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(ConnectionInfo) == 24, "ConnectionInfo");
#endif

#endif
