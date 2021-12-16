#include "ray_tracing/FinalCompositingPass.h"

#include <vkb/ShaderProgram.h>

#include "util/Util.h"
#include "DescriptorSetUtils.h"
#include "AssetRegistry.h"
#include "ray_tracing/RayPipelineBuilder.h"



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
        // G-Buffer bindings first
        .addBinding(vk::DescriptorType::eStorageImage, 1,
                    vk::ShaderStageFlagBits::eCompute | ALL_RAY_PIPELINE_STAGE_FLAGS)
        .addBinding(vk::DescriptorType::eStorageImage, 1,
                    vk::ShaderStageFlagBits::eCompute | ALL_RAY_PIPELINE_STAGE_FLAGS)
        .addBinding(vk::DescriptorType::eStorageImage, 1,
                    vk::ShaderStageFlagBits::eCompute | ALL_RAY_PIPELINE_STAGE_FLAGS)
        .addBinding(vk::DescriptorType::eCombinedImageSampler, 1,
                    vk::ShaderStageFlagBits::eCompute | ALL_RAY_PIPELINE_STAGE_FLAGS)
        // Ray-Buffer bindings
        .addBinding(vk::DescriptorType::eStorageImage, 1,
                    vk::ShaderStageFlagBits::eCompute | ALL_RAY_PIPELINE_STAGE_FLAGS)
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

    computePipelineLayout(
        window.getDevice(),
        {
            *inputLayout,
            *outputLayout,
            info.assetRegistry->getDescriptorSetProvider().getDescriptorSetLayout()
        },
        {}
    ),
    computePipeline(makeComputePipeline(
        window.getDevice(),
        computePipelineLayout,
        vkb::createShaderModule(
            window.getDevice(),
            vkb::readFile(TRC_SHADER_DIR"/compositing.comp.spv")
        )
    ))
{
    assert(info.gBuffer != nullptr);
    assert(info.rayBuffer != nullptr);
    assert(info.assetRegistry != nullptr);

    computePipelineLayout.addStaticDescriptorSet(0, inputSetProvider);
    computePipelineLayout.addStaticDescriptorSet(1, outputSetProvider);
    computePipelineLayout.addStaticDescriptorSet(2, info.assetRegistry->getDescriptorSetProvider());

    inputSets = {
        window.getSwapchain(),
        [&](ui32 i) -> vk::UniqueDescriptorSet {
            auto set = std::move(
                window.getDevice()->allocateDescriptorSetsUnique({ *pool, *inputLayout })[0]
            );

            vk::Sampler depthSampler = *depthSamplers.emplace_back(
                window.getDevice()->createSamplerUnique(
                    vk::SamplerCreateInfo(
                        {},
                        vk::Filter::eLinear, vk::Filter::eLinear,
                        vk::SamplerMipmapMode::eNearest,
                        vk::SamplerAddressMode::eRepeat, // u
                        vk::SamplerAddressMode::eRepeat, // v
                        vk::SamplerAddressMode::eRepeat  // w
                    )
                )
            );

            auto& g = info.gBuffer->getAt(i);
            auto& r = info.rayBuffer->getAt(i);
            std::vector<vk::DescriptorImageInfo> gImages{
                { {}, g.getImageView(GBuffer::eNormals),   vk::ImageLayout::eGeneral },
                { {}, g.getImageView(GBuffer::eAlbedo),    vk::ImageLayout::eGeneral },
                { {}, g.getImageView(GBuffer::eMaterials), vk::ImageLayout::eGeneral },
            };
            std::vector<vk::DescriptorImageInfo> depthImage{
                { depthSampler, g.getImageView(GBuffer::eDepth), vk::ImageLayout::eShaderReadOnlyOptimal },
            };

            std::vector<vk::DescriptorImageInfo> rayImages{
                { {}, r.getImageView(RayBuffer::eReflections), vk::ImageLayout::eGeneral },
            };

            std::vector<vk::WriteDescriptorSet> writes{
                { *set, 0, 0, vk::DescriptorType::eStorageImage, gImages },
                { *set, 3, 0, vk::DescriptorType::eCombinedImageSampler, depthImage },
                { *set, 4, 0, vk::DescriptorType::eStorageImage, rayImages },
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

auto trc::rt::FinalCompositingPass::getInputImageDescriptor() const
    -> const DescriptorProviderInterface&
{
    return inputSetProvider;
}
