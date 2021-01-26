#include <iostream>

#include <trc/Torch.h>
#include <trc/TorchResources.h>
#include <trc/ui/Window.h>
#include <trc/ui/torch/GuiIntegration.h>
using namespace trc::basic_types;
namespace ui = trc::ui;
#include <ui/elements/Quad.h>
#include <ui/elements/Text.h>

int main()
{
    {
        auto renderer = trc::init();
        auto scene = std::make_unique<trc::Scene>();
        auto camera = std::make_unique<trc::Camera>();
        ui::Window window{
            std::make_unique<trc::TorchWindowInformationProvider>(vkb::getSwapchain())
        };

        // Add gui pass and stage to render graph
        auto renderPass = trc::RenderPass::createAtNextIndex<trc::GuiRenderPass>(
            window,
            vkb::FrameSpecificObject<vk::Image>{
                [](ui32 i) { return vkb::getSwapchain().getImage(i); }
            }
        ).first;
        auto& graph = renderer->getRenderGraph();
        graph.after(trc::RenderStageTypes::getDeferred(), trc::getGuiRenderStage());
        graph.addPass(trc::getGuiRenderStage(), renderPass);

        // Create some gui elements
        auto quad = window.create<ui::Quad>().makeUnique();
        window.getRoot().attach(*quad);
        quad->setPos({ 0.5f, 0.0f });
        quad->setSize({ 0.1f, 0.15f });

        auto child = window.create<ui::Quad>().makeUnique();
        quad->attach(*child);
        child->setPos({ 0.15f, 0.4f });

        while (vkb::getSwapchain().isOpen())
        {
            vkb::pollEvents();
            renderer->drawFrame(*scene, *camera);
        }

        vkb::getDevice()->waitIdle();
    }

    trc::terminate();

    std::cout << "Done.\n";
    return 0;
}
