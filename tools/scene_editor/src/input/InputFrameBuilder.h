#pragma once

#include <cassert>
#include <concepts>

#include "input/InputStructs.h"

class CommandExecutionContext;
class InputFrame;

/**
 * @brief Typed interface to set up input frames with reference to a state object.
 */
template<std::derived_from<InputFrame> Frame>
struct InputFrameBuilder
{
    auto getFrame() -> Frame& {
        return *frame;
    }

    template<std::invocable<Frame&> F>
    void on(UserInput input, F&& callback)
    {
        frame->on(input, [frame=this->frame, cb=std::forward<F>(callback)](auto&){
            std::invoke(cb, *frame);
        });
    }

    template<std::invocable<Frame&, CommandExecutionContext&> F>
    void on(UserInput input, F&& callback)
    {
        frame->on(input, [frame=this->frame, cb=std::forward<F>(callback)](auto& ctx){
            std::invoke(cb, *frame, ctx);
        });
    }

    template<std::invocable<Frame&, Scroll> F>
    void onScroll(F&& callback)
    {
        frame->onScroll(
            [frame=this->frame, cb=std::forward<F>(callback)]
            (auto&, auto&& scroll) {
                std::invoke(cb, *frame, scroll);
            }
        );
    }

    template<std::invocable<Frame&, CommandExecutionContext&, Scroll> F>
    void onScroll(F&& callback)
    {
        frame->onScroll(
            [frame=this->frame, cb=std::forward<F>(callback)]
            (auto& ctx, auto&& scroll) {
                std::invoke(cb, *frame, ctx, scroll);
            }
        );
    }

    template<std::invocable<Frame&, CursorMovement> F>
    void onCursorMove(F&& callback)
    {
        frame->onCursorMove(
            [frame=this->frame, cb=std::forward<F>(callback)]
            (auto&, auto&& cursorMove) {
                std::invoke(cb, *frame, cursorMove);
            }
        );
    }

    template<std::invocable<Frame&, CommandExecutionContext&, CursorMovement> F>
    void onCursorMove(F&& callback)
    {
        frame->onCursorMove(
            [frame=this->frame, cb=std::forward<F>(callback)]
            (auto& ctx, auto&& cursorMove) {
                std::invoke(cb, *frame, ctx, cursorMove);
            }
        );
    }

protected:
    friend CommandExecutionContext;
    explicit InputFrameBuilder(Frame* frame) : frame(frame) {
        assert(frame != nullptr);
    }

private:
    Frame* frame;
};
