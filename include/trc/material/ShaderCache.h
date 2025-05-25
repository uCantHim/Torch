#pragma once

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "trc/Types.h"

namespace trc
{
    namespace fs = std::filesystem;

    class ShaderCache
    {
    public:
        struct Entry
        {
            std::vector<ui32> spirvCode;

            // Time when SPIR-V was generated.
            std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();
        };

        explicit ShaderCache(const fs::path& cacheFile);

        auto query(const std::string& glsl) -> std::optional<Entry>;
        void store(const std::string& glsl, const Entry& entry);

        void clear();

    private:
        void tryCreateTable();

        struct Impl;
        s_ptr<Impl> impl;
    };
} // namespace trc
