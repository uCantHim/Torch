#pragma once

#include <trc/Camera.h>
#include <trc/base/Buffer.h>
#include <trc/base/FrameClock.h>
#include <trc/base/FrameSpecificObject.h>
#include <trc/core/DescriptorProvider.h>

using namespace trc::basic_types;

class CameraDescriptor : public trc::DescriptorProviderInterface
{
public:
    CameraDescriptor(const trc::FrameClock& clock, const trc::Device& device);

    void update(const trc::Camera& camera);

    auto getDescriptorSetLayout() const noexcept -> vk::DescriptorSetLayout override;
    void bindDescriptorSet(
        vk::CommandBuffer cmdBuf,
        vk::PipelineBindPoint bindPoint,
        vk::PipelineLayout pipelineLayout,
        ui32 setIndex
    ) const override;

private:
    struct CameraMatrices
    {
        mat4 viewMatrix;
        mat4 projMatrix;
        mat4 inverseViewMatrix;
        mat4 inverseProjMatrix;
    };

    const trc::FrameClock& clock;

    vk::UniqueDescriptorSetLayout layout;
    vk::UniqueDescriptorPool pool;
    vk::UniqueDescriptorSet set;

    trc::Buffer buffer;
    CameraMatrices* bufferMap;
};
