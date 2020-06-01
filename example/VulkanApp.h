#pragma once
#ifndef VULKANAPP_H
#define VULKANAPP_H

#include <stdexcept>

#include "vkb/Buffer.h"
#include "Scene.h"

class VulkanApp : public vkb::VulkanBase, public vkb::SwapchainDependentResource
{
public:
    VulkanApp();

    void run();

    void signalRecreateRequired() override;
    void recreate(vkb::Swapchain& swapchain) override;
    void signalRecreateFinished() override;

private:
    static constexpr size_t TRIANGLELIST_VERTEX_COUNT = 100;
    static constexpr size_t TRIANGLELIST_COUNT = 1;

    void init();
    void tick();
    void destroy();

    void makeRenderEnvironment();
    void createPipeline(vkb::Swapchain& swapchain);

    vk::UniqueDescriptorPool descriptorPool;
    vk::UniqueDescriptorSetLayout standardDescriptorSetLayout;
    std::vector<vk::PushConstantRange> standardPushConstantRanges;

    PipelineLayout pipelineLayout;

    vk::UniqueDescriptorSet matrixBufferDescriptorSet;
    vkb::Buffer matrixBuffer;
    vkb::Image texture;
    vk::UniqueImageView textureImageView;

    Renderpass* renderpass{ nullptr };
    Scene scene;
};



#endif
