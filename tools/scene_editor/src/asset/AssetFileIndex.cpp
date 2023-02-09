#include "AssetFileIndex.h"

#include <fstream>

#include <nlohmann/json.hpp>
namespace nl = nlohmann;



namespace nlohmann
{
    /**
     * Specialize adl_serializer for trc::AssetPath because it is not
     * default constructible.
     */
    template<>
    struct adl_serializer<trc::AssetPath>
    {
        static auto from_json(const json& j) -> trc::AssetPath
        {
            return trc::AssetPath{ j.get<std::string>() };
        }

        static void to_json(json& j, const trc::AssetPath& path)
        {
            j = path.string();
        }
    };
}

void to_json(nl::json& j, const AssetFileIndex::AssetInfo& info)
{
    j = { { "type", static_cast<ui32>(info.type) } };
}

void from_json(const nl::json& j, AssetFileIndex::AssetInfo& info)
{
    assert(j.at("type").get<ui32>() < static_cast<ui32>(AssetType::eMaxEnum));

    info.type = static_cast<AssetType>(j.at("type").get<ui32>());
}



void AssetFileIndex::load(const fs::path& indexFile)
{
    std::ifstream file(indexFile);
    if (!file.is_open())
    {
        throw std::runtime_error("[In AssetFileIndex::save]:"
                                 " Unable to open file " + indexFile.string() + " for reading.");
    }

    load(file);
}

void AssetFileIndex::load(std::istream& is)
{
    try {
        std::scoped_lock lock(indexLock);
        nl::json json = nl::json::parse(is);
        index = json.at("availableAssets").get<decltype(index)>();
    }
    catch (const nl::json::parse_error&) {
        throw std::runtime_error("[In AssetFileIndex::load]: Input does not contain valid JSON.");
    }


    return;

    for (auto it = index.begin(); it != index.end(); /*nothing*/)
    {
        const auto& [path, _] = *it;
        if (!fs::is_regular_file(trc::util::getAssetStorageDirectory() / path.string())) {
            it = index.erase(it);
        }
        else {
            ++it;
        }
    }
}

void AssetFileIndex::save(const fs::path& indexFile)
{
    std::ofstream file(indexFile);
    if (!file.is_open())
    {
        throw std::runtime_error("[In AssetFileIndex::save]:"
                                 " Unable to open file " + indexFile.string() + " for writing.");
    }

    save(file);
}

void AssetFileIndex::save(std::ostream& os)
{
    std::shared_lock lock(indexLock);
    os << nl::json{
        { "availableAssets", index }
    };
}

bool AssetFileIndex::contains(const trc::AssetPath& path) const
{
    std::shared_lock lock(indexLock);
    return index.contains(path);
}

auto AssetFileIndex::getType(const trc::AssetPath& path) const -> std::optional<AssetType>
{
    std::shared_lock lock(indexLock);

    auto it = index.find(path);
    if (it != index.end()) {
        return it->second.type;
    }
    return std::nullopt;
}

void AssetFileIndex::erase(const trc::AssetPath& path)
{
    std::scoped_lock lock(indexLock);
    index.erase(path);
}
