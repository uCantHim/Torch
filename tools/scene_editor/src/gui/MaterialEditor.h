#pragma once

#include <string>
#include <functional>

#include <trc/assets/Material.h>

#include "ImguiUtil.h"

namespace gui
{
    class MaterialEditorWindow
    {
    public:
        MaterialEditorWindow(trc::MaterialID material,
                             std::function<void(trc::MaterialID, trc::MaterialData)> onSave);

        bool operator()();

    private:
        bool windowOpen{ true };
        std::string title;
        std::function<void(trc::MaterialID, trc::MaterialData)> onSave;

        trc::MaterialID material;
        trc::MaterialData data;
    };
} // namespace gui
