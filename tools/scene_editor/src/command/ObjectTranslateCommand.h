#pragma once

#include "input/Command.h"

class App;

class ObjectTranslateCommand : public Command
{
public:
    explicit ObjectTranslateCommand(App& app);

    void execute(CommandExecutionContext& ctx) override;

private:
    App* app;
};
