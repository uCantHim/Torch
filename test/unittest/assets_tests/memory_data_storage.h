#include <unordered_map>
#include <vector>

#include <trc/util/DataStorage.h>
#include <trc_util/MemoryStream.h>

using namespace trc::basic_types;

/**
 * A simple in-memory implementation of trc::DataStorage. We use this to store
 * data at asset paths.
 */
class MemoryStorage : public trc::DataStorage
{
public:
    auto read(const path& path) -> s_ptr<std::istream> override {
        auto it = storage.find(path);
        if (it == storage.end()) {
            return nullptr;
        }

        auto& data = it->second;
        return std::make_shared<trc::util::MemoryStream>((char*)data.data(), data.size());
    }

    auto write(const path& path) -> s_ptr<std::ostream> override {
        auto [it, success] = storage.try_emplace(path, std::vector<std::byte>(1000000));
        auto& data = it->second;
        return std::make_shared<trc::util::MemoryStream>((char*)data.data(), data.size());
    }

    bool remove(const path& path) override {
        return storage.erase(path) > 0;
    }
private:
    std::unordered_map<path, std::vector<std::byte>> storage;
};
