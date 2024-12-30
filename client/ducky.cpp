#include "ducky.hpp"
#include "FastEngine/C_scene.hpp"
#include "FastEngine/object/C_objTilemap.hpp"
#include "FastEngine/manager/audio_manager.hpp"
#include "FastEngine/C_random.hpp"
#include "FastEngine/extra/extra_pathFinding.hpp"

//Ducky

Ducky::Ducky(fge::Vector2f const& position)
{
    this->setPosition(position);
}

FGE_OBJ_UPDATE_BODY(Ducky)
{
    FGE_OBJ_UPDATE_CALL(this->g_objAnim);

    auto const delta = fge::DurationToSecondFloat(deltaTime);

    switch (this->g_state)
    {
    case States::IDLE: {
        this->g_time += delta;
        if (this->g_time >= this->g_timeBeforeWalk)
        {
            this->g_time = 0.0f;
            this->g_timeBeforeWalk = fge::_random.range(F_DUCK_WALK_TIME_MIN_S, F_DUCK_WALK_TIME_MAX_S);

            this->computeRandomWalkPath();
            if (this->g_walkPath.empty())
            {
                break;
            }
            this->g_objAnim.getAnimation().setGroup("walk");
            this->g_objAnim.getAnimation().setFrame(0);
            this->g_state = States::WALKING;
        }
        break;
    } case States::WALKING: {
        if (this->g_walkPath.empty())
        {
            this->g_objAnim.getAnimation().setGroup("idle");
            this->g_objAnim.getAnimation().setFrame(0);
            this->g_state = States::IDLE;
            break;
        }

        auto const nextPosition = this->g_walkPath.front();

        this->g_objAnim.getAnimation().setHorizontalFlip(this->getPosition().x > nextPosition.x);

        this->setPosition(fge::ReachVector(this->getPosition(), nextPosition, F_DUCK_SPEED, delta));

        if (this->getPosition() == nextPosition)
        {
            this->g_walkPath.erase(this->g_walkPath.begin());
        }
        break;
    }}

}
FGE_OBJ_DRAW_BODY(Ducky)
{
    auto copyStates = states.copy();
    copyStates._resTransform.set(target.requestGlobalTransform(*this, copyStates._resTransform));

    this->g_objAnim.draw(target, copyStates);
}

void Ducky::first(fge::Scene &scene)
{
    this->g_objAnim.setAnimation(fge::Animation{"ducky_1", "idle"});
    this->g_objAnim.getAnimation().setLoop(true);
    this->g_objAnim.scale(0.5f);
    this->g_objAnim.centerOriginFromLocalBounds();

    this->g_timeBeforeWalk = fge::_random.range(F_DUCK_WALK_TIME_MIN_S, F_DUCK_WALK_TIME_MAX_S);
}

void Ducky::callbackRegister(fge::Event &event, fge::GuiElementHandler *guiElementHandlerPtr)
{
}

const char * Ducky::getClassName() const
{
    return "FISH_DUCKY";
}
const char * Ducky::getReadableClassName() const
{
    return "ducky";
}

fge::RectFloat Ducky::getGlobalBounds() const
{
    return this->getTransform() * this->g_objAnim.getGlobalBounds();
}
fge::RectFloat Ducky::getLocalBounds() const
{
    return this->g_objAnim.getLocalBounds();
}

void Ducky::computeRandomWalkPath()
{
    auto scene = this->_myObjectData.lock()->getScene();
    this->g_walkPath.clear();

    if (auto map = scene->getFirstObj_ByTag("map"))
    {
        auto const objectsLayer = map->getObject<fge::ObjTileMap>()->findLayerName("Objects")->get()->as<fge::TileLayer>();
        auto const waterLayer = map->getObject<fge::ObjTileMap>()->findLayerName("Water")->get()->as<fge::TileLayer>();
        auto const mapSize = waterLayer->getTiles().getSize();

        std::size_t randomTry = 20;
        while (randomTry--)
        {
            //Random position
            auto const randomPos = fge::Vector2i{
                fge::_random.range<std::size_t>(0, mapSize.x - 1),
                fge::_random.range<std::size_t>(0, mapSize.y - 1)
            };

            //Check if the position is walkable
            if (waterLayer->getTiles().get(randomPos).getGid() != 0)
            {
                continue;
            }

            //Compute path
            fge::AStar::Generator generator;
            generator.setWorldSize({static_cast<int>(mapSize.x), static_cast<int>(mapSize.y)});
            for (std::size_t y=0; y<mapSize.y; ++y)
            {
                for (std::size_t x=0; x<mapSize.x; ++x)
                {
                    if (waterLayer->getTiles().get(x, y).getGid() != 0)
                    {
                        generator.addCollision({static_cast<int>(x), static_cast<int>(y)});
                    }
                    else if (objectsLayer->getTiles().get(x, y).getGid() != 0 &&
                        !objectsLayer->getTiles().get(x, y).getTileData()->_collisionRects.empty())
                    {
                        generator.addCollision({static_cast<int>(x), static_cast<int>(y)});
                    }
                }
            }

            auto const duckyPosition = waterLayer->getGridPosition(this->getPosition()).value_or(fge::Vector2size{});
            auto const path = generator.findPath(duckyPosition, randomPos);

            if (path.empty())
            {
                continue;
            }

            for (auto const& node : path)
            {
                this->g_walkPath.emplace_back(
                    16.0f * static_cast<float>(node.x) + 8.0f,
                    16.0f * static_cast<float>(node.y) + 8.0f
                );
            }

            break;
        }
    }
}
