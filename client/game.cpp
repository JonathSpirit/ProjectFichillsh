#include "game.hpp"
#include "../share/player.hpp"
#include "FastEngine/C_random.hpp"
#include "FastEngine/manager/audio_manager.hpp"
#include "fish.hpp"
#include <iostream>

//GameHandler

GameHandler::GameHandler(fge::Scene& scene, fge::net::ClientSideNetUdp& network) :
        g_scene(&scene),
        g_network(&network)
{
#if F_TESTING_MINIGAME == 1
    this->g_fishCountDown = 1.0f;
#else
    this->g_fishCountDown = fge::_random.range(F_GAME_FISH_COUNTDOWN_MIN, F_GAME_FISH_COUNTDOWN_MAX);
#endif
}
GameHandler::~GameHandler()
{
    b2DestroyWorld(this->g_bworld);
}

void GameHandler::createWorld()
{
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = {0.0f, 0.0f};
    this->g_bworld = b2CreateWorld(&worldDef);
}
b2WorldId GameHandler::getWorld() const
{
    return this->g_bworld;
}
void GameHandler::pushStaticCollider(fge::RectFloat const& rect)
{
    b2BodyDef groundBodyDef = b2DefaultBodyDef();

    auto const position = rect.getPosition() + rect.getSize() / 2.0f;
    groundBodyDef.position = {position.x, position.y};
    groundBodyDef.type = b2_staticBody;

    b2BodyId bodyId = b2CreateBody(this->g_bworld, &groundBodyDef);

    b2Polygon groundBox = b2MakeBox(rect._width / 2.0f, rect._height / 2.0f);

    b2ShapeDef groundShapeDef = b2DefaultShapeDef();
    b2CreatePolygonShape(bodyId, &groundShapeDef, &groundBox);
}
void GameHandler::updateWorld()
{
    constexpr float timeStep = 1.0f / 60.0f;
    constexpr int subStepCount = 4;

    b2World_Step(this->g_bworld, timeStep, subStepCount);
}

Player* GameHandler::getPlayer() const
{
    if (auto player = this->g_scene->getFirstObj_ByTag("player"))
    { //Should be always valid
        return player->getObject<Player>();
    }
    return nullptr;
}
fge::Scene& GameHandler::getScene() const
{
    return *this->g_scene;
}

void GameHandler::update(fge::DeltaTime const& deltaTime)
{
    this->updateWorld();

    this->g_checkTime += deltaTime;
    if (this->g_checkTime >= std::chrono::milliseconds(F_GAME_CHECK_TIME_MS))
    {
        auto player = this->getPlayer();

        if (player->isFishing())
        {
            if (--this->g_fishCountDown == 0)
            {
#if F_TESTING_MINIGAME == 1
                this->g_fishCountDown = 1.0f;
#else
                this->g_fishCountDown = fge::_random.range(F_GAME_FISH_COUNTDOWN_MIN, F_GAME_FISH_COUNTDOWN_MAX);
#endif
                this->g_scene->newObject<Minigame>({FGE_SCENE_PLAN_HIGH_TOP + 1});
            }
        }

        this->g_checkTime = std::chrono::milliseconds(0);
    }
}

void GameHandler::pushCaughtFishEvent(std::string const& fishName) const
{
    auto& packet = this->g_network->startReturnEvent(fge::net::ReturnEvents::REVT_CUSTOM);
    packet->packet() << StatEvents::CAUGHT_FISH << fishName;
    this->g_network->endReturnEvent();
}
void GameHandler::pushChatEvent(std::string const& chat) const
{
    if (chat.size() > F_NET_CHAT_MAX_SIZE)
    {
        return;
    }

    auto& packet = this->g_network->startReturnEvent(fge::net::ReturnEvents::REVT_CUSTOM);
    packet->packet() << StatEvents::PLAYER_CHAT << chat;
    this->g_network->endReturnEvent();
}

bool GameHandler::loadPlayerCollectionFromFile()
{
    this->g_fishPlayerCollection.clear();

    nlohmann::json jsonFish;
    if (!fge::LoadJsonFromFile(F_COLLECTION_FILE, jsonFish))
    {
        return false;
    }

    for (auto const& [name, instance]: jsonFish.items())
    {
        auto const fishInstance = instance.get<FishInstance>();
        this->g_fishPlayerCollection[name] = fishInstance;
        if (name != fishInstance._name)
        {
            return false;
        }
    }

    return true;
}
void GameHandler::savePlayerCollectionToFile() const
{
    nlohmann::json jsonFish;

    for (auto const& [name, instance]: this->g_fishPlayerCollection)
    {
        jsonFish[name] = instance;
    }

    fge::SaveJsonToFile(F_COLLECTION_FILE, jsonFish);
}
GameHandler::FishCollectionData const& GameHandler::getFishPlayerCollection() const
{
    return this->g_fishPlayerCollection;
}
GameHandler::NewRecords_t GameHandler::addFishToPlayerCollection(FishInstance const& fish)
{
    auto const it = this->g_fishPlayerCollection.find(fish._name);
    if (it == this->g_fishPlayerCollection.end())
    {
        this->g_fishPlayerCollection.emplace(fish._name, fish);
        this->savePlayerCollectionToFile();
        return RECORD_ALL;
    }

    NewRecords_t newRecords = RECORD_NONE;
    if (fish._length > it->second._length)
    {
        //New record for length
        it->second._length = fish._length;
        newRecords |= RECORD_LENGTH;
    }

    if (fish._weight > it->second._weight)
    {
        //New record for weight
        it->second._weight = fish._weight;
        newRecords |= RECORD_WEIGHT;
    }

    if (fish._starCount > it->second._starCount)
    {
        //New record for star count
        it->second._starCount = fish._starCount;
        newRecords |= RECORD_STARS;
    }

    if (newRecords != RECORD_NONE)
    {
        this->savePlayerCollectionToFile();
    }
    return newRecords;
}

