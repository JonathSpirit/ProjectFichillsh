#include "player.hpp"
#include "FastEngine/C_random.hpp"
#include "FastEngine/C_scene.hpp"

//FishBait

FishBait::FishBait(fge::Vector2i const& throwDirection, fge::Vector2f const& position) :
        g_throwDirection(throwDirection),
        g_startPosition(position)
{
    fge::Vector2f throwOffset{0.0f, 0.0f};

    if (throwDirection == F_DIRECTION_UP)
    {
        throwOffset = {3.0f, 0.0f};
    }
    else if (throwDirection == F_DIRECTION_DOWN)
    {
        throwOffset = {-3.0f, 0.0f};
    }
    else if (throwDirection == F_DIRECTION_LEFT)
    {
        throwOffset = {0.0f, 3.0f};
    }
    else if (throwDirection == F_DIRECTION_RIGHT)
    {
        throwOffset = {0.0f, 3.0f};
    }

    this->setPosition(position + throwOffset + static_cast<fge::Vector2f>(throwDirection) * 10.0f);
}

FGE_OBJ_UPDATE_BODY(FishBait)
{
    auto const delta = fge::DurationToSecondFloat(deltaTime);
    this->g_time += delta;

    switch (this->g_state)
    {
    case Stats::THROWING:
    {
        auto sinusValue = std::sin(this->g_time * F_BAIT_SPEED);

        //y = sin(t * speed)
        //t = asin(y) / speed
        //t = asin(1) / speed
        //t = PI/(2*speed)
        if (this->g_time >= static_cast<float>(FGE_MATH_PI)/(2.0f * F_BAIT_SPEED))
        {
            this->g_state = Stats::WAITING;
            this->g_time = 0.0f;
            sinusValue = 1.0f;
        }

        auto const newPosition = this->g_startPosition + static_cast<fge::Vector2f>(this->g_throwDirection)
                * (1.0f / glm::length(static_cast<fge::Vector2f>(this->g_throwDirection))) * F_BAIT_THROW_LENGTH * sinusValue;
        this->setPosition(newPosition);
        this->g_staticPosition = this->getPosition();
    }
        break;
    case Stats::WAITING:
    {
        auto sinx = std::sin(this->g_time);
        auto siny = std::sin(this->g_time * 1.7f);

        this->setPosition(this->g_staticPosition + fge::Vector2f{sinx, siny});
    }
        break;
    case Stats::CATCHING:
    {
        auto const posXeffect = fge::_random.range(-1.0f, 1.0f);
        this->setPosition(this->g_staticPosition + fge::Vector2f{posXeffect, 0.0f});
    }
        break;
    }
}

void FishBait::first(fge::Scene &scene)
{
    this->g_startPosition = this->getPosition();
    this->_netSyncMode = NetSyncModes::NO_SYNC;
}

void FishBait::callbackRegister(fge::Event &event, fge::GuiElementHandler *guiElementHandlerPtr)
{
}

const char * FishBait::getClassName() const
{
    return "FISH_BAIT";
}

const char * FishBait::getReadableClassName() const
{
    return "fish bait";
}

fge::RectFloat FishBait::getGlobalBounds() const
{
    return Object::getGlobalBounds();
}
fge::RectFloat FishBait::getLocalBounds() const
{
    return Object::getLocalBounds();
}

bool FishBait::isWaiting() const
{
    return this->g_state == Stats::WAITING;
}

void FishBait::catchingFish()
{
    this->g_state = Stats::CATCHING;
    this->g_time = 0.0f;
}
void FishBait::endCatchingFish()
{
    this->g_state = Stats::WAITING;
    this->g_time = 0.0f;
}

//Player

FGE_OBJ_UPDATE_BODY(Player)
{
    FGE_OBJ_UPDATE_CALL(this->g_objAnim);

#ifndef FGE_DEF_SERVER
    switch (this->g_state)
    {
    case Stats::WALKING: {
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
            this->g_direction = moveDirection;
        }

        this->move({moveDirection.x * F_PLAYER_SPEED, moveDirection.y * F_PLAYER_SPEED});
        this->g_objAnim.getAnimation().setGroup(animationName);
        break;
    } case Stats::IDLE:
        break;
    case Stats::THROWING:
        if (auto fishBait = this->g_fishBait.lock())
        {
            if (fishBait->getObject<FishBait>()->isWaiting())
            {
                this->g_state = Stats::FISHING;
            }
        }
        else
        {
            this->g_state = Stats::WALKING;
            this->g_objAnim.getAnimation().setLoop(true);
        }
        break;
    case Stats::FISHING:
        if (event.isKeyPressed(SDLK_w) || event.isKeyPressed(SDLK_s) || event.isKeyPressed(SDLK_a) || event.isKeyPressed(SDLK_d))
        {
            if (auto fishBait = this->g_fishBait.lock())
            {
                scene.delObject(fishBait->getSid());
            }

            this->g_state = Stats::WALKING;
            this->g_objAnim.getAnimation().setLoop(true);
        }
        break;
    }
#endif

    //Update position
    //auto const bpos = b2Body_GetPosition(this->g_bodyId);
    //this->setPosition({bpos.x, bpos.y});
}

void Player::first(fge::Scene &scene)
{
    this->g_objAnim.setAnimation(fge::Animation{"human_1", "idle_down"});
    this->g_objAnim.getAnimation().setLoop(true);
    this->g_objAnim.centerOriginFromLocalBounds();
    this->_netSyncMode = NetSyncModes::NO_SYNC;
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

void Player::setDirection(fge::Vector2i const& direction)
{
    this->g_direction = direction;
}
void Player::setStat(Stats stat)
{
    this->g_state = stat;
}
fge::Vector2i Player::getDirection() const
{
    return this->g_direction;
}
Player::Stats Player::getStat() const
{
    return this->g_state;
}

bool Player::isFishing() const
{
    return this->g_state == Stats::FISHING;
}

void Player::catchingFish()
{
    if (this->g_state == Stats::FISHING)
    {
        this->g_state = Stats::CATCHING;

        if (auto fishBait = this->g_fishBait.lock())
        {
            fishBait->getObject<FishBait>()->catchingFish();
        }
    }
}
void Player::endCatchingFish()
{
    if (this->g_state == Stats::CATCHING)
    {
        if (auto fishBait = this->g_fishBait.lock())
        {
            this->_myObjectData.lock()->getScene()->delObject(fishBait->getSid());
        }
        this->g_objAnim.getAnimation().setLoop(true);
        this->g_state = Stats::WALKING;
    }
}
