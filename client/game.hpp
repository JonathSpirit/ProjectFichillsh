#pragma once

#include "FastEngine/C_scene.hpp"
#include "box2d/box2d.h"
#include <memory>

class GameHandler
{
public:
    GameHandler() = default;
    ~GameHandler();

    void createWorld();
    [[nodiscard]] b2WorldId getWorld() const;
    void pushStaticCollider(fge::RectFloat const& rect);
    void updateWorld();

private:
    b2WorldId g_bworld;
};

extern std::unique_ptr<GameHandler> gGameHandler;