void GameHandler::openPlayerCollection()
{
    if (this->isPlayerCollectionOpen())
    {
        return;
    }

    auto player = this->getPlayer();
    if (player->getState() != Player::States::WALKING)
    {
        return;
    }
    player->setState(Player::States::IDLE);

    this->g_scene->newObject<FishCollection>({FGE_SCENE_PLAN_HIGH_TOP + 1});
}
bool GameHandler::isPlayerCollectionOpen() const
{
    return this->g_scene->getFirstObj_ByClass("FISH_COLLECTION") != nullptr;
}

std::unique_ptr<GameHandler> gGameHandler;

//Minigame

FGE_OBJ_UPDATE_BODY(Minigame)
{
    auto const delta = fge::DurationToSecondFloat(deltaTime);
    this->g_currentTime += delta;

    //Update slider
    if (event.isKeyPressed(SDLK_w))
    {
        this->g_sliderVelocity = std::clamp<float>(this->g_sliderVelocity - F_MINIGAME_SLIDER_ACCELERATION * delta,
                                                   -F_MINIGAME_SLIDER_MAX_VELOCITY, F_MINIGAME_SLIDER_MAX_VELOCITY);
    }
    else if (event.isKeyPressed(SDLK_s))
    {
        this->g_sliderVelocity = std::clamp<float>(this->g_sliderVelocity + F_MINIGAME_SLIDER_ACCELERATION * delta,
                                                   -F_MINIGAME_SLIDER_MAX_VELOCITY, F_MINIGAME_SLIDER_MAX_VELOCITY);
    }
    else
    {
        this->g_sliderVelocity =
                std::clamp<float>(this->g_sliderVelocity + F_MINIGAME_SLIDER_FRICTION * delta *
                                                                   (this->g_sliderVelocity > 0.0f ? -1.0f : 1.0f),
                                  -F_MINIGAME_SLIDER_MAX_VELOCITY, F_MINIGAME_SLIDER_MAX_VELOCITY);
    }

    this->g_gaugeSlider.move({0.0f, this->g_sliderVelocity * delta});
    if (this->g_gaugeSlider.getPosition().y < this->g_gauge.getPosition().y - this->g_gauge.getSize().y / 2.0f)
    {
        this->g_gaugeSlider.setPosition({this->g_gaugeSlider.getPosition().x,
                                         this->g_gauge.getPosition().y - this->g_gauge.getSize().y / 2.0f});
        this->g_sliderVelocity = 0.0f;
    }
    else if (this->g_gaugeSlider.getPosition().y > this->g_gauge.getPosition().y + this->g_gauge.getSize().y / 2.0f)
    {
        this->g_gaugeSlider.setPosition({this->g_gaugeSlider.getPosition().x,
                                         this->g_gauge.getPosition().y + this->g_gauge.getSize().y / 2.0f});
        this->g_sliderVelocity = 0.0f;
    }

    //Check if fish is inside the gauge slider
    auto const sliderBounds = this->g_gaugeSlider.getGlobalBounds();
    auto fishBounds = this->g_fish.getGlobalBounds();
    fishBounds._y += fishBounds._height / 4.0f;
    fishBounds._height /= 2.0f;
    float posXeffect = 0.0f;
    bool caughting = false;
    if (auto rect = sliderBounds.findIntersection(fishBounds))
    {
        if (rect->getSize().y >= fishBounds.getSize().y - 0.02f)
        { //Fish is inside the slider
            posXeffect = fge::_random.range(-2.0f, 2.0f);
            caughting = true;
        }
    }

    if (!caughting)
    { //Loosing life
        for (std::size_t i = 0; i < this->g_hearts.size(); ++i)
        {
            auto& heart = this->g_hearts[i];
            if (heart.getScale().x > 0.0f)
            {
                heart.scale(1.0f - F_MINIGAME_LOOSING_HEARTS_SPEED * delta);
                if (heart.getScale().x <= 0.1f)
                { //Loosing a heart
                    heart.setScale(0.0f);
                    Mix_PlayChannel(-1, fge::audio::gManager.getElement("loose_life")->_ptr.get(), 0);
                    if (i == this->g_hearts.size() - 1)
                    { //Lost the last heart
                        scene.delUpdatedObject();
                        Mix_PlayChannel(-1, fge::audio::gManager.getElement("loose_fish")->_ptr.get(), 0);
                        return;
                    }
                }
                break;
            }
        }
    }
    else
    { //Fish loosing time
        this->g_fishRemainingTime -= delta;
        if (this->g_fishRemainingTime <= 0.0f)
        {
            Mix_PlayChannel(-1, fge::audio::gManager.getElement("victory_fish")->_ptr.get(), 0);

            auto const result = gGameHandler->addFishToPlayerCollection(this->g_fishReward);

            scene.newObject<FishAward>({FGE_SCENE_PLAN_HIGH_TOP + 2}, this->g_fishReward, result);
            gGameHandler->pushCaughtFishEvent(this->g_fishReward._name);

            scene.delUpdatedObject();
            return;
        }
    }

    //Update fish position
    auto const posy = this->g_sinusFunction(this->g_currentTime);
    this->g_fish.setPosition({this->g_gauge.getPosition().x + posXeffect, posy});
}

