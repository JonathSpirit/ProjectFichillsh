#pragma once

#include "FastEngine/object/C_objSprite.hpp"
#include "FastEngine/object/C_object.hpp"
#include "FastEngine/object/C_objAnim.hpp"
#include "box2d/box2d.h"

#define F_PLAYER_SPEED 30.0f
#define F_BAIT_SPEED 2.0f
#define F_BAIT_THROW_LENGTH 22.0f

class FishBait : public fge::Object
{
public:
    FishBait() = default;
    FishBait(fge::Vector2i const& throwDirection, fge::Vector2f const& position);
    ~FishBait() override = default;

    FGE_OBJ_UPDATE_DECLARE
    FGE_OBJ_DRAW_DECLARE

    void first(fge::Scene &scene) override;

    void callbackRegister(fge::Event &event, fge::GuiElementHandler *guiElementHandlerPtr) override;

    const char * getClassName() const override;
    const char * getReadableClassName() const override;

    [[nodiscard]] fge::RectFloat getGlobalBounds() const override;
    [[nodiscard]] fge::RectFloat getLocalBounds() const override;

    [[nodiscard]] bool isWaiting() const;

private:
    fge::ObjSprite g_objSprite;
    fge::Vector2i g_throwDirection;
    enum class States
    {
        THROWING,
        WAITING
    } g_state = States::THROWING;
    float g_time = 0.0f;
    fge::Vector2f g_startPosition;
};

class Player : public fge::Object
{
public:
    Player() = default;
    ~Player() override = default;

    FGE_OBJ_UPDATE_DECLARE
    FGE_OBJ_DRAW_DECLARE

    void first(fge::Scene &scene) override;

    void callbackRegister(fge::Event &event, fge::GuiElementHandler *guiElementHandlerPtr) override;

    const char * getClassName() const override;
    const char * getReadableClassName() const override;

    [[nodiscard]] fge::RectFloat getGlobalBounds() const override;
    [[nodiscard]] fge::RectFloat getLocalBounds() const override;

    void boxMove(fge::Vector2f const& move);
    [[nodiscard]] bool isFishing() const;

    void catchingFish();
    void endCatchingFish();

private:
    fge::ObjAnimation g_objAnim;
    b2BodyId g_bodyId;
    enum class States
    {
        WALKING,
        IDLE,
        THROWING,
        FISHING,
        CATCHING
    } g_state = States::WALKING;
    fge::ObjectDataWeak g_fishBait;
    fge::Vector2i g_direction{0, 1};
};