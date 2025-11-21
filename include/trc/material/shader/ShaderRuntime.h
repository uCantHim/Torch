#pragma once

#include <limits>
#include <optional>
#include <string>
#include <vector>

#include <trc_util/Assert.h>

#include "trc/Types.h"
#include "trc/VulkanInclude.h"

namespace trc::shader
{
    struct ShaderProgramData;

    /**
     * Handle to a resource. Used to interact with the shader runtime.
     */
    struct Resource
    {
        auto getName() const -> const std::string& {
            return hnd->name;
        }

        auto getInternalId() const -> ui32 {
            return hnd->internalId;
        }

    private:
        friend class ShaderProgramRuntime;

        Resource(std::string name, ui32 id)
            : hnd(std::make_shared<Hnd>(std::move(name), id))
        {}

        struct Hnd {
            std::string name;
            ui32 internalId;
        };
        s_ptr<Hnd> hnd;
    };

    /**
     * @brief A handle to a push constant value in a shader program.
     *
     * Queried from `ShaderProgramRuntime` objects.
     */
    struct PushConstant : Resource { using Resource::Resource; };

    class ShaderProgramRuntime
    {
    public:
        ShaderProgramRuntime(const ShaderProgramRuntime&) = default;
        ShaderProgramRuntime(ShaderProgramRuntime&&) noexcept = default;
        ShaderProgramRuntime& operator=(const ShaderProgramRuntime&) = default;
        ShaderProgramRuntime& operator=(ShaderProgramRuntime&&) noexcept = default;
        ~ShaderProgramRuntime() noexcept = default;

        explicit ShaderProgramRuntime(const ShaderProgramData& program);

        /**
         * @brief Create a copy of the shader runtime
         *
         * Creates a 'fork' of the current state of the object. The fork will
         * refer to the same shader resources, but different runtime values
         * (such as push constant default values) can be configured for it.
         *
         * This is just a wrapper for the copy constructor.
         */
        auto clone() const -> u_ptr<ShaderProgramRuntime>;

        /**
         * Retrieve a handle to a push constant value.
         *
         * The handle is used to upload data to the push constant it refers to.
         * Store the handle and re-use it to make push constant accesses
         * efficient.
         *
         * @param name A unique identifier string. This string is set in the
         *             parameter struct `CapabilityConfig::PushConstant` that is
         *             passed to `CapabilityConfig::addResource`.
         *
         * @throw std::out_of_range if no push constant `name` exists in the
         *        program.
         */
        auto getPushConstantHandle(const std::string& name) -> PushConstant;

        /**
         * Retrieve a handle to a push constant value.
         *
         * The handle is used to upload data to the push constant it refers to.
         * Store the handle and re-use it to make push constant accesses
         * efficient.
         *
         * The std::string_view overload exists so that we can use constexpr-
         * declared global constants as names.
         *
         * @param name A unique identifier string. This string is set in the
         *             parameter struct `CapabilityConfig::PushConstant` that is
         *             passed to `CapabilityConfig::addResource`.
         *
         * @throw std::out_of_range if no push constant `name` exists in the
         *        program.
         */
        auto getPushConstantHandle(std::string_view name) -> PushConstant;

        /**
         * Retrieve a handle to a push constant value, if that value exists in
         * the shader program.
         *
         * The handle is used to upload data to the push constant it refers to.
         * Store the handle and re-use it to make push constant accesses
         * efficient.
         *
         * @param name A unique identifier string. This string is set in the
         *             parameter struct `CapabilityConfig::PushConstant` that is
         *             passed to `CapabilityConfig::addResource`.
         */
        auto tryGetPushConstantHandle(const std::string& name) -> std::optional<PushConstant>;

        /**
         * Retrieve a handle to a push constant value, if that value exists in
         * the shader program.
         *
         * The handle is used to upload data to the push constant it refers to.
         * Store the handle and re-use it to make push constant accesses
         * efficient.
         *
         * The std::string_view overload exists so that we can use constexpr-
         * declared global constants as names.
         *
         * @param name A unique identifier string. This string is set in the
         *             parameter struct `CapabilityConfig::PushConstant` that is
         *             passed to `CapabilityConfig::addResource`.
         */
        auto tryGetPushConstantHandle(std::string_view name) -> std::optional<PushConstant>;