FGE_OBJ_DRAW_BODY(Minigame)
{
    auto const viewBackup = target.getView();
    target.setView(target.getDefaultView());

    auto copyStates = states.copy();
    copyStates._resTransform.set(target.requestGlobalTransform(*this, copyStates._resTransform));

    this->g_frame.draw(target, copyStates);
    this->g_gauge.draw(target, copyStates);
    this->g_gaugeSlider.draw(target, copyStates);
    this->g_fish.draw(target, copyStates);

    for (auto const& heart: this->g_hearts)
    {
        heart.draw(target, copyStates);
    }

    target.setView(viewBackup);
}

void Minigame::first(fge::Scene& scene)
{
    this->_drawMode = DrawModes::DRAW_ALWAYS_DRAWN;

    this->g_currentTime = 0.0f;

    this->g_gauge.setSize({20.0f, 200.0f});
    this->g_gauge.setFillColor(fge::Color::Transparent);
    this->g_gauge.setOutlineColor(fge::Color::Black);
    this->g_gauge.setOutlineThickness(1.0f);
    this->g_gauge.centerOriginFromLocalBounds();

    this->g_gaugeSlider.setSize({20.0f, 50.0f});
    this->g_gaugeSlider.setFillColor(fge::Color::Green);
    this->g_gaugeSlider.centerOriginFromLocalBounds();

    this->g_frame.setTexture("fishingFrame");
    this->g_frame.scale(5.0f);
    this->g_frame.centerOriginFromLocalBounds();

    this->g_fish.setTexture("fishingIcon");
    this->g_fish.centerOriginFromLocalBounds();

    //Generate fish reward
    this->g_fishReward = gFishManager.generateRandomFish();
    auto const fish = gFishManager.getElement(this->g_fishReward._name);

    //Generate difficulty
    this->g_difficulty = F_MINIGAME_DIFFICULTY_START;
    switch (fish->_ptr->_rarity)
    {
    case FishData::Rarity::COMMON:
        this->g_difficulty += F_MINIGAME_DIFFICULTY_RARITY_COMMON;
        break;
    case FishData::Rarity::UNCOMMON:
        this->g_difficulty += F_MINIGAME_DIFFICULTY_RARITY_UNCOMMON;
        break;
    case FishData::Rarity::RARE:
        this->g_difficulty += F_MINIGAME_DIFFICULTY_RARITY_RARE;
        break;
    }
    this->g_difficulty *= static_cast<float>(this->g_fishReward._starCount);
    this->g_difficulty += fge::_random.range(0.0f, F_MINIGAME_DIFFICULTY_RANDOM_VARIATION);

    //Setup hearts
    for (std::size_t i = 0; i < this->g_hearts.size(); ++i)
    {
        this->g_hearts[i].setTexture("hearts");
        this->g_hearts[i].setTextureRect({{0, 0}, {16, 16}});
        this->g_hearts[i].centerOriginFromLocalBounds();
        this->g_hearts[i].setPosition({-60.0f, -20.0f + 60.0f * static_cast<float>(i)});
        this->g_hearts[i].scale(7.0f);
    }

    this->g_fishRemainingTime = fge::ConvertRange(this->g_difficulty, F_MINIGAME_DIFFICULTY_START,
                                                  F_MINIGAME_DIFFICULTY_MAX, F_MINIGAME_TIME_MIN, F_MINIGAME_TIME_MAX);

    //Generate fish positions
    auto sinusQuantity = static_cast<std::size_t>(
            std::round(fge::ConvertRange(this->g_difficulty, F_MINIGAME_DIFFICULTY_START, F_MINIGAME_DIFFICULTY_MAX,
                                         F_MINIGAME_SINUS_QUANTITY_MIN, F_MINIGAME_SINUS_QUANTITY_MAX)));

    std::vector<float> sinusValues(sinusQuantity, 0.0f);
    std::vector<float> sinusOffset(sinusQuantity, 0.0f);
    for (std::size_t i = 0; i < sinusValues.size(); ++i)
    {
        auto frequency = fge::ConvertRange(this->g_difficulty, F_MINIGAME_DIFFICULTY_START, F_MINIGAME_DIFFICULTY_MAX,
                                           F_MINIGAME_SINUS_FREQ_MIN, F_MINIGAME_SINUS_FREQ_MAX);
        frequency += fge::_random.range(-F_MINIGAME_SINUS_FREQ_VARIATION, F_MINIGAME_SINUS_FREQ_VARIATION);

        sinusValues[i] = 2.0f * static_cast<float>(FGE_MATH_PI) * frequency;
        sinusOffset[i] = fge::_random.range(0.0f, 30.0f);

        std::cout << "Sinus " << i << ": frequency: " << frequency << " offset: " << sinusOffset[i] << std::endl;
    }

    this->g_sinusFunction = [sinusValues, sinusQuantity, sinusOffset, this](float const time) {
        float value = 0.0f;
        for (std::size_t i = 0; i < sinusValues.size(); ++i)
        {
            value += std::sin((time + sinusOffset[i]) * sinusValues[i]) * (1.0f / static_cast<float>(sinusQuantity));
        }
        return value * this->g_gauge.getSize().y / 2.0f + this->g_gauge.getPosition().y / 2.0f;
    };

    auto target = scene.getLinkedRenderTarget();
    this->setPosition({target->getSize().x / 2.0f, target->getSize().y / 2.0f});

    if (auto obj = scene.getFirstObj_ByTag("topMap"))
    {
        obj->getObject()->_drawMode = DrawModes::DRAW_ALWAYS_DRAWN;
    }
    auto player = gGameHandler->getPlayer();
    player->catchingFish();
}
void Minigame::removed(fge::Scene& scene)
{
    if (auto obj = scene.getFirstObj_ByTag("topMap"))
    {
        obj->getObject()->_drawMode = DrawModes::DRAW_ALWAYS_HIDDEN;
    }
    auto player = gGameHandler->getPlayer();
    player->endCatchingFish();
}

