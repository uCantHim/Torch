#pragma once

#include <memory>
#include <string>
#include <vector>

#include "trc/base/Device.h"

namespace trc
{
    /**
     * @brief Create a shader module from shader code
     */
    auto makeShaderModule(const trc::Device& device, const std::string& code)
        -> vk::UniqueShaderModule;

    class ShaderProgram
    {
    public:
        explicit ShaderProgram(const trc::Device& device);

        void addStage(vk::ShaderStageFlagBits type, const std::string& shaderCode);
        void addStage(vk::ShaderStageFlagBits type,
                      const std::string& shaderCode,
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

        std::vector<vk::UniqueShaderModule> modules;
        std::vector<std::unique_ptr<vk::SpecializationInfo>> specInfos;
        std::vector<vk::PipelineShaderStageCreateInfo> createInfos;
    };
} // namespace trc
