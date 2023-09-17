#include "Controls.h"

#include <fstream>

#include <trc/base/event/Event.h>
#include <trc/base/event/InputState.h>
#include <trc/core/Window.h>
#include <trc_util/Timer.h>

#include "GraphSerializer.h"
#include "ManipulationActions.h"
#include "MaterialEditorGui.h"



struct ZoomState
{
    i32 zoomLevel{ 0 };
};

struct ButtonState
{
    struct Drag
    {
        vec2 initialMousePos{ 0, 0 };
        vec2 dragIncrement{ 0, 0 };
    };

    bool wasPressed{ false };   // True if the primary mouse button was pressed at all
    bool wasClicked{ false };   // True if the primary mouse button was pressed and released
                                // without dragging the mouse
    bool wasReleased{ false };  // True if the primary mouse button was released

    std::optional<Drag> drag{ std::nullopt };
};

struct MouseState
{
    vec2 currentMousePos{ 0, 0 };

    ButtonState selectButton;  // Primary mouse button
    ButtonState cameraMoveButton;
};

struct KeyboardState
{
    bool didUndo{ false };
    bool didRedo{ false };

    bool didDelete{ false };
    bool didSelectAll{ false };
    bool didUnselectAll{ false };

    bool didSave{ false };
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

void MaterialEditorControls::update(GraphScene& graph, GraphManipulator& manip)
{
    constexpr float kZoomStep{ 0.15f };
    constexpr trc::Key kChainSelectionModKey{ trc::Key::left_shift };

    static std::optional<SocketID> draggedSocket;

    const auto [mousePos, selectButton, cameraButton] = updateMouse();
    const auto zoomState = updateZoom();
    const auto kbState = updateKeyboard();

    // Derived states
    const bool anyDragActive = selectButton.drag || cameraButton.drag;

    // Calculate hover
    const vec2 worldPos = camera->unproject(mousePos, 0.0f, window->getSize());
    auto [hoveredNode, hoveredSocket] = graph.findHover(worldPos);

    // Only result in graphical hover highlight when we're not doing
    // something else while hovering
    graph.interaction.hoveredNode = anyDragActive ? std::nullopt : hoveredNode;
    graph.interaction.hoveredSocket = (anyDragActive && !draggedSocket) ? std::nullopt : hoveredSocket;

    // Receive keyboard input
    if (!anyDragActive)
    {
        if (kbState.didUndo) manip.undoLastAction();
        if (kbState.didRedo) manip.reapplyLastUndoneAction();

        if (kbState.didDelete)
        {
            auto& selected = graph.interaction.selectedNodes;
            manip.applyAction(std::make_unique<action::MultiAction>(
                selected | std::views::transform(
                    [](NodeID node){ return std::make_unique<action::RemoveNode>(node); }
                )
            ));
            selected.clear();
        }
        if (kbState.didSelectAll) {
            graph.interaction.selectedNodes = { graph.graph.nodeInfo.keyBegin(),
                                                graph.graph.nodeInfo.keyEnd() };
        }
        if (kbState.didUnselectAll) {
            graph.interaction.selectedNodes.clear();
        }

        if (kbState.didSave) {
            std::ofstream file(".matedit_save", std::ios::binary);
            serializeGraph(graph, file);
        }
    }

    // Calculate node selection based on hover information
    if (selectButton.wasClicked && hoveredNode)
    {
        auto& selected = graph.interaction.selectedNodes;

        if (trc::Keyboard::isPressed(kChainSelectionModKey))
        {
            // Toggle hovered node's existence in the set of selected nodes
            if (selected.contains(*hoveredNode)) {
                selected.erase(*hoveredNode);
            }
            else {
                selected.emplace(*hoveredNode);
            }
        }
        else {
            selected.clear();
            selected.emplace(*hoveredNode);
        }
    }
    if (selectButton.wasClicked && !hoveredNode) {
        graph.interaction.selectedNodes.clear();
    }

    // Click on a hovered socket
    if (hoveredSocket && selectButton.wasPressed)
    {
        if (graph.graph.link.contains(*hoveredSocket)) {
            graph.graph.unlinkSockets(*hoveredSocket);
        }
        draggedSocket = *hoveredSocket;
    }

    // Mouse drag while not dragging a link from a socket
    if (selectButton.drag && !draggedSocket)
    {
        // Move selected nodes around
        if (hoveredNode && !graph.interaction.multiSelectBox)
        {
            auto& selection = graph.interaction.selectedNodes;
            // Special case: Create new selection if the mouse tries to drag
            // a non-selected node.
            if (!selection.contains(*hoveredNode))
            {
                selection.clear();
                selection.emplace(*hoveredNode);
            }

            // If at least one node is hovered, move the entire selection
            const vec2 move = toWorldDir(selectButton.drag->dragIncrement);
            for (const NodeID node : selection) {
                graph.layout.nodeSize.get(node).origin += move;
            }
        }
        // Drag a multi-select box
        else {
            const vec2 boxPos = toWorldPos(selectButton.drag->initialMousePos);
            const vec2 boxSize = toWorldPos(mousePos) - boxPos;
            const Hitbox box{ boxPos, boxSize };
            graph.interaction.multiSelectBox = box;
            selectNodesInArea(box, graph, false);
        }
    }

    // Resolve a multi-select box that's currently being dragged
    if (selectButton.wasReleased && graph.interaction.multiSelectBox)
    {
        selectNodesInArea(*graph.interaction.multiSelectBox, graph, false);
        graph.interaction.multiSelectBox.reset();
    }

    if (cameraButton.drag)
    {
        const vec2 move = toWorldDir(cameraButton.drag->dragIncrement);
        camera->translate(vec3(move, 0.0f));
    }

    // Resolve creation of links between sockets
    if (selectButton.wasReleased)
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

auto MaterialEditorControls::toWorldPos(vec2 screenPos) const -> vec2
{
    return camera->unproject(screenPos, 0.0f, window->getSize());
}

auto MaterialEditorControls::toWorldDir(vec2 screenPos) const -> vec2
{
    return (screenPos / vec2(window->getSize())) * cameraViewportSize;
}

void MaterialEditorControls::selectNodesInArea(Hitbox area, GraphScene& graph, bool append)
{
    if (!append) {
        graph.interaction.selectedNodes.clear();
    }

    for (const auto& [node, hitbox] : graph.layout.nodeSize.items())
    {
        const vec2 midPoint = hitbox.origin + hitbox.extent * 0.5f;
        if (isInside(midPoint, area)) {
            graph.interaction.selectedNodes.emplace(node);
        }
    }
}



auto updateMouse() -> MouseState
{
    constexpr trc::MouseButton kCameraMoveButton{ trc::MouseButton::middle };
    constexpr trc::MouseButton kSelectButton{ trc::MouseButton::left };
    constexpr ui32 kFramesUntilDrag{ 5 };

    struct ButtonUpdater
    {
        ButtonUpdater(trc::MouseButton button) : button(button) {}

        auto update(const bool mouseWasMoved) -> ButtonState
        {
            ButtonState res{
                .wasPressed=trc::Mouse::wasPressed(button),
                .wasReleased=trc::Mouse::wasReleased(button),
            };

            if (res.wasPressed)
            {
                framesMoved = 0;
                initialMousePos = trc::Mouse::getPosition();
            }
            if (res.wasReleased)
            {
                if (framesMoved < kFramesUntilDrag) {
                    res.wasClicked = true;
                }
                framesMoved = 0;
            }

            // Test if mouse was moved
            if (mouseWasMoved) ++framesMoved;
            if ((framesMoved >= kFramesUntilDrag) && trc::Mouse::isPressed(button))
            {
                res.drag = ButtonState::Drag{
                    .initialMousePos = initialMousePos,
                    .dragIncrement = trc::Mouse::getPosition() - prevMousePos,
                };
            }
            prevMousePos = trc::Mouse::getPosition();

            return res;
        }

        trc::MouseButton button;

        ui32 framesMoved{ 0 };
        vec2 initialMousePos{ 0, 0 };
        vec2 prevMousePos{ 0, 0 };
    };

    static ButtonUpdater cameraButton{ kCameraMoveButton };
    static ButtonUpdater selectButton{ kSelectButton };

    const bool mouseWasMoved = trc::Mouse::wasMoved();
    MouseState res{
        .currentMousePos = trc::Mouse::getPosition(),
        .selectButton = selectButton.update(mouseWasMoved),
        .cameraMoveButton = cameraButton.update(mouseWasMoved),
    };

    return res;
}

auto updateZoom() -> ZoomState
{
    return globalZoomState;
}

auto updateKeyboard() -> KeyboardState
{
    constexpr float kMillisWithingDoubleClick{ 200 };
    constexpr trc::Key kUndoKey{ trc::Key::z };
    constexpr trc::Key kUndoKeyMod{ trc::Key::left_ctrl };
    constexpr trc::Key kRedoKeyMod{ trc::Key::left_shift };
    constexpr trc::Key kDeleteKey{ trc::Key::del };
    constexpr trc::Key kSelectAllKey{ trc::Key::a };
    constexpr trc::Key kSaveKey{ trc::Key::s };
    constexpr trc::Key kSaveKeyMod{ trc::Key::left_ctrl };

    KeyboardState res;

    // Undo/redo
    if (trc::Keyboard::wasPressed(kUndoKey) && trc::Keyboard::isPressed(kUndoKeyMod))
    {
        if (trc::Keyboard::isPressed(kRedoKeyMod)) {
            res.didRedo = true;
        }
        else {
            res.didUndo = true;
        }
    }

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

    if (trc::Keyboard::wasPressed(kSaveKey) && trc::Keyboard::isPressed(kSaveKeyMod)) {
        res.didSave = true;
    }

    return res;
}
