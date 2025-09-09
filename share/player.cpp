#include "player.hpp"
#ifndef FGE_DEF_SERVER
    #include "../client/game.hpp"
    #include "FastEngine/manager/audio_manager.hpp"
#else
    #include "FastEngine/C_scene.hpp"
#endif //FGE_DEF_SERVER
#include "FastEngine/C_random.hpp"
#include "FastEngine/object/C_objTilelayer.hpp"
#include "network.hpp"
#include <iostream>

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

#ifdef FGE_DEF_SERVER
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
        if (this->g_time >= static_cast<float>(FGE_MATH_PI) / (2.0f * F_BAIT_SPEED))
        {
            this->g_state = Stats::WAITING;
            this->g_time = 0.0f;
            sinusValue = 1.0f;
        }

        auto const newPosition = this->g_startPosition +
                                 static_cast<fge::Vector2f>(this->g_throwDirection) *
                                         (1.0f / glm::length(static_cast<fge::Vector2f>(this->g_throwDirection))) *
                                         F_BAIT_THROW_LENGTH * sinusValue;
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
#else
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
        if (this->g_time >= static_cast<float>(FGE_MATH_PI) / (2.0f * F_BAIT_SPEED))
        {
            //Check if the bait is in water
            if (auto const firstLayer = scene.getFirstObj_ByTag("map"))
            {
                auto const map = firstLayer->getObject<fge::ObjTileLayer>()->getTileMap();
                auto const waterLayer = map->findLayerName("Water")->get()->as<fge::TileLayer>();
                auto const tileGrid = waterLayer->getGridPosition(this->getPosition());
                if (!tileGrid || waterLayer->getGid(*tileGrid) == 0)
                { //Not in water
                    scene.delUpdatedObject();
                    return;
                }
                auto const& waterTile = waterLayer->getTiles().get(*tileGrid);
                auto const& collisionRects = waterTile.getTileData()->_collisionRects;

                bool isInside = false;
                for (auto const& rect: collisionRects)
                {
                    auto frect = static_cast<fge::RectFloat>(rect);
                    frect._x += waterTile.getPosition().x;
                    frect._y += waterTile.getPosition().y;
                    if (frect.contains(this->getPosition()))
                    {
                        isInside = true;
                        break;
                    }
                }

                if (!isInside)
                {
                    scene.delUpdatedObject();
                    return;
                }
            }

            Mix_PlayChannel(-1, fge::audio::gManager.getElement("splash")->_ptr.get(), 0);
            this->g_state = Stats::WAITING;
            this->g_time = 0.0f;
            sinusValue = 1.0f;
        }

        auto const newPosition = this->g_startPosition +
                                 static_cast<fge::Vector2f>(this->g_throwDirection) *
                                         (1.0f / glm::length(static_cast<fge::Vector2f>(this->g_throwDirection))) *
                                         F_BAIT_THROW_LENGTH * sinusValue;
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
FGE_OBJ_DRAW_BODY(FishBait)
{
    auto copyStats = states.copy();
    copyStats._resTransform.set(target.requestGlobalTransform(*this, copyStats._resTransform));

    this->g_objSprite.draw(target, copyStats);
}
#endif //FGE_DEF_SERVER

void FishBait::first(fge::Scene& scene)
{
    this->g_objSprite.setTexture("fishBait_1");
    this->g_objSprite.centerOriginFromLocalBounds();
    this->g_objSprite.scale(0.9f);
    this->g_startPosition = this->getPosition();
    this->_netSyncMode = NetSyncModes::NO_SYNC;
}

void FishBait::callbackRegister(fge::Event& event, fge::GuiElementHandler* guiElementHandlerPtr) {}

char const* FishBait::getClassName() const
{
    return "FISH_BAIT";
}

char const* FishBait::getReadableClassName() const
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

//PlayerChatMessage
PlayerChatMessage::PlayerChatMessage(tiny_utf8::string message, fge::ObjectSid playerSid) :
        g_objText(std::move(message), "default", {}, 44),
        g_playerSid(playerSid)
{
    this->g_objText.scale(0.2f);
    this->g_objText.setFillColor(fge::Color::White);
    this->g_objText.setOutlineColor(fge::Color::Black);
    this->g_objText.setOutlineThickness(1.0f);
    this->g_objText.setPosition({0.0f, -20.0f});
}

