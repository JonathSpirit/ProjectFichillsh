#include "player.hpp"

FGE_OBJ_UPDATE_BODY(Player)
{
    FGE_OBJ_UPDATE_CALL(this->g_objAnim);

    //Player movement and animation
    std::string animationName;

    fge::Vector2i moveDirection{0, 0};
    if (event.isKeyPressed(SDLK_w))
    {
        moveDirection.y = -1;
        animationName += "_up";
    }
    else if (event.isKeyPressed(SDLK_s))
    {
        moveDirection.y = 1;
        animationName += "_down";
    }

    if (event.isKeyPressed(SDLK_a))
    {
        moveDirection.x = -1;
        animationName += "_left";
    }
    else if (event.isKeyPressed(SDLK_d))
    {
        moveDirection.x = 1;
        animationName += "_right";
    }

    if (animationName.empty())
    {//Idle
        animationName = this->g_objAnim.getAnimation().getGroup()->_groupName;
        animationName = animationName.substr(animationName.find('_'));
        animationName = "idle" + animationName;
    }
    else
    {//Walking
        animationName = "walk" + animationName;
    }

    auto const delta = fge::DurationToSecondFloat(deltaTime);
    this->move(static_cast<fge::Vector2f>(moveDirection) * F_PLAYER_SPEED * delta);

    this->g_objAnim.getAnimation().setGroup(animationName);

    //Camera movement
    auto view = target.getView();
    view.setCenter(this->getPosition());
    target.setView(view);
}
FGE_OBJ_DRAW_BODY(Player)
{
    auto copyStates = states.copy();
    copyStates._resTransform.set(target.requestGlobalTransform(*this, copyStates._resTransform));

    this->g_objAnim.draw(target, copyStates);
}

void Player::first(fge::Scene &scene)
{
    this->g_objAnim.setAnimation(fge::Animation{"human_1", "idle_down"});
    this->g_objAnim.getAnimation().setLoop(true);
    auto const size = static_cast<fge::Vector2f>(this->g_objAnim.getTextureRect().getSize()) / 2.0f;
    this->g_objAnim.setOrigin(size);
}

void Player::callbackRegister(fge::Event &event, fge::GuiElementHandler *guiElementHandlerPtr)
{
}

const char * Player::getClassName() const
{
    return "FISH_PLAYER";
}
const char * Player::getReadableClassName() const
{
    return "player";
}

fge::RectFloat Player::getGlobalBounds() const
{
    return this->getTransform() * this->g_objAnim.getGlobalBounds();
}
fge::RectFloat Player::getLocalBounds() const
{
    return this->g_objAnim.getLocalBounds();
}
