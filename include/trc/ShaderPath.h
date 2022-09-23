#pragma once

#include <filesystem>

#include "util/Pathlet.h"

namespace trc
{
    namespace fs = std::filesystem;

    class ShaderPath
    {
    public:
        ShaderPath(const ShaderPath&) = default;
        ShaderPath(ShaderPath&&) noexcept = default;
        ShaderPath& operator=(const ShaderPath&) = default;
        ShaderPath& operator=(ShaderPath&&) noexcept = default;
        ~ShaderPath() noexcept = default;

        explicit ShaderPath(fs::path path);

        /**
         * @return Pathlet The shader's path fragment relative to the
         *                 shader directory.
         */
        auto getSourcePath() const -> const util::Pathlet&;

        /**
         * @return fs::path The file path of the corresponding SPIRV binary.
         *                  Is usually just the source path with a ".spv"
         *                  extension.
         */
        auto getBinaryPath() const -> util::Pathlet;

        auto operator<=>(const ShaderPath&) const = default;

    private:
        util::Pathlet pathlet;
    };
} // namespace trc

template<>
struct std::hash<trc::ShaderPath>
{
    auto operator()(const trc::ShaderPath& path) const noexcept
    {
        return hash<trc::util::Pathlet>{}(path.getSourcePath());
    }
};
