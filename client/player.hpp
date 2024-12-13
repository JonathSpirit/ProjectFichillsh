#pragma once

#include "FastEngine/object/C_object.hpp"
#include "FastEngine/object/C_objAnim.hpp"

#define F_PLAYER_SPEED 60.0f

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

private:
    fge::ObjAnimation g_objAnim;
    bool g_isUsingRod = false;
};