#pragma once

#include <concepts>
#include <memory>

class CommandExecutionContext;

class Command
{
public:
    Command(const Command&) = delete;
    Command(Command&&) noexcept = delete;
    Command& operator=(const Command&) = delete;
    Command& operator=(Command&&) noexcept = delete;

    Command() = default;
    virtual ~Command() noexcept = default;

    virtual void execute(CommandExecutionContext& ctx) = 0;
};

/**
 * @brief A function that can be invoked as a command.
 */
template<typename T>
concept CommandFunctionT = std::invocable<T, CommandExecutionContext&>;

/**
 * @brief Create a command object from a function.
 *
 * Convenience.
 */
template<CommandFunctionT F>
auto makeCommand(F&& func) -> std::unique_ptr<Command>
{
    struct FunctionalCommandImpl : Command
    {
        FunctionalCommandImpl(F&& f) : func(std::move(f)) {}
        void execute(CommandExecutionContext& ctx) override {
            func(ctx);
        };
        F func;
    };

    return std::make_unique<FunctionalCommandImpl>(std::forward<F>(func));
}

/**
 * @brief Create a command object from a function.
 *
 * Convenience.
 */
template<std::invocable F>
auto makeCommand(F&& func) -> std::unique_ptr<Command>
{
    struct FunctionalCommandImpl : Command
    {
        FunctionalCommandImpl(F&& f) : func(std::move(f)) {}
        void execute(CommandExecutionContext&) override {
            func();
        };
        F func;
    };

    return std::make_unique<FunctionalCommandImpl>(std::forward<F>(func));
}
