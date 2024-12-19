#include "player.hpp"
#include "game.hpp"

//FishBait

FishBait::FishBait(fge::Vector2i const& throwDirection, fge::Vector2f const& position) :
        g_throwDirection(throwDirection),
        g_startPosition(position)
{
    this->setPosition(position);
}

FGE_OBJ_UPDATE_BODY(FishBait)
{
    switch (this->g_state)
    {
    case States::THROWING:
    {
        auto const delta = fge::DurationToSecondFloat(deltaTime);
        this->g_time += delta;
        auto sinusValue = std::sin(this->g_time * F_BAIT_SPEED);

        //y = sin(t * speed)
        //t = asin(y) / speed
        //t = asin(1) / speed
        //t = PI/(2*speed)
        if (this->g_time >= static_cast<float>(FGE_MATH_PI)/(2.0f * F_BAIT_SPEED))
        {
            this->g_state = States::WAITING;
            sinusValue = 1.0f;
        }

        auto const newPosition = this->g_startPosition + static_cast<fge::Vector2f>(this->g_throwDirection) * F_BAIT_THROW_LENGTH * sinusValue;
        this->setPosition(newPosition);
    }
        break;
    case States::WAITING:
        break;
    }
}

FGE_OBJ_DRAW_BODY(FishBait)
{
    auto copyStates = states.copy();
    copyStates._resTransform.set(target.requestGlobalTransform(*this, copyStates._resTransform));

    this->g_objSprite.draw(target, copyStates);
}

void FishBait::first(fge::Scene &scene)
{
    this->g_objSprite.setTexture("fishBait_1");
    this->g_objSprite.centerOriginFromLocalBounds();
    this->g_startPosition = this->getPosition();
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
    return this->getTransform() * this->g_objSprite.getGlobalBounds();
}
fge::RectFloat FishBait::getLocalBounds() const
{
    return this->g_objSprite.getLocalBounds();
}

bool FishBait::isWaiting() const
{
    return this->g_state == States::WAITING;
}

//Player

FGE_OBJ_UPDATE_BODY(Player)
{
    FGE_OBJ_UPDATE_CALL(this->g_objAnim);

    switch (this->g_state)
    {
    case States::WALKING: {
        //Player movement and animation
        std::string animationName;

        if (event.isKeyPressed(SDLK_SPACE))
        {
            animationName = this->g_objAnim.getAnimation().getGroup()->_groupName;
            animationName = animationName.substr(animationName.find('_'));
            animationName = "rod" + animationName;

            this->g_objAnim.getAnimation().setGroup(animationName);
            this->g_objAnim.getAnimation().setFrame(0);
            this->g_fishBait = scene.newObject(FGE_NEWOBJECT(FishBait, this->g_direction, this->getPosition()));

            b2Body_SetLinearVelocity(this->g_bodyId, {0.0f, 0.0f});

            this->g_state = States::THROWING;
            this->g_objAnim.getAnimation().setLoop(false);
            break;
        }

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

        b2Body_SetLinearVelocity(this->g_bodyId, {
            static_cast<float>(moveDirection.x) * F_PLAYER_SPEED,
            static_cast<float>(moveDirection.y) * F_PLAYER_SPEED
        });
        this->g_objAnim.getAnimation().setGroup(animationName);
        break;
    } case States::IDLE:
        break;
    case States::THROWING:
        if (auto fishBait = this->g_fishBait.lock())
        {
            if (fishBait->getObject<FishBait>()->isWaiting())
            {
                this->g_state = States::FISHING;
            }
        }
        else
        {
            this->g_state = States::WALKING;
            this->g_objAnim.getAnimation().setLoop(true);
        }
        break;
    case States::FISHING:
        if (event.isKeyPressed(SDLK_w) || event.isKeyPressed(SDLK_s) || event.isKeyPressed(SDLK_a) || event.isKeyPressed(SDLK_d))
        {
            if (auto fishBait = this->g_fishBait.lock())
            {
                scene.delObject(fishBait->getSid());
            }

            this->g_state = States::WALKING;
            this->g_objAnim.getAnimation().setLoop(true);
        }
        break;
    }

    //Camera movement
    auto view = target.getView();
    view.setCenter(this->getPosition());
    target.setView(view);

    //Update position
    auto const bpos = b2Body_GetPosition(this->g_bodyId);
    this->setPosition({bpos.x, bpos.y});
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
    this->g_objAnim.centerOriginFromLocalBounds();

    //Create the player's body
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.position = {this->getPosition().x, this->getPosition().y};
    bodyDef.type = b2_dynamicBody;
    bodyDef.fixedRotation = true;

    this->g_bodyId = b2CreateBody(gGameHandler->getWorld(), &bodyDef);

    b2Polygon dynamicBox = b2MakeRoundedBox(2.0f, 2.0f, 2.0f);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;
    shapeDef.friction = 1.0f;

    b2CreatePolygonShape(this->g_bodyId, &shapeDef, &dynamicBox);
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

void Player::boxMove(fge::Vector2f const& move)
{
    b2Body_SetTransform(this->g_bodyId,
        {this->getPosition().x + move.x, this->getPosition().y + move.y},
        b2MakeRot(0.0f));
    this->move(move);
}
bool Player::isFishing() const
{
    return this->g_state == States::FISHING;
}

void Player::catchingFish()
{
    if (this->g_state == States::FISHING)
    {
        this->g_state = States::CATCHING;
    }
}
void Player::endCatchingFish()
{
    if (this->g_state == States::CATCHING)
    {
        this->g_state = States::WALKING;
    }
}
