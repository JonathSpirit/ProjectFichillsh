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
#include "player.hpp"

#define F_TICK_TIME 50

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

        if (!network.start<fge::net::PacketLZ4>(port, fge::net::IpAddress::Ipv4Any, fge::net::IpAddress::Types::Ipv4))
        {
            std::cout << "Can't start network\n";
            return;
        }

        fge::Event event;

        fge::Clock mainClock;

        //Init managers
        fge::texture::gManager.initialize();
        //fge::font::gManager.initialize();
        //fge::shader::gManager.initialize();
        fge::anim::gManager.initialize();

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

            /**CHECK ALL CLIENT FOR TIMEOUT**/
            {
                auto pckTimeout = fge::net::TransmissionPacket::create(SERVER_GOODBYE);
                pckTimeout->doNotDiscard().doNotReorder().packet() << "timeout";

                auto lock = networkFlux._clients.acquireLock();

                for (auto client = networkFlux._clients.begin(lock); client != networkFlux._clients.end(lock);)
                {
                    auto timeout = std::chrono::duration_cast<fge::DeltaTime>(std::chrono::milliseconds{client->second->_data["timeout"].get<std::chrono::milliseconds::rep>().value()});
                    timeout -= deltaTime;
                    client->second->_data["timeout"] = std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count();

                    if (timeout <= fge::DeltaTime{0})
                    {//Remove user
                        network.sendTo(pckTimeout, client->first);

                        std::cout << "bad client "<< client->first._ip.toString().value_or("UNKNOWN") <<" timeout !\n";

                        auto const playerId = this->getPlayerId(client->first);
                        if (auto player = this->getFirstObj_ByTag("player_" + playerId))
                        {
                            this->delObject(player->getSid());
                        }
                        this->removePlayerId(playerId);

                        auto nextClient = client; ++nextClient;
                        networkFlux._clients.remove(client->first);
                        client = nextClient;
                    }
                    else
                    {
                        ++client;
                    }
                }
            }

            //Receive packets
            fge::net::FluxPacketPtr netPckFlux;
            fge::net::ClientSharedPtr client;
            while (networkFlux.process(client, netPckFlux, true) == fge::net::FluxProcessResults::RETRIEVABLE)
            {
                switch (static_cast<PacketHeaders>(netPckFlux->retrieveHeaderId().value()))
                {
                case CLIENT_HELLO:{
                    if (client)
                    {
                        auto packet = fge::net::TransmissionPacket::create(CLIENT_HELLO);
                        packet->doNotDiscard().doNotReorder().packet() << false << "Client already known and connected";
                        network.sendTo(packet, netPckFlux->getIdentity());
                    }
                    else
                    {
                        using namespace fge::net::rules;
                        std::string dataHello;
                        std::string dataStringSeq;
                        auto err = RValid(RSizeMustEqual<std::string>(sizeof(F_NET_CLIENT_HELLO) - 1, {netPckFlux->packet(), &dataHello}))
                            .and_then([&](auto& chain) {
                                return RValid(RSizeMustEqual<std::string>(sizeof(F_NET_STRING_SEQ) - 1, chain.newChain(&dataStringSeq)));
                            }).end();

                        if (err)
                        {
                            std::cout << "Error in CLIENT_HELLO: \n";
                            err->dump(std::cout);
                            break;
                        }

                        if (!netPckFlux->endReached())
                        {
                            std::cout << "Error in CLIENT_HELLO: Remaining data at the end of the packet\n";
                            break;
                        }

                        if (dataHello != F_NET_CLIENT_HELLO || dataStringSeq != F_NET_STRING_SEQ)
                        {
                            auto packet = fge::net::TransmissionPacket::create(CLIENT_HELLO);
                            packet->doNotDiscard().doNotReorder().packet() << false << "Bad strings";
                            network.sendTo(packet, netPckFlux->getIdentity());
                        }
                        else
                        {
                            auto packet = fge::net::TransmissionPacket::create(CLIENT_HELLO);
                            packet->doNotDiscard().doNotReorder().packet() << true << F_NET_SERVER_HELLO;
                            network.sendTo(packet, netPckFlux->getIdentity());

                            client = std::make_shared<fge::net::Client>();
                            client->setSkey( fge::net::Client::GenerateSkey() );
                            client->_data["timeout"] = F_NET_CLIENT_TIMEOUT_HELLO_MS.count();
                            client->_data["stat"] = ClientNetStats::SAY_HELLO;
                            networkFlux._clients.add(netPckFlux->getIdentity(), client);

                            std::cout << "New client say hello " << netPckFlux->getIdentity()._ip.toString().value_or("UNKNOWN") << " " << netPckFlux->getIdentity()._port << "\n";
                        }
                    }
                    break;
                }case CLIENT_ASK_CONNECT:{
                    if (!client)
                    {
                        auto packet = fge::net::TransmissionPacket::create(CLIENT_ASK_CONNECT);
                        packet->doNotDiscard().doNotReorder().packet() << false << "Unknown client";
                        network.sendTo(packet, netPckFlux->getIdentity());
                        break;
                    }

                    //Check current stat
                    if (*client->_data["stat"].get<ClientNetStats>() == ClientNetStats::CONNECTED)
                    {
                        auto packet = fge::net::TransmissionPacket::create(CLIENT_ASK_CONNECT);
                        packet->doNotDiscard().doNotReorder().packet() << false << "Client already connected";
                        network.sendTo(packet, netPckFlux->getIdentity());
                        break;
                    }

                    fge::Vector2f position;
                    netPckFlux->packet() >> position;

                    if (!netPckFlux->isValid())
                    {
                        std::cout << "Error in CLIENT_ASK_CONNECT: Invalid data\n";
                        break;
                    }

                    if (!netPckFlux->endReached())
                    {
                        std::cout << "Error in CLIENT_ASK_CONNECT: Remaining data at the end of the packet\n";
                        break;
                    }

                    client->_data["stat"] = ClientNetStats::CONNECTED;
                    client->_data["timeout"] = F_NET_CLIENT_TIMEOUT_CONNECT_MS.count();

                    auto player = this->newObject<Player>();
                    auto playerId = this->generatePlayerId(netPckFlux->getIdentity());
                    player->_tags.add("player_" + playerId);
                    player->setPosition(position);

                    auto packet = fge::net::TransmissionPacket::create(CLIENT_ASK_CONNECT);
                    packet->doNotDiscard().doNotReorder().packet() << true;
                    this->packFullUpdate(networkFlux, netPckFlux->getIdentity(), *packet);
                    network.sendTo(packet, netPckFlux->getIdentity());

                    std::cout << "Client connected " << netPckFlux->getIdentity()._ip.toString().value_or("UNKNOWN") << " " << netPckFlux->getIdentity()._port << "\n";
                    break;
                }case CLIENT_GOODBYE:{
                    if (client)
                    {
                        networkFlux._clients.remove(netPckFlux->getIdentity());
                        this->removePlayerId(netPckFlux->getIdentity());
                    }
                    break;
                }case CLIENT_STATS:{
                    if (!client)
                    {
                        break;
                    }

                    if (*client->_data["stat"].get<ClientNetStats>() != ClientNetStats::CONNECTED)
                    {
                        break;
                    }

                    client->_latencyPlanner.unpack(netPckFlux.get(), *client);
                    if (auto latency = client->_latencyPlanner.getLatency())
                    {
                        if (latency.value() > 200)
                        {
                            client->setSTOCLatency_ms(200);
                        }
                        else
                        {
                            client->setSTOCLatency_ms(latency.value());
                        }
                    }
                    if (auto latency = client->_latencyPlanner.getOtherSideLatency())
                    {
                        client->setCTOSLatency_ms(latency.value());
                    }

                    auto playerObj = this->getFirstObj_ByTag("player_" + this->getPlayerId(netPckFlux->getIdentity()))->getObject<Player>();

                    using namespace fge::net::rules;
                    auto err = RValid<fge::Vector2f>({netPckFlux->packet()})
                        .and_then([&](auto& chain) {
                            playerObj->setPosition(chain.value());
                            return RValid(chain.template newChain<fge::Vector2i>());
                        }).and_then([&](auto& chain) {
                            playerObj->setDirection(chain.value());
                            return RValid(chain.template newChain<Player::Stats_t>());
                        }).and_then([&](auto& chain) {
                            //TODO: set stat
                            return RValid(chain.template newChain<fge::net::SizeType>());
                        }).end(); //TODO: read events

                    if (err)
                    {
                        std::cout << "Error in CLIENT_STATS: \n";
                        err->dump(std::cout);
                        break;
                    }

                    if (!netPckFlux->endReached())
                    {
                        std::cout << "Error in CLIENT_STATS: Remaining data at the end of the packet\n";
                        break;
                    }

                    client->_data["timeout"] = F_NET_CLIENT_TIMEOUT_CONNECT_MS.count();
                    break;
                }default:
                    break;
                }
            }

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
                    if (itClient->second->_data["stat"].get<ClientNetStats>() != ClientNetStats::CONNECTED)
                    {
                        continue;
                    }

                    if (itClient->second->isPendingPacketsEmpty())
                    {
                        mustNotify = true;

                        auto transmissionPacket = fge::net::TransmissionPacket::create();

                        /*if (clientData->_needFullUpdate) TODO: full update
                        {
                            clientData->_needFullUpdate = false;
                            transmissionPacket->packet().setHeader(PRS_S_FULL_UPDATE);
                            transmissionPacket->doNotDiscard();

                            itClient->second->_latencyPlanner.pack(transmissionPacket);

                            transmissionPacket->packet() << clientData->_playerData->_player._camLocation;

                            scene->pack(transmissionPacket->packet());
                            itClient->second->pushPacket(std::move(transmissionPacket));
                            continue;
                        }*/

                        transmissionPacket->packet().setHeader(SERVER_UPDATE);

                        itClient->second->_latencyPlanner.pack(transmissionPacket);
                        this->packUpdate(networkFlux, itClient->first, *transmissionPacket);
                        //this->packModification(transmissionPacket->packet(), itClient->first);
                        //this->packWatchedEvent(transmissionPacket->packet(), itClient->first);

                        itClient->second->pushPacket(std::move(transmissionPacket));
                    }
                }
                if (mustNotify)
                {
                    network.notifyTransmission();
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

    void packUpdate(fge::net::ServerNetFluxUdp& networkFlux, fge::net::Identity const& identity, fge::net::TransmissionPacket& packet)
    {
        //TODO: only modified player stats, and events

        auto const yourPlayerId = this->getPlayerId(identity);
        if (yourPlayerId.empty())
        {
            //Should not really happen
            return;
        }

        fge::net::SizeType playerCount = 0;
        auto const playerCountRewritePos = packet.packet().getDataSize();
        packet.packet().append(sizeof playerCount);

        {
            auto lock = networkFlux._clients.acquireLock();
            for (auto it=networkFlux._clients.begin(lock), end=networkFlux._clients.end(lock); it != end; ++it)
            {
                if (it->second->_data["stat"].get<ClientNetStats>() != ClientNetStats::CONNECTED)
                {
                    continue;
                }

                auto const playerId = this->getPlayerId(it->first);

                if (playerId == yourPlayerId)
                {
                    continue;
                }

                packet.packet() << playerId;

                auto const player = this->getFirstObj_ByTag("player_" + playerId);
                auto const playerObj = player->getObject<Player>();
                packet.packet() << playerObj->getPosition()
                    << playerObj->getDirection()
                    << playerObj->getStat();

                //EVENT_COUNT
                packet.packet() << fge::net::SizeType{0};

                ++playerCount;
            }
        }

        packet.packet().pack(playerCountRewritePos, &playerCount, sizeof playerCount);
    }
    void packFullUpdate(fge::net::ServerNetFluxUdp& networkFlux, fge::net::Identity const& identity, fge::net::TransmissionPacket& packet)
    {
        auto const yourPlayerId = this->getPlayerId(identity);
        if (yourPlayerId.empty())
        {
            //Should not really happen
            return;
        }

        packet.packet() << yourPlayerId;

        fge::net::SizeType playerCount = 0;
        auto const playerCountRewritePos = packet.packet().getDataSize();
        packet.packet().append(sizeof playerCount);

        {
            auto lock = networkFlux._clients.acquireLock();
            for (auto it=networkFlux._clients.begin(lock), end=networkFlux._clients.end(lock); it != end; ++it)
            {
                if (it->second->_data["stat"].get<ClientNetStats>() != ClientNetStats::CONNECTED)
                {
                    continue;
                }

                auto const playerId = this->getPlayerId(it->first);

                if (playerId == yourPlayerId)
                {
                    continue;
                }

                packet.packet() << playerId;

                auto const player = this->getFirstObj_ByTag("player_" + playerId);
                auto const playerObj = player->getObject<Player>();
                packet.packet() << playerObj->getPosition()
                    << playerObj->getDirection()
                    << playerObj->getStat();

                //EVENT_COUNT
                packet.packet() << fge::net::SizeType{0};

                ++playerCount;
            }
        }

        packet.packet().pack(playerCountRewritePos, &playerCount, sizeof playerCount);
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

private:
    std::unordered_map<fge::net::Identity, std::string, fge::net::IdentityHash> g_playerIds;
    std::unordered_map<std::string, fge::net::Identity> g_playerIdentities;
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