void Minigame::callbackRegister(fge::Event& event, fge::GuiElementHandler* guiElementHandlerPtr) {}

char const* Minigame::getClassName() const
{
    return "FISH_MINIGAME";
}

char const* Minigame::getReadableClassName() const
{
    return "minigame";
}

fge::RectFloat Minigame::getGlobalBounds() const
{
    return Object::getGlobalBounds();
}

fge::RectFloat Minigame::getLocalBounds() const
{
    return Object::getLocalBounds();
}

//FishAward

FishAward::FishAward(FishInstance const& fishReward, GameHandler::NewRecords_t newRecords)
{
    this->g_newRecords = newRecords;
    this->g_fishReward = fishReward;

    auto const fish = gFishManager.getElement(this->g_fishReward._name);
    this->g_fish.setTexture(fish->_ptr->_textureName);
    this->g_fish.setTextureRect(fish->_ptr->_textureRect);
    this->g_fish.scale(20.0f);
    this->g_fish.centerOriginFromLocalBounds();
}

void FishAward::update(fge::RenderTarget& target, fge::Event& event, fge::DeltaTime const& deltaTime, fge::Scene& scene)
{
    auto const delta = fge::DurationToSecondFloat(deltaTime);
    this->g_currentTime += delta;
    auto sinus = std::sin(this->g_currentTime * 2.0f);
    this->g_fish.setRotation(sinus * 10.0f);

    this->setPosition(fge::ReachVector(this->getPosition(), this->g_positionGoal, 300.0f, delta));

    for (std::size_t i = 0; i < this->g_stars.getSpriteCount(); ++i)
    {
        float const value = 2.0f * std::sin((this->g_currentTime + 0.2f * static_cast<float>(i)) * 2.0f);

        auto& transformable = *this->g_stars.getTransformable(i);
        transformable.setOrigin({transformable.getOrigin().x, 8.0f + value});
    }

    if (this->g_currentTime >= 5.0f)
    { //Start to fade out
        auto const alphaFloat = static_cast<float>(this->g_fish.getColor()._a) - 0.5f * delta;
        uint8_t const alpha = alphaFloat < 0.0f ? 0 : static_cast<uint8_t>(alphaFloat);

        this->g_fish.setColor(fge::SetAlpha(this->g_fish.getColor(), alpha));
        for (std::size_t i = 0; i < this->g_stars.getSpriteCount(); ++i)
        {
            this->g_stars.setColor(i, fge::Color(255, 255, 255, alpha));
        }
        this->g_text.setFillColor(fge::SetAlpha(this->g_text.getFillColor(), alpha));
        this->g_text.setOutlineColor(fge::SetAlpha(this->g_text.getOutlineColor(), alpha));
        this->g_textFishAttributes.setFillColor(fge::SetAlpha(this->g_textFishAttributes.getFillColor(), alpha));
        this->g_textFishAttributes.setOutlineColor(fge::SetAlpha(this->g_textFishAttributes.getOutlineColor(), alpha));
        this->g_newRecordsText.setFillColor(fge::SetAlpha(this->g_newRecordsText.getFillColor(), alpha));
        this->g_newRecordsText.setOutlineColor(fge::SetAlpha(this->g_newRecordsText.getOutlineColor(), alpha));
        this->g_newStarsRecordText.setFillColor(fge::SetAlpha(this->g_newStarsRecordText.getFillColor(), alpha));
        this->g_newStarsRecordText.setOutlineColor(fge::SetAlpha(this->g_newStarsRecordText.getOutlineColor(), alpha));

        if (this->g_fish.getColor()._a == 0)
        {
            scene.delUpdatedObject();
        }
    }
}

