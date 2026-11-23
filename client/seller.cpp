#include "seller.hpp"
#include "../share/player.hpp"

#include "FastEngine/C_random.hpp"
#include "FastEngine/C_scene.hpp"
#include "FastEngine/object/C_objTilelayer.hpp"

//Seller

Seller::Seller(fge::Vector2f const& position)
{
    this->setPosition(position);
}

FGE_OBJ_UPDATE_BODY(Seller)
{
    FGE_OBJ_UPDATE_CALL(this->g_objAnim);
    FGE_OBJ_UPDATE_CALL(this->g_objAnimShadow);

    auto const delta = fge::DurationToSecondFloat(deltaTime);

    //Update plan
    if (this->g_player.lock()->getObject()->getPosition().y < this->getPosition().y)
    {
        if (!this->g_transitionLast)
        {
            this->g_transitionLast = true;
            scene.setObjectPlan(this->_myObjectData.lock()->getSid(), this->g_transitionPlan + 1);
        }
    }
    else
    {
        if (this->g_transitionLast)
        {
            this->g_transitionLast = false;
            scene.setObjectPlan(this->_myObjectData.lock()->getSid(), this->g_transitionPlan - 1);
        }
    }

    if (glm::distance(this->g_player.lock()->getObject()->getPosition(), this->getPosition()) <=
        F_SELLER_ACTIONABLE_DISTANCE)
    {
        if (!this->g_actionable)
        {
            this->g_actionable = true;
            this->g_objSpriteAction._drawMode = DrawModes::DRAW_DEFAULT;
            this->g_player.lock()->getObject<Player>()->callCmd("actionable_set", this, {}, &scene);
        }
    }
    else
    {
        if (this->g_actionable)
        {
            this->g_actionable = false;
            this->g_objSpriteAction._drawMode = DrawModes::DRAW_ALWAYS_HIDDEN;
            this->g_player.lock()->getObject<Player>()->callCmd("actionable_set", nullptr, {}, &scene);
        }
    }
}
FGE_OBJ_DRAW_BODY(Seller)
{
    auto copyStates = states.copy();
    copyStates._resTransform.set(target.requestGlobalTransform(*this, copyStates._resTransform));

    this->g_objAnimShadow.draw(target, copyStates);
    this->g_objAnim.draw(target, copyStates);
    if (this->g_objSpriteAction._drawMode != DrawModes::DRAW_ALWAYS_HIDDEN)
    {
        this->g_objSpriteAction.draw(target, copyStates);
    }
}

void Seller::first(fge::Scene& scene)
{
    auto player = scene.getFirstObj_ByTag("player");
    this->g_player = player;
    this->g_transitionPlan = player->getPlan();

    //TODO: replace duck texture with a real seller :)
    this->g_objAnim.setAnimation(fge::Animation{"ducky_1", "idle"});
    this->g_objAnim.getAnimation().setLoop(true);
    this->g_objAnim.scale(0.5f);
    this->g_objAnim.centerOriginFromLocalBounds();
    this->g_objAnim.setColor(fge::Color(100, 200, 200, 255));

    this->g_objAnimShadow.setAnimation(fge::Animation{"ducky_1_shadow", "idle"});
    this->g_objAnimShadow.getAnimation().setLoop(true);
    this->g_objAnimShadow.centerOriginFromLocalBounds();
    this->g_objAnimShadow.setRotation(20.0f);
    this->g_objAnimShadow.move({4.0f, 0.0f});
    this->g_objAnimShadow.scale(0.5f);
    this->g_objAnimShadow.scale({0.8f, 0.7f});
    this->g_objAnimShadow.setColor({255, 255, 255, 30});

    this->g_objSpriteAction.setTexture("fishingIcon");
    this->g_objSpriteAction.centerOriginFromLocalBounds();
    this->g_objSpriteAction.setScale(0.3f);
    this->g_objSpriteAction.move({0.0f, -10.0f});

    this->g_objSpriteAction._drawMode = DrawModes::DRAW_ALWAYS_HIDDEN;
}

void Seller::callbackRegister(fge::Event& event, fge::GuiElementHandler* guiElementHandlerPtr) {}

char const* Seller::getClassName() const
{
    return "FISH_SELLER";
}
char const* Seller::getReadableClassName() const
{
    return "seller";
}

fge::RectFloat Seller::getGlobalBounds() const
{
    return this->getTransform() * this->g_objAnim.getGlobalBounds();
}
fge::RectFloat Seller::getLocalBounds() const
{
    return this->g_objAnim.getLocalBounds();
}
