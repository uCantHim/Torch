#pragma once

#include "input/Command.h"

class ObjectTranslateCommand : public Command
{
public:
    void execute(CommandExecutionContext& ctx) override;
};
