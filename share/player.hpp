#pragma once

#include "FastEngine/C_scene.hpp"
#include "FastEngine/object/C_objAnim.hpp"
#include "FastEngine/object/C_objSprite.hpp"
#include "FastEngine/object/C_objText.hpp"
#include "FastEngine/object/C_object.hpp"
#ifndef FGE_DEF_SERVER
    #include "box2d/box2d.h"
#endif

#define F_PLAYER_SPEED 30.0f
#define F_BAIT_SPEED 2.0f
#define F_BAIT_THROW_LENGTH 12.0f

#define F_DIRECTION_UP                                                                                                 \
    fge::Vector2i                                                                                                      \
    {                                                                                                                  \
        0, -1                                                                                                          \
    }
#define F_DIRECTION_DOWN                                                                                               \
    fge::Vector2i                                                                                                      \
    {                                                                                                                  \
        0, 1                                                                                                           \
    }
#define F_DIRECTION_LEFT                                                                                               \
    fge::Vector2i                                                                                                      \
    {                                                                                                                  \
        -1, 0                                                                                                          \
    }
#define F_DIRECTION_RIGHT                                                                                              \
    fge::Vector2i                                                                                                      \
    {                                                                                                                  \
        1, 0                                                                                                           \
    }
#define F_DIRECTION_UP_LEFT                                                                                            \
    fge::Vector2i                                                                                                      \
    {                                                                                                                  \
        -1, -1                                                                                                         \
    }
#define F_DIRECTION_UP_RIGHT                                                                                           \
    fge::Vector2i                                                                                                      \
    {                                                                                                                  \
        1, -1                                                                                                          \
    }
#define F_DIRECTION_DOWN_LEFT                                                                                          \
    fge::Vector2i                                                                                                      \
    {                                                                                                                  \
        -1, 1                                                                                                          \
    }
#define F_DIRECTION_DOWN_RIGHT                                                                                         \
    fge::Vector2i                                                                                                      \
    {                                                                                                                  \
        1, 1                                                                                                           \
    }

class FishBait : public fge::Object
{
public:
    FishBait() = default;
    FishBait(fge::Vector2i const& throwDirection, fge::Vector2f const& position);
    ~FishBait() override = default;

    FGE_OBJ_UPDATE_DECLARE
    FGE_OBJ_DRAW_DECLARE

    void first(fge::Scene& scene) override;

    void callbackRegister(fge::Event& event, fge::GuiElementHandler* guiElementHandlerPtr) override;

    char const* getClassName() const override;
    char const* getReadableClassName() const override;

    [[nodiscard]] fge::RectFloat getGlobalBounds() const override;
    [[nodiscard]] fge::RectFloat getLocalBounds() const override;

    [[nodiscard]] bool isWaiting() const;

    void catchingFish();
    void endCatchingFish();

private:
    fge::ObjSprite g_objSprite;
    fge::Vector2i g_throwDirection;
    enum class Stats
    {
        THROWING,
        WAITING,
        CATCHING
    } g_state = Stats::THROWING;
    float g_time = 0.0f;
    fge::Vector2f g_startPosition;
    fge::Vector2f g_staticPosition;
};

class PlayerChatMessage : public fge::Object
{
public:
    PlayerChatMessage() = default;
    PlayerChatMessage(tiny_utf8::string message, fge::ObjectSid playerSid = FGE_SCENE_BAD_SID);
    ~PlayerChatMessage() override = default;

    FGE_OBJ_UPDATE_DECLARE
    FGE_OBJ_DRAW_DECLARE

    void first(fge::Scene& scene) override;

    char const* getClassName() const override;
    char const* getReadableClassName() const override;

    [[nodiscard]] fge::RectFloat getGlobalBounds() const override;
    [[nodiscard]] fge::RectFloat getLocalBounds() const override;

private:
    fge::ObjText g_objText;
    float g_time = 0.0f;
    bool g_fading = false;
    fge::ObjectSid g_playerSid = FGE_SCENE_BAD_SID;
    fge::ObjectDataWeak g_player;
};

class Player : public fge::Object, public fge::Subscriber
{
public:
    enum class States : uint8_t
    {
        WALKING,
        IDLE,
        THROWING,
        FISHING,
        CATCHING,
        CHATTING
    };
    using Stats_t = std::underlying_type_t<States>;

    Player() = default;
    ~Player() override = default;

    FGE_OBJ_UPDATE_DECLARE
    FGE_OBJ_DRAW_DECLARE

    void first(fge::Scene& scene) override;

    void callbackRegister(fge::Event& event, fge::GuiElementHandler* guiElementHandlerPtr) override;

    char const* getClassName() const override;
    char const* getReadableClassName() const override;

    [[nodiscard]] fge::RectFloat getGlobalBounds() const override;
    [[nodiscard]] fge::RectFloat getLocalBounds() const override;

    void networkRegister() override;

    void pack(fge::net::Packet& pck) override;
    void unpack(fge::net::Packet const& pck) override;

    [[nodiscard]] States getState() const;
    [[nodiscard]] fge::Vector2i const& getDirection() const;

    void setDirection(fge::Vector2i const& direction);
    void setState(States state);
    void setServerPosition(fge::Vector2f const& position);
    void setServerDirection(fge::Vector2i const& direction);
    void setServerState(States state);

    void startChatting(fge::Event& event);

    void boxMove(fge::Vector2f const& move);
    [[nodiscard]] bool isFishing() const;

    void catchingFish();
    void endCatchingFish();

    void allowUserControl(bool allow);

private:
    fge::ObjAnimation g_objAnim;
    fge::ObjAnimation g_objAnimShadow;
    fge::ObjText g_objChatText;
#ifdef FGE_DEF_SERVER
    unsigned int g_bodyId;
#else
    b2BodyId g_bodyId;
#endif
    States g_state = States::WALKING;
    States g_serverState = States::WALKING;
    fge::ObjectDataWeak g_fishBait;
    fge::Vector2i g_direction{0, 1};
    fge::Vector2f g_serverPosition;
    std::vector<float> g_transitionYPoints;
    fge::ObjectPlan g_transitionPlan;
    bool g_transitionLast = false;
    int g_audioWalking = -1;
    bool g_isUserControlled = true;
};