        /**
         * Retrieve the index of a descriptor set.
         *
         * @param name A unique identifier string. This string is set in the
         *             parameter struct `CapabilityConfig::DescriptorBinding`
         *             that is passed to `CapabilityConfig::addResource`.
         *
         * @throw std::out_of_range if no descriptor set `name` exists in the
         *        shader program.
         */
        auto getDescriptorSetIndex(const std::string& name) -> ui32;

        /**
         * Retrieve the index of a descriptor set.
         *
         * The std::string_view overload exists so that we can use constexpr-
         * declared global constants as names.
         *
         * @param name A unique identifier string. This string is set in the
         *             parameter struct `CapabilityConfig::DescriptorBinding`
         *             that is passed to `CapabilityConfig::addResource`.
         *
         * @throw std::out_of_range if no descriptor set `name` exists in the
         *        shader program.
         */
        auto getDescriptorSetIndex(std::string_view name) -> ui32;

        /**
         * Retrieve the index of a descriptor set, if that descriptor exists in
         * the shader program.
         *
         * @param name A unique identifier string. This string is set in the
         *             parameter struct `CapabilityConfig::DescriptorBinding`
         *             that is passed to `CapabilityConfig::addResource`.
         */
        auto tryGetDescriptorSetIndex(const std::string& name) -> std::optional<ui32>;

        /**
         * Retrieve the index of a descriptor set, if that descriptor exists in
         * the shader program.
         *
         * The std::string_view overload exists so that we can use constexpr-
         * declared global constants as names.
         *
         * @param name A unique identifier string. This string is set in the
         *             parameter struct `CapabilityConfig::DescriptorBinding`
         *             that is passed to `CapabilityConfig::addResource`.
         */
        auto tryGetDescriptorSetIndex(std::string_view name) -> std::optional<ui32>;

        /**
         * @brief Upload data for a push constant.
         *
         * @throw std::invalid_argument if `pcHandle` is not a valid handle to a
         *        push constant in the shader program.
         */
        void pushConstants(vk::CommandBuffer cmdBuf,
                           vk::PipelineLayout layout,
                           PushConstant pcHandle,
                           const void* data, size_t size) const;

        /**
         * @brief Upload data for a push constant.
         *
         * @throw std::invalid_argument if `pcHandle` is not a valid handle to a
         *        push constant in the shader program.
         */
        template<typename T>
        void pushConstants(vk::CommandBuffer cmdBuf,
                           vk::PipelineLayout layout,
                           PushConstant pcHandle,
                           T&& value) const
        {
            pushConstants(cmdBuf, layout, pcHandle, &value, sizeof(T));
        }

        /**
         * Set a default value for a push constant.
         *
         * Use `ShaderProgramRuntime::uploadPushConstantDefaultValues` to upload
         * all values configured via this function to the device.
         *
         * @param data Will be copied into the runtime's internal storage.
         *             Size must not exceed the push constant value's size
         *             in the shader.
         *
         * @throw std::invalid_argument if `pcHandle` is not a valid handle to a
         *        push constant in the shader program.
         */
        void setPushConstantDefaultValue(PushConstant pcHandle, std::span<const std::byte> data);

        /**
         * Upload all default push constant values previously set via
         * `setPushConstantDefaultValue` to the device.
         */
        void uploadPushConstantDefaultValues(vk::CommandBuffer cmdBuf, vk::PipelineLayout layout);

    private:
        struct PushConstantRange
        {
            ui32 offset;
            ui32 size;
            vk::ShaderStageFlags stages;
        };

        bool exists(const PushConstant& hnd) const;

        void doPushConstants(vk::CommandBuffer cmdBuf,
                             vk::PipelineLayout layout,
                             ui32 internalId,
                             const void* data, size_t size) const;

        s_ptr<const std::vector<PushConstantRange>> pc;
        s_ptr<const std::unordered_map<std::string, ui32>> descriptorSetIndices;
        s_ptr<const std::unordered_map<std::string, PushConstant>> pcHandlesByName;
        vk::ShaderStageFlags allStages;

        std::vector<std::pair<ui32, std::vector<std::byte>>> pushConstantData;
    };
} // namespace trc::shader
