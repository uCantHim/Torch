#pragma once

#include <trc/Types.h>
using namespace trc::basic_types;

#include "input/Command.h"

class Scene;

class ObjectScaleCommand : public Command
{
public:
    explicit ObjectScaleCommand(s_ptr<Scene> scene);

    void execute(CommandExecutionContext& ctx) override;

private:
    s_ptr<Scene> scene;
};
