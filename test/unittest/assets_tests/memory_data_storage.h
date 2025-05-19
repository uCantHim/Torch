#pragma once

#include <iosfwd>
#include <unordered_map>
#include <sstream>
#include <string>

#include <trc/util/DataStorage.h>

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
        return std::make_shared<std::stringstream>(*data);
    }

    auto write(const path& path) -> s_ptr<std::ostream> override
    {
        // TODO: This is a bit of a hack (a small one). Implement a real memory
        // stream that own its backing memory and can expand it as required
        // (starts at 0 and only allocates what is needed).
        auto [it, success] = storage.try_emplace(path, std::make_unique<std::string>());
        return s_ptr<std::stringstream>{
            new std::stringstream{ *it->second },
            [str=it->second.get()](std::stringstream* ss) {
                *str = ss->str();
                delete ss;
            }
        };
    }

    bool remove(const path& path) override {
        return storage.erase(path) > 0;
    }
private:
    std::unordered_map<path, u_ptr<std::string>> storage;
};
