#include "Controls.h"

#include <cassert>
#include <fstream>
#include <variant>

#include <trc/base/event/Event.h>
#include <trc/base/event/InputState.h>
#include <trc/core/Window.h>
#include <trc/material/ShaderCodeCompiler.h>
#include <trc/material/ShaderModuleCompiler.h>
#include <trc/material/TorchMaterialSettings.h>
#include <trc_util/Timer.h>

#include "ControlState.h"
#include "GraphCompiler.h"
#include "ManipulationActions.h"
#include "MaterialEditorGui.h"



struct ZoomState
{
    i32 zoomLevel{ 0 };
};

struct MouseState
{
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

struct ControlInput
{
    KeyboardState kbState;
    MouseState mouseState;
    ZoomState zoomState;

    std::optional<NodeID> hoveredNode;
    std::optional<SocketID> hoveredSocket;
    std::optional<SocketID> hoveredInputField;

    const trc::Window* window;
    const trc::Camera* camera;

    // Scale camera drag distance by size of the orthogonal camera
    const vec2 cameraViewportSize{ 1.0f, 1.0f };

    auto toWorldPos(vec2 screenPos) const -> vec2 {
        return camera->unproject(screenPos, 0.0f, window->getSize());
    }
    auto toScreenDir(vec2 screenPos) const -> vec2 {
        return (screenPos / vec2(window->getSize())) * cameraViewportSize;
    }
};

struct ControlOutput
{
    GraphScene& graph;
    MaterialEditorCommands& manip;

    trc::Camera* camera;
    vec2 cameraViewportSize{ 1.0f, 1.0f };

    void setCameraProjection(float l, float r, float b, float t, float near, float far)
    {
        camera->makeOrthogonal(l, r, b, t, near, far);
        cameraViewportSize = { r - l, t - b };
    }

    /**
     * @param bool append If true, add selected nodes to the current selection.
     *                    Otherwise overwrite the current selection with nodes
     *                    in the selection area.
     */
    void selectNodesInArea(Hitbox area, bool append)
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
};

auto updateMouse() -> MouseState;
auto updateZoom() -> ZoomState;
auto updateKeyboard() -> KeyboardState;

ZoomState globalZoomState{ 0 };

struct CreateLinkState : ControlState
{
    CreateLinkState(SocketID from, GraphTopology& graph)
        : draggedSocket(from)
    {
        if (graph.link.contains(from)) {
            graph.unlinkSockets(from);
        }
    }

    SocketID draggedSocket;

    auto update(const ControlInput& in, ControlOutput& out)
        -> StateResult override
    {
        auto& graph = out.graph.graph;
        auto& selectButton = in.mouseState.selectButton;

        // Resolve creation of links between sockets
        if (selectButton.wasReleased)
        {
            // If, during button release, a socket is hovered, link that socket
            // to the original one.
            if (in.hoveredSocket && draggedSocket != in.hoveredSocket)
            {
                if (graph.link.contains(*in.hoveredSocket)) {
                    graph.unlinkSockets(*in.hoveredSocket);
                }
                graph.linkSockets(draggedSocket, *in.hoveredSocket);
            }
            return PopState{};
        }

        return NoAction{};
    }
};

struct MoveCameraState : ControlState
{
    auto update(const ControlInput& in, ControlOutput& out)
        -> StateResult override
    {
        const auto& cameraButton = in.mouseState.cameraMoveButton;
        if (!cameraButton.drag) {
            return PopState{};
        }

        const vec2 move = in.toScreenDir(cameraButton.drag->dragIncrement);
        out.camera->translate(vec3(move, 0.0f));

        return NoAction{};
    }
};

struct MoveSelectedNodesState : ControlState
{
    auto update(const ControlInput& in, ControlOutput& out)
        -> StateResult override
    {
        const auto& selectButton = in.mouseState.selectButton;
        if (!selectButton.drag) {
            return PopState{};
        }

        const auto& selection = out.graph.interaction.selectedNodes;
        auto& layout = out.graph.layout;

        // If at least one node is hovered, move the entire selection
        const vec2 move = in.toScreenDir(selectButton.drag->dragIncrement);
        for (const NodeID node : selection) {
            layout.nodeSize.get(node).origin += move;
        }

        return NoAction{};
    }
};

struct MultiSelectState : ControlState
{
    auto update(const ControlInput& in, ControlOutput& out)
        -> StateResult override
    {
        const auto& selectButton = in.mouseState.selectButton;
        auto& graph = out.graph;
        if (!selectButton.drag)
        {
            graph.interaction.multiSelectBox.reset();
            return PopState{};
        }

        const vec2 boxPos = in.toWorldPos(selectButton.drag->initialMousePos);
        const vec2 boxSize = in.toWorldPos(in.mouseState.currentMousePos) - boxPos;
        const Hitbox box{ boxPos, boxSize };
        graph.interaction.multiSelectBox = box;
        out.selectNodesInArea(box, false);

        return NoAction{};
    }
};

struct DefaultControlState : ControlState
{
    auto update(const ControlInput& in, ControlOutput& out)
        -> StateResult override
    {
        constexpr float kZoomStep{ 0.15f };
        constexpr trc::Key kChainSelectionModKey{ trc::Key::left_shift };

        const auto& kbState = in.kbState;
        const auto& mouseState = in.mouseState;
        const auto& zoomState = in.zoomState;
        auto& graph = out.graph;

        // Process keyboard input
        if (auto action = processKeyboardInput(kbState, out.graph, out.manip)) {
            return std::move(action.value());
        }

        // Calculate hover
        const vec2 worldPos = in.toWorldPos(mouseState.currentMousePos);
        auto [hoveredNode, hoveredSocket, hoveredDeco] = graph.findHover(worldPos);

        // Process mouse input
        if (mouseState.selectButton.wasClicked)
        {
            auto& selection = graph.interaction.selectedNodes;

            // If the user clicked on a linked socket, unlink it.
            // Otherwise, if the user clicked on a node, select it.
            // Otherwise unselect all nodes.
            if (hoveredSocket && graph.graph.link.contains(*hoveredSocket)) {
                graph.graph.unlinkSockets(*hoveredSocket);
            }
            else if (hoveredNode)
            {
                if (!trc::Keyboard::isPressed(kChainSelectionModKey)) {
                    selection.clear();
                }

                // Toggle hovered node's existence in the set of selected nodes
                if (selection.contains(*hoveredNode)) {
                    selection.erase(*hoveredNode);
                }
                else {
                    selection.emplace(*hoveredNode);
                }
            }
            else {
                selection.clear();
            }
        }

        if (mouseState.selectButton.drag)
        {
            if (hoveredSocket) {
                return PushState{ std::make_unique<CreateLinkState>(*hoveredSocket, graph.graph) };
            }
            else if (hoveredNode)
            {
                auto& selection = graph.interaction.selectedNodes;

                // Special case: Create new selection if the mouse tries to drag
                // a non-selected node.
                if (!selection.contains(*hoveredNode))
                {
                    selection.clear();
                    selection.emplace(*hoveredNode);
                }
                return PushState{ std::make_unique<MoveSelectedNodesState>() };
            }
            else {
                return PushState{ std::make_unique<MultiSelectState>() };
            }
        }

        if (mouseState.cameraMoveButton.drag) {
            return PushState{ std::make_unique<MoveCameraState>() };
        }

        // Camera zoom
        const float zoomLevel = static_cast<float>(glm::max(zoomState.zoomLevel, 0));
        const vec2 x = { 0.0f - zoomLevel * kZoomStep, 1.0f + zoomLevel * kZoomStep };
        const vec2 y = x / in.window->getAspectRatio();
        out.setCameraProjection(x[0], x[1], y[0], y[1], -10.0f, 10.0f);

        // Write output state
        graph.interaction.hoveredNode = hoveredNode;
        graph.interaction.hoveredSocket = hoveredSocket;

        return NoAction{};
    }

