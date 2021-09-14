#pragma once

#include <string>
#include <filesystem>
namespace fs = std::filesystem;

namespace shader_edit
{
    /**
     * @brief
     */
    class ShaderCodeSource
    {
    public:
        ShaderCodeSource() = default;
        virtual ~ShaderCodeSource() = default;

        virtual auto getCode() const -> std::string = 0;
    };

    /**
     * @brief
     */
    class FileSource : public ShaderCodeSource
    {
    public:
        /**
         * @brief
         *
         * @param const fs::path& filePath
         */
        explicit FileSource(const fs::path& filePath);

        auto getCode() const -> std::string override;

    private:
        fs::path filePath;
    };

    /**
     * @brief
     */
    class StringSource : public ShaderCodeSource
    {
    public:
        /**
         * @brief
         *
         * @param const fs::path& filePath
         */
        explicit StringSource(std::string str);

        auto getCode() const -> std::string override;

    private:
        std::string str;
    };
} // namespace shader_edit
