#include "game.hpp"

#include "player.hpp"
#include "FastEngine/C_random.hpp"

GameHandler::GameHandler(fge::Scene& scene) :
    g_scene(&scene)
{
    this->g_fishCountDown = fge::_random.range(F_GAME_FISH_COUNTDOWN_MIN, F_GAME_FISH_COUNTDOWN_MAX);
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

                //TODO: Run the fishing minigame
                this->g_scene->delObject(this->g_scene->getFirstObj_ByClass("FISH_BAIT")->getSid());
            }
        }

        this->g_checkTime = std::chrono::milliseconds(0);
    }
}

std::unique_ptr<GameHandler> gGameHandler;
