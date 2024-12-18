#pragma once

#include "FastEngine/C_scene.hpp"
#include "box2d/box2d.h"
#include <memory>

#define F_GAME_CHECK_TIME_MS 200
#define F_GAME_FISH_COUNTDOWN_MAX 200
#define F_GAME_FISH_COUNTDOWN_MIN 30

class Player;

class GameHandler
{
public:
    GameHandler(fge::Scene& scene);
    ~GameHandler();

    void createWorld();
    [[nodiscard]] b2WorldId getWorld() const;
    void pushStaticCollider(fge::RectFloat const& rect);
    void updateWorld();

    [[nodiscard]] Player* getPlayer() const;
    [[nodiscard]] fge::Scene& getScene() const;

    void update(fge::DeltaTime const& deltaTime);

private:
    b2WorldId g_bworld;
    fge::DeltaTime g_checkTime{0};
    fge::Scene* g_scene;
    unsigned int g_fishCountDown = 0;
};

extern std::unique_ptr<GameHandler> gGameHandler;
