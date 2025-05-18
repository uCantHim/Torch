#pragma once

#include <cloth/cloth.h>
#include <trc/material/shader/CapabilityConfig.h>
#include <trc/material/shader/ShaderOutputInterface.h>

class BackendConfig
{
public:
    virtual ~BackendConfig() noexcept = default;

    virtual auto makeCapabilityConfig() -> trc::shader::CapabilityConfig = 0;
    virtual auto makeOutputConfig() -> std::unique_ptr<cloth::ShaderOutputImpl> = 0;
};
