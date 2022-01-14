#include <trc/Torch.h>
#include <trc/experimental/ImguiIntegration.h>
#include <imgui.h>
namespace trc {
    namespace imgui = experimental::imgui;
}
namespace ig = ImGui;
using namespace trc::basic_types;

int main()
{
    {
        ivec2 size{ 800, 600 };
        ivec2 pos{ 100, 100 };
        trc::WindowCreateInfo info{ .size=size, .pos=pos };
        trc::TorchStack torch({}, info);

        auto& window = torch.getWindow();
        auto imgui = trc::imgui::initImgui(window, torch.getRenderConfig().getLayout());

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
            ig::InputInt2("", &size.x);
            ig::PopItemWidth();
            ig::SameLine();
            if (ig::Button("resize")) {
                window.resize(size.x, size.y);
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

            window.drawFrame(torch.makeDrawConfig(scene, camera));
        }
    }

    trc::terminate();

    return 0;
}
