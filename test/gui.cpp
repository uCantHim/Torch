#include <iostream>

#include <trc/Torch.h>
#include <trc/drawable/Drawable.h>
#include <trc/ui/torch/GuiIntegration.h>
using namespace trc::basic_types;

#include <ui/Window.h>
#include <ui/elements/Quad.h>
#include <ui/elements/Text.h>
#include <ui/elements/InputField.h>
#include <ui/elements/Button.h>
namespace ui = trc::ui;
using namespace ui::size_literals;

int main()
{
    {
        // Gui requires the storage flag to be set on swapchain images
        auto torch = trc::initFull();
        auto& swapchain = torch->getWindow();
        auto& ar = torch->getAssetManager();

        trc::Scene scene;
        trc::Camera camera;
        const auto [width, height] = swapchain.getImageExtent();
        camera.lookAt(vec3(0, 2, 4), vec3(0, 0, 0), vec3(0, 1, 0));
        camera.makePerspective(float(width) / float(height), 45.0f, 0.1f, 100.0f);

        // Initialize GUI
        auto guiStack = trc::initGui(torch->getDevice(), swapchain);
        ui::Window* window = guiStack.window.get();
        trc::integrateGui(guiStack, torch->getRenderConfig().getLayout());


        // Now, after intialization, is it possible to load fonts
        const ui32 font = ui::FontRegistry::addFont(TRC_TEST_FONT_DIR"/hack_mono.ttf", 40);

        // Create some gui elements
        auto quad = window->create<ui::Quad>().makeUnique();
        window->getRoot().attach(*quad);
        quad->setPos(0.5f, 0.0f);
        quad->setSize(0.1f, 0.15f);
        quad->style.background = vec4(0.3f, 0.3f, 0.7f, 0.5f);

        quad->addEventListener([](const ui::event::Click& e) {
            std::cout << "Click on first quad\n";
        });

        auto child = window->create<ui::Quad>().makeUnique();
        quad->attach(*child);
        child->setPos(0.15f, 0.4f);
        child->addEventListener([](const ui::event::Click& e) {
            std::cout << "Click on second quad\n";
        });
        child->style.background = vec4(1.0f, 0.0f, 1.0f, 1.0f );
        child->style.borderThickness = 2;
        child->style.borderColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);

        // Text element with line breaks
        auto text = window->create<ui::Text>(
            "Hello World! and some more text…"
            "\n»this line« contains some cool special characters: “µ” · ħŋſđðſđ"
            "\nNewlines working: ∞",
            font, 30
        ).makeUnique();
        window->getRoot().attach(*text);
        text->setPos(0.2f, 0.6f);

        // Multiple text elements with different font sizes
        for (float i = 0.0f; ui32 fontSize : { 20, 28, 32, 40, 48, })
        {
            auto& el = window->create<ui::Text>(
                "Placeholdertext for font size " + std::to_string(fontSize) + "! :D",
                font, fontSize
            ).makeRef();
            window->getRoot().attach(el);
            el.setPos(0.4f, 0.1f + (i += 0.05f));
        }

        // Input field
        auto input = window->create<ui::InputField>().makeUnique();
        input->setSize(150_px, 40_px);
        input->setPos(0.1_n, 300_px);
        window->getRoot().attach(*input);

        // Button
        auto& button = window->create<ui::Button>(
            "Click me!",
            []() { std::cout << "I've been clicked :D\n"; }
        ).makeRef();
        window->getRoot().attach(button);
        button.setPos(0.7_n, 0.8_n);
        button.style.background = vec4(1.0f, 1.0f, 0.2f, 1.0f);
        button.style.foreground = vec4(0.0f, 0.0f, 0.0f, 1.0f);
        button.setPadding(0.5_n, 1.0_n);
        button.setFontSize(40);

        // Also add a world-space object
        trc::Light light = scene.getLights().makeSunLight(vec3(1.0f), vec3(0, -1, -1), 0.4f);
        trc::Drawable plane(
            ar.createAsset<trc::Geometry>(trc::makePlaneGeo()),
            ar.createAsset<trc::Material>(trc::MaterialDeviceHandle{ .color=vec4(0.3f, 0.7f, 0.2f, 1.0f) }),
            scene
        );
        plane.rotateX(glm::radians(30.0f));

        while (swapchain.isOpen())
        {
            vkb::pollEvents();
            torch->drawFrame(torch->makeDrawConfig(scene, camera));
        }
    }

    trc::terminate();

    std::cout << "Done.\n";
    return 0;
}