FGE_OBJ_UPDATE_BODY(PlayerChatMessage)
{
    FGE_OBJ_UPDATE_CALL(this->g_objText);

    if (auto player = this->g_player.lock())
    {
        this->setPosition(player->getObject()->getPosition());
    }

    this->g_time += fge::DurationToSecondFloat(deltaTime);

    if (this->g_fading)
    {
        auto alpha = 255 - static_cast<uint8_t>(this->g_time / 3.0f * 255.0f);
        this->g_objText.setFillColor(fge::SetAlpha(this->g_objText.getFillColor(), alpha));
        this->g_objText.setOutlineColor(fge::SetAlpha(this->g_objText.getOutlineColor(), alpha));
        if (alpha == 0)
        {
            scene.delUpdatedObject();
        }
        return;
    }

    if (this->g_time >= 0.1f)
    {
        this->g_time = 0.0f;

        this->g_fading = true;
        for (auto& character: this->g_objText.getCharacters())
        {
            if (!character.isVisible())
            {
                character.setVisibility(true);
                this->g_fading = false;

                auto const posx = this->g_objText.getGlobalBounds()._width / 2.0f;
                this->g_objText.setPosition({-posx, this->g_objText.getPosition().y});
                return;
            }
        }
    }
}
#ifndef FGE_DEF_SERVER
FGE_OBJ_DRAW_BODY(PlayerChatMessage)
{
    auto copyStates = states.copy();
    copyStates._resTransform.set(target.requestGlobalTransform(*this, copyStates._resTransform));

    this->g_objText.draw(target, copyStates);
}
#endif

void PlayerChatMessage::first(fge::Scene& scene)
{
    for (auto& character: this->g_objText.getCharacters())
    {
        character.setVisibility(false);
    }

    this->g_player = scene.getObject(this->g_playerSid);
}

char const* PlayerChatMessage::getClassName() const
{
    return "FISH_PLAYER_CHAT_MESSAGE";
}

char const* PlayerChatMessage::getReadableClassName() const
{
    return "player chat message";
}

fge::RectFloat PlayerChatMessage::getGlobalBounds() const
{
    return this->getTransform() * this->g_objText.getGlobalBounds();
}

fge::RectFloat PlayerChatMessage::getLocalBounds() const
{
    return this->g_objText.getLocalBounds();
}

//Player

