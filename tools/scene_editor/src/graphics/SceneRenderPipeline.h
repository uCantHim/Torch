#pragma once

#include <trc/ImageClear.h>
#include <trc/ImguiIntegration.h>
#include <trc/Torch.h>

using namespace trc::basic_types;

inline auto makeSceneRenderPipeline(trc::Window& window, trc::AssetRegistry& assets)
    -> u_ptr<trc::RenderPipeline>
{
    static constexpr vec3 kClearColor{ 0.12f, 0.12f, 0.12f };

    // Configuration
    trc::imgui::disableInsertingImguiEventCallbacks();

    // Create a Torch pipeline with imgui and image clear plugins enabled.
    return trc::makeTorchRenderPipeline(
        window.getInstance(),
        window,
        trc::TorchPipelineCreateInfo{
            .assetRegistry=assets,
            .assetDescriptorCreateInfo={},
        },
        { /* Additional render plugins */
            trc::imgui::buildImguiRenderPlugin,
            [](auto&&) -> trc::PluginBuilder {
                return trc::buildRenderTargetImageClearPlugin(
                    vk::ClearColorValue{ kClearColor.r, kClearColor.g, kClearColor.b, 1.0f }
                );
            },
        }
    );
}
