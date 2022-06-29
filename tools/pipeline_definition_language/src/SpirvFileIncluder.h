#pragma once

#include <cassert>
#include <cstring>

#include <unordered_map>
#include <mutex>
#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;

#include <shaderc/shaderc.hpp>

class FileIncluder : public shaderc::CompileOptions::IncluderInterface
{
public:
    explicit FileIncluder(const fs::path& _basePath, std::vector<fs::path> additionalPaths = {})
        :
        includePaths(std::move(additionalPaths))
    {
        if (_basePath.is_absolute()) {
            includePaths.insert(includePaths.begin(), _basePath);
        }
        else {
            includePaths.insert(includePaths.begin(), fs::current_path() / _basePath);
        }
    }

    struct IncludeResult
    {
        fs::path fullPath;
        std::unique_ptr<shaderc_include_result> result;
        std::string content;
    };

    // Handles shaderc_include_resolver_fn callbacks.
    auto GetInclude(const char* requested_source,
                    shaderc_include_type type,
                    const char* requesting_source,
                    size_t) -> shaderc_include_result* override
    {
        auto makeFullPath = [&](const fs::path& base) {
            if (type == shaderc_include_type::shaderc_include_type_relative)
            {
                const auto parentPath = fs::path{ requesting_source }.parent_path();
                if (!parentPath.is_absolute()) {
                    return base / parentPath / requested_source;
                }
            }
            return base / requested_source;
        };

        // Test all available include paths
        const fs::path path = [&]{
            for (const auto& base : includePaths)
            {
                const auto path = makeFullPath(base);
                if (fs::is_regular_file(path)) {
                    return path;
                }
            }
            return fs::path{};
        }();

        auto result = std::make_unique<shaderc_include_result>();
        auto res = result.get();

        std::ifstream file(path);
        if (!file.is_open())
        {
            res->source_name = "";
            res->source_name_length = 0;
            res->content = "Unable to open file";
            res->content_length = strlen(res->content);
            return res;
        }

        // Insert pending include result into list
        auto& data = [&]() -> IncludeResult& {
            std::scoped_lock lock(pendingResultsLock);
            auto [it, success] = pendingResults.try_emplace(res, path, std::move(result));
            assert(success);

            return it->second;
        }();

        std::stringstream ss;
        ss << file.rdbuf();
        data.content = ss.str();

        res->source_name        = data.fullPath.c_str();
        res->source_name_length = data.fullPath.string().size();
        res->content            = data.content.c_str();
        res->content_length     = data.content.size();

        return res;
    }

    // Handles shaderc_include_result_release_fn callbacks.
    void ReleaseInclude(shaderc_include_result* data) override
    {
        std::scoped_lock lock(pendingResultsLock);
        pendingResults.erase(data);
    }

private:
    std::vector<fs::path> includePaths;

    std::mutex pendingResultsLock;
    std::unordered_map<shaderc_include_result*, IncludeResult> pendingResults;
};