void FishAward::draw(fge::RenderTarget& target, fge::RenderStates const& states) const
{
    auto const backupView = target.getView();
    target.setView(target.getDefaultView());

    auto copyStates = states.copy();
    copyStates._resTransform.set(target.requestGlobalTransform(*this, copyStates._resTransform));

    this->g_fish.draw(target, copyStates);
    this->g_stars.draw(target, copyStates);
    this->g_text.draw(target, copyStates);
    this->g_textFishAttributes.draw(target, copyStates);
    this->g_newRecordsText.draw(target, copyStates);
    this->g_newStarsRecordText.draw(target, copyStates);

    target.setView(backupView);
}

void FishAward::first(fge::Scene& scene)
{
    this->_drawMode = DrawModes::DRAW_ALWAYS_DRAWN;

    auto target = scene.getLinkedRenderTarget();
    this->g_positionGoal = {static_cast<float>(target->getSize().x) / 2.0f,
                            static_cast<float>(target->getSize().y) / 2.0f};
    this->setPosition({this->g_positionGoal.x, static_cast<float>(target->getSize().y) * 1.2f});

    constexpr float starsInterval = 80.0f;
    fge::RectInt starsTextureRect{{0, 0}, {16, 16}};

    if (this->g_fishReward._starCount > 4)
    {
        starsTextureRect._y = 32;
    }
    else if (this->g_fishReward._starCount > 2)
    {
        starsTextureRect._y = 16;
    }

    auto const rarity = gFishManager.getElement(this->g_fishReward._name)->_ptr->_rarity;
    switch (rarity)
    {
    case FishData::Rarity::COMMON:
        starsTextureRect._x = 0;
        break;
    case FishData::Rarity::UNCOMMON:
        starsTextureRect._x = 16;
        break;
    case FishData::Rarity::RARE:
        starsTextureRect._x = 32;
        break;
    }

    this->g_stars.setTexture("stars");
    for (std::size_t i = 0; i < this->g_fishReward._starCount; ++i)
    {
        auto& transform = this->g_stars.addSprite(starsTextureRect);
        transform.move({starsInterval * static_cast<float>(i), 0.0f});
        transform.setOrigin({0.0f, 8.0f});
        transform.scale(5.0f);
    }
    this->g_stars.setPosition({-static_cast<float>(this->g_fishReward._starCount) * starsInterval * 0.5f, 120.0f});

    this->g_text.setFont("default");
    this->g_text.setCharacterSize(40);
    this->g_text.setFillColor(fge::Color::White);
    this->g_text.setOutlineColor(fge::Color::Black);
    this->g_text.setOutlineThickness(1.8f);

    this->g_text.setString("You caught a fish!\n   -> " + this->g_fish.getTexture().getName());
    this->g_text.centerOriginFromLocalBounds();
    this->g_text.setPosition({0.0f, 200.0f});

    this->g_textFishAttributes.setFont("default");
    this->g_textFishAttributes.setCharacterSize(30);
    this->g_textFishAttributes.setFillColor(fge::Color::White);
    this->g_textFishAttributes.setOutlineColor(fge::Color::Black);
    this->g_textFishAttributes.setOutlineThickness(1.0f);

    this->g_textFishAttributes.setString(std::format("Weight: {:.3f} kg\t\tLength: {:.2f} cm",
                                                     this->g_fishReward._weight, this->g_fishReward._length));
    this->g_textFishAttributes.centerOriginFromLocalBounds();
    this->g_textFishAttributes.setPosition({0.0f, 250.0f});

    this->g_newRecordsText.setFont("default");
    this->g_newRecordsText.setCharacterSize(25);
    this->g_newRecordsText.setFillColor(fge::Color::Yellow);
    this->g_newRecordsText.setOutlineColor(fge::Color::Black);
    this->g_newRecordsText.setOutlineThickness(1.0f);

    tiny_utf8::string newRecordsStr;
    if ((this->g_newRecords & GameHandler::RECORD_WEIGHT) > 0)
    {
        newRecordsStr += "*new*";
    }
    if ((this->g_newRecords & GameHandler::RECORD_LENGTH) > 0)
    {
        newRecordsStr += "           *new*";
    }
    this->g_newRecordsText.setString(std::move(newRecordsStr));
    this->g_newRecordsText.centerOriginFromLocalBounds();
    this->g_newRecordsText.setPosition({0.0f, 270.0f});

    this->g_newStarsRecordText.setFont("default");
    this->g_newStarsRecordText.setCharacterSize(25);
    this->g_newStarsRecordText.setFillColor(fge::Color::Yellow);
    this->g_newStarsRecordText.setOutlineColor(fge::Color::Black);
    this->g_newStarsRecordText.setOutlineThickness(1.0f);
    if ((this->g_newRecords & GameHandler::RECORD_STARS) > 0)
    {
        this->g_newStarsRecordText.setString("*new*");
    }
    else
    {
        this->g_newStarsRecordText.setString("");
    }
    this->g_newStarsRecordText.setPosition(
            {static_cast<float>(this->g_fishReward._starCount) * starsInterval / 2.0f, 120.0f});
}

void FishAward::callbackRegister(fge::Event& event, fge::GuiElementHandler* guiElementHandlerPtr) {}

