#pragma once

#include "FastEngine/object/C_objSprite.hpp"
#include "FastEngine/object/C_object.hpp"
#include "FastEngine/C_scene.hpp"
#include "box2d/box2d.h"
#include <memory>
#include <FastEngine/object/C_objRectangleShape.hpp>

#define F_GAME_CHECK_TIME_MS 200
#define F_GAME_FISH_COUNTDOWN_MAX 200
#define F_GAME_FISH_COUNTDOWN_MIN 30

#define F_MINIGAME_BASE_TIME_S 10.0f
#define F_MINIGAME_DIFFICULTY_TIME_RATIO 0.7f
#define F_MINIGAME_DIFFICULTY_SPEED_RATIO 0.01f
#define F_MINIGAME_DELTA_TIME_S 0.01f

#define F_MINIGAME_SLIDER_ACCELERATION 300.0f
#define F_MINIGAME_SLIDER_MAX_VELOCITY 400.0f
#define F_MINIGAME_SLIDER_FRICTION 300.0f

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
    b2WorldId g_bworld{};
    fge::DeltaTime g_checkTime{0};
    fge::Scene* g_scene;
    unsigned int g_fishCountDown = 0;
};

extern std::unique_ptr<GameHandler> gGameHandler;

class Minigame : public fge::Object
{
public:
    Minigame() = default;
    Minigame(unsigned int difficulty);
    ~Minigame() override = default;

    FGE_OBJ_UPDATE_DECLARE
    FGE_OBJ_DRAW_DECLARE

    void first(fge::Scene &scene) override;
    void removed(fge::Scene &scene) override;

    void callbackRegister(fge::Event &event, fge::GuiElementHandler *guiElementHandlerPtr) override;

    const char * getClassName() const override;
    const char * getReadableClassName() const override;

    [[nodiscard]] fge::RectFloat getGlobalBounds() const override;
    [[nodiscard]] fge::RectFloat getLocalBounds() const override;

private:
    unsigned int g_difficulty;
    std::vector<float> g_fishPositions;
    float g_fishTotalTime;
    float g_currentTime;

    fge::ObjRectangleShape g_gauge;
    fge::ObjRectangleShape g_gaugeSlider;
    fge::ObjSprite g_frame;
    fge::ObjSprite g_fish;

    float g_sliderVelocity = 0.0f;
};
