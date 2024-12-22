#include "game.hpp"

#include "player.hpp"
#include "FastEngine/C_random.hpp"

//GameHandler

GameHandler::GameHandler(fge::Scene& scene) :
    g_scene(&scene)
{
    this->g_fishCountDown = 1;//fge::_random.range(F_GAME_FISH_COUNTDOWN_MIN, F_GAME_FISH_COUNTDOWN_MAX);
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

    b2Polygon groundBox = b2MakeBox(rect._width/2.0f, rect._height/2.0f);

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
    if (auto player = this->g_scene->getFirstObj_ByClass("FISH_PLAYER"))
    {//Should be always valid
        return player->getObject<Player>();
    }
    return nullptr;
}
fge::Scene & GameHandler::getScene() const
{
    return *this->g_scene;
}

void GameHandler::update(fge::DeltaTime const &deltaTime)
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
                this->g_fishCountDown = fge::_random.range(F_GAME_FISH_COUNTDOWN_MIN, F_GAME_FISH_COUNTDOWN_MAX);
                this->g_scene->newObject<Minigame>({FGE_SCENE_PLAN_HIGH_TOP + 1}, 4);
            }
        }

        this->g_checkTime = std::chrono::milliseconds(0);
    }
}

std::unique_ptr<GameHandler> gGameHandler;

//Minigame

Minigame::Minigame(unsigned int difficulty) :
        g_difficulty(std::clamp<unsigned int>(difficulty, 0, 10))
{}

FGE_OBJ_UPDATE_BODY(Minigame)
{
    auto const delta = fge::DurationToSecondFloat(deltaTime);
    this->g_currentTime += delta;
    if (this->g_currentTime >= this->g_fishPositions.size() * F_MINIGAME_DELTA_TIME_S)
    {
        this->g_currentTime = 0.0f;
    }

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
        this->g_sliderVelocity = std::clamp<float>(this->g_sliderVelocity + F_MINIGAME_SLIDER_FRICTION * delta * (this->g_sliderVelocity > 0.0f ? -1.0f : 1.0f),
            -F_MINIGAME_SLIDER_MAX_VELOCITY, F_MINIGAME_SLIDER_MAX_VELOCITY);
    }

    this->g_gaugeSlider.move({0.0f, this->g_sliderVelocity * delta});
    if (this->g_gaugeSlider.getPosition().y < this->g_gauge.getPosition().y - this->g_gauge.getSize().y/2.0f)
    {
        this->g_gaugeSlider.setPosition({this->g_gaugeSlider.getPosition().x, this->g_gauge.getPosition().y - this->g_gauge.getSize().y/2.0f});
        this->g_sliderVelocity = 0.0f;
    }
    else if (this->g_gaugeSlider.getPosition().y > this->g_gauge.getPosition().y + this->g_gauge.getSize().y/2.0f)
    {
        this->g_gaugeSlider.setPosition({this->g_gaugeSlider.getPosition().x, this->g_gauge.getPosition().y + this->g_gauge.getSize().y/2.0f});
        this->g_sliderVelocity = 0.0f;
    }

    //Check if fish is inside the gauge slider
    auto const sliderBounds = this->g_gaugeSlider.getGlobalBounds();
    auto const fishBounds = this->g_fish.getGlobalBounds();
    float posXeffect = 0.0f;
    bool caughting = false;
    if (auto rect = sliderBounds.findIntersection(fishBounds))
    {
        if (rect->getSize().y >= fishBounds.getSize().y)
        {//Fish is inside the slider
            posXeffect = fge::_random.range(-2.0f, 2.0f);
            caughting = true;
        }
    }

    if (!caughting)
    {//Loosing life
        for (std::size_t i=0; i<this->g_hearts.size(); ++i)
        {
            auto& heart = this->g_hearts[i];
            if (heart.getScale().x > 0.0f)
            {
                heart.scale(1.0f - F_MINIGAME_LOOSING_HEARTS_SPEED * delta);
                if (heart.getScale().x <= 0.1f)
                {//Loosing a heart
                    heart.setScale(0.0f);
                    if (i == this->g_hearts.size()-1)
                    {//Lost the last heart
                        scene.delUpdatedObject();
                        return;
                    }
                }
                break;
            }
        }
    }
    else
    {//Fish loosing time
        this->g_fishRemainingTime -= delta;
        if (this->g_fishRemainingTime <= 0.0f)
        {
            ///TODO: Wining the minigame
            scene.delUpdatedObject();
            return;
        }
    }

    //Update fish position
    auto index = std::clamp<std::size_t>(this->g_currentTime / F_MINIGAME_DELTA_TIME_S, 0, this->g_fishPositions.size()-1);
    this->g_fish.setPosition({this->g_gauge.getPosition().x + posXeffect, this->g_fishPositions[index]});
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

    for (auto const& heart : this->g_hearts)
    {
        heart.draw(target, copyStates);
    }

    target.setView(viewBackup);
}

void Minigame::first(fge::Scene &scene)
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

    for (std::size_t i=0; i<this->g_hearts.size(); ++i)
    {
        this->g_hearts[i].setTexture("fishBait_1");
        this->g_hearts[i].centerOriginFromLocalBounds();
        this->g_hearts[i].setPosition({-60.0f, -20.0f + 60.0f * i});
        this->g_hearts[i].scale(10.0f);
    }

    this->g_fishRemainingTime = F_MINIGAME_BASE_TIME_S + static_cast<float>(this->g_difficulty) * F_MINIGAME_DIFFICULTY_TIME_RATIO;

    //Generate fish positions
    auto const frequency = (this->g_difficulty+1) * F_MINIGAME_DIFFICULTY_SPEED_RATIO;
    auto const speed = 2.0f * FGE_MATH_PI * frequency;
    auto const sinPeriod = 1.0f / frequency;
    this->g_fishPositions.resize(sinPeriod / F_MINIGAME_DELTA_TIME_S);

    for (std::size_t i=0; i<this->g_fishPositions.size(); ++i)
    {
        this->g_fishPositions[i] = std::sin(static_cast<float>(i) * F_MINIGAME_DELTA_TIME_S * speed)
                * this->g_gauge.getSize().y / 2.0f + this->g_gauge.getPosition().y / 2.0f;
    }

    auto target = scene.getLinkedRenderTarget();
    this->setPosition({target->getSize().x/2.0f, target->getSize().y/2.0f});

    if (auto obj = scene.getFirstObj_ByTag("topMap"))
    {
        obj->getObject()->_drawMode = DrawModes::DRAW_ALWAYS_DRAWN;
    }
    if (auto obj = scene.getFirstObj_ByClass("FISH_PLAYER"))
    {
        obj->getObject<Player>()->catchingFish();
    }
}
void Minigame::removed(fge::Scene &scene)
{
    if (auto obj = scene.getFirstObj_ByTag("topMap"))
    {
        obj->getObject()->_drawMode = DrawModes::DRAW_ALWAYS_HIDDEN;
    }
    if (auto obj = scene.getFirstObj_ByClass("FISH_PLAYER"))
    {
        obj->getObject<Player>()->endCatchingFish();
    }
}

void Minigame::callbackRegister(fge::Event &event, fge::GuiElementHandler *guiElementHandlerPtr)
{
}

const char * Minigame::getClassName() const
{
    return "FISH_MINIGAME";
}

const char * Minigame::getReadableClassName() const
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
