#pragma once

#include <mutex>
#include <vector>

#include "trc/base/Device.h"

#include "trc/Types.h"
#include "trc/VulkanInclude.h"
#include "trc/core/DescriptorProvider.h"

namespace trc
{
    /**
     * The SharedDescriptorSet is intended to be used as a collection of
     * descriptor bindings that each 'belong' to a different service and
     * are logically managed in an independent, decentralized manner.
     *
     * Changes to individual bindings are collectively executed in the
     * `SharedDescriptorSet::update` method.
     *
     * Can only be created with the static `SharedDescriptorSet::build`
     * function.
     */
    class SharedDescriptorSet
    {
    private:
        friend class Builder;
        SharedDescriptorSet() = default;

    public:
        SharedDescriptorSet(const SharedDescriptorSet&) = delete;
        SharedDescriptorSet(SharedDescriptorSet&&) noexcept = delete;
        auto operator=(const SharedDescriptorSet&) -> SharedDescriptorSet&;
        auto operator=(SharedDescriptorSet&&) noexcept -> SharedDescriptorSet&;

        ~SharedDescriptorSet() noexcept = default;

        class Builder;

        /**
         * Build a descriptor set.
         */
        static auto build() -> Builder;

        auto getDescriptorSetLayout() const -> vk::DescriptorSetLayout;
        auto getProvider() const -> s_ptr<const DescriptorProviderInterface>;

        /**
         * Execute necessary descriptor updates.
         *
         * Updates can be enqueued for individual bindings with the
         * `Binding::update` method.
         */
        void update(const Device& device);

        class Binding
        {
        public:
            /**
             * Can only be constructed by `SharedDescriptorSet::Builder`.
             */
            Binding() = delete;

            Binding(const Binding&) = default;
            Binding(Binding&&) = default;
            auto operator=(const Binding&) -> Binding& = default;
            auto operator=(Binding&&) -> Binding& = default;
            ~Binding() = default;

            /**
             * @return ui32 The binding's index in the shared descriptor set
             */
            auto getBindingIndex() const -> ui32;

            void update(ui32 arrayElem, vk::DescriptorBufferInfo buffer);
            void update(ui32 firstArrayElem,
                        const vk::ArrayProxy<const vk::DescriptorBufferInfo>& buffers);

            void update(ui32 arrayElem, vk::DescriptorImageInfo image);
            void update(ui32 firstArrayElem,
                        const vk::ArrayProxy<const vk::DescriptorImageInfo>& images);

            void update(ui32 arrayElem, vk::BufferView view);
            void update(ui32 firstArrayElem,
                        const vk::ArrayProxy<const vk::BufferView>& views);

        private:
            friend Builder;
            Binding(s_ptr<SharedDescriptorSet> set, ui32 bindingIndex);

            s_ptr<SharedDescriptorSet> set;
            ui32 bindingIndex;
        };

        class Builder
        {
        public:
            Builder();

            void addLayoutFlag(vk::DescriptorSetLayoutCreateFlags flags);
            void addPoolFlag(vk::DescriptorPoolCreateFlags flags);
            auto addBinding(vk::DescriptorType type,
                            ui32 count,
                            vk::ShaderStageFlags stages,
                            vk::DescriptorBindingFlags flags = {})
                -> Binding;

            /**
             * @brief Finalize and return the SharedDescriptorSet
             *
             * May only be called once per builder object.
             *
             * @throw std::runtime_error if `build` is called multiple times on
             *        the same builder.
             */
            auto build(const Device& device) -> s_ptr<SharedDescriptorSet>;

        private:
            friend SharedDescriptorSet;

            vk::DescriptorSetLayoutCreateFlags layoutFlags;
            vk::DescriptorPoolCreateFlags poolFlags;
            std::vector<vk::DescriptorBindingFlags> bindingFlags;

            std::vector<vk::DescriptorSetLayoutBinding> bindings;

            // Created with the builder so it can be passed to the constructor
            // of `Binding` objects when they are created.
            s_ptr<SharedDescriptorSet> set;
        };

    private:
        void build(const Device& device, const Builder& builder);


        /////////////////////////
        // Basic device resources

        vk::UniqueDescriptorSetLayout layout;
        vk::UniqueDescriptorPool pool;
        vk::UniqueDescriptorSet set;
        s_ptr<DescriptorProvider> provider{ new DescriptorProvider{{}} };

        std::vector<vk::DescriptorSetLayoutBinding> bindings;


        /////////////////////
        // Descriptor updates

        struct UpdateContainer
        {
            UpdateContainer(const UpdateContainer&) = delete;
            auto operator=(const UpdateContainer&) -> UpdateContainer& = delete;

            UpdateContainer() = default;
            UpdateContainer(UpdateContainer&&) noexcept = default;
            auto operator=(UpdateContainer&&) noexcept -> UpdateContainer& = default;
            ~UpdateContainer() noexcept = default;

            UpdateContainer(const vk::ArrayProxy<const vk::DescriptorBufferInfo>& buffers)
                : bufferInfos(buffers.begin(), buffers.end()), imageInfos(), bufferViews() {}
            UpdateContainer(const vk::ArrayProxy<const vk::DescriptorImageInfo>& images)
                : bufferInfos(), imageInfos(images.begin(), images.end()), bufferViews() {}
            UpdateContainer(const vk::ArrayProxy<const vk::BufferView>& views)
                : bufferInfos(), imageInfos(), bufferViews(views.begin(), views.end()) {}

            std::vector<vk::DescriptorBufferInfo> bufferInfos;
            std::vector<vk::DescriptorImageInfo> imageInfos;
            std::vector<vk::BufferView> bufferViews;
        };

        void update(ui32 binding,
                    ui32 firstArrayElem,
                    const vk::ArrayProxy<const vk::DescriptorBufferInfo>& buffers);
        void update(ui32 binding,
                    ui32 firstArrayElem,
                    const vk::ArrayProxy<const vk::DescriptorImageInfo>& images);
        void update(ui32 binding,
                    ui32 firstArrayElem,
                    const vk::ArrayProxy<const vk::BufferView>& bufferViews);

        std::mutex descriptorUpdateLock;
        std::vector<UpdateContainer> updateStructs;
        std::vector<vk::WriteDescriptorSet> writes;
    };
} // namespace trc
