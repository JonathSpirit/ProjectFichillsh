#pragma once

#include "FastEngine/C_rect.hpp"
#include "FastEngine/manager/C_baseManager.hpp"

#define F_FISH_STAR_MAX 5
#define F_FISH_FORCE_FISH 0
#define F_FISH_FORCE_FISHNAME "duck-fish" // Set the fish to give in minigame when F_FISH_FORCE_FISH is 1

#define F_FISH_RARITY_COMMON_VALUE 10.0f
#define F_FISH_RARITY_UNCOMMON_VALUE 25.0f
#define F_FISH_RARITY_RARE_VALUE 50.0f
#define F_FISH_WEIGHT_MIN_VALUE 1.0f
#define F_FISH_WEIGHT_MAX_VALUE 10.0f
#define F_FISH_LENGTH_MIN_VALUE 1.0f
#define F_FISH_LENGTH_MAX_VALUE 10.0f

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
    uint8_t _starCount = 1;
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
                      float weightMin,
                      float weightMax,
                      float lengthMin,
                      float lengthMax,
                      FishData::Rarity rarity,
                      std::filesystem::path const& path);

    std::string const& getRandomFishName() const;
    [[nodiscard]] FishInstance generateRandomFish() const;

    [[nodiscard]] std::optional<float> retrieveMoneyValue(FishInstance const& instance) const;
};

extern FishManager gFishManager;

void to_json(nlohmann::json& j, FishInstance const& p);
void from_json(nlohmann::json const& j, FishInstance& p);
