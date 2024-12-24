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
    this->g_fishNames.clear();
}

bool FishManager::loadFromFile(std::string_view fishName, std::optional<fge::RectInt> textureRect,
    std::filesystem::path const &path)
{
    if (fishName.empty())
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
    block->_ptr->_textureRect = textureRect.value_or(fge::RectInt{{0,0}, fge::texture::gManager.getElement(fishName)->_ptr->getSize()});
    block->_valid = true;

    if (this->push(fishName, std::move(block)))
    {
        this->g_fishNames.emplace_back(fishName);
        return true;
    }
    return false;
}

std::string const& FishManager::getRandomFishName() const
{
    return this->g_fishNames[fge::_random.range<std::size_t>(0, this->g_fishNames.size() - 1)];
}

FishManager gFishManager;