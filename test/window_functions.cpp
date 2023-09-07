#include <trc/Torch.h>
#include <trc/ImguiIntegration.h>
#include <imgui.h>

using namespace trc::basic_types;
namespace ig = ImGui;

int main()
{
    {
        ivec2 windowSize{ 800, 600 };
        ivec2 windowPos{ 100, 100 };
        auto torch = trc::initFull({}, {}, { .size=windowSize, .pos=windowPos });

        auto& window = torch->getWindow();
        auto imgui = trc::imgui::initImgui(window, torch->getRenderConfig().getRenderGraph());

        trc::Scene scene;
        trc::Camera camera;

        bool floating{ false };
        bool decorated{ true };
        bool hidden{ false };
        trc::Timer windowHiddenTimer;
        float opacity{ 1.0f };
        bool forcedAspectRatio{ false };

        while (window.isOpen() && !window.isPressed(trc::Key::escape))
        {
            trc::pollEvents();

            trc::imgui::beginImguiFrame();
            ig::Begin("Options");
            ig::PushItemWidth(100.0f);
            ig::InputInt2("", &windowSize.x);
            ig::PopItemWidth();
            ig::SameLine();
            if (ig::Button("resize")) {
                window.resize(windowSize.x, windowSize.y);
            }
            if (ig::Checkbox("Floating", &floating)) {
                window.setFloating(floating);
            }
            if (ig::Checkbox("Decorated", &decorated)) {
                window.setDecorated(decorated);
            }
            if (ig::Button("Maximize")) {
                window.maximize();
            }
            if (ig::Button("Minimize"))
            {
                window.minimize();
                hidden = true;
                windowHiddenTimer.reset();
            }
            ig::SameLine();
            ig::Text("(current: %i)", window.isMaximized());
            if (ig::Button("Restore")) {
                window.restore();
            }
            if (ig::Button("Hide"))
            {
                hidden = true;
                window.hide();
                windowHiddenTimer.reset();
            }
            if (ig::SliderFloat("Opacity", &opacity, 0.0f, 1.0f, "%.2f")) {
                window.setOpacity(opacity);
            }
            if (ig::Checkbox("Force aspect ratio", &forcedAspectRatio)) {
                window.forceAspectRatio(forcedAspectRatio);
            }
            ig::End();

            // Un-hide window after some time
            if (hidden && windowHiddenTimer.duration() > 2000.0f)
            {
                hidden = false;
                window.show();
            }

            torch->drawFrame(camera, scene);
        }
    }

    trc::terminate();

    return 0;
}
