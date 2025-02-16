#include "viewport/ViewportTreeController.h"

#include <imgui.h>
#include <trc/TorchRenderStages.h>
#include <trc/core/Window.h>
#include <trc_util/Assert.h>

namespace ig = ImGui;

/** @brief Translate the ImGuiMouseCursor enum to Torch's CursorShape. */
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

constexpr
auto makeSplitLine(ViewportTree::SplitLine split, ui32 lineOffset)
    -> std::pair<vec2, vec2>
{
    const auto [pos, size] = split.getSplitArea();
    return {
        split.isHorizontal() ? vec2{ pos.x, pos.y + lineOffset }
                             : vec2{ pos.x + lineOffset, pos.y },
        split.isHorizontal() ? vec2{ pos.x + size.x, pos.y + lineOffset }
                             : vec2{ pos.x + lineOffset, pos.y + size.y },
    };
}

constexpr vec4 kSplitLineColor{ 1, 1, 1, 1 };

/**
 * @brief Resizes a viewport split by dragging a split line.
 */
class ViewportSplitDragCommand : public Command
{
public:
    explicit ViewportSplitDragCommand(ViewportTree* tree, s_ptr<PrimitiveDrawList> drawList)
        : tree(tree), drawList(drawList)
    {}

    void execute(CommandExecutionContext& ctx) override
    {
        auto elem = tree->findAt(ctx.mouse().getCursorPos());
        if (elem && std::holds_alternative<ViewportTree::SplitLine>(*elem))
        {
            auto split = std::get<ViewportTree::SplitLine>(*elem);

            // Show the initial split line
            const auto [from, to] = makeSplitLine(split, split.getLocation());
            drawList->pushLine(from, to, vec4{ 1, 1, 1, 1 });

            // Set up nested input frame
            auto frame = ctx.pushFrame(SplitLineDragState{ .split=split, .drawList=drawList, });
            frame.onCursorMove([](auto& state, auto&&, const CursorMovement& cursor)
            {
                const auto area = state.split.getSplitArea();
                const auto size = state.split.isHorizontal() ? area.size.y : area.size.x;

                // Clamp new location to [0, size)
                const i32 diff = state.split.isHorizontal() ? cursor.offset.y : cursor.offset.x;
                state.splitLocation = glm::max(i32(state.splitLocation) + diff, 0);
                state.splitLocation = glm::min(state.splitLocation, size - 1);

                // Show the new split line
                const auto [from, to] = makeSplitLine(state.split, state.splitLocation);
                state.drawList->clear();
                state.drawList->pushLine(from, to, kSplitLineColor);
            });
            frame.on(
                { trc::MouseButton::left, trc::InputAction::release },
                [](SplitLineDragState& state, auto&&)
                {
                    state.split.setLocation(SplitLocation::makePixel(state.splitLocation));
                    state.exitFrame();
                    state.drawList->clear();
                }
            );
        }
        else {
            ctx.discardEvent();
        }
    }

private:
    struct SplitLineDragState : public InputFrame
    {
        ViewportTree::SplitLine split;
        ui32 splitLocation{ split.getLocation() };

        s_ptr<PrimitiveDrawList> drawList;
    };

    ViewportTree* tree;
    s_ptr<PrimitiveDrawList> drawList;
};

struct ViewportTreeVisualizeCommand : public Command
{
    static constexpr vec4 kViewportColor{ 1, 1, 1, 0.3f };
    static constexpr vec4 kViewportBorderColor{ 1, 1, 1, 1 };
    static constexpr vec4 kSplitLineColor{ 1.0f, 0.5f, 0.3f, 1.0f };

    ViewportTreeVisualizeCommand(s_ptr<ViewportTree> tree,
                                 s_ptr<PrimitiveDrawList> drawList)
        : tree(tree), drawList(drawList)
    {}

    void execute(CommandExecutionContext& ctx) override
    {
        highlightElemAt(ctx.mouse().getCursorPos());

        auto frame = ctx.pushFrame();
        frame.onCursorMove([this](auto&, const CursorMovement& cursor) {
            drawList->clear();
            highlightElemAt(cursor.position);
        });
        frame.on({ trc::Key::left_alt, trc::KeyModFlagBits::alt, trc::InputAction::release },
            [this](auto& state) {
                state.exitFrame();
                drawList->clear();
            }
        );
    }

    void pushRect(const ViewportArea& area)
    {
        const vec2 ll{ area.pos.x, area.pos.y + area.size.y };
        const vec2 ur{ area.pos.x + area.size.x, area.pos.y };
        drawList->pushQuad(ll, ur, kViewportColor);
        drawList->pushLine(ll,             { ur.x, ll.y }, kViewportBorderColor);
        drawList->pushLine(ll,             { ll.x, ur.y }, kViewportBorderColor);
        drawList->pushLine({ ll.x, ur.y }, ur,             kViewportBorderColor);
        drawList->pushLine({ ur.x, ll.y }, ur,             kViewportBorderColor);
    };

