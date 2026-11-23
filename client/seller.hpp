#pragma once

#include "FastEngine/C_scene.hpp"
#include "FastEngine/object/C_objAnim.hpp"
#include "FastEngine/object/C_objSprite.hpp"
#include "FastEngine/object/C_object.hpp"

#define F_SELLER_ACTIONABLE_DISTANCE 30.0f

class Seller : public fge::Object
{
public:
    Seller() = default;
    explicit Seller(fge::Vector2f const& position);
    ~Seller() override = default;

    FGE_OBJ_UPDATE_DECLARE
    FGE_OBJ_DRAW_DECLARE

    void first(fge::Scene& scene) override;

    void callbackRegister(fge::Event& event, fge::GuiElementHandler* guiElementHandlerPtr) override;

    char const* getClassName() const override;
    char const* getReadableClassName() const override;

    [[nodiscard]] fge::RectFloat getGlobalBounds() const override;
    [[nodiscard]] fge::RectFloat getLocalBounds() const override;

private:
    fge::ObjAnimation g_objAnim;
    fge::ObjAnimation g_objAnimShadow;
    fge::ObjSprite g_objSpriteAction;

    fge::ObjectDataWeak g_player;
    fge::ObjectPlan g_transitionPlan;
    bool g_transitionLast = false;

    bool g_actionable = false;
};