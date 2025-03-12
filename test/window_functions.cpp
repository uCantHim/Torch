#include <imgui.h>

#include <trc/ImguiIntegration.h>
#include <trc/Torch.h>
#include <trc_util/Timer.h>

using namespace trc::basic_types;
namespace ig = ImGui;

auto toString(trc::CursorShape shape) -> const char*
{
    switch (shape)
    {
    case trc::CursorShape::eArrow: return "Arrow";
    case trc::CursorShape::eBeam: return "Text Insertion Beam";
    case trc::CursorShape::eCrosshair: return "Crosshair";
    case trc::CursorShape::ePointingHand: return "Pointing Hand";
    case trc::CursorShape::eResize: return "Resize";
    case trc::CursorShape::eResizeHorizontal: return "Resize Horizontally";
    case trc::CursorShape::eResizeVertical: return "Resize Vertically";
    case trc::CursorShape::eResizeDiagonalULtoLR: return "Resize UL - LR";
    case trc::CursorShape::eResizeDiagonalURtoLL: return "Resize LL - UR";
    case trc::CursorShape::eNotAllowed: return "Operation Not Allowed";
    }
}

auto toTorchEnum(ImGuiMouseCursor cursor) -> trc::CursorShape
{
    switch (cursor)
    {
    case ImGuiMouseCursor_Arrow: return trc::CursorShape::eArrow;
    case ImGuiMouseCursor_Hand: return trc::CursorShape::ePointingHand;
    case ImGuiMouseCursor_TextInput: return trc::CursorShape::eBeam;
    case ImGuiMouseCursor_ResizeEW: return trc::CursorShape::eResizeHorizontal;
    case ImGuiMouseCursor_ResizeNS: return trc::CursorShape::eResizeVertical;
    case ImGuiMouseCursor_ResizeNESW: return trc::CursorShape::eResizeDiagonalURtoLL;
    case ImGuiMouseCursor_ResizeNWSE: return trc::CursorShape::eResizeDiagonalULtoLR;
    case ImGuiMouseCursor_ResizeAll: return trc::CursorShape::eResize;
    case ImGuiMouseCursor_NotAllowed: return trc::CursorShape::eNotAllowed;
    default:
        throw std::logic_error("Invalid ImGuiMouseCursor value.");
    }
}

constexpr trc::CursorShape kAllCursorShapes[]{
    trc::CursorShape::eArrow,
    trc::CursorShape::eBeam,
    trc::CursorShape::eCrosshair,
    trc::CursorShape::ePointingHand,
    trc::CursorShape::eResize,
    trc::CursorShape::eResizeHorizontal,
    trc::CursorShape::eResizeVertical,
    trc::CursorShape::eResizeDiagonalULtoLR,
    trc::CursorShape::eResizeDiagonalURtoLL,
    trc::CursorShape::eNotAllowed,
};

int main()
{
    {
        ivec2 windowSize{ 800, 600 };
        ivec2 windowPos{ 100, 100 };
        auto torch = trc::initFull(
            trc::TorchStackCreateInfo{ .plugins{ trc::imgui::buildImguiRenderPlugin } },
            trc::InstanceCreateInfo{},
            trc::WindowCreateInfo{ .size=windowSize, .pos=windowPos }
        );

        auto scene = std::make_shared<trc::Scene>();
        auto camera = std::make_shared<trc::Camera>();
        auto vp = torch->makeFullscreenViewport(camera, scene);

        auto& window = torch->getWindow();

        bool floating{ false };
        bool decorated{ true };
        bool hidden{ false };
        trc::Timer windowHiddenTimer;
        float opacity{ 1.0f };
        bool forcedAspectRatio{ false };

        // Disable imgui setting the mouse cursor image itself.
        ig::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

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

            static trc::CursorShape selectedCursor{ trc::CursorShape::eDefault };
            if (ig::BeginCombo("Cursor Shape", toString(selectedCursor)))
            {
                for (auto shape : kAllCursorShapes)
                {
                    if (ig::Selectable(toString(shape), shape == selectedCursor)) {
                        selectedCursor = shape;
                    }
                }
                ig::EndCombo();
            }

            // Honor requests from ImGui to set the mouse cursor shape.
            // Overrides cursor setting via the dropdown menu.
            const auto imguiCursor = ig::GetMouseCursor();
            if (imguiCursor != ImGuiMouseCursor_None && imguiCursor != ImGuiMouseCursor_Arrow) {
                window.setCursorShape(toTorchEnum(imguiCursor));
            }
            else {
                window.setCursorShape(selectedCursor);
            }

            // End the frame.
            ig::End();

            // Un-hide window after some time
            if (hidden && windowHiddenTimer.duration() > 2000.0f)
            {
                hidden = false;
                window.show();
            }

            torch->drawFrame(vp);
        }

        torch->waitForAllFrames();
    }

    trc::terminate();

    return 0;
}
