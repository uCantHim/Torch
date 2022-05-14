#pragma once

#include <cstring>
#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;

#include <shaderc/shaderc.hpp>

class FileIncluder : public shaderc::CompileOptions::IncluderInterface
{
public:
    explicit FileIncluder(const fs::path& _basePath)
    {
        if (_basePath.is_absolute()) {
            basePath = _basePath;
        }
        else {
            basePath = fs::current_path() / _basePath;
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
        const fs::path path = [&]{
            if (type == shaderc_include_type::shaderc_include_type_relative) {
                return basePath / fs::path{ requesting_source }.parent_path() / requested_source;
            }
            return basePath / requested_source;
        }();

        auto result = std::make_unique<shaderc_include_result>();
        auto res = result.get();
        auto [it, success] = pendingResults.try_emplace(res, path, std::move(result));
        assert(success);

        std::ifstream file(path);
        if (!file.is_open())
        {
            res->source_name = "";
            res->source_name_length = 0;
            res->content = "Unable to open file";
            res->content_length = strlen(res->content);
            return res;
        }

        auto& data = it->second;
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
        [[maybe_unused]]
        size_t erased = pendingResults.erase(data);
        assert(erased == 1);
    }

private:
    fs::path basePath;
    std::unordered_map<shaderc_include_result*, IncludeResult> pendingResults;
};