    static auto processKeyboardInput(const KeyboardState& kbState,
                                     GraphScene& graph,
                                     MaterialEditorCommands& manip)
        -> std::optional<StateResult>
    {
        if (trc::Keyboard::wasPressed(trc::Key::m)) {
            manip.compileMaterial();
        }

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
            manip.saveGraphToCurrentFile();
        }

        return std::nullopt;
    }
};



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

    stateStack.push(std::make_unique<DefaultControlState>());
}

MaterialEditorControls::~MaterialEditorControls() noexcept = default;

void MaterialEditorControls::update(GraphScene& graph, MaterialEditorCommands& manip)
{
    assert(!stateStack.empty());

    // Update/receive user input
    ControlInput in{
        .kbState=updateKeyboard(),
        .mouseState=updateMouse(),
        .zoomState=updateZoom(),
        .hoveredNode{}, .hoveredSocket{},
        .window=window,
        .camera=camera,
        .cameraViewportSize=cameraViewportSize,
    };
    ControlOutput out{
        .graph=graph,
        .manip=manip,
        .camera=camera,
        .cameraViewportSize=cameraViewportSize,
    };

    // Prepare pre-calculated input to the control state
    auto hoverResult = graph.findHover(in.toWorldPos(in.mouseState.currentMousePos));
    in.hoveredNode = hoverResult.hoveredNode;
    in.hoveredSocket = hoverResult.hoveredSocket;
    in.hoveredInputField = hoverResult.hoveredInputField;

    // Run the currently active control state
    auto result = stateStack.top()->update(in, out);

    // Perform requested state transitions
    std::visit(trc::util::VariantVisitor{
        [this](PushState& push){ stateStack.push(std::move(push.newState)); },
        [this](PopState&){ stateStack.pop(); assert(!stateStack.empty()); },
        [](NoAction&){},
    }, result);

    // Post-process output
    this->cameraViewportSize = out.cameraViewportSize;
}



auto updateMouse() -> MouseState
{
    constexpr trc::MouseButton kCameraMoveButton{ trc::MouseButton::middle };
    constexpr trc::MouseButton kSelectButton{ trc::MouseButton::left };
    constexpr ui32 kFramesUntilDrag{ 5 };

    struct ButtonUpdater
    {
        ButtonUpdater(trc::MouseButton button) : button(button) {}

        auto update(const bool mouseWasMoved) -> MouseState::ButtonState
        {
            MouseState::ButtonState res{
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
                res.drag = MouseState::ButtonState::Drag{
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
