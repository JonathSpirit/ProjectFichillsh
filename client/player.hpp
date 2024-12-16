#pragma once

#include "FastEngine/object/C_object.hpp"
#include "FastEngine/object/C_objAnim.hpp"
#include "box2d/box2d.h"

#define F_PLAYER_SPEED 30.0f

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

private:
    fge::ObjAnimation g_objAnim;
    bool g_isUsingRod = false;
    b2BodyId g_bodyId;
};