#ifdef FGE_DEF_SERVER
FGE_OBJ_UPDATE_BODY(Player)
{
    FGE_OBJ_UPDATE_CALL(this->g_objAnim);
}
#else
FGE_OBJ_UPDATE_BODY(Player)
{
    FGE_OBJ_UPDATE_CALL(this->g_objAnim);
    FGE_OBJ_UPDATE_CALL(this->g_objAnimShadow);

    if (!this->g_isUserControlled)
    {
        switch (this->g_state)
        {
        case States::WALKING:
        {
            //Player movement and animation
            fge::Vector2i moveDirection{0, 0};
            auto const positionDiff = this->g_serverPosition - this->getPosition();

            std::string animationName;
            if (positionDiff.y <= -0.1f)
            {
                moveDirection.y = -1;
                animationName += "_up";
            }
            else if (positionDiff.y >= 0.1f)
            {
                moveDirection.y = 1;
                animationName += "_down";
            }

            if (positionDiff.x <= -0.1f)
            {
                moveDirection.x = -1;
                animationName += "_left";
            }
            else if (positionDiff.x >= 0.1f)
            {
                moveDirection.x = 1;
                animationName += "_right";
            }

            if (animationName.empty())
            { //Idle
                if (this->g_direction.y == -1)
                {
                    animationName += "_up";
                }
                else if (this->g_direction.y == 1)
                {
                    animationName += "_down";
                }

                if (this->g_direction.x == -1)
                {
                    animationName += "_left";
                }
                else if (this->g_direction.x == 1)
                {
                    animationName += "_right";
                }

                animationName = "idle" + animationName;
            }
            else
            { //Walking
                animationName = "walk" + animationName;
                //this->g_direction = moveDirection;
            }

            this->g_objAnim.getAnimation().setGroup(animationName);
            this->g_objAnimShadow.getAnimation().setGroup(animationName);
            break;
        }
        default:
            break;
        }

        auto const delta = fge::DurationToSecondFloat(deltaTime);
        this->setPosition(fge::ReachVector(this->getPosition(), this->g_serverPosition, F_PLAYER_SPEED * 2.0f, delta));
        return;
    }

    switch (this->g_state)
    {
    case States::WALKING:
    {
        //Player movement and animation
        std::string animationName;

        if (event.isKeyPressed(SDLK_SPACE))
        {
            animationName = this->g_objAnim.getAnimation().getGroup()->_groupName;
            animationName = animationName.substr(animationName.find('_'));
            animationName = "rod" + animationName;

            this->g_objAnim.getAnimation().setGroup(animationName);
            this->g_objAnim.getAnimation().setFrame(0);
            this->g_objAnimShadow.getAnimation().setGroup(animationName);
            this->g_objAnimShadow.getAnimation().setFrame(0);
            this->g_fishBait = scene.newObject(FGE_NEWOBJECT(FishBait, this->g_direction, this->getPosition()));

            b2Body_SetLinearVelocity(this->g_bodyId, {0.0f, 0.0f});

            Mix_PlayChannel(-1, fge::audio::gManager.getElement("swipe")->_ptr.get(), 0);
            this->g_state = States::THROWING;
            this->g_objAnim.getAnimation().setLoop(false);
            this->g_objAnimShadow.getAnimation().setLoop(false);

            if (this->g_audioWalking != -1)
            {
                Mix_HaltChannel(this->g_audioWalking);
                this->g_audioWalking = -1;
            }
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
        { //Idle
            animationName = this->g_objAnim.getAnimation().getGroup()->_groupName;
            animationName = animationName.substr(animationName.find('_'));
            animationName = "idle" + animationName;
            if (this->g_audioWalking != -1)
            {
                if (Mix_Playing(this->g_audioWalking) != 0)
                {
                    Mix_HaltChannel(this->g_audioWalking);
                }
                this->g_audioWalking = -1;
            }

            if (event.isKeyPressed(SDLK_t))
            {
                this->startChatting(event);
            }
        }
        else
        { //Walking
            if (this->g_audioWalking == -1 || Mix_Playing(this->g_audioWalking) == 0)
            {
                this->g_audioWalking =
                        Mix_PlayChannel(-1, fge::audio::gManager.getElement("walk_grass")->_ptr.get(), -1);
            }
            animationName = "walk" + animationName;
            this->g_direction = moveDirection;
        }

        b2Body_SetLinearVelocity(this->g_bodyId, {static_cast<float>(moveDirection.x) * F_PLAYER_SPEED,
                                                  static_cast<float>(moveDirection.y) * F_PLAYER_SPEED});
        this->g_objAnim.getAnimation().setGroup(animationName);
        this->g_objAnimShadow.getAnimation().setGroup(animationName);
        break;
    }
    case States::IDLE:
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
            this->g_objAnimShadow.getAnimation().setLoop(true);
        }
        break;
    case States::FISHING:
        if (event.isKeyPressed(SDLK_w) || event.isKeyPressed(SDLK_s) || event.isKeyPressed(SDLK_a) ||
            event.isKeyPressed(SDLK_d))
        {
            if (auto fishBait = this->g_fishBait.lock())
            {
                scene.delObject(fishBait->getSid());
            }

            this->g_state = States::WALKING;
            this->g_objAnim.getAnimation().setLoop(true);
            this->g_objAnimShadow.getAnimation().setLoop(true);
        }
        break;
    case States::CHATTING:
        break;
    }

    //Camera movement
    auto view = target.getView();
    view.setCenter(this->getPosition());
    target.setView(view);

    //Update position
    auto const bpos = b2Body_GetPosition(this->g_bodyId);
    this->setPosition({bpos.x, bpos.y});
    this->g_serverPosition = this->getPosition();

    //Update plan
    float distanceDown = std::numeric_limits<float>::max();
    float distanceUp = std::numeric_limits<float>::max();
    for (auto const& y: this->g_transitionYPoints)
    {
        auto const distance = y - (this->getPosition().y + this->g_objAnim.getOrigin().y / 2.0f);
        if (distance > 0.0f)
        {
            if (distance < distanceDown)
            {
                distanceDown = distance;
            }
        }
        else
        {
            if (-distance < distanceUp)
            {
                distanceUp = -distance;
            }
        }
    }
    distanceUp -=
            4.0f; //TODO: this offset is here to mitigate the effect when 2 rocks are very close, find a better solution
    if (distanceUp < distanceDown)
    {
        if (!this->g_transitionLast)
        {
            this->g_transitionLast = true;
            scene.setObjectPlanBot(this->_myObjectData.lock()->getSid());
        }
    }
    else
    {
        if (this->g_transitionLast)
        {
            this->g_transitionLast = false;
            scene.setObjectPlanTop(this->_myObjectData.lock()->getSid());
        }
    }
}
FGE_OBJ_DRAW_BODY(Player)
{
    auto copyStats = states.copy();
    copyStats._resTransform.set(target.requestGlobalTransform(*this, states._resTransform));

    this->g_objAnimShadow.draw(target, copyStats);
    this->g_objAnim.draw(target, copyStats);
}
#endif //FGE_DEF_SERVER

