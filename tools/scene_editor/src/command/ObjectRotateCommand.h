#pragma once

#include "input/InputCommand.h"

class ObjectRotateCommand : public InputCommand
{
public:
    ObjectRotateCommand() = default;

    void execute(CommandCall& call) override;
};
