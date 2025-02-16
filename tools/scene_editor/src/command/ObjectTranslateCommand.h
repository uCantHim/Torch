#pragma once

#include <trc/Types.h>
using namespace trc::basic_types;

#include "input/Command.h"

class Scene;

class ObjectTranslateCommand : public Command
{
public:
    explicit ObjectTranslateCommand(s_ptr<Scene> scene);

    void execute(CommandExecutionContext& ctx) override;

private:
    s_ptr<Scene> scene;
};