void Player::first(fge::Scene& scene)
{
    this->g_objAnim.setAnimation(fge::Animation{"human_1", "idle_down"});
    this->g_objAnim.getAnimation().setLoop(true);
    this->g_objAnim.centerOriginFromLocalBounds();

    this->g_objAnimShadow.setAnimation(fge::Animation{"human_1_shadow", "idle_down"});
    this->g_objAnimShadow.getAnimation().setLoop(true);
    this->g_objAnimShadow.centerOriginFromLocalBounds();
    this->g_objAnimShadow.setRotation(20.0f);
    this->g_objAnimShadow.move({4.0f, 0.0f});
    this->g_objAnimShadow.scale({0.8f, 0.7f});
    this->g_objAnimShadow.setColor({255, 255, 255, 30});

    this->g_objChatText->setFont("default");
    this->g_objChatText->setCharacterSize(44);
    this->g_objChatText->scale(0.2f);
    this->g_objChatText->setFillColor(fge::Color::White);
    this->g_objChatText->setOutlineColor(fge::Color::Black);
    this->g_objChatText->setOutlineThickness(1.0f);
    this->g_objChatText->setPosition({0.0f, -20.0f});

    this->g_objChatText->_drawMode = DrawModes::DRAW_ALWAYS_HIDDEN;

    this->g_objChatText.detach(this, FGE_SCENE_PLAN_TOP);

#ifndef FGE_DEF_SERVER
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

    if (this->_myObjectData.lock()->getContextFlags().has(fge::OBJ_CONTEXT_NETWORK))
    {
        this->allowUserControl(false);
        this->_tags.add("multiplayer");
    }

    //Build layer transition lines for the player
    this->g_transitionYPoints.clear();
    auto const tilemap = scene.getFirstObj_ByTag("map")->getObject<fge::ObjTileLayer>()->getTileMap();
    auto const mapDepthObjects = tilemap->findLayerName("DepthObjects")->get()->as<fge::TileLayer>();
    fge::Vector2i tileSize{0, 0};
    for (auto const& tile: mapDepthObjects->getTiles())
    {
        auto const gid = tile.getGid();
        if (gid != FGE_LAYER_BAD_ID)
        {
            if (tileSize.x == 0 && tileSize.y == 0)
            {
                tileSize = tile.getTileSet()->getTileSize();
            }

            auto const position =
                    mapDepthObjects->getPosition() + tile.getPosition() + static_cast<fge::Vector2f>(tileSize) / 2.0f;
            this->g_transitionYPoints.emplace_back(position.y);
        }
    }
    this->g_transitionPlan = tilemap->retrieveGeneratedTilelayerObject("DepthObjects")->getPlan();
    scene.setObjectPlan(this->_myObjectData.lock()->getSid(), this->g_transitionPlan);
#endif

    this->networkRegister();
}

void Player::callbackRegister(fge::Event& event, fge::GuiElementHandler* guiElementHandlerPtr) {}

char const* Player::getClassName() const
{
    return "FISH_PLAYER";
}
char const* Player::getReadableClassName() const
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

