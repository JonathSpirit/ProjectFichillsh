#include "FastEngine/C_clock.hpp"
#include "FastEngine/fge_includes.hpp"
#include "FastEngine/graphic/C_renderWindow.hpp"
#include "FastEngine/manager/anim_manager.hpp"
#include "FastEngine/manager/reg_manager.hpp"
#include "FastEngine/network/C_server.hpp"
#include "FastEngine/object/C_objRenderMap.hpp"
#include "FastEngine/object/C_objSpriteBatches.hpp"
#include "FastEngine/object/C_objTilelayer.hpp"
#include "SDL.h"

#include "../share/network.hpp"
#include "../share/player.hpp"
#include "ducky.hpp"
#include "fish.hpp"
#include "game.hpp"

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
        nlohmann::json config;
        if (!fge::LoadJsonFromFile("server.json", config))
        {
            std::cout << "Can't load server.json, continuing anyway\n";
        }

        fge::net::IpAddress serverIp = config.value<std::string>("ip", F_NET_DEFAULT_IP);
        fge::net::Port serverPort = config.value<fge::net::Port>("port", F_NET_DEFAULT_PORT);
        bool onlineMode = config.value<bool>("online", F_NET_DEFAULT_ONLINE_MODE);

        config = nlohmann::json{{"ip", serverIp.toString().value_or(F_NET_DEFAULT_IP)},
                                {"port", serverPort},
                                {"online", onlineMode}};

        if (!fge::SaveJsonToFile("server.json", config, 4))
        {
            std::cout << "Can't save server.json, continuing anyway\n";
        }

        std::string const versioningString = F_NET_STRING_SEQ + fge::string::ToStr(F_NET_SERVER_COMPATIBILITY_VERSION);

        network._client.setCTOSLatency_ms(5);

        gGameHandler = std::make_unique<GameHandler>(*this, network);
        gGameHandler->createWorld();

        fge::Event event;
        fge::GuiElementHandler guiElementHandler(event, renderWindow);
        guiElementHandler.setEventCallback();

        uint16_t badPacketUpdatesCount = 0;

        this->setCallbackContext({&event, &guiElementHandler});
        this->setLinkedRenderTarget(&renderWindow);

        fge::Clock mainClock;

        //Init managers
        fge::texture::gManager.initialize();
        fge::font::gManager.initialize();
        fge::shader::gManager.initialize();
        fge::anim::gManager.initialize();
        fge::audio::gManager.initialize();
        gFishManager.initialize();

        //Load object (mostly for network)
        fge::reg::RegisterNewClass(std::make_unique<fge::reg::Stamp<Player>>());

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
        fge::texture::gManager.loadFromFile("book_1", "resources/sprites/book_1.png");
        fge::texture::gManager.loadFromFile("arrows", "resources/sprites/arrows.png");
        fge::texture::gManager.loadFromFile("close_1", "resources/sprites/close_1.png");

        //Load animations
        fge::anim::gManager.loadFromFile("human_1", "resources/sprites/human_1.json");
        fge::anim::gManager.loadFromFile("ducky_1", "resources/sprites/ducky_1.json");

        //Create player shadow
        {
            fge::anim::gManager.duplicate("human_1", "human_1_shadow");
            auto humanAnimData = fge::anim::gManager.getElement("human_1_shadow");

            fge::Surface surface(humanAnimData->_ptr->_tilesetTexture->copyToSurface());
            for (int i = 0; i < surface.getSize().x; ++i)
            {
                for (int j = 0; j < surface.getSize().y; ++j)
                {
                    auto color = fge::Color(0, 0, 0, surface.getPixel(i, j).value()._a);
                    surface.setPixel(i, j, color);
                }
            }
            humanAnimData->_ptr->_tilesetTexture = std::make_shared<fge::TextureType>(fge::vulkan::GetActiveContext());
            humanAnimData->_ptr->_tilesetTexture->create(surface.get());
        }

        //Create ducky shadow
        {
            fge::anim::gManager.duplicate("ducky_1", "ducky_1_shadow");
            auto duckyAnimData = fge::anim::gManager.getElement("ducky_1_shadow");

            fge::Surface surface(duckyAnimData->_ptr->_tilesetTexture->copyToSurface());
            for (int i = 0; i < surface.getSize().x; ++i)
            {
                for (int j = 0; j < surface.getSize().y; ++j)
                {
                    auto color = fge::Color(0, 0, 0, surface.getPixel(i, j).value()._a);
                    surface.setPixel(i, j, color);
                }
            }
            duckyAnimData->_ptr->_tilesetTexture = std::make_shared<fge::TextureType>(fge::vulkan::GetActiveContext());
            duckyAnimData->_ptr->_tilesetTexture->create(surface.get());
        }

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
        gFishManager.loadFromFile("algae", 0.01f, 0.05f, 1.0f, 5.0f, FishData::Rarity::COMMON,
                                  "resources/sprites/fishes/algae.png");
        gFishManager.loadFromFile("anchovy", 0.02f, 0.10f, 2.0f, 8.0f, FishData::Rarity::COMMON,
                                  "resources/sprites/fishes/fish-anchovy.png");
        gFishManager.loadFromFile("butter", 0.05f, 0.20f, 3.0f, 12.0f, FishData::Rarity::COMMON,
                                  "resources/sprites/fishes/fish-butter.png");
        gFishManager.loadFromFile("gulf-croaker", 0.08f, 0.30f, 4.0f, 15.0f, FishData::Rarity::COMMON,
                                  "resources/sprites/fishes/fish-gulf-croaker.png");
        gFishManager.loadFromFile("halfbeak", 0.03f, 0.12f, 2.5f, 10.0f, FishData::Rarity::COMMON,
                                  "resources/sprites/fishes/fish-halfbeak.png");
        gFishManager.loadFromFile("herring", 0.04f, 0.15f, 3.0f, 11.0f, FishData::Rarity::COMMON,
                                  "resources/sprites/fishes/fish-herring.png");
        gFishManager.loadFromFile("pollock", 0.20f, 1.00f, 10.0f, 40.0f, FishData::Rarity::UNCOMMON,
                                  "resources/sprites/fishes/fish-pollock.png");
        gFishManager.loadFromFile("sandlance", 0.01f, 0.07f, 1.5f, 7.0f, FishData::Rarity::COMMON,
                                  "resources/sprites/fishes/fish-sandlance.png");
        gFishManager.loadFromFile("sardine", 0.02f, 0.09f, 2.0f, 9.0f, FishData::Rarity::COMMON,
                                  "resources/sprites/fishes/fish-sardine.png");
        gFishManager.loadFromFile("shrimp-anemone", 0.005f, 0.02f, 0.5f, 2.0f, FishData::Rarity::COMMON,
                                  "resources/sprites/fishes/shrimp-anemone.png");
        gFishManager.loadFromFile("shrimp-northern-prawn", 0.01f, 0.04f, 1.0f, 3.0f, FishData::Rarity::COMMON,
                                  "resources/sprites/fishes/shrimp-northern-prawn.png");
        gFishManager.loadFromFile("krill", 0.001f, 0.005f, 0.8f, 5.0f, FishData::Rarity::COMMON,
                                  "resources/sprites/fishes/krill.png");
        gFishManager.loadFromFile("krill-1", 0.001f, 0.005f, 0.2f, 6.0f, FishData::Rarity::COMMON,
                                  "resources/sprites/fishes/krill-1.png");

        gFishManager.loadFromFile("bronze-striped-grunt", 0.10f, 0.50f, 5.0f, 20.0f, FishData::Rarity::UNCOMMON,
                                  "resources/sprites/fishes/fish-bronze-striped-grunt.png");
        gFishManager.loadFromFile("krill-2", 0.001f, 0.005f, 1.0f, 10.0f, FishData::Rarity::UNCOMMON,
                                  "resources/sprites/fishes/krill-2.png");
        gFishManager.loadFromFile("zoo-plankton", 0.0005f, 0.002f, 0.1f, 0.5f, FishData::Rarity::UNCOMMON,
                                  "resources/sprites/fishes/zoo-plankton.png");
        gFishManager.loadFromFile("zoo-plankton-small", 0.0002f, 0.001f, 0.05f, 0.3f, FishData::Rarity::UNCOMMON,
                                  "resources/sprites/fishes/zoo-plankton-small.png");

        gFishManager.loadFromFile("krill-3", 0.001f, 0.005f, 0.2f, 1.2f, FishData::Rarity::RARE,
                                  "resources/sprites/fishes/krill-3.png");
        gFishManager.loadFromFile("squid-reef", 0.10f, 0.50f, 5.0f, 20.0f, FishData::Rarity::RARE,
                                  "resources/sprites/fishes/squid-reef.png");

        //Prepare the view
        auto view = renderWindow.getView();
        view.zoom(0.1f);
        view.setCenter({0, 0});
        renderWindow.setView(view);

        // Creating objects
        auto* objTopMap = this->newObject<fge::ObjRenderMap>({FGE_SCENE_PLAN_HIGH_TOP});
        objTopMap->setClearColor(fge::Color{50, 50, 50, 140});
        objTopMap->_tags.add("topMap");
        objTopMap->_drawMode = fge::Object::DrawModes::DRAW_ALWAYS_HIDDEN;

        // Create a tileMap object
        auto tilemap = fge::TileMap::create();

