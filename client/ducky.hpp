#pragma once

#include "FastEngine/C_scene.hpp"
#include "FastEngine/object/C_objAnim.hpp"
#include "FastEngine/object/C_object.hpp"

#define F_DUCK_SPEED 50.0f
#define F_DUCK_WALK_TIME_MIN_S 3.0f
#define F_DUCK_WALK_TIME_MAX_S 14.0f
#define F_DUCK_QUACK_TIME_MIN_S 10.0f
#define F_DUCK_QUACK_TIME_MAX_S 80.0f

class Ducky : public fge::Object
{
public:
    Ducky() = default;
    explicit Ducky(fge::Vector2f const& position);
    ~Ducky() override = default;

    FGE_OBJ_UPDATE_DECLARE
    FGE_OBJ_DRAW_DECLARE

    void first(fge::Scene& scene) override;

    void callbackRegister(fge::Event& event, fge::GuiElementHandler* guiElementHandlerPtr) override;

    char const* getClassName() const override;
    char const* getReadableClassName() const override;

    [[nodiscard]] fge::RectFloat getGlobalBounds() const override;
    [[nodiscard]] fge::RectFloat getLocalBounds() const override;

private:
    void computeRandomWalkPath();

    fge::ObjAnimation g_objAnim;
    enum class States
    {
        IDLE,
        WALKING
    } g_state = States::IDLE;
    fge::Vector2i g_direction{1, 0};
    int g_audioWalking = -1;
    float g_time = 0.0f;
    float g_timeBeforeWalk = 0.0f;
    std::vector<fge::Vector2f> g_walkPath;
    bool g_handlingQuack = false;

    fge::ObjectDataWeak g_player;
    fge::ObjectPlan g_transitionPlan;
    bool g_transitionLast = false;

    static bool gQuackHandled;
    static float gTimeBeforeQuack;
};