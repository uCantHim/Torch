#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "ShaderCodeSource.h"
#include "LayoutQualifier.h"

namespace shader_edit
{
    /**
     * @brief
     */
    struct Version
    {
        int versionNumber;
    };

    struct Extension
    {
        std::string name;
        bool required;
    };

    struct ShaderInput
    {
        std::unordered_map<std::string, LayoutQualifier> layoutQualifiers;
        std::unique_ptr<ShaderCodeSource> code;
    };
} // namespace shader_edit