char const* FishAward::getClassName() const
{
    return "FISH_AWARD";
}

char const* FishAward::getReadableClassName() const
{
    return "fish award";
}

fge::RectFloat FishAward::getGlobalBounds() const
{
    return Object::getGlobalBounds();
}

fge::RectFloat FishAward::getLocalBounds() const
{
    return Object::getLocalBounds();
}

//MultiplayerFishAward

MultiplayerFishAward::MultiplayerFishAward(std::string const& fishName, fge::Vector2f const& position)
{
    auto const fish = gFishManager.getElement(fishName);
    this->g_fish.setTexture(fish->_ptr->_textureName);
    this->g_fish.setTextureRect(fish->_ptr->_textureRect);
    this->g_fish.scale(0.6f);
    this->g_fish.centerOriginFromLocalBounds();

    this->setPosition(position);
}

void MultiplayerFishAward::update(fge::RenderTarget& target,
                                  fge::Event& event,
                                  fge::DeltaTime const& deltaTime,
                                  fge::Scene& scene)
{
    auto const delta = fge::DurationToSecondFloat(deltaTime);
    this->g_currentTime += delta;
    auto sinus = std::sin(this->g_currentTime * 2.0f);
    this->g_fish.setRotation(sinus * 10.0f);

    this->setPosition(fge::ReachVector(this->getPosition(), this->g_positionGoal, 2.0f, delta));

    if (this->g_currentTime >= 5.0f)
    { //Start to fade out
        auto const alphaFloat = static_cast<float>(this->g_fish.getColor()._a) - 0.5f * delta;
        uint8_t const alpha = alphaFloat < 0.0f ? 0 : static_cast<uint8_t>(alphaFloat);

        this->g_fish.setColor(fge::SetAlpha(this->g_fish.getColor(), alpha));
        this->g_text.setFillColor(fge::SetAlpha(this->g_text.getFillColor(), alpha));
        this->g_text.setOutlineColor(fge::SetAlpha(this->g_text.getOutlineColor(), alpha));

        if (this->g_fish.getColor()._a == 0)
        {
            scene.delUpdatedObject();
        }
    }
}

void MultiplayerFishAward::draw(fge::RenderTarget& target, fge::RenderStates const& states) const
{
    auto copyStates = states.copy();
    copyStates._resTransform.set(target.requestGlobalTransform(*this, copyStates._resTransform));

    this->g_fish.draw(target, copyStates);
    this->g_text.draw(target, copyStates);
}

void MultiplayerFishAward::first(fge::Scene& scene)
{
    this->_drawMode = DrawModes::DRAW_ALWAYS_DRAWN;

    this->g_positionGoal = this->getPosition() + fge::Vector2f{0.0f, -6.0f};

    this->g_text.setFont("default");
    this->g_text.setCharacterSize(40);
    this->g_text.scale(0.3f);
    this->g_text.setString(this->g_fish.getTexture().getName());
    this->g_text.setFillColor(fge::Color::White);
    this->g_text.setOutlineColor(fge::Color::Black);
    this->g_text.setOutlineThickness(1.0f);
    this->g_text.centerOriginFromLocalBounds();
    this->g_text.setPosition({0.0f, 8.0f});
}

void MultiplayerFishAward::callbackRegister(fge::Event& event, fge::GuiElementHandler* guiElementHandlerPtr) {}

char const* MultiplayerFishAward::getClassName() const
{
    return "MFISH_AWARD";
}

char const* MultiplayerFishAward::getReadableClassName() const
{
    return "multiplayer fish award";
}

fge::RectFloat MultiplayerFishAward::getGlobalBounds() const
{
    return Object::getGlobalBounds();
}

fge::RectFloat MultiplayerFishAward::getLocalBounds() const
{
    return Object::getLocalBounds();
}

//FishCollection

FGE_OBJ_UPDATE_BODY(FishCollection) {}

FGE_OBJ_DRAW_BODY(FishCollection)
{
    auto const viewBackup = target.getView();
    target.setView(target.getDefaultView());

    auto copyStates = states.copy();
    copyStates._resTransform.set(target.requestGlobalTransform(*this, copyStates._resTransform));

    this->g_book.draw(target, copyStates);
    this->g_buttonNextPage->draw(target, copyStates);
    this->g_buttonLastPage->draw(target, copyStates);
    this->g_buttonClose->draw(target, copyStates);

    auto const startIndex = this->g_currentPage * (F_COLLECTION_MAX_COL * F_COLLECTION_MAX_ROW);
    auto const endIndex = startIndex + F_COLLECTION_MAX_COL * F_COLLECTION_MAX_ROW * 2;

    for (std::size_t i = startIndex; i < endIndex; ++i)
    {
        if (i >= this->g_fishEntries.size())
        {
            break;
        }

        auto const& entry = this->g_fishEntries[i];
        entry._sprite.draw(target, copyStates);
    }
    for (std::size_t i = startIndex; i < endIndex; ++i)
    {
        if (i >= this->g_fishEntries.size())
        {
            break;
        }

        auto const& entry = this->g_fishEntries[i];
        entry._textName.draw(target, copyStates);
        entry._textAttributes.draw(target, copyStates);
    }

    target.setView(viewBackup);
}

