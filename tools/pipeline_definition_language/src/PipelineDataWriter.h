#pragma once

#include <string>

struct PipelineDesc;
class LineWriter;

namespace util
{
    auto convertFormat(const std::string& str) -> std::string;
    auto getFormatByteSize(const std::string& str) -> size_t;
}

auto makePipelineDefinitionDataInit(const PipelineDesc& pipeline, LineWriter& nl) -> std::string;
