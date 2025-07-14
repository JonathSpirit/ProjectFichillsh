#pragma once

#include "FastEngine/C_rect.hpp"
#include "FastEngine/manager/C_baseManager.hpp"

struct FishData
{
    enum class Rarity
    {
        COMMON,
        UNCOMMON,
        RARE
    };

    std::string _textureName;
    fge::RectInt _textureRect;
    float _weightMin = 0.0f;
    float _weightMax = 0.0f;
    float _lengthMin = 0.0f;
    float _lengthMax = 0.0f;
    Rarity _rarity = Rarity::COMMON;
};

struct FishInstance
{
    std::string _name;
    float _weight = 0.0f;
    float _length = 0.0f;
    uint8_t _starCount = 1; //1 to 5 stars
};

struct FishDataBlock : fge::manager::BaseDataBlock<FishData>
{};

class FishManager : public fge::manager::BaseManager<FishData, FishDataBlock>
{
public:
    using BaseManager::BaseManager;

    bool initialize() override;
    void uninitialize() override;

    bool loadFromFile(std::string_view fishName,
                        float weightMin, float weightMax,
                        float lengthMin, float lengthMax,
                        FishData::Rarity rarity,
                        std::filesystem::path const &path);

    std::string const& getRandomFishName() const;
    [[nodiscard]] FishInstance generateRandomFish() const;
};

extern FishManager gFishManager;