    void highlightElemAt(const ivec2 pos)
    {
        if (auto elem = tree->findAt(pos))
        {
            std::visit(trc::util::VariantVisitor{
                [&](Viewport* vp) {
                    pushRect(vp->getSize());
                },
                [&](ViewportTree::SplitLine split) {
                    pushRect(split.getSplitArea());
                    const auto [from, to] = makeSplitLine(split, split.getLocation());
                    drawList->pushLine(from, to, kSplitLineColor);
                },
            }, *elem);
        }
    };

    s_ptr<ViewportTree> tree;
    s_ptr<PrimitiveDrawList> drawList;
};



ViewportTreeController::ViewportTreeController(
    s_ptr<ViewportTree> _tree,
    trc::Window* _window,
    GraphicsStack& graphics)
    :
    window(_window),
    tree(_tree),
    primitiveRenderer(
        &graphics.getDevice(),
        window->getImageFormat(),
        trc::DefaultDeviceMemoryAllocator{}
    ),
    drawList(std::make_shared<PrimitiveDrawList>())
{
    assert_arg(_window != nullptr);
    assert_arg(_tree != nullptr);

    auto& frame = rootHandler.getRootFrame();
    frame.on(
        trc::MouseButton::left,
        std::make_unique<ViewportSplitDragCommand>(_tree.get(), drawList)
    );
    frame.onCursorMove([this](CommandExecutionContext& ctx, const CursorMovement& cursor)
    {
        drawList->clear();

        std::optional<trc::CursorShape> selectedCursor;
        if (auto elem = tree->findAt(cursor.position))
        {
            std::visit(trc::util::VariantVisitor{
                [](Viewport*) {},
                [&](ViewportTree::SplitLine split) {
                    selectedCursor = split.isHorizontal() ? trc::CursorShape::eResizeVertical
                                                          : trc::CursorShape::eResizeHorizontal;
                    const auto [from, to] = makeSplitLine(split, split.getLocation());
                    drawList->pushLine(from, to, kSplitLineColor);
                },
            }, *elem);
        }

        // Honor requests from ImGui to set the mouse cursor shape.
        // Overrides cursor setting via the dropdown menu.
        //
        // TODO: Use custom window class that wraps a `setCursorShape` function
        // around this stuff.
        const auto imguiCursor = ig::GetMouseCursor();
        if (imguiCursor != ImGuiMouseCursor_None && imguiCursor != ImGuiMouseCursor_Arrow) {
            window->setCursorShape(toTorchEnum(imguiCursor));
        }
        else if (selectedCursor) {
            window->setCursorShape(*selectedCursor);
        }
        else {
            window->setCursorShape(trc::CursorShape::eDefault);
        }

        ctx.discardEvent();
    });

    // A cute debugging tool.
    // Hold <left-alt> to highlight hovered viewports/splits.
    frame.on({ trc::Key::left_alt, trc::InputAction::press },
             std::make_unique<ViewportTreeVisualizeCommand>(tree, drawList));
}

void ViewportTreeController::draw(trc::Frame& frame)
{
    tree->draw(frame);

    static auto primStage = trc::RenderStage::make("primitive_draw");
    trc::RenderGraph graph;
    graph.createOrdering(trc::stages::post, primStage);
    graph.createOrdering(primStage, trc::stages::renderTargetImageFinalize);
    frame.mergeRenderGraph(graph);
    trc::Viewport windowVp{
        trc::makeRenderTarget(*window).getCurrentRenderImage(),
        { { 0, 0 }, window->getSize() }
    };
    primitiveRenderer.draw(*drawList, frame, primStage, windowVp);
}

void ViewportTreeController::resize(const ViewportArea& newArea)
{
    tree->resize(newArea);
}

auto ViewportTreeController::getSize() -> ViewportArea
{
    return tree->getSize();
}

auto ViewportTreeController::notify(const UserInput& input) -> NotifyResult
{
    if (rootHandler.notify(input) == NotifyResult::eConsumed) {
        return NotifyResult::eConsumed;
    }
    if (auto vp = tree->findViewportAt(cursorPos)) {
        return vp->notify(input);
    }
    return NotifyResult::eRejected;
}

auto ViewportTreeController::notify(const Scroll& scroll) -> NotifyResult
{
    if (rootHandler.notify(scroll) == NotifyResult::eConsumed) {
        return NotifyResult::eConsumed;
    }
    if (auto vp = tree->findViewportAt(cursorPos)) {
        return vp->notify(scroll);
    }
    return NotifyResult::eRejected;
}

auto ViewportTreeController::notify(const CursorMovement& cursor) -> NotifyResult
{
    cursorPos = cursor.position;

    if (rootHandler.notify(cursor) == NotifyResult::eConsumed) {
        return NotifyResult::eConsumed;
    }

    if (auto vp = tree->findViewportAt(cursorPos))
    {
        const auto [vpPos, vpSize] = vp->getSize();
        CursorMovement childCursorMove{
            .position = cursor.position - vec2{vpPos},
            .offset   = cursor.offset,
            .areaSize = vpSize,
        };

        return vp->notify(childCursorMove);
    }
    return NotifyResult::eRejected;
}

auto ViewportTreeController::getInputHandler() -> InputFrame&
{
    return rootHandler.getRootFrame();
}
