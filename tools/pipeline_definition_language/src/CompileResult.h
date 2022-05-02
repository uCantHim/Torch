#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

#include "FlagTable.h"
#include "UniqueName.h"

/**
 * Either a reference of an inline object
 */
template<typename T>
using ObjectReference = std::variant<UniqueName, T>;

struct ShaderDesc
{
    std::string source;
    std::unordered_map<std::string, std::string> variables;
};

struct ProgramDesc
{
    std::optional<ObjectReference<ShaderDesc>> vert;
    std::optional<ObjectReference<ShaderDesc>> geom;
    std::optional<ObjectReference<ShaderDesc>> tese;
    std::optional<ObjectReference<ShaderDesc>> tesc;
    std::optional<ObjectReference<ShaderDesc>> frag;
};

struct PipelineDesc
{
    ObjectReference<ProgramDesc> program;
};

template<typename T>
struct VariantGroup
{
    explicit VariantGroup(std::string baseName) : baseName(std::move(baseName)) {}

    std::string baseName;
    std::vector<size_t> flagTypes;
    std::unordered_map<UniqueName, T> variants;
};

struct CompileResult
{
    template<typename T>
    using SingleOrVariant = std::variant<T, VariantGroup<T>>;

    FlagTable flagTable;

    std::unordered_map<std::string, SingleOrVariant<ShaderDesc>> shaders;
    std::unordered_map<std::string, SingleOrVariant<ProgramDesc>> programs;
    std::unordered_map<std::string, SingleOrVariant<PipelineDesc>> pipelines;
};
