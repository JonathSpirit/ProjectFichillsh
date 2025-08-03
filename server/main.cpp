#include "FastEngine/manager/reg_manager.hpp"
#include "FastEngine/network/C_server.hpp"
#include "FastEngine/fge_version.hpp"
#include "FastEngine/C_scene.hpp"
#include "FastEngine/C_clock.hpp"
#include "FastEngine/C_random.hpp"
#include "SDL.h"

#include <iostream>
#include <memory>
#include <csignal>

#include "../share/network.hpp"
#include "../share/player.hpp"

std::atomic_bool gRunning = true;

void signalCallbackHandler(int signum)
{
    if (signum == SIGINT || signum == SIGTERM)
    {
        std::cout << "received external interrupt signal !" << std::endl;
        gRunning = false;
    }
}

class Scene : public fge::Scene
{
public:
    ~Scene() override = default;

    void run(fge::net::ServerSideNetUdp& network)
    {
        auto& networkFlux = *network.getDefaultFlux();

        nlohmann::json config;
        if (!fge::LoadJsonFromFile("server.json", config))
        {
            std::cout << "Can't load server.json\n";
            return;
        }

        fge::net::Port port = config["port"].get<fge::net::Port>();

        std::string const versioningString = F_NET_STRING_SEQ + fge::string::ToStr(F_NET_SERVER_COMPATIBILITY_VERSION);
        network.setVersioningString(versioningString);

        if (!network.start(port, fge::net::IpAddress::Ipv4Any, fge::net::IpAddress::Types::Ipv4))
        {
            std::cout << "Can't start network\n";
            return;
        }

        this->g_playerEvents = this->_netList.push<std::remove_pointer_t<decltype(this->g_playerEvents)> >();

        fge::Event event;

        fge::Clock mainClock;

        //Init managers
        fge::texture::gManager.initialize();
        //fge::font::gManager.initialize();
        //fge::shader::gManager.initialize();
        fge::anim::gManager.initialize();

        //Load object (mostly for network)
        fge::reg::RegisterNewClass(std::make_unique<fge::reg::Stamp<Player>>());

        //Load textures
        //fge::texture::gManager.loadFromFile("OutdoorsTileset", "resources/tilesets/OutdoorsTileset.png");
        //fge::texture::gManager.loadFromFile("fishBait_1", "resources/sprites/fishBait_1.png");
        //fge::texture::gManager.loadFromFile("fishingFrame", "resources/sprites/fishingFrame.png");
        //fge::texture::gManager.loadFromFile("fishingIcon", "resources/sprites/fishingIcon.png");
        //fge::texture::gManager.loadFromFile("stars", "resources/sprites/stars.png");
        //fge::texture::gManager.loadFromFile("hearts", "resources/sprites/hearts.png");

        //Load animations
        //fge::anim::gManager.loadFromFile("human_1", "resources/sprites/human_1.json");
        //fge::anim::gManager.loadFromFile("ducky_1", "resources/sprites/ducky_1.json");

        //Load fonts
        //fge::font::gManager.loadFromFile("default", "resources/fonts/ttf/monogram.ttf");

        //Load fishes
        //gFishManager.loadFromFile("algae", std::nullopt, "resources/sprites/fishes/algae.png");
        //gFishManager.loadFromFile("anchovy", std::nullopt, "resources/sprites/fishes/fish-anchovy.png");
        //gFishManager.loadFromFile("bronze-striped-grunt", std::nullopt, "resources/sprites/fishes/fish-bronze-striped-grunt.png");
        //gFishManager.loadFromFile("butter", std::nullopt, "resources/sprites/fishes/fish-butter.png");
        //gFishManager.loadFromFile("gulf-croaker", std::nullopt, "resources/sprites/fishes/fish-gulf-croaker.png");
        //gFishManager.loadFromFile("halfbeak", std::nullopt, "resources/sprites/fishes/fish-halfbeak.png");
        //gFishManager.loadFromFile("herring", std::nullopt, "resources/sprites/fishes/fish-herring.png");
        //gFishManager.loadFromFile("pollock", std::nullopt, "resources/sprites/fishes/fish-pollock.png");
        //gFishManager.loadFromFile("sandlance", std::nullopt, "resources/sprites/fishes/fish-sandlance.png");
        //gFishManager.loadFromFile("sardine", std::nullopt, "resources/sprites/fishes/fish-sardine.png");
        //gFishManager.loadFromFile("krill", std::nullopt, "resources/sprites/fishes/krill.png");
        //gFishManager.loadFromFile("krill-1", std::nullopt, "resources/sprites/fishes/krill-1.png");
        //gFishManager.loadFromFile("krill-2", std::nullopt, "resources/sprites/fishes/krill-2.png");
        //gFishManager.loadFromFile("krill-3", std::nullopt, "resources/sprites/fishes/krill-3.png");
        //gFishManager.loadFromFile("shrimp-anemone", std::nullopt, "resources/sprites/fishes/shrimp-anemone.png");
        //gFishManager.loadFromFile("shrimp-northern-prawn", std::nullopt, "resources/sprites/fishes/shrimp-northern-prawn.png");
        //gFishManager.loadFromFile("squid-reef", std::nullopt, "resources/sprites/fishes/squid-reef.png");
        //gFishManager.loadFromFile("zoo-plankton", std::nullopt, "resources/sprites/fishes/zoo-plankton.png");
        //gFishManager.loadFromFile("zoo-plankton-small", std::nullopt, "resources/sprites/fishes/zoo-plankton-small.png");

        std::chrono::microseconds tickTime{0};

        networkFlux._clients.watchEvent(true);

        //Handling clients timeout
        networkFlux._onClientTimeout.addLambda([&](fge::net::ClientSharedPtr client, fge::net::Identity const& id) {
            std::cout << "client "<< id.toString() << " timeout !\n";
            this->disconnectPlayer(id);
        });
        networkFlux._onClientDisconnected.addLambda([&](fge::net::ClientSharedPtr client, fge::net::Identity const& id) {
            std::cout << "client "<< id.toString() << " disconnected !\n";
            this->disconnectPlayer(id);
        });

        //Handling clients connection
        networkFlux._onClientConnected.addLambda([](fge::net::ClientSharedPtr const& client, fge::net::Identity const& id) {
            std::cout << "client "<< id.toString() << " is connected and now try to authenticate !\n";
        });

        //Handling clients return packet
        networkFlux._onClientReturnEvent.addLambda([&](fge::net::ClientSharedPtr const& client, fge::net::Identity id,
                                                       fge::net::ReceivedPacketPtr const& packet) {
                                                           using namespace fge::net::rules;
            auto err = RStrictLess<StatEvents>(StatEvents::EVENT_COUNT, {packet->packet()})
                   .and_then([&](auto& chain) {
                       switch (chain.value())
                       {
                       case StatEvents::CAUGHT_FISH:{
                           std::string fishName;
                           chain.packet() >> fishName;
                           if (chain.packet().isValid())
                           {
                               auto const playerId = this->getPlayerId(id);
                               std::cout << "Player " << playerId << " caught a fish " << fishName << "\n";
                               this->g_playerEvents->pushEventIgnore(std::make_pair(StatEvents::CAUGHT_FISH, PlayerEventData{playerId, fishName}), id);
                           }
                           break;
                       }}
                       return chain;
                   }).end();

            if (err)
            {
                std::cout << "Error in client packet: \n";
                err->dump(std::cout);
            }
        });

        networkFlux._onClientReturnPacket.addLambda([&](fge::net::ClientSharedPtr const& client, fge::net::Identity id,
                                                       fge::net::ReceivedPacketPtr const& packet) {
            if (client->getStatus().getNetworkStatus() != fge::net::ClientStatus::NetworkStatus::AUTHENTICATED)
            {
                return;
            }

            auto playerObj = this->findPlayerObject(this->getPlayerId(id));

            using namespace fge::net::rules;
            auto err = RValid<fge::Vector2f>(*packet)
                .and_then([&](auto& chain) {
                    playerObj->setPosition(chain.value());
                    return RValid<fge::Vector2i>(chain);
                }).and_then([&](auto& chain) {
                    playerObj->setDirection(chain.value());
                    return RValid<Player::Stats_t>(chain);
                }).and_then([&](auto& chain) {
                    playerObj->setStat(static_cast<Player::Stats>(chain.value()));
                    return chain;
                }).end();

            this->unpackNeededUpdate(packet->packet(), id);

            if (err)
            {
                std::cout << "Error in client packet: \n";
                err->dump(std::cout);
                return;
            }
            if (!packet->endReached())
            {
                std::cout << "Error in client packet: Remaining data at the end of the packet\n";
                return;
            }

            //We reset the timeout
            client->getStatus().resetTimeout();
        });

        while (gRunning)
        {
            std::chrono::microseconds tickDelay = std::chrono::milliseconds{F_TICK_TIME} - tickTime;
            if (tickDelay.count() <= 0)
            {
                std::cout << "Can't keep up with the tick " << tickTime.count() << '\n';
            }
            else
            {
                fge::Sleep(tickDelay);
            }

            auto tickTimeStart = std::chrono::steady_clock::now();

            auto const deltaTime = std::chrono::duration_cast<fge::DeltaTime>(mainClock.restart());

            //Receive packets
            fge::net::ReceivedPacketPtr netPacket;
            fge::net::ClientSharedPtr client;
            fge::net::FluxProcessResults processResult;
            do
            {
                processResult = networkFlux.process(client, netPacket, true);
                if (processResult != fge::net::FluxProcessResults::USER_RETRIEVABLE)
                {
                    continue;
                }

                switch (static_cast<PacketHeaders>(netPacket->retrieveHeaderId().value()))
                {
                case CLIENT_ASK_CONNECT:{
                    //Check current stat
                    if (client->getStatus().getNetworkStatus() == fge::net::ClientStatus::NetworkStatus::AUTHENTICATED)
                    {
                        auto packet = fge::net::CreatePacket(CLIENT_ASK_CONNECT);
                        packet->doNotDiscard().doNotReorder().packet() << false << "Client already connected";
                        client->pushPacket(std::move(packet));
                        break;
                    }

                    using namespace fge::net::rules;
                    std::string dataHello;
                    auto err = RValid(RSizeMustEqual<std::string>(sizeof(F_NET_CLIENT_HELLO) - 1, {netPacket->packet(), &dataHello})).end();

                    if (err)
                    {
                        std::cout << "Error in CLIENT_HELLO: \n";
                        err->dump(std::cout);
                        client->disconnect();
                        break;
                    }

                    fge::Vector2f position;
                    netPacket->packet() >> position;

                    if (!netPacket->isValid())
                    {
                        std::cout << "Error in CLIENT_ASK_CONNECT: Invalid data\n";
                        client->disconnect();
                        break;
                    }
                    if (!netPacket->endReached())
                    {
                        std::cout << "Error in CLIENT_ASK_CONNECT: Remaining data at the end of the packet\n";
                        client->disconnect();
                        break;
                    }
                    if (dataHello != F_NET_CLIENT_HELLO)
                    {
                        auto packet = fge::net::CreatePacket(CLIENT_ASK_CONNECT);
                        packet->doNotDiscard().doNotReorder().packet() << false << "Bad strings";
                        client->pushPacket(std::move(packet));
                        client->disconnect();
                    }

                    auto player = this->newObject<Player>();
                    auto playerId = this->generatePlayerId(netPacket->getIdentity());
                    player->_properties["playerId"] = playerId;
                    player->setPosition(position);
                    player->_netList.ignoreClient(netPacket->getIdentity());

                    client->getStatus().setNetworkStatus(fge::net::ClientStatus::NetworkStatus::AUTHENTICATED);
                    client->getStatus().setTimeout(F_NET_CLIENT_TIMEOUT_CONNECT_MS);

                    auto packet = fge::net::CreatePacket(CLIENT_ASK_CONNECT);
                    packet->doNotDiscard().doNotReorder().packet() << true << F_NET_SERVER_HELLO;
                    this->packFullUpdate(networkFlux, netPacket->getIdentity(), packet);
                    client->pushPacket(std::move(packet));

                    std::cout << "Client connected " << netPacket->getIdentity().toString() << "\n";
                    break;
                }default:
                    break;
                }
            }
            while (processResult != fge::net::FluxProcessResults::NONE_AVAILABLE);

            /**MAIN LOOP**/

            ///CLIENTS CHECKUP
            this->clientsCheckup(networkFlux._clients); //clients checkup

            ///UPDATING SCENE
            this->update(event, deltaTime);

            ///SENDING DATA
            {
                auto lock = networkFlux._clients.acquireLock();

                bool mustNotify = false;

                for (auto itClient = networkFlux._clients.begin(lock); itClient != networkFlux._clients.end(lock); ++itClient)
                {
                    auto& currentClient = itClient->second._client;

                    if (currentClient->getStatus().getNetworkStatus() != fge::net::ClientStatus::NetworkStatus::AUTHENTICATED)
                    {
                        continue;
                    }

                    if (currentClient->isPendingPacketsEmpty())
                    {
                        mustNotify = true;

                        auto packet = fge::net::CreatePacket();

                        /*if (clientData->_needFullUpdate) TODO: full update
                        {
                            clientData->_needFullUpdate = false;
                            packet->packet().setHeader(PRS_S_FULL_UPDATE);
                            packet->doNotDiscard();

                            currentClient->_latencyPlanner.pack(packet);

                            packet->packet() << clientData->_playerData->_player._camLocation;

                            scene->pack(packet->packet());
                            currentClient->pushPacket(std::move(packet));
                            continue;
                        }*/

                        packet->setHeaderId(SERVER_UPDATE);

                        currentClient->_latencyPlanner.pack(packet);
                        this->packModification(packet->packet(), itClient->first);
                        //this->packWatchedEvent(packet->packet(), itClient->first);

                        currentClient->pushPacket(std::move(packet));
                    }
                }
                if (mustNotify)
                {
                    network.notifyTransmission();
                }
                for (auto itClient = networkFlux._clients.begin(lock); itClient != networkFlux._clients.end(lock); ++itClient)
                {
                    auto& currentClient = itClient->second._client;
                    if (currentClient->getStatus().getNetworkStatus() != fge::net::ClientStatus::NetworkStatus::AUTHENTICATED)
                    {
                        continue;
                    }
                    currentClient->_data.delProperty("caughtFish");
                }
            }

            networkFlux._clients.clearClientEvent();

            //Tick time
            tickTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - tickTimeStart);
        }