void Player::networkRegister()
{
    this->_netList.clear();

#ifdef FGE_DEF_SERVER
    this->_netList.pushTrivial<fge::Vector2f>(fge::DataAccessor<fge::Vector2f>{
            &this->getPosition(), [&](auto const& position) { this->setPosition(position); }});
    this->_netList.pushTrivial<fge::Vector2i>(fge::DataAccessor<fge::Vector2i>{
            &this->g_direction, [&](auto const& direction) { this->setDirection(direction); }});
    this->_netList.pushTrivial<States>(fge::DataAccessor<States>{&this->g_state});
    this->_netList.push<fge::net::NetworkTypePropertyList<std::string>>(&this->_properties, "playerId");
#else
    this->_netList.pushTrivial<fge::Vector2f>(fge::DataAccessor<fge::Vector2f>{&this->g_serverPosition});
    this->_netList.pushTrivial<fge::Vector2i>(fge::DataAccessor<fge::Vector2i>{
            &this->g_direction, [&](auto const& direction) { this->setServerDirection(direction); }});
    this->_netList.pushTrivial<States>(
            fge::DataAccessor<States>{&this->g_state, [&](auto const& stat) { this->setServerState(stat); }});
    this->_netList.push<fge::net::NetworkTypePropertyList<std::string>>(&this->_properties, "playerId")
            ->needExplicitUpdate();
#endif
}

void Player::pack(fge::net::Packet& pck)
{
    pck << this->getPosition() << this->g_direction << this->g_state
        << *this->_properties["playerId"].getPtr<std::string>(); //TODO: a bit unsafe
}
void Player::unpack(fge::net::Packet const& pck)
{
    fge::Vector2f position;
    fge::Vector2i direction;
    States stat;
    std::string playerId;

    pck >> position >> direction >> stat >> playerId;

    this->setPosition(position);
    this->setServerPosition(position);
    this->setServerDirection(direction);
    this->setServerState(stat);
    this->_properties["playerId"] = playerId;
}

Player::States Player::getState() const
{
    return this->g_state;
}
fge::Vector2i const& Player::getDirection() const
{
    return this->g_direction;
}

void Player::setDirection(fge::Vector2i const& direction)
{
    this->g_direction = direction;
}
void Player::setState(States state)
{
    this->g_state = state;
    if (state == States::IDLE)
    {
        auto animationName = this->g_objAnim.getAnimation().getGroup()->_groupName;
        animationName = animationName.substr(animationName.find('_'));
        animationName = "idle" + animationName;
        if (this->g_audioWalking != -1)
        {
            if (Mix_Playing(this->g_audioWalking) != 0)
            {
                Mix_HaltChannel(this->g_audioWalking);
            }
            this->g_audioWalking = -1;
        }

        b2Body_SetLinearVelocity(this->g_bodyId, {0.0f, 0.0f});
        this->g_objAnim.getAnimation().setGroup(animationName);
        this->g_objAnimShadow.getAnimation().setGroup(animationName);
    }
}
void Player::setServerPosition(fge::Vector2f const& position)
{
    this->g_serverPosition = position;
}
void Player::setServerDirection(fge::Vector2i const& direction)
{
    this->g_direction = direction;
}
void Player::setServerState(States state)
{
    auto& scene = *this->_myObjectData.lock()->getScene();

    if (this->g_serverState == States::WALKING && (state == States::FISHING || state == States::THROWING))
    {
        std::string animationName = this->g_objAnim.getAnimation().getGroup()->_groupName;
        animationName = animationName.substr(animationName.find('_'));
        animationName = "rod" + animationName;

        this->g_objAnim.getAnimation().setGroup(animationName);
        this->g_objAnim.getAnimation().setFrame(0);
        this->g_objAnimShadow.getAnimation().setGroup(animationName);
        this->g_objAnimShadow.getAnimation().setFrame(0);
        this->g_fishBait = scene.newObject(FGE_NEWOBJECT(FishBait, this->g_direction, this->getPosition()));

        this->g_state = States::THROWING;
        this->g_objAnim.getAnimation().setLoop(false);
        this->g_objAnimShadow.getAnimation().setLoop(false);
    }
    else if ((this->g_serverState == States::CATCHING || this->g_serverState == States::FISHING ||
              this->g_serverState == States::THROWING) &&
             state == States::WALKING)
    {
        if (auto fishBait = this->g_fishBait.lock())
        {
            scene.delObject(fishBait->getSid());
        }
        this->g_objAnim.getAnimation().setLoop(true);
        this->g_objAnimShadow.getAnimation().setLoop(true);
    }
    else if (this->g_serverState == States::FISHING && state == States::CATCHING)
    {
        if (auto fishBait = this->g_fishBait.lock())
        {
            fishBait->getObject<FishBait>()->catchingFish();
        }
    }

    this->g_serverState = state;
    this->g_state = this->g_serverState;
}

