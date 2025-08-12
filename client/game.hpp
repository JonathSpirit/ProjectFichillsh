#pragma once

#include "FastEngine/C_scene.hpp"
#include "FastEngine/network/C_server.hpp"
#include "FastEngine/object/C_objRectangleShape.hpp"
#include "FastEngine/object/C_objSprite.hpp"
#include "FastEngine/object/C_objSpriteBatches.hpp"
#include "FastEngine/object/C_objText.hpp"
#include "FastEngine/object/C_object.hpp"

#include "../share/network.hpp"
#include "box2d/box2d.h"
#include "updater.hpp"
#include <memory>

#include "fish.hpp"

#define F_TAG_MAJOR 0
#define F_TAG_MINOR 2
#define F_TAG_PATCH 0
#define F_TAG_STR                                                                                                      \
    "v" GRUPDATER_TOSTRING(F_TAG_MAJOR) "." GRUPDATER_TOSTRING(F_TAG_MINOR) "." GRUPDATER_TOSTRING(F_TAG_PATCH)
#define F_TAG                                                                                                          \
    updater::Tag                                                                                                       \
    {                                                                                                                  \
        F_TAG_MAJOR, F_TAG_MINOR, F_TAG_PATCH                                                                          \
    }
#define F_OWNER "JonathSpirit"
#define F_REPO "ProjectFichillsh"
#define F_TEMP_DIR "temp/"

#define F_TESTING_MINIGAME 0 // Set to 1 to test the minigame without waiting for the fish countdown

#define F_GAME_CHECK_TIME_MS 200
#define F_GAME_FISH_COUNTDOWN_MAX 180
#define F_GAME_FISH_COUNTDOWN_MIN 30

#define F_MINIGAME_DIFFICULTY_START 0.0f
#define F_MINIGAME_DIFFICULTY_RARITY_COMMON 10.0f
#define F_MINIGAME_DIFFICULTY_RARITY_UNCOMMON 20.0f
#define F_MINIGAME_DIFFICULTY_RARITY_RARE 30.0f
#define F_MINIGAME_DIFFICULTY_RANDOM_VARIATION 15.0f
#define F_MINIGAME_DIFFICULTY_MAX                                                                                      \
    (F_MINIGAME_DIFFICULTY_START + F_MINIGAME_DIFFICULTY_RARITY_RARE * F_FISH_STAR_MAX +                               \
     F_MINIGAME_DIFFICULTY_RANDOM_VARIATION)
#define F_MINIGAME_TIME_MAX 16.0f
#define F_MINIGAME_TIME_MIN 10.0f
#define F_MINIGAME_SINUS_QUANTITY_MAX 5.0f
#define F_MINIGAME_SINUS_QUANTITY_MIN 1.0f
#define F_MINIGAME_SINUS_FREQ_MAX 0.4f
#define F_MINIGAME_SINUS_FREQ_MIN 0.1f
#define F_MINIGAME_SINUS_FREQ_VARIATION 0.1f

#define F_MINIGAME_SLIDER_ACCELERATION 600.0f
#define F_MINIGAME_SLIDER_MAX_VELOCITY 200.0f
#define F_MINIGAME_SLIDER_FRICTION 180.0f

#define F_MINIGAME_LOOSING_HEARTS_SPEED 2.0f
#define F_MINIGAME_HEARTS_COUNT 3

class Player;

class GameHandler
{
public:
    GameHandler(fge::Scene& scene, fge::net::ClientSideNetUdp& network);
    ~GameHandler();

    void createWorld();
    [[nodiscard]] b2WorldId getWorld() const;
    void pushStaticCollider(fge::RectFloat const& rect);
    void updateWorld();

    [[nodiscard]] Player* getPlayer() const;
    [[nodiscard]] fge::Scene& getScene() const;

    void update(fge::DeltaTime const& deltaTime);

    void pushCaughtFishEvent(std::string const& fishName) const;
    void pushChatEvent(std::string const& chat) const;

private:
    b2WorldId g_bworld{};
    fge::DeltaTime g_checkTime{0};
    fge::Scene* g_scene;
    fge::net::ClientSideNetUdp* g_network;
    unsigned int g_fishCountDown = 0;
};

extern std::unique_ptr<GameHandler> gGameHandler;

class Minigame : public fge::Object
{
public:
    Minigame() = default;
    ~Minigame() override = default;

    FGE_OBJ_UPDATE_DECLARE
    FGE_OBJ_DRAW_DECLARE

    void first(fge::Scene& scene) override;
    void removed(fge::Scene& scene) override;

    void callbackRegister(fge::Event& event, fge::GuiElementHandler* guiElementHandlerPtr) override;

    char const* getClassName() const override;
    char const* getReadableClassName() const override;

    [[nodiscard]] fge::RectFloat getGlobalBounds() const override;
    [[nodiscard]] fge::RectFloat getLocalBounds() const override;

private:
    FishInstance g_fishReward;
    float g_difficulty{0.0f};
    std::function<float(float)> g_sinusFunction;
    float g_currentTime{0.0f};
    float g_fishRemainingTime{0.0f};
    std::array<fge::ObjSprite, F_MINIGAME_HEARTS_COUNT> g_hearts;

    fge::ObjRectangleShape g_gauge;
    fge::ObjRectangleShape g_gaugeSlider;
    fge::ObjSprite g_frame;
    fge::ObjSprite g_fish;

    float g_sliderVelocity = 0.0f;
};

class FishAward : public fge::Object
{
public:
    FishAward() = default;
    FishAward(FishInstance const& fishReward);
    ~FishAward() override = default;

    FGE_OBJ_UPDATE_DECLARE
    FGE_OBJ_DRAW_DECLARE

    void first(fge::Scene& scene) override;

    void callbackRegister(fge::Event& event, fge::GuiElementHandler* guiElementHandlerPtr) override;

    char const* getClassName() const override;
    char const* getReadableClassName() const override;

    [[nodiscard]] fge::RectFloat getGlobalBounds() const override;
    [[nodiscard]] fge::RectFloat getLocalBounds() const override;

private:
    FishInstance g_fishReward;
    fge::ObjSprite g_fish;
    fge::ObjSpriteBatches g_stars;
    fge::ObjText g_text;
    fge::ObjText g_textFishAttributes;
    float g_currentTime = 0.0f;
    fge::Vector2f g_positionGoal;
};

class MultiplayerFishAward : public fge::Object
{
public:
    MultiplayerFishAward() = default;
    MultiplayerFishAward(std::string const& fishName, fge::Vector2f const& position);
    ~MultiplayerFishAward() override = default;

    FGE_OBJ_UPDATE_DECLARE
    FGE_OBJ_DRAW_DECLARE

    void first(fge::Scene& scene) override;

    void callbackRegister(fge::Event& event, fge::GuiElementHandler* guiElementHandlerPtr) override;

    char const* getClassName() const override;
    char const* getReadableClassName() const override;

    [[nodiscard]] fge::RectFloat getGlobalBounds() const override;
    [[nodiscard]] fge::RectFloat getLocalBounds() const override;

private:
    fge::ObjSprite g_fish;
    fge::ObjText g_text;
    float g_currentTime = 0.0f;
    fge::Vector2f g_positionGoal;
};