void FishCollection::first(fge::Scene& scene)
{
    this->_drawMode = DrawModes::DRAW_ALWAYS_DRAWN;

    auto const screenCenter = static_cast<fge::Vector2f>(scene.getLinkedRenderTarget()->getSize()) / 2.0f;

    this->g_book.setTexture("book_1");
    this->g_book.scale(14.0f);
    this->g_book.centerOriginFromLocalBounds();
    this->g_book.setPosition(screenCenter);

    this->g_buttonClose->setTexture("close_1");
    this->g_buttonClose->setPosition(screenCenter + fge::Vector2f{550.0f, -320.0f});
    this->g_buttonClose->scale(4.0f);
    this->g_buttonClose->ownViewExplicitlySetDefaultView(true);
    this->g_buttonClose->centerOriginFromLocalBounds();
    this->g_buttonClose->_onButtonPressed.addLambda([&](fge::ObjButton* button) {
        if (auto const objectData = this->_myObjectData.lock())
        {
            gGameHandler->getPlayer()->setState(Player::States::WALKING);
            objectData->getScene()->delObject(objectData->getSid());
        }
    });

    this->g_buttonNextPage->setTexture("arrows");
    this->g_buttonNextPage->setTextureOnRect({{16, 0}, {16, 16}});
    this->g_buttonNextPage->setTextureOffRect({{16, 0}, {16, 16}});
    this->g_buttonNextPage->setPosition(screenCenter + fge::Vector2f{560.0f, 170.0f});
    this->g_buttonNextPage->scale(5.0f);
    this->g_buttonNextPage->ownViewExplicitlySetDefaultView(true);
    this->g_buttonNextPage->centerOriginFromLocalBounds();
    this->g_buttonNextPage->_onButtonPressed.addLambda([&](fge::ObjButton* button) {
        if (this->g_currentPage + 2 >= this->g_maxPage)
        {
            return;
        }
        this->g_currentPage += 2;
    });

    this->g_buttonLastPage->setTexture("arrows");
    this->g_buttonLastPage->setTextureOnRect({{0, 0}, {16, 16}});
    this->g_buttonLastPage->setTextureOffRect({{0, 0}, {16, 16}});
    this->g_buttonLastPage->setPosition(screenCenter + fge::Vector2f{-560.0f, 170.0f});
    this->g_buttonLastPage->scale(5.0f);
    this->g_buttonLastPage->ownViewExplicitlySetDefaultView(true);
    this->g_buttonLastPage->centerOriginFromLocalBounds();
    this->g_buttonLastPage->_onButtonPressed.addLambda([&](fge::ObjButton* button) {
        this->g_currentPage -= std::min(this->g_currentPage, static_cast<std::size_t>(2));
    });

    auto const& fishCollection = gGameHandler->getFishPlayerCollection();
    std::size_t index = 0;
    bool leftPage = true;
    for (auto const& [name, instance]: fishCollection)
    {
        auto& entry = this->g_fishEntries.emplace_back();

        std::cout << "FishCollection: adding fish " << name << " at index " << index << std::endl;
        std::cout << "\tcol: " << (index % F_COLLECTION_MAX_COL) << " row: " << (index / F_COLLECTION_MAX_COL)
                  << std::endl;

        auto fishData = gFishManager.getElement(instance._name);

        fge::Vector2f positionOffset = {-450.0f, -240.0f};
        if (!leftPage)
        {
            positionOffset.x = 200.0f;
        }

        entry._sprite.setTexture(fishData->_ptr->_textureName);
        entry._sprite.setTextureRect(fishData->_ptr->_textureRect);
        entry._sprite.centerOriginFromLocalBounds();
        entry._sprite.scale(8.0f);
        entry._sprite.setPosition(screenCenter + positionOffset +
                                  fge::Vector2f{static_cast<float>(index % F_COLLECTION_MAX_COL) * 240.0f,
                                                static_cast<float>(index / F_COLLECTION_MAX_COL) * 150.0f});

        entry._textName.setString(instance._name);
        entry._textName.setFont("default");
        entry._textName.setCharacterSize(24);
        entry._textName.setFillColor(fge::Color::Black);
        entry._textName.setPosition(entry._sprite.getPosition() + fge::Vector2f{-70.0f, 30.0f});

        entry._textAttributes.setString(std::format("Length: {:.2f} cm\nWeight: {:.3f} kg\nStars: {}", instance._length,
                                                    instance._weight, instance._starCount));
        entry._textAttributes.setFont("default");
        entry._textAttributes.setCharacterSize(24);
        entry._textAttributes.setFillColor(fge::Color::Black);
        entry._textAttributes.setPosition(entry._sprite.getPosition() + fge::Vector2f{-70.0f, 50.0f});

        ++index;

        if (index >= F_COLLECTION_MAX_COL * F_COLLECTION_MAX_ROW)
        {
            leftPage = !leftPage;
            index = 0;
        }
    }
    this->g_maxPage = fishCollection.size() / (F_COLLECTION_MAX_COL * F_COLLECTION_MAX_ROW);
    this->g_currentPage = 0;

    std::cout << "current: " << this->g_currentPage << " max: " << this->g_maxPage << std::endl;
}

