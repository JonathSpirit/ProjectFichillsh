#include "FastEngine/fge_includes.hpp"
#include "FastEngine/manager/reg_manager.hpp"
#include "FastEngine/manager/anim_manager.hpp"
#include "FastEngine/network/C_server.hpp"
#include "FastEngine/graphic/C_renderWindow.hpp"
#include "FastEngine/object/C_objTilemap.hpp"
#include "FastEngine/object/C_objSpriteBatches.hpp"
#include "FastEngine/object/C_objRenderMap.hpp"
#include "FastEngine/C_clock.hpp"
#include "SDL.h"

#include "player.hpp"
#include "game.hpp"
#include "fish.hpp"
#include "ducky.hpp"

#include <iostream>
#include <memory>

#define BAD_PACKET_LIMIT 10
#define RETURN_PACKET_DELAYms 100

#define SHOW_COLLIDERS 0

std::atomic_bool gAskForFullUpdate = false;

class Scene : public fge::Scene
{
public:
    ~Scene() override = default;

    void run(fge::RenderWindow& renderWindow, fge::net::ClientSideNetUdp& network)
    {
        network._client.setCTOSLatency_ms(5);

        gGameHandler = std::make_unique<GameHandler>(*this);
        gGameHandler->createWorld();

        fge::Event event;
        fge::GuiElementHandler guiElementHandler(event, renderWindow);
        guiElementHandler.setEventCallback();

        uint16_t badPacketUpdatesCount = 0;

        this->setCallbackContext({&event, &guiElementHandler});
        this->setLinkedRenderTarget(&renderWindow);

        fge::Clock returnPacketClock;
        fge::Clock mainClock;

        //Init managers
        fge::texture::gManager.initialize();
        fge::font::gManager.initialize();
        fge::shader::gManager.initialize();
        fge::anim::gManager.initialize();
        fge::audio::gManager.initialize();
        gFishManager.initialize();

        //Load shaders
        fge::shader::gManager.loadFromFile(
            FGE_OBJSHAPE_INSTANCES_SHADER_VERTEX, "resources/shaders/objShapeInstances_vertex.vert",
            fge::vulkan::Shader::Type::SHADER_VERTEX, fge::shader::ShaderInputTypes::SHADER_GLSL);
        fge::shader::gManager.loadFromFile(
                FGE_OBJSPRITEBATCHES_SHADER_FRAGMENT, "resources/shaders/objSpriteBatches_fragment.frag",
                fge::vulkan::Shader::Type::SHADER_FRAGMENT, fge::shader::ShaderInputTypes::SHADER_GLSL);
        fge::shader::gManager.loadFromFile(
                FGE_OBJSPRITEBATCHES_SHADER_VERTEX, "resources/shaders/objSpriteBatches_vertex.vert",
                fge::vulkan::Shader::Type::SHADER_VERTEX, fge::shader::ShaderInputTypes::SHADER_GLSL);

        //Load textures
        fge::texture::gManager.loadFromFile("OutdoorsTileset", "resources/tilesets/OutdoorsTileset.png");
        fge::texture::gManager.loadFromFile("fishBait_1", "resources/sprites/fishBait_1.png");
        fge::texture::gManager.loadFromFile("fishingFrame", "resources/sprites/fishingFrame.png");
        fge::texture::gManager.loadFromFile("fishingIcon", "resources/sprites/fishingIcon.png");
        fge::texture::gManager.loadFromFile("stars", "resources/sprites/stars.png");
        fge::texture::gManager.loadFromFile("hearts", "resources/sprites/hearts.png");

        //Load animations
        fge::anim::gManager.loadFromFile("human_1", "resources/sprites/human_1.json");
        fge::anim::gManager.loadFromFile("ducky_1", "resources/sprites/ducky_1.json");

        //Load fonts
        fge::font::gManager.loadFromFile("default", "resources/fonts/ttf/monogram.ttf");

        //Load sounds
        fge::audio::gManager.loadFromFile("loose_life", "resources/audio/negative_1.ogg");
        fge::audio::gManager.loadFromFile("loose_fish", "resources/audio/negative_2.ogg");
        fge::audio::gManager.loadFromFile("walk_grass", "resources/audio/walk_grass_1.ogg");
        Mix_VolumeChunk(fge::audio::gManager.getElement("walk_grass")->_ptr.get(), MIX_MAX_VOLUME);
        fge::audio::gManager.loadFromFile("swipe", "resources/audio/swipe_1.ogg");
        fge::audio::gManager.loadFromFile("splash", "resources/audio/splash_1.ogg");
        fge::audio::gManager.loadFromFile("victory_fish", "resources/audio/positive_1.ogg");
        fge::audio::gManager.loadFromFile("ducky", "resources/audio/duck_1.ogg");

        //Load fishes
        gFishManager.loadFromFile("algae", std::nullopt, "resources/sprites/fishes/algae.png");
        gFishManager.loadFromFile("anchovy", std::nullopt, "resources/sprites/fishes/fish-anchovy.png");
        gFishManager.loadFromFile("bronze-striped-grunt", std::nullopt, "resources/sprites/fishes/fish-bronze-striped-grunt.png");
        gFishManager.loadFromFile("butter", std::nullopt, "resources/sprites/fishes/fish-butter.png");
        gFishManager.loadFromFile("gulf-croaker", std::nullopt, "resources/sprites/fishes/fish-gulf-croaker.png");
        gFishManager.loadFromFile("halfbeak", std::nullopt, "resources/sprites/fishes/fish-halfbeak.png");
        gFishManager.loadFromFile("herring", std::nullopt, "resources/sprites/fishes/fish-herring.png");
        gFishManager.loadFromFile("pollock", std::nullopt, "resources/sprites/fishes/fish-pollock.png");
        gFishManager.loadFromFile("sandlance", std::nullopt, "resources/sprites/fishes/fish-sandlance.png");
        gFishManager.loadFromFile("sardine", std::nullopt, "resources/sprites/fishes/fish-sardine.png");
        gFishManager.loadFromFile("krill", std::nullopt, "resources/sprites/fishes/krill.png");
        gFishManager.loadFromFile("krill-1", std::nullopt, "resources/sprites/fishes/krill-1.png");
        gFishManager.loadFromFile("krill-2", std::nullopt, "resources/sprites/fishes/krill-2.png");
        gFishManager.loadFromFile("krill-3", std::nullopt, "resources/sprites/fishes/krill-3.png");
        gFishManager.loadFromFile("shrimp-anemone", std::nullopt, "resources/sprites/fishes/shrimp-anemone.png");
        gFishManager.loadFromFile("shrimp-northern-prawn", std::nullopt, "resources/sprites/fishes/shrimp-northern-prawn.png");
        gFishManager.loadFromFile("squid-reef", std::nullopt, "resources/sprites/fishes/squid-reef.png");
        gFishManager.loadFromFile("zoo-plankton", std::nullopt, "resources/sprites/fishes/zoo-plankton.png");
        gFishManager.loadFromFile("zoo-plankton-small", std::nullopt, "resources/sprites/fishes/zoo-plankton-small.png");

        //Prepare the view
        auto view = renderWindow.getView();
        view.zoom(0.1f);
        view.setCenter({0,0});
        renderWindow.setView(view);

        // Creating objects
        auto* objPlayer = this->newObject<Player>();

        auto* objTopMap = this->newObject<fge::ObjRenderMap>({FGE_SCENE_PLAN_HIGH_TOP});
        objTopMap->setClearColor(fge::Color{50, 50, 50, 140});
        objTopMap->_tags.add("topMap");
        objTopMap->_drawMode = fge::Object::DrawModes::DRAW_ALWAYS_HIDDEN;

        // Create a tileMap object
        auto* objMap = this->newObject<fge::ObjTileMap>({FGE_SCENE_PLAN_BACK});
        objMap->_drawMode = fge::Object::DrawModes::DRAW_ALWAYS_DRAWN;
        objMap->_tags.add("map");

#if SHOW_COLLIDERS
        std::vector<fge::ObjRectangleShape*> objRectCollider;
#endif

        //Load the tileMap from a "tiled" json
        objMap->loadFromFile("resources/map_1/map_1.json", false);
        for (auto const& layer : objMap->getTileLayers())
        {
            if (layer->getType() != fge::BaseLayer::Types::TILE_LAYER)
            {
                continue;
            }

            for (auto const& tile : layer->as<fge::TileLayer>()->getTiles())
            {
                if (tile.getGid() == 0)
                {
                    continue;
                }
                auto const& collisions = tile.getTileData()->_collisionRects;
                for (auto const& collision : collisions)
                {
                    auto collisionRect = static_cast<fge::RectFloat>(collision);
                    collisionRect._x += tile.getPosition().x;
                    collisionRect._y += tile.getPosition().y;

                    gGameHandler->pushStaticCollider(collisionRect);
#if SHOW_COLLIDERS
                    objRectCollider.emplace_back(new fge::ObjRectangleShape(collisionRect.getSize()));
                    objRectCollider.back()->setPosition(collisionRect.getPosition());
                    objRectCollider.back()->setFillColor(fge::SetAlpha(fge::Color::Red, 100));
                    objRectCollider.back()->setOutlineColor(fge::Color::Black);
                    objRectCollider.back()->setOutlineThickness(1.0f);
#endif
                }
            }
        }
        auto const mapBounds = objMap->findLayerName("Water")->get()->as<fge::TileLayer>()->getGlobalBounds();

        //Wall colliders
        gGameHandler->pushStaticCollider(
            {mapBounds.getPosition()-fge::Vector2f{16.0f, 0.0f},
                {16.0f, 500.0f}}); //Left
        gGameHandler->pushStaticCollider(
            {mapBounds.getPosition()-fge::Vector2f{0.0f, 16.0f},
                {500.0f, 16.0f}}); //Top
        gGameHandler->pushStaticCollider(
            {{mapBounds.getPosition().x+mapBounds._width, mapBounds.getPosition().y},
                {16.0f, 500.0f}}); //Right
        gGameHandler->pushStaticCollider(
            {{mapBounds.getPosition().x, mapBounds.getPosition().y+mapBounds._height},
                {500.0f, 16.0f}}); //Bottom

#if SHOW_COLLIDERS
        for (auto const& obj : objRectCollider)
        {
            this->newObject(FGE_NEWOBJECT_PTR(obj), FGE_SCENE_PLAN_TOP);
        }
#endif

        //Load spawn point
        auto const specialObjects = objMap->findLayerName("SpecialObjects")->get()->as<fge::ObjectGroupLayer>();
        objPlayer->boxMove(specialObjects->findObjectName("spawn")->_position);

        //Load duckies
        for (auto const& object : specialObjects->getObjects())
        {
            if (object._name == "duckySpawn")
            {
                this->newObject<Ducky>(object._position);
            }
        }

        bool running = true;
        while (running)
        {
            //Update events
            event.process(10);
            if (event.isEventType(SDL_QUIT))
            {
                running = false;
            }

            //Update
            auto const deltaTime = std::chrono::duration_cast<fge::DeltaTime>(mainClock.restart());
            this->update(renderWindow, event, deltaTime);
            gGameHandler->update(deltaTime);

            //Drawing
            auto imageIndex = renderWindow.prepareNextFrame(nullptr, FGE_RENDER_TIMEOUT_BLOCKING);
            if (imageIndex != FGE_RENDER_BAD_IMAGE_INDEX)
            {
                fge::vulkan::GetActiveContext()._garbageCollector.setCurrentFrame(renderWindow.getCurrentFrame());

                renderWindow.beginRenderPass(imageIndex);

                this->draw(renderWindow);

                renderWindow.endRenderPass();

                renderWindow.display(imageIndex);
            }

            //Receive packets
            fge::net::FluxPacketPtr netPckFlux;
            while (network.process(netPckFlux) == fge::net::FluxProcessResults::RETRIEVABLE)
            {
                switch (netPckFlux->retrieveHeaderId().value())
                {
                default:
                    break;
                }

                if (badPacketUpdatesCount >= BAD_PACKET_LIMIT && !gAskForFullUpdate)
                {
                    std::cout << "Too many bad packets" << std::endl;
                    gAskForFullUpdate = true;
                }
            }

            //Return packet
            if ( returnPacketClock.reached(std::chrono::milliseconds(RETURN_PACKET_DELAYms)) )
            {
                /*if (sc::GameHandler().getReturnPacketCommandSize() > 0)
                {
                    std::cout << "test" << std::endl;
                }

                auto transmissionPacket = sc::GetGameHandler().prepareAndRetrieveReturnPacket();

                //Packet latency planner
                network._client._latencyPlanner.pack(transmissionPacket);

                //Pack needed update
                this->packNeededUpdate(transmissionPacket->packet());

                network._client.pushPacket(std::move(transmissionPacket));*/

                returnPacketClock.restart();
            }
        }

        network.stop();

        fge::texture::gManager.uninitialize();
        fge::font::gManager.uninitialize();
        fge::shader::gManager.uninitialize();
        fge::anim::gManager.uninitialize();

        gGameHandler.reset();
    }
};

