#pragma once

#include <nlohmann/json.hpp>

#include "trc_util/Exception.h"
#include "CompileConfiguration.h"

namespace shader_edit
{
    namespace nl = nlohmann;

    class ParseError : public trc::Exception
    {
    public:
        ParseError() = default;
        ParseError(std::string str) : trc::Exception(str) {}
    };

    /**
     * @brief
     *
     * @param const nl::json& json
     *
     * @return CompileConfiguration
     */
    auto parseConfig(const nl::json& json) -> CompileConfiguration;

    /**
     * @brief
     *
     * @param std::istream& is
     *
     * @return CompileConfiguration
     */
    auto parseConfigJson(std::istream& is) -> CompileConfiguration;
} // namespace shader_edit
