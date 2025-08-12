#pragma once
#include "FastEngine/manager/network_manager.hpp"
#include "FastEngine/network/C_server.hpp"

#define F_NET_DEFAULT_IP "127.0.0.1"
#define F_NET_DEFAULT_PORT 27421
#define F_NET_DEFAULT_ONLINE_MODE false
#define F_NET_STRING_SEQ "ProjectFichillshGG"
#define F_NET_CLIENT_HELLO "Hello"
#define F_NET_SERVER_HELLO "Hi"
#define F_NET_SERVER_COMPATIBILITY_VERSION                                                                             \
    uint32_t                                                                                                           \
    {                                                                                                                  \
        1                                                                                                              \
    }
#define F_NET_CHAT_MAX_SIZE 30

#define F_NET_CLIENT_TIMEOUT_CONNECT_MS                                                                                \
    std::chrono::milliseconds                                                                                          \
    {                                                                                                                  \
        3000                                                                                                           \
    }
#define F_NET_CLIENT_TIMEOUT_HELLO_MS                                                                                  \
    std::chrono::milliseconds                                                                                          \
    {                                                                                                                  \
        3000                                                                                                           \
    }
#define F_NET_SERVER_TIMEOUT_MS                                                                                        \
    std::chrono::milliseconds                                                                                          \
    {                                                                                                                  \
        3000                                                                                                           \
    }

#define F_NET_CLIENT_TIMEOUT_RECEIVE                                                                                   \
    std::chrono::milliseconds                                                                                          \
    {                                                                                                                  \
        1200                                                                                                           \
    }

#define F_TICK_TIME 50

enum class StatEvents : uint8_t
{
    CAUGHT_FISH,
    PLAYER_DISCONNECTED,
    PLAYER_CHAT,
    EVENT_COUNT
};
using StatEvents_t = std::underlying_type_t<StatEvents>;

struct PlayerEventData
{
    std::string _playerId;
    std::string _data;
};

inline fge::net::Packet& operator<<(fge::net::Packet& pck, PlayerEventData const& event)
{
    pck << event._playerId;
    pck << event._data;
    return pck;
}
inline fge::net::Packet const& operator>>(fge::net::Packet const& pck, PlayerEventData& event)
{
    pck >> event._playerId;
    pck >> event._data;
    return pck;
}

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