void FishCollection::callbackRegister(fge::Event& event, fge::GuiElementHandler* guiElementHandlerPtr)
{
    this->g_buttonNextPage->callbackRegister(event, guiElementHandlerPtr);
    this->g_buttonLastPage->callbackRegister(event, guiElementHandlerPtr);
    this->g_buttonClose->callbackRegister(event, guiElementHandlerPtr);
}

char const* FishCollection::getClassName() const
{
    return "FISH_COLLECTION";
}

char const* FishCollection::getReadableClassName() const
{
    return "fish player collection";
}

fge::RectFloat FishCollection::getGlobalBounds() const
{
    return Object::getGlobalBounds();
}

fge::RectFloat FishCollection::getLocalBounds() const
{
    return Object::getLocalBounds();
}

//FishCollectionIcon

FGE_OBJ_UPDATE_BODY(FishCollectionIcon)
{
    float const delta = fge::DurationToSecondFloat(deltaTime);
    this->g_time += delta;

    switch (this->g_state)
    {
    case States::IDLE_HIDDEN:
        break;
    case States::SHOWING:
        this->setOrigin(fge::ReachVector(this->getOrigin(), {0.0f, 0.0f}, 300.0f, delta));
        if (this->g_time >= 1.0f)
        {
            this->g_time = 0.0f;
            this->g_state = States::IDLE_SHOWN;
        }
        break;
    case States::IDLE_SHOWN:
        if (this->g_time >= 3.0f)
        {
            this->g_time = 0.0f;
            this->g_state = States::HIDING;
        }
        break;
    case States::HIDING:
        this->setOrigin(
                fge::ReachVector(this->getOrigin(), {0.0f, 16.0f * this->g_button->getScale().x}, 300.0f, delta));
        if (this->g_time >= 1.0f)
        {
            this->g_time = 0.0f;
            this->g_state = States::IDLE_HIDDEN;
        }
        break;
    }
}

FGE_OBJ_DRAW_BODY(FishCollectionIcon)
{
    auto const viewBackup = target.getView();
    target.setView(target.getDefaultView());

    auto copyStates = states.copy();
    copyStates._resTransform.set(target.requestGlobalTransform(*this, copyStates._resTransform));

    this->g_button->draw(target, copyStates);
    this->g_text->draw(target, copyStates);

    target.setView(viewBackup);
}

void FishCollectionIcon::first(fge::Scene& scene)
{
    this->_drawMode = DrawModes::DRAW_ALWAYS_DRAWN;

    this->g_button->setTexture("book_3");
    this->g_button->scale(7.0f);
    this->g_button->ownViewExplicitlySetDefaultView(true);
    this->g_button->centerOriginFromLocalBounds();
    this->g_button->setColor(fge::SetAlpha(fge::Color::White, 240));
    this->g_button->_onButtonPressed.addLambda([&](fge::ObjButton* button) { gGameHandler->openPlayerCollection(); });
    this->g_button->setPriority(1);

    this->g_text->setFont("default");
    this->g_text->setCharacterSize(80);
    this->g_text->scale(1.2f);
    this->g_text->setFillColor(fge::SetAlpha(fge::Color::White, 240));
    this->g_text->setOutlineColor(fge::SetAlpha(fge::Color::Black, 240));
    this->g_text->setOutlineThickness(1.0f);
    this->g_text->setString("B");
    this->g_text->centerOriginFromLocalBounds();

    this->setAnchor(Anchor::Types::ANCHOR_UPRIGHT_CORNER,
                    {Anchor::Shifts::SHIFT_NEGATIVE_BOUNDS, Anchor::Shifts::SHIFT_NONE});
    this->updateAnchor();
    this->g_text->setPosition(this->g_button->getPosition() + fge::Vector2f{-20.0f, 20.0f});
}

void FishCollectionIcon::callbackRegister(fge::Event& event, fge::GuiElementHandler* guiElementHandlerPtr)
{
    this->g_button->callbackRegister(event, guiElementHandlerPtr);

    guiElementHandlerPtr->_onGuiResized.addLambda([&](fge::GuiElementHandler const& gui, fge::Vector2f const& arg) {
        this->updateAnchor();
        this->g_text->setPosition(this->g_button->getPosition() + fge::Vector2f{-20.0f, 20.0f});
    });

    event._onMouseMotion.addLambda([&](fge::Event const& evt, SDL_MouseMotionEvent const& arg) {
        if (gGameHandler->isPlayerCollectionOpen())
        {
            return;
        }

        if (this->g_state == States::HIDING || this->g_state == States::IDLE_HIDDEN)
        {
            this->g_state = States::SHOWING;
        }
        this->g_time = 0.0f;
    });
}

char const* FishCollectionIcon::getClassName() const
{
    return "FISH_COLLECTION_ICON";
}

char const* FishCollectionIcon::getReadableClassName() const
{
    return "fish player collection icon";
}

fge::RectFloat FishCollectionIcon::getGlobalBounds() const
{
    return this->getTransform() * this->g_button->getGlobalBounds();
}

fge::RectFloat FishCollectionIcon::getLocalBounds() const
{
    return this->g_button->getLocalBounds();
}