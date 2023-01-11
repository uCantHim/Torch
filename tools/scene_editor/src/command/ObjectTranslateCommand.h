#pragma once

#include "input/InputCommand.h"

class ObjectTranslateCommand : public InputCommand
{
public:
    explicit ObjectTranslateCommand(App& app);

    void execute(CommandCall& call) override;

private:
    App* app;
};
