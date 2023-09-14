#include "Controls.h"

#include <trc/base/event/Event.h>
#include <trc/base/event/InputState.h>
#include <trc/core/Window.h>
#include <trc_util/Timer.h>

#include "MaterialEditorGui.h"



struct ZoomState
{
    i32 zoomLevel{ 0 };
};

struct MouseState
{
    struct Drag
    {
        vec2 initialMousePos{ 0, 0 };
        vec2 dragIncrement{ 0, 0 };
    };

    vec2 currentMousePos{ 0, 0 };
    bool wasPressed{ false };  // True if the primary mouse button was pressed at all
    bool wasReleased{ false };  // True if the primary mouse button was released
    bool wasClicked{ false };  // True if the primary mouse button was pressed and released
                               // without dragging the mouse

    std::optional<Drag> drag;
};

struct KeyboardState
{
    bool didDelete{ false };
    bool didSelectAll{ false };
    bool didUnselectAll{ false };
};

auto updateMouse() -> MouseState;
auto updateZoom() -> ZoomState;
auto updateKeyboard() -> KeyboardState;

ZoomState globalZoomState{ 0 };



MaterialEditorControls::MaterialEditorControls(
    trc::Window& window,
    MaterialEditorGui& gui,
    trc::Camera& camera,
    const MaterialEditorControlsCreateInfo& info)
    :
    window(&window),
    camera(&camera),
    gui(&gui)
{
    constexpr trc::Key kCancelKey{ trc::Key::escape };
    constexpr trc::MouseButton kContextMenuOpenKey{ trc::MouseButton::right };

    trc::Keyboard::init();
    trc::Mouse::init();
    globalZoomState.zoomLevel = info.initialZoomLevel;

    trc::on<trc::ScrollEvent>([](const trc::ScrollEvent& e) {
        globalZoomState.zoomLevel += e.yOffset < 0.0f ? 1 : -1;
        globalZoomState.zoomLevel = glm::max(0, globalZoomState.zoomLevel);
    });

    trc::on<trc::MouseClickEvent>([&gui](auto&& e) {
        if (e.button == kContextMenuOpenKey) {
            gui.openContextMenu(trc::Mouse::getPosition());
        }
        else {
            gui.closeContextMenu();
        }
    });
    trc::on<trc::KeyPressEvent>([&gui](auto&& e) {
        if (e.key == kCancelKey) {
            gui.closeContextMenu();
        }
    });
}