        network.stop();

        fge::texture::gManager.uninitialize();
        //fge::font::gManager.uninitialize();
        //fge::shader::gManager.uninitialize();
        fge::anim::gManager.uninitialize();
    }

    void packFullUpdate(fge::net::ServerNetFluxUdp& networkFlux, fge::net::Identity const& identity, fge::net::TransmitPacketPtr& packet)
    {
        auto const yourPlayerId = this->getPlayerId(identity);
        if (yourPlayerId.empty())
        {
            //Should not really happen
            return;
        }

        packet->packet() << yourPlayerId;

        this->pack(packet->packet(), identity);
    }

    void disconnectPlayer(fge::net::Identity const& id)
    {
        auto const playerId = this->getPlayerId(id);
        if (playerId.empty())
        {
            return;
        }

        auto player = this->findPlayerObject(playerId);
        if (player == nullptr)
        {
            std::cout << "Player object not found for playerId: " << playerId << "\n";
            return;
        }

        this->delObject(player->_myObjectData.lock()->getSid());
        this->removePlayerId(playerId);
        this->g_playerEvents->pushEventIgnore(std::make_pair(StatEvents::PLAYER_DISCONNECTED, PlayerEventData{playerId, ""}), id);
    }

    std::string const& generatePlayerId(fge::net::Identity const& identity)
    {
        auto const it = this->g_playerIds.find(identity);
        if (it != this->g_playerIds.end())
        {
            return it->second;
        }

        std::string newPlayerId;
        do
        {
            newPlayerId = fge::_random.randStr(32);
        }
        while (this->g_playerIdentities.contains(newPlayerId));

        this->g_playerIds[identity] = newPlayerId;
        this->g_playerIdentities[newPlayerId] = identity;
        return this->g_playerIds[identity];
    }
    std::string getPlayerId(fge::net::Identity const& identity)
    {
        auto const it = this->g_playerIds.find(identity);
        if (it != this->g_playerIds.end())
        {
            return it->second;
        }
        return std::string{};
    }
    void removePlayerId(fge::net::Identity const& identity)
    {
        auto const it = this->g_playerIds.find(identity);
        if (it != this->g_playerIds.end())
        {
            this->g_playerIdentities.erase(it->second);
            this->g_playerIds.erase(it);
        }
    }
    void removePlayerId(std::string const& playerId)
    {
        auto const it = this->g_playerIdentities.find(playerId);
        if (it != this->g_playerIdentities.end())
        {
            this->g_playerIds.erase(it->second);
            this->g_playerIdentities.erase(it);
        }
    }

    Player* findPlayerObject(std::string const& playerId) const
    {
        fge::ObjectContainer container;
        if (this->getAllObj_ByClass("FISH_PLAYER", container) == 0)
        {
            return nullptr;
        }

        for (auto const& obj : container)
        {
            if (obj->getObject()->_properties["playerId"] == playerId)
            {
                return obj->getObject<Player>();
            }
        }

        return nullptr;
    }

