#pragma once

#include <filesystem>
#include <memory>
#include <vector>

#include "trc/base/Device.h"

namespace trc
{
    namespace fs = std::filesystem;

    /**
     * @brief Read a file into a std::vector<uint32_t>
     *
     * @throw std::runtime_error if the file cannot be opened or the file's
     *        size is not a multiple of four.
     */
    auto readSpirvFile(const fs::path& path) -> std::vector<uint32_t>;

    /**
     * @brief Create a shader module from shader code
     */
    auto makeShaderModule(const trc::Device& device, const std::vector<uint32_t>& code)
        -> vk::UniqueShaderModule;

    class ShaderProgram
    {
    public:
        explicit ShaderProgram(const trc::Device& device);

        void addStage(vk::ShaderStageFlagBits type, std::vector<uint32_t> shaderCode);
        void addStage(vk::ShaderStageFlagBits type,
                      std::vector<uint32_t> shaderCode,
                      vk::SpecializationInfo specializationInfo);

        void setSpecialization(vk::ShaderStageFlagBits stage, vk::SpecializationInfo info);

        /**
         * @brief Retrieve program information for pipeline creation
         *
         * As programs are never really explicitly created 'objects' in
         * Vulkan, this returns the collection of shader modules that has
         * been created by previous calls to `ShaderProgram::addStage`.
         *
         * The user must ensure that the returned reference (or a copy of
         * its value) never outlives the `ShaderProgram` object.
         */
        auto getStageCreateInfo() const & -> const std::vector<vk::PipelineShaderStageCreateInfo>&;

    private:
        const Device& device;

        std::vector<std::vector<uint32_t>> shaderCodes;

        std::vector<vk::UniqueShaderModule> modules;
        std::vector<std::unique_ptr<vk::SpecializationInfo>> specInfos;
        std::vector<vk::PipelineShaderStageCreateInfo> createInfos;
    };
} // namespace trc
