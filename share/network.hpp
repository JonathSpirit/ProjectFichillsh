#pragma once
#include "FastEngine/manager/network_manager.hpp"
#include <chrono>

#define F_NET_STRING_SEQ "ProjectFichillshGG"
#define F_NET_CLIENT_HELLO "Hello"
#define F_NET_SERVER_HELLO "Hi"
#define F_NET_SERVER_COMPATIBILITY_VERSION uint32_t{1}

#define F_NET_CLIENT_TIMEOUT_CONNECT_MS std::chrono::milliseconds{3000}
#define F_NET_CLIENT_TIMEOUT_HELLO_MS std::chrono::milliseconds{3000}
#define F_NET_SERVER_TIMEOUT_MS std::chrono::milliseconds{3000}

#define F_NET_CLIENT_TIMEOUT_RECEIVE std::chrono::milliseconds{1200}

#define F_TICK_TIME 50

enum class StatEvents : uint8_t
{
    CAUGHT_FISH,
    EVENT_COUNT
};
using StatEvents_t = std::underlying_type_t<StatEvents>;

enum PacketHeaders : fge::net::ProtocolPacket::IdType
{
    //Client to server
    CLIENT_ASK_CONNECT = FGE_NET_CUSTOM_ID_START,
    /*
     * - CLIENT_HELLO
     * - PLAYER_POSITION
     *
     * Response:
     * - BOOL_VALID
     * - SERVER_HELLO
     * - FULL_UPDATE
     * or
     * - BOOL_VALID
     * - BAD_STRING
     */

    //Server to client
    SERVER_UPDATE,
    /*
     * - LATENCY_PLANNER
     * - PLAYER_COUNT:
     * - - PLAYER_ID
     * - - PLAYER_POSITION
     * - - PLAYER_DIRECTION
     * - - PLAYER_STAT
     * - - EVENT_COUNT:
     * - - - EVENT_TYPE
     * - - - EVENT_DATA
     *
     * Response:
     * N/A
     */
    SERVER_FULL_UPDATE
    /*
     * - YOUR_PLAYER_ID
     * - PLAYER_COUNT:
     * - - PLAYER_ID
     * - - PLAYER_POSITION
     * - - PLAYER_DIRECTION
     * - - PLAYER_STAT
     * - - EVENT_COUNT:
     * - - - EVENT_TYPE
     * - - - EVENT_DATA
     *
     * Response:
     * N/A
     */
};