private:
    std::unordered_map<fge::net::Identity, std::string, fge::net::IdentityHash> g_playerIds;
    std::unordered_map<std::string, fge::net::Identity> g_playerIdentities;
    fge::net::NetworkTypeEvents<StatEvents, PlayerEventData>* g_playerEvents{nullptr};
};

int main(int argc, char *argv[])
{
    using namespace fge::vulkan;

    //Verify update TODO: GRUpdater isn't available for linux yet
    /*std::filesystem::remove_all(F_TEMP_DIR);
    if (auto const extractPath = updater::MakeAvailable(F_TAG, F_OWNER, F_REPO, F_TEMP_DIR, true))
    {
        if (updater::RequestApplyUpdate(*extractPath, std::filesystem::current_path() / argv[0]))
        {
            return 0;
        }
    }*/

    if (std::signal(SIGINT, signalCallbackHandler) == SIG_ERR)
    {
        std::cout << "can't set the signal handler ! (continuing anyway)" << std::endl;
    }

    std::cout << "FastEngine version: " << FGE_VERSION_FULL_WITHTAG_STRING << std::endl;
    if (fge::IsEngineBuiltInDebugMode())
    {
        std::cout << "Built in debug mode" << std::endl;
    }

    if ( !fge::net::Socket::initSocket() )
    {
        return -1;
    }

    fge::net::ServerSideNetUdp network;

    //Loading resources

    //Loading scenes
    std::unique_ptr<Scene> clientShareStatsScene  = std::make_unique<Scene>();
    clientShareStatsScene->run(network);
    clientShareStatsScene.reset();

    //Unloading resources

    SDL_Quit();

    return 0;
}