void Player::startChatting([[maybe_unused]] fge::Event& event)
{
#ifndef FGE_DEF_SERVER
    this->g_state = States::CHATTING;
    this->g_objChatText->setString(">");
    auto const posx = this->g_objChatText->getGlobalBounds()._width / 2.0f;
    this->g_objChatText->setPosition({-posx, this->g_objChatText->getPosition().y});
    this->g_objChatText->_drawMode = DrawModes::DRAW_ALWAYS_DRAWN;

    event._onTextInput.addLambda([&](fge::Event const& evt, SDL_TextInputEvent const& arg) {
        auto const key = evt.getKeyUnicode();

        //Ignore Unicode control char
        if (key < 32 || (key > 127 && key < 161))
        {
            return;
        }

        auto str = this->g_objChatText->getString();
        if (str.size() - 1 < F_NET_CHAT_MAX_SIZE)
        {
            str += key;
            this->g_objChatText->setString(std::move(str));
        }

        auto const posx = this->g_objChatText->getGlobalBounds()._width / 2.0f;
        this->g_objChatText->setPosition({-posx, this->g_objChatText->getPosition().y});
    }, this);
    event._onKeyDown.addLambda([&](fge::Event const& evt, SDL_KeyboardEvent const& arg) {
        if (arg.keysym.sym == SDLK_BACKSPACE)
        {
            auto str = this->g_objChatText->getString();
            if (str.size() > 1)
            {
                str.pop_back();
                this->g_objChatText->setString(std::move(str));
            }
        }
        else if (arg.keysym.sym == SDLK_RETURN || arg.keysym.sym == SDLK_ESCAPE)
        {
            this->g_state = States::WALKING;
            this->g_objChatText->_drawMode = DrawModes::DRAW_ALWAYS_HIDDEN;
            evt._onTextInput.delSub(this);
            evt._onKeyDown.delSub(this);
            auto str = this->g_objChatText->getString();
            if (this->g_objChatText->getString().size() > 1)
            {
                str.erase(0, 1); //Remove the '>' character
                gGameHandler->pushChatEvent(str.cpp_str());
            }
        }

        auto const posx = this->g_objChatText->getGlobalBounds()._width / 2.0f;
        this->g_objChatText->setPosition({-posx, this->g_objChatText->getPosition().y});
    }, this);
#endif // FGE_DEF_SERVER
}

void Player::boxMove(fge::Vector2f const& move)
{
    if (this->g_isUserControlled)
    {
#ifndef FGE_DEF_SERVER
        b2Body_SetTransform(this->g_bodyId, {this->getPosition().x + move.x, this->getPosition().y + move.y},
                            b2MakeRot(0.0f));
#endif
    }
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

        if (auto fishBait = this->g_fishBait.lock())
        {
            fishBait->getObject<FishBait>()->catchingFish();
        }
    }
}
void Player::endCatchingFish()
{
    if (this->g_state == States::CATCHING)
    {
        if (auto fishBait = this->g_fishBait.lock())
        {
            this->_myObjectData.lock()->getScene()->delObject(fishBait->getSid());
        }
        this->g_objAnim.getAnimation().setLoop(true);
        this->g_objAnimShadow.getAnimation().setLoop(true);
        this->g_state = States::WALKING;
    }
}

void Player::allowUserControl(bool allow)
{
    this->g_isUserControlled = allow;
    if (!allow)
    {
#ifndef FGE_DEF_SERVER
        if (b2Body_IsValid(this->g_bodyId))
        {
            b2DestroyBody(this->g_bodyId);
        }
#endif
    }
}
