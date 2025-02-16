#pragma once

#include <trc/Torch.h>
using namespace trc::basic_types;

struct WindowRenderer
{
    s_ptr<trc::Window> window;
    u_ptr<trc::Renderer> renderer;
    u_ptr<trc::RenderPipeline> renderPipeline;

    auto getWindow() -> trc::Window&;
    auto getRenderPipeline() -> trc::RenderPipeline&;

    auto makeViewport(const trc::RenderArea& extent,
                      s_ptr<trc::Camera> camera,
                      s_ptr<trc::Scene> scene)
        -> trc::ViewportHandle;

    auto makeFrame() -> u_ptr<trc::Frame>;
    void submitFrame(u_ptr<trc::Frame> frame);
};

class GraphicsStack
{
public:
    GraphicsStack();
    ~GraphicsStack() noexcept;

    auto getDevice() -> trc::Device&;
    auto getInstance() -> trc::Instance&;

    auto makeWindow(trc::AssetRegistry& assets) -> u_ptr<WindowRenderer>;

private:
    static constexpr vec3 kClearColor{ 0.12f, 0.12f, 0.12f };

    trc::Instance instance;
};
