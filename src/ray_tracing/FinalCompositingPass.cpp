#include "ray_tracing/FinalCompositingPass.h"

#include <vkb/ShaderProgram.h>

#include "util/Util.h"
#include "DescriptorSetUtils.h"



auto trc::rt::getFinalCompositingStage() -> RenderStageType::ID
{
    static auto id = RenderStageType::createAtNextIndex(FinalCompositingPass::NUM_SUBPASSES).first;
    return id;
}



trc::rt::FinalCompositingPass::FinalCompositingPass(
    const Window& window,
    const FinalCompositingPassCreateInfo& info)
    :
    RenderPass({}, NUM_SUBPASSES),
    swapchain(window.getSwapchain()),
    computeGroupSize(uvec3(
        glm::ceil(vec2(window.getSwapchain().getSize()) / vec2(COMPUTE_LOCAL_SIZE)),
        1
    )),
    pool([&] {
        const ui32 frameCount{ window.getSwapchain().getFrameCount() };
        std::vector<vk::DescriptorPoolSize> poolSizes{
            // GBuffer images
            { vk::DescriptorType::eStorageImage, GBuffer::NUM_IMAGES * frameCount },
            // RayBuffer images
            { vk::DescriptorType::eStorageImage, RayBuffer::NUM_IMAGES * frameCount },
            // Swapchain output image
            { vk::DescriptorType::eStorageImage, frameCount },
        };
        return window.getDevice()->createDescriptorPoolUnique({
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            // Two distinct sets: one for inputs, one for output
            2 * frameCount,
            poolSizes
        });
    }()),
    // Layouts
    inputLayout(buildDescriptorSetLayout()
        .addBinding(vk::DescriptorType::eStorageImage, GBuffer::NUM_IMAGES,
                    vk::ShaderStageFlagBits::eCompute)
        .addBinding(vk::DescriptorType::eStorageImage, RayBuffer::NUM_IMAGES,
                    vk::ShaderStageFlagBits::eCompute)
        .build(window.getDevice())
    ),
    outputLayout(buildDescriptorSetLayout()
        .addBinding(vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute)
        .build(window.getDevice())
    ),

    inputSets(window.getSwapchain()),
    outputSets(window.getSwapchain()),
    inputSetProvider({}, { window.getSwapchain() }),
    outputSetProvider({}, { window.getSwapchain() }),

    computePipeline(makeComputePipeline(
        window.getDevice(),
        makePipelineLayout(window.getDevice(),
            { *inputLayout, *outputLayout },
            {}
        ),
        vkb::createShaderModule(
            window.getDevice(),
            vkb::readFile(TRC_SHADER_DIR"/compositing.comp.spv")
        )
    ))
{
    inputSets = {
        window.getSwapchain(),
        [&](ui32 i) -> vk::UniqueDescriptorSet {
            auto set = std::move(
                window.getDevice()->allocateDescriptorSetsUnique({ *pool, *inputLayout })[0]
            );

            auto& g = info.gBuffer->getAt(i);
            auto& r = info.rayBuffer->getAt(i);
            std::vector<vk::DescriptorImageInfo> firstBinding{
                { {}, g.getImageView(GBuffer::eNormals),   vk::ImageLayout::eGeneral },
                { {}, g.getImageView(GBuffer::eAlbedo),    vk::ImageLayout::eGeneral },
                { {}, g.getImageView(GBuffer::eMaterials), vk::ImageLayout::eGeneral },
            };
            std::vector<vk::DescriptorImageInfo> secondBinding{
                { {}, r.getImageView(RayBuffer::eReflections), vk::ImageLayout::eGeneral },
            };

            std::vector<vk::WriteDescriptorSet> writes{
                { *set, 0, 0, vk::DescriptorType::eStorageImage, firstBinding },
                { *set, 1, 0, vk::DescriptorType::eStorageImage, secondBinding },
            };
            window.getDevice()->updateDescriptorSets(writes, {});

            return set;
        }
    };
    outputSets = {
        window.getSwapchain(),
        [&](ui32 i) -> vk::UniqueDescriptorSet {
            auto set = std::move(
                window.getDevice()->allocateDescriptorSetsUnique({ *pool, *outputLayout })[0]
            );

            auto& swapchain = window.getSwapchain();
            vk::DescriptorImageInfo imageInfo({}, swapchain.getImageView(i), vk::ImageLayout::eGeneral);
            vk::WriteDescriptorSet write(*set, 0, 0, vk::DescriptorType::eStorageImage, imageInfo);
            window.getDevice()->updateDescriptorSets(write, {});

            return set;
        }
    };

    inputSetProvider = { *inputLayout, inputSets };
    outputSetProvider = { *outputLayout, outputSets };

    computePipeline.addStaticDescriptorSet(0, inputSetProvider);
    computePipeline.addStaticDescriptorSet(1, outputSetProvider);
}

void trc::rt::FinalCompositingPass::begin(vk::CommandBuffer cmdBuf, vk::SubpassContents)
{
    // Swapchain image: ePresentSrcKHR -> eGeneral
    cmdBuf.pipelineBarrier(
        vk::PipelineStageFlagBits::eRayTracingShaderKHR,
        vk::PipelineStageFlagBits::eAllCommands,
        vk::DependencyFlagBits::eByRegion,
        {}, {},
        vk::ImageMemoryBarrier(
            vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite,
            {},
            vk::ImageLayout::ePresentSrcKHR,
            vk::ImageLayout::eGeneral,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            swapchain.getImage(swapchain.getCurrentFrame()),
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
        )
    );

    computePipeline.bind(cmdBuf);
    computePipeline.bindStaticDescriptorSets(cmdBuf);
    computePipeline.bindDefaultPushConstantValues(cmdBuf);
    auto [x, y, z] = computeGroupSize;
    cmdBuf.dispatch(x, y, z);

    // Swapchain image: eGeneral -> ePresentSrcKHR
    cmdBuf.pipelineBarrier(
        vk::PipelineStageFlagBits::eRayTracingShaderKHR,
        vk::PipelineStageFlagBits::eAllCommands,
        vk::DependencyFlagBits::eByRegion,
        {}, {},
        vk::ImageMemoryBarrier(
            vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite,
            {},
            vk::ImageLayout::eGeneral,
            vk::ImageLayout::ePresentSrcKHR,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            swapchain.getImage(swapchain.getCurrentFrame()),
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
        )
    );
}

void trc::rt::FinalCompositingPass::end(vk::CommandBuffer)
{
}