int main(int argc, char *argv[])
{
    using namespace fge::vulkan;

    std::cout << "FastEngine version: " << FGE_VERSION_FULL_WITHTAG_STRING << std::endl;
    if (fge::IsEngineBuiltInDebugMode())
    {
        std::cout << "Built in debug mode" << std::endl;
    }

    //Remove Vulkan validation layer
    InstanceLayers.clear();
    InstanceLayers.push_back("VK_LAYER_LUNARG_monitor");
    //InstanceLayers.push_back("VK_LAYER_KHRONOS_validation");

    if ( !fge::net::Socket::initSocket() )
    {
        return -1;
    }

    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "system");

    auto instance = Context::init(SDL_INIT_VIDEO | SDL_INIT_EVENTS, "Fichillsh [v0.1.0]");
    Context::enumerateExtensions();

    SurfaceSDLWindow window(instance, FGE_WINDOWPOS_CENTERED, {1366,768}, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    // Check that the window was successfully created
    if (!window.isCreated())
    {
        // In the case that the window could not be made...
        std::cout << "Could not create window: " << SDL_GetError() << std::endl;
        return 1;
    }

    Context vulkanContext(window);
    vulkanContext._garbageCollector.enable(true);

    fge::RenderWindow renderWindow(vulkanContext, window);
    renderWindow.setClearColor(fge::Color(239, 205, 173));
    renderWindow.setPresentMode(VK_PRESENT_MODE_FIFO_KHR);
    //renderWindow.setVerticalSyncEnabled( sc::ConfigVsync() );

    fge::net::ClientSideNetUdp network;

    //Loading resources

    //Loading scenes
    std::unique_ptr<Scene> currentScene  = std::make_unique<Scene>();

    do
    {
        GetActiveContext()._garbageCollector.enable(true);

        currentScene->run(renderWindow, network);

        GetActiveContext().waitIdle();
        GetActiveContext()._garbageCollector.enable(false);
        currentScene.reset();
    }
    while (currentScene);

    //Unloading resources

    renderWindow.destroy();

    vulkanContext.destroy();

    window.destroy();
    instance.destroy();
    SDL_Quit();

    return 0;
}
