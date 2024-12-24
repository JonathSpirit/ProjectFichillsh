#pragma once

#include "FastEngine/C_rect.hpp"
#include "FastEngine/manager/C_baseManager.hpp"

struct FishData
{
    std::string _textureName;
    fge::RectInt _textureRect;
};

struct FishDataBlock : fge::manager::BaseDataBlock<FishData>
{};

class FishManager : public fge::manager::BaseManager<FishData, FishDataBlock>
{
public:
    using BaseManager::BaseManager;

    bool initialize() override;
    void uninitialize() override;

    bool loadFromFile(std::string_view fishName, std::optional<fge::RectInt> textureRect, std::filesystem::path const& path);

    std::string const& getRandomFishName() const;

private:
    std::vector<std::string> g_fishNames;
};

extern FishManager gFishManager;