#if SHOW_COLLIDERS
        std::vector<fge::ObjRectangleShape*> objRectCollider;
#endif

        //Load the tileMap from a "tiled" json
        tilemap->loadFromFile("resources/map_1/map_1.json");
        tilemap->generateObjects(*this, FGE_SCENE_PLAN_HIDE_BACK - 10);
        tilemap->_generatedObjects.front().lock()->getObject()->_tags.add(
                "map"); //Add tag to the first tile layer object, in order to find it later

        for (auto const& layer: tilemap->_layers)
        {
            if (layer->getType() != fge::BaseLayer::Types::TILE_LAYER)
            {
                continue;
            }

            for (auto const& tile: layer->as<fge::TileLayer>()->getTiles())
            {
                if (tile.getGid() == 0)
                {
                    continue;
                }
                auto const& collisions = tile.getTileData()->_collisionRects;
                for (auto const& collision: collisions)
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
        auto const mapBounds = tilemap->findLayerName("Water")->get()->as<fge::TileLayer>()->getGlobalBounds();

        //Player
        auto* objPlayer = this->newObject<Player>();
        objPlayer->_tags.add("player");

        //Wall colliders
        gGameHandler->pushStaticCollider(
                {mapBounds.getPosition() - fge::Vector2f{16.0f, 0.0f}, {16.0f, 500.0f}}); //Left
        gGameHandler->pushStaticCollider({mapBounds.getPosition() - fge::Vector2f{0.0f, 16.0f}, {500.0f, 16.0f}}); //Top
        gGameHandler->pushStaticCollider(
                {{mapBounds.getPosition().x + mapBounds._width, mapBounds.getPosition().y}, {16.0f, 500.0f}}); //Right
        gGameHandler->pushStaticCollider(
                {{mapBounds.getPosition().x, mapBounds.getPosition().y + mapBounds._height}, {500.0f, 16.0f}}); //Bottom

#if SHOW_COLLIDERS
        for (auto const& obj: objRectCollider)
        {
            this->newObject(FGE_NEWOBJECT_PTR(obj), FGE_SCENE_PLAN_TOP);
        }
#endif

        //Load spawn point
        auto const specialObjects = tilemap->findLayerName("SpecialObjects")->get()->as<fge::ObjectGroupLayer>();
        objPlayer->boxMove(specialObjects->findObjectName("spawn")->_position);

        //Load duckies
        for (auto const& object: specialObjects->getObjects())
        {
            if (object._name == "duckySpawn")
            {
                this->newObject<Ducky>(object._position);
            }
        }

        //Load player collection
        if (!gGameHandler->loadPlayerCollectionFromFile())
        {
            std::cout << "Can't load player collection\n";
        }

        //Setup events
        event._onKeyUp.addLambda([](fge::Event const& evt, SDL_KeyboardEvent const& arg) {
            if (arg.keysym.sym == SDLK_b)
            {
                gGameHandler->openPlayerCollection();
            }
        });

        //Setup network events
        network._onClientDisconnected.addLambda([&](fge::net::ClientSideNetUdp& net) {
            std::cout << "Connection lost ! (disconnected from server)" << std::endl;
            this->stopNetwork(network);
        });
        network._onClientTimeout.addLambda([&](fge::net::ClientSideNetUdp& net) {
            std::cout << "Connection lost ! (timeout)" << std::endl;
            this->stopNetwork(network);
        });
        network._onTransmitReturnPacket.addLambda(
                [&](fge::net::ClientSideNetUdp& net, fge::net::TransmitPacketPtr& packet) {
            //Pack data
            packet->packet() << objPlayer->getPosition() << objPlayer->getDirection() << objPlayer->getState();

            //Pack needed update
            this->packNeededUpdate(packet->packet());
        });

        this->g_playerEvents = this->_netList.push<std::remove_pointer_t<decltype(this->g_playerEvents)>>();
        this->g_playerEvents->_onEvent.addLambda([&](auto const& event) {
            Player* player = this->findPlayerObject(event.second._playerId);
            if (player == nullptr)
            {
                return;
            }

            switch (event.first)
            {
            case StatEvents::CAUGHT_FISH:
                this->newObject<MultiplayerFishAward>({FGE_SCENE_PLAN_TOP}, event.second._data, player->getPosition());
                break;
            case StatEvents::PLAYER_DISCONNECTED:
                this->removeNetworkPlayer(event.second._playerId);
                break;
            case StatEvents::PLAYER_CHAT:
                this->newObject<PlayerChatMessage>({FGE_SCENE_PLAN_TOP}, event.second._data,
                                                   player->_myObjectData.lock()->getSid());
                break;
            default:
                break;
            }
        });

        //Connect to the server
        if (!onlineMode ||
            !network.start(0, fge::net::IpAddress::Ipv4Any, serverPort, serverIp, fge::net::IpAddress::Types::Ipv4))
        {
            std::cout << "Can't start network\n";
        }
        else
        {
            //Start connection process
            auto connectResult = network.connect(versioningString);

            connectResult.wait();
            if (!connectResult.get())
            {
                std::cout << "Can't connect to the server\n";
                this->stopNetwork(network);
            }
            else
            {
                //Asking for connection
                auto packet = fge::net::CreatePacket(CLIENT_ASK_CONNECT);
                packet->doNotDiscard().doNotReorder().packet() << F_NET_CLIENT_HELLO << objPlayer->getPosition();
                network._client.pushPacket(std::move(packet));
                network.notifyTransmission();
                if (network.waitForPackets(F_NET_CLIENT_TIMEOUT_RECEIVE) > 0)
                {
                    if (auto netPacket = network.popNextPacket())
                    {
                        if (netPacket->retrieveHeaderId().value() == CLIENT_ASK_CONNECT)
                        {
                            network._client.getStatus().resetTimeout();

                            bool valid;
                            netPacket->packet() >> valid;
                            if (valid)
                            {
                                using namespace fge::net::rules;
                                std::string dataHello;
                                auto err = RValid(RSizeMustEqual<std::string>(sizeof(F_NET_SERVER_HELLO) - 1,
                                                                              {netPacket->packet(), &dataHello}))
                                                   .end();

                                if (err || dataHello != F_NET_SERVER_HELLO)
                                {
                                    std::cout << "Error, bad server hello: \n";
                                    err->dump(std::cout);
                                    this->stopNetwork(network);
                                }
                                else
                                {
                                    std::cout << "Connected to the server\n";
                                    this->applyFullUpdate(netPacket->packet());
                                    network.enableReturnPacket(true);
                                    network._client.setPacketReturnRate(
                                            std::chrono::milliseconds(RETURN_PACKET_DELAYms));
                                    network._client.getPacketReorderer().setMaximumSize(
                                            FGE_NET_PACKET_REORDERER_CACHE_COMPUTE(RETURN_PACKET_DELAYms, F_TICK_TIME));
                                }
                            }
                            else
                            {
                                std::string dataString;
                                netPacket->packet() >> dataString;
                                std::cout << "Server refused connection: " << dataString << std::endl;
                                this->stopNetwork(network);
                            }
                        }
                    }
                }
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
            fge::net::ReceivedPacketPtr netPacket;
            fge::net::FluxProcessResults processResult;
            do {
                processResult = network.process(netPacket);
                if (processResult != fge::net::FluxProcessResults::USER_RETRIEVABLE)
                {
                    continue;
                }

                switch (static_cast<PacketHeaders>(netPacket->retrieveHeaderId().value()))
                {
                case SERVER_UPDATE:
                    //Unpack latency planner
                    network._client._latencyPlanner.unpack(netPacket.get(), network._client);

                    if (auto latency = network._client._latencyPlanner.getLatency())
                    {
                        network._client.setCTOSLatency_ms(latency.value());
                    }
                    if (auto latency = network._client._latencyPlanner.getOtherSideLatency())
                    {
                        network._client.setSTOCLatency_ms(latency.value());
                    }

                    //Unpack data
                    {
                        fge::Scene::UpdateCountRange updateCountRange{};
                        this->unpackModification(netPacket->packet(), updateCountRange, true);
                    }
                    network._client.getStatus().resetTimeout();
                    break;
                case SERVER_FULL_UPDATE:
                    this->applyFullUpdate(netPacket->packet());
                    network._client.getStatus().resetTimeout();
                    break;
                default:
                    break;
                }

                if (badPacketUpdatesCount >= BAD_PACKET_LIMIT && !gAskForFullUpdate)
                {
                    std::cout << "Too many bad packets" << std::endl;
                    gAskForFullUpdate = true;
                }
            } while (processResult != fge::net::FluxProcessResults::NONE_AVAILABLE);

            if (!network.isRunning())
            {
                continue;
            }
        }

        network.disconnect().wait();
        network.stop();

        fge::texture::gManager.uninitialize();
        fge::font::gManager.uninitialize();
        fge::shader::gManager.uninitialize();
        fge::anim::gManager.uninitialize();

        gGameHandler.reset();
    }

    void removeNetworkPlayer(std::string const& playerId)
    {
        auto player = this->findPlayerObject(playerId);
        if (player == nullptr)
        {
            return;
        }
        this->delObject(player->_myObjectData.lock()->getSid());
    }
    void removeNetworkElement()
    {
        fge::ObjectContainer container;
        this->getAllObj_ByTag("multiplayer", container);
        for (auto const& obj: container)
        {
            this->delObject(obj->getSid());
        }
    }
    void stopNetwork(fge::net::ClientSideNetUdp& network)
    {
        network.stop();
        this->removeNetworkElement();
    }
    void applyFullUpdate(fge::net::Packet& packet)
    {
        this->removeNetworkElement(); //TODO: remove only the ones that are not in the server

        std::string myPlayerId;
        packet >> myPlayerId;

        this->_properties["playerId"] = myPlayerId;

        this->unpack(packet, false);
    }

    Player* findPlayerObject(std::string const& playerId) const
    {
        fge::ObjectContainer container;
        if (this->getAllObj_ByClass("FISH_PLAYER", container) == 0)
        {
            return nullptr;
        }

        for (auto const& obj: container)
        {
            if (obj->getObject()->_properties["playerId"] == playerId)
            {
                return obj->getObject<Player>();
            }
        }

        return nullptr;
    }

private:
    fge::net::NetworkTypeEvents<StatEvents, PlayerEventData>* g_playerEvents{nullptr};
};

int main(int argc, char* argv[])
{
    using namespace fge::vulkan;

    //Verify update
    std::filesystem::remove_all(F_TEMP_DIR);
    if (auto const extractPath = updater::MakeAvailable(F_TAG, F_OWNER, F_REPO, F_TEMP_DIR, true))
    {
        SDL_MessageBoxData messageBoxData;
        messageBoxData.flags = SDL_MESSAGEBOX_INFORMATION;
        messageBoxData.window = nullptr;
        messageBoxData.title = "Game update";
        messageBoxData.message = "A new version of the game is available, do you want to update now? (from Github)";
        SDL_MessageBoxButtonData buttons[] = {
                {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "Yes"},
                {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 1, "No"},
        };
        messageBoxData.numbuttons = std::size(buttons);
        messageBoxData.buttons = buttons;
        int buttonId = 1; // Default to "No"
        if (SDL_ShowMessageBox(&messageBoxData, &buttonId) < 0)
        {
            std::cout << "Error displaying message box: " << SDL_GetError() << std::endl;
            std::cout << "Can't ask for update, if you want to update it, please go to the game main website"
                      << std::endl;
        }
        else
        {
            if (buttonId == 1)
            {
                std::cout << "Update cancelled" << std::endl;
            }
            else
            {
                std::cout << "Updating game..." << std::endl;
                if (updater::RequestApplyUpdate(*extractPath, std::filesystem::current_path() / argv[0]))
                {
                    return 0;
                }
            }
        }
    }

    std::cout << "FastEngine version: " << FGE_VERSION_FULL_WITHTAG_STRING << std::endl;
    if (fge::IsEngineBuiltInDebugMode())
    {
        std::cout << "Built in debug mode" << std::endl;
    }

    //Remove Vulkan validation layer
    InstanceLayers.clear();
    InstanceLayers.push_back("VK_LAYER_LUNARG_monitor");
    //InstanceLayers.push_back("VK_LAYER_KHRONOS_validation");

    if (!fge::net::Socket::initSocket())
    {
        return -1;
    }

    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "system");

    auto instance = Context::init(SDL_INIT_VIDEO | SDL_INIT_EVENTS,
                                  "Fichillsh [" F_TAG_STR "] on FastEngine [" FGE_VERSION_FULL_WITHTAG_STRING "]");
    Context::enumerateExtensions();

    SurfaceSDLWindow window(instance, FGE_WINDOWPOS_CENTERED, {1366, 768},
                            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

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
    std::unique_ptr<Scene> currentScene = std::make_unique<Scene>();

    do {
        GetActiveContext()._garbageCollector.enable(true);

        currentScene->run(renderWindow, network);

        GetActiveContext().waitIdle();
        GetActiveContext()._garbageCollector.enable(false);
        currentScene.reset();
    } while (currentScene);

    //Unloading resources

    renderWindow.destroy();

    vulkanContext.destroy();

    window.destroy();
    instance.destroy();
    SDL_Quit();

    return 0;
}
