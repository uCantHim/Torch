#include <iostream>

#include <trc/Torch.h>
#include <trc/base/event/Event.h>
#include <trc/drawable/Drawable.h>
#include <trc/ui/torch/GuiIntegration.h>
using namespace trc::basic_types;

#include <trc/ui/Window.h>
#include <trc/ui/elements/Button.h>
#include <trc/ui/elements/InputField.h>
#include <trc/ui/elements/Quad.h>
#include <trc/ui/elements/Text.h>
namespace ui = trc::ui;
using namespace ui::size_literals;

int main()
{
    {
        // Create a GUI root.
        auto window = std::make_shared<ui::Window>(trc::ui::WindowCreateInfo{
            .keyMap{
                .escape     = static_cast<int>(trc::Key::escape),
                .backspace  = static_cast<int>(trc::Key::backspace),
                .enter      = static_cast<int>(trc::Key::enter),
                .tab        = static_cast<int>(trc::Key::tab),
                .del        = static_cast<int>(trc::Key::del),
                .arrowLeft  = static_cast<int>(trc::Key::arrow_left),
                .arrowRight = static_cast<int>(trc::Key::arrow_right),
                .arrowUp    = static_cast<int>(trc::Key::arrow_up),
                .arrowDown  = static_cast<int>(trc::Key::arrow_down),
            }
        });

        // Notify GUI when window size changes
        trc::on<trc::SwapchainResizeEvent>([&](const trc::SwapchainResizeEvent& e) {
            window->setSize(e.newSize);
        });

        // Notify GUI of mouse clicks
        trc::on<trc::MouseClickEvent>([&](const trc::MouseClickEvent& e) {
            vec2 pos = e.swapchain->getMousePosition();
            window->signalMouseClick(pos.x, pos.y);
        });

        // Notify GUI of key events
        trc::on<trc::KeyPressEvent>([&](auto& e) {
            window->signalKeyPress(static_cast<int>(e.key));
        });
        trc::on<trc::KeyRepeatEvent>([&](auto& e) {
            window->signalKeyRepeat(static_cast<int>(e.key));
        });
        trc::on<trc::KeyReleaseEvent>([&](auto& e) {
            window->signalKeyRelease(static_cast<int>(e.key));
        });
        trc::on<trc::CharInputEvent>([&](auto& e) {
            window->signalCharInput(e.character);
        });

        // Initialize Torch with a GUI plugin.
        auto torch = trc::initFull({
            .plugins{ trc::buildGuiRenderPlugin(window) }
        });
        auto& swapchain = torch->getWindow();
        auto& ar = torch->getAssetManager();

        // Now, after intialization, is it possible to load fonts
        const ui32 font = ui::FontRegistry::addFont(TRC_TEST_FONT_DIR"/hack_mono.ttf", 40);

        // Create some gui elements
        auto quad = window->create<ui::Quad>();
        window->getRoot().attach(quad);
        quad->setPos(0.5f, 0.0f);
        quad->setSize(0.1f, 0.15f);
        quad->style.background = vec4(0.3f, 0.3f, 0.7f, 0.5f);

        quad->addEventListener([](const ui::event::Click& e) {
            std::cout << "Click on first quad\n";
        });

        auto child = window->create<ui::Quad>();
        quad->attach(child);
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
        );
        window->getRoot().attach(text);
        text->setPos(0.2f, 0.6f);

        // Multiple text elements with different font sizes
        for (float i = 0.0f; ui32 fontSize : { 20, 28, 32, 40, 48, })
        {
            auto el = window->create<ui::Text>(
                "Placeholdertext for font size " + std::to_string(fontSize) + "! :D",
                font, fontSize
            );
            window->getRoot().attach(el);
            el->setPos(0.4f, 0.1f + (i += 0.05f));
        }

        // Input field
        auto input = window->create<ui::InputField>();
        input->setSize(150_px, 40_px);
        input->setPos(0.1_n, 300_px);
        window->getRoot().attach(input);

        // Button
        auto button = window->create<ui::Button>(
            "Click me!",
            []() { std::cout << "I've been clicked :D\n"; }
        );
        window->getRoot().attach(button);
        button->setPos(0.7_n, 0.8_n);
        button->style.background = vec4(1.0f, 1.0f, 0.2f, 1.0f);
        button->style.foreground = vec4(0.0f, 0.0f, 0.0f, 1.0f);
        button->setPadding(0.5_n, 1.0_n);
        button->setFontSize(40);

        // Create some world-space stuff
        auto scene = std::make_shared<trc::Scene>();
        auto camera = std::make_shared<trc::Camera>();
        auto vp = torch->makeFullscreenViewport(camera, scene);

        const auto [width, height] = swapchain.getImageExtent();
        camera->lookAt(vec3(0, 2, 4), vec3(0, 0, 0), vec3(0, 1, 0));
        camera->makePerspective(float(width) / float(height), 45.0f, 0.1f, 100.0f);

        auto light = scene->getLights().makeSunLight(vec3(1.0f), vec3(0, -1, -1), 0.4f);
        auto plane = scene->makeDrawable({
            ar.create<trc::Geometry>(trc::makePlaneGeo()),
            ar.create<trc::Material>(trc::makeMaterial({ .color=vec4(0.3f, 0.7f, 0.2f, 1.0f) }))
        });
        plane->rotateX(glm::radians(30.0f));

        while (swapchain.isOpen())
        {
            trc::pollEvents();
            torch->drawFrame(vp);
        }

        torch->waitForAllFrames();
    }

    trc::terminate();

    std::cout << "Done.\n";
    return 0;
}
