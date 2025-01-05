#pragma once
#include "FastEngine/manager/network_manager.hpp"
#include <chrono>

#define F_NET_STRING_SEQ "ProjectFichillshGG"
#define F_NET_CLIENT_HELLO "Hello"
#define F_NET_SERVER_HELLO "Hi"

#define F_NET_CLIENT_TIMEOUT_CONNECT_MS std::chrono::milliseconds{3000}
#define F_NET_CLIENT_TIMEOUT_HELLO_MS std::chrono::milliseconds{3000}
#define F_NET_SERVER_TIMEOUT_MS std::chrono::milliseconds{3000}

#define F_NET_CLIENT_TIMEOUT_RECEIVE std::chrono::milliseconds{1200}

enum class ClientNetStats
{
    SAY_HELLO,
    CONNECTED
};

enum PacketHeaders : FGE_NET_HEADER_TYPE
{
    //Client to server
    CLIENT_HELLO = FGE_NET_HEADERID_START,
    /*
     * - CLIENT_HELLO
     * - STRING_SEQ
     *
     * Response:
     * - BOOL_VALID
     * - SERVER_HELLO
     * or
     * N/A (if unknown client)
     * or
     * - BOOL_VALID
     * - BAD_STRING
     */
    CLIENT_ASK_CONNECT,
    /*
     * - PLAYER_POSITION
     *
     * Response:
     * - BOOL_VALID
     * - FULL_UPDATE
     * or
     * - BOOL_VALID
     * - BAD_STRING
     */
    CLIENT_GOODBYE,
    /*
     * N/A
     *
     * Response:
     * SERVER_GOODBYE
     */
    CLIENT_STATS,
    /*
     * - LATENCY_PLANNER
     * - PLAYER_POSITION
     * - PLAYER_DIRECTION
     * - PLAYER_STAT
     * - EVENT_COUNT:
     * - - EVENT_TYPE
     * - - EVENT_DATA
     *
     * Response:
     * N/A
     */

    //Server to client
    SERVER_GOODBYE,
    /*
     * - STRING_REASON
     *
     * Response:
     * N/A
     */
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
