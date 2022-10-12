#pragma once

#include <cassert>
#include <cstring>

#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
namespace fs = std::filesystem;

#include <shaderc/shaderc.hpp>

namespace spirv
{
    class FileIncluder : public shaderc::CompileOptions::IncluderInterface
    {
    public:
        /**
         * @param fs::path basePath Primary include path
         * @param std::vector<fs::path> additionalPaths An optional list of
         *        additional include paths.
         */
        explicit FileIncluder(const fs::path& basePath, std::vector<fs::path> additionalPaths = {});

        /** Handles shaderc_include_resolver_fn callbacks. */
        auto GetInclude(const char* requested_source,
                        shaderc_include_type type,
                        const char* requesting_source,
                        size_t)
            -> shaderc_include_result* override;

        /** Handles shaderc_include_result_release_fn callbacks. */
        void ReleaseInclude(shaderc_include_result* data) override;

    private:
        struct IncludeResult
        {
            IncludeResult(const fs::path& fullPath, std::unique_ptr<shaderc_include_result> res)
                : fullPath(fullPath.string()), result(std::move(res)) {}

            std::string fullPath;
            std::unique_ptr<shaderc_include_result> result;
            std::string content;
        };

        std::vector<fs::path> includePaths;

        std::mutex pendingResultsLock;
        std::unordered_map<shaderc_include_result*, IncludeResult> pendingResults;
    };
} // namespace spirv
