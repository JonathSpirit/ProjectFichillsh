#include "fish.hpp"
#include "FastEngine/manager/texture_manager.hpp"
#include "FastEngine/C_random.hpp"

bool FishManager::initialize()
{
    if (this->isInitialized())
    {
        return true;
    }

    this->_g_badElement = std::make_shared<DataBlockType>();
    this->_g_badElement->_ptr = std::make_shared<DataType>();
    this->_g_badElement->_ptr->_textureRect = fge::RectInt({0, 0}, {FGE_TEXTURE_BAD_W, FGE_TEXTURE_BAD_H});
    this->_g_badElement->_valid = false;
    return true;
}

void FishManager::uninitialize()
{
    BaseManager::uninitialize();
}

bool FishManager::loadFromFile(std::string_view fishName,
                                float weightMin, float weightMax,
                                float lengthMin, float lengthMax,
                                FishData::Rarity rarity,
                                std::filesystem::path const &path)
{
    if (fishName.empty() ||
        weightMin < 0.0f || weightMax < 0.0f ||
        lengthMin < 0.0f || lengthMax < 0.0f ||
        weightMin > weightMax || lengthMin > lengthMax)
    {
        return false;
    }

    if (!fge::texture::gManager.contains(fishName))
    {
        if (!fge::texture::gManager.loadFromFile(fishName, path))
        {
            return false;
        }
    }

    DataBlockPointer block = std::make_shared<DataBlockType>();
    block->_ptr = std::make_shared<DataType>();
    block->_ptr->_textureName = fishName;
    block->_ptr->_textureRect = fge::RectInt{{0,0}, fge::texture::gManager.getElement(fishName)->_ptr->getSize()};
    block->_ptr->_weightMin = weightMin;
    block->_ptr->_weightMax = weightMax;
    block->_ptr->_lengthMin = lengthMin;
    block->_ptr->_lengthMax = lengthMax;
    block->_ptr->_rarity = rarity;
    block->_valid = true;

    if (this->push(fishName, std::move(block)))
    {
        return true;
    }
    return false;
}

std::string const& FishManager::getRandomFishName() const
{
    auto index = fge::_random.range<std::size_t>(0, this->size() - 1);

    auto lock = this->acquireLock();
    auto it = this->begin(lock);

    while (index != 0)
    {
        ++it;
        --index;
    }

    return it->first;
}
FishInstance FishManager::generateRandomFish() const
{
    FishInstance fishInstance;
    fishInstance._name = this->getRandomFishName();
    auto const& fishData = this->getElement(fishInstance._name);

    fishInstance._weight = fge::_random.range(fishData->_ptr->_weightMin, fishData->_ptr->_weightMax);
    fishInstance._length = fge::_random.range(fishData->_ptr->_lengthMin, fishData->_ptr->_lengthMax);

    fishInstance._starCount = 1; //Default to 1 star

    //Cut the weight to 3 part, where the first part is 0 star added, the second part is 1 star added, and the third part is 2 stars added
    auto const weightDelta = (fishData->_ptr->_weightMax - fishData->_ptr->_weightMin) / 3.0f;
    if (fishInstance._weight > fishData->_ptr->_weightMin + 2.0f * weightDelta)
    {
        fishInstance._starCount += 2; //2 stars
    }
    else if (fishInstance._weight >  fishData->_ptr->_weightMin + weightDelta)
    {
        fishInstance._starCount = 1; //1 star
    }

    //Same for the length
    auto const lengthDelta = (fishData->_ptr->_lengthMax - fishData->_ptr->_lengthMin) / 3.0f;
    if (fishInstance._length > fishData->_ptr->_lengthMin + 2.0f * lengthDelta)
    {
        fishInstance._starCount += 2; //2 stars
    }
    else if (fishInstance._length > fishData->_ptr->_lengthMin + lengthDelta)
    {
        fishInstance._starCount = 1; //1 star
    }

    return fishInstance;
}

FishManager gFishManager;