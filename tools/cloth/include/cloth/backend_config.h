#pragma once

#include <memory>

#include <trc/material/MaterialShaderImpl.h>

#include "full_id.h"

namespace cloth
{
    class BuiltinProvider;

    class BackendConfig
    {
    public:
        virtual ~BackendConfig() noexcept = default;

        virtual auto getBuiltins() -> BuiltinProvider& = 0;
        virtual auto outputBuiltinToParameter(const FullId& out)
            -> std::optional<trc::MaterialShaderImpl::OutputParameter> = 0;

        virtual auto makeCapabilityConfig() -> std::unique_ptr<trc::shader::CapabilityConfig> = 0;
        virtual auto makeOutputConfig() -> std::unique_ptr<trc::MaterialShaderImpl> = 0;
    };
} // namespace cloth