void MaterialEditorControls::update(GraphScene& graph)
{
    constexpr float kZoomStep{ 0.15f };
    constexpr trc::Key kChainSelectionModKey{ trc::Key::left_shift };

    const auto mouseState = updateMouse();
    const auto zoomState = updateZoom();
    const auto kbState = updateKeyboard();

    // Calculate hover
    const vec2 worldPos = camera->unproject(mouseState.currentMousePos, 0.0f, window->getSize());
    auto [node, hoveredSocket] = graph.findHover(worldPos);
    graph.interaction.hoveredNode = node;
    graph.interaction.hoveredSocket = hoveredSocket;

    // Receive keyboard input
    if (!mouseState.drag)
    {
        if (kbState.didDelete)
        {
            auto& selected = graph.interaction.selectedNodes;
            if (node && selected.contains(*node)) {
                graph.interaction.hoveredNode.reset();
            }
            for (auto node : selected) {
                graph.removeNode(node);
            }
            selected.clear();
        }
        if (kbState.didSelectAll) {
            graph.interaction.selectedNodes = { graph.graph.nodeInfo.keyBegin(),
                                                graph.graph.nodeInfo.keyEnd() };
        }
        if (kbState.didUnselectAll) {
            graph.interaction.selectedNodes.clear();
        }
    }

    // Calculate node selection based on hover information
    if (mouseState.wasPressed && node)
    {
        auto& selected = graph.interaction.selectedNodes;

        if (trc::Keyboard::isPressed(kChainSelectionModKey))
        {
            // Toggle hovered node's existence in the set of selected nodes
            if (selected.contains(*node)) {
                selected.erase(*node);
            }
            else {
                selected.emplace(*node);
            }
        }
        else {
            selected.clear();
            selected.emplace(*node);
        }
    }
    if (mouseState.wasClicked && !node) {
        graph.interaction.selectedNodes.clear();
    }

    // Movement via dragging
    static std::optional<SocketID> draggedSocket;
    if (hoveredSocket && mouseState.wasPressed)
    {
        if (graph.graph.link.contains(*hoveredSocket)) {
            graph.graph.unlinkSockets(*hoveredSocket);
        }
        draggedSocket = *hoveredSocket;
    }

    // Mouse drag while not dragging a link from a socket
    if (mouseState.drag && !draggedSocket)
    {
        const vec2 move = (mouseState.drag->dragIncrement / vec2(window->getSize()))
                          * cameraViewportSize;

        if (graph.interaction.hoveredNode)
        {
            // If at least one node is hovered, move the entire selection
            for (const NodeID node : graph.interaction.selectedNodes) {
                graph.layout.nodeSize.get(node).origin += move;
            }
        }
        else {
            camera->translate(vec3(move, 0.0f));
        }
    }

    // Resolve creation of links between sockets
    if (mouseState.wasReleased)
    {
        if (draggedSocket && hoveredSocket && draggedSocket != hoveredSocket)
        {
            if (graph.graph.link.contains(*hoveredSocket)) {
                graph.graph.unlinkSockets(*hoveredSocket);
            }
            graph.graph.linkSockets(*draggedSocket, *hoveredSocket);
        }
        draggedSocket.reset();
    }

    // Camera zoom
    const float zoomLevel = static_cast<float>(glm::max(zoomState.zoomLevel, 0));
    const vec2 x = { 0.0f - zoomLevel * kZoomStep, 1.0f + zoomLevel * kZoomStep };
    const vec2 y = x / window->getAspectRatio();
    camera->makeOrthogonal(x[0], x[1], y[0], y[1], -10.0f, 10.0f);
    cameraViewportSize = { x[1] - x[0], y[1] - y[0] };
}

auto updateMouse() -> MouseState
{
    constexpr trc::MouseButton kDragButton{ trc::MouseButton::left };
    constexpr ui32 kFramesUntilDrag{ 5 };

    static ui32 framesMoved{ 0 };
    static vec2 initialMousePos{ 0, 0 };
    static vec2 prevMousePos{ 0, 0 };

    MouseState res{
        .currentMousePos = trc::Mouse::getPosition(),
        .drag = std::nullopt,
    };

    if (trc::Mouse::wasPressed(kDragButton))
    {
        framesMoved = 0;
        initialMousePos = trc::Mouse::getPosition();
        res.wasPressed = true;
    }
    if (trc::Mouse::wasReleased(kDragButton))
    {
        res.wasReleased = true;
        if (framesMoved < kFramesUntilDrag) {
            res.wasClicked = true;
        }
        framesMoved = 0;
    }

    // Test if mouse was moved
    if (trc::Mouse::wasMoved() && trc::Mouse::isPressed(kDragButton))
    {
        ++framesMoved;
        if (framesMoved >= kFramesUntilDrag)
        {
            res.drag = MouseState::Drag{
                .initialMousePos = initialMousePos,
                .dragIncrement = trc::Mouse::getPosition() - prevMousePos,
            };
        }
    }
    prevMousePos = trc::Mouse::getPosition();

    return res;
}

auto updateZoom() -> ZoomState
{
    return globalZoomState;
}

auto updateKeyboard() -> KeyboardState
{
    constexpr float kMillisWithingDoubleClick{ 200 };
    constexpr trc::Key kDeleteKey{ trc::Key::del };
    constexpr trc::Key kSelectAllKey{ trc::Key::a };

    KeyboardState res;

    // Delete all selected nodes
    if (trc::Keyboard::wasPressed(kDeleteKey))
    {
        res.didDelete = true;
    }

    // Select all nodes
    static trc::Timer timeSinceSelectAll;
    if (trc::Keyboard::wasPressed(kSelectAllKey))
    {
        if (timeSinceSelectAll.duration() <= kMillisWithingDoubleClick) {
            res.didUnselectAll = true;
        }
        else {
            res.didSelectAll = true;
            timeSinceSelectAll.reset();
        }
    }

    return res;
}
