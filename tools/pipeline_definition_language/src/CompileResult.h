#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

#include "FlagTable.h"

struct UniqueName
{
public:
    UniqueName(const UniqueName&) = default;
    UniqueName(UniqueName&&) noexcept = default;
    auto operator=(const UniqueName&) -> UniqueName& = default;
    auto operator=(UniqueName&&) noexcept -> UniqueName& = default;
    ~UniqueName() = default;

    UniqueName(std::string str);
    UniqueName(std::string str, std::vector<VariantFlag> flags);

    bool operator==(const UniqueName&) const = default;

    auto hash() const -> size_t;

    bool hasFlags() const;
    auto getFlags() const -> const std::vector<VariantFlag>&;
    auto getBaseName() const -> const std::string&;
    auto getUniqueName() const -> const std::string&;

private:
    std::string name;
    std::vector<VariantFlag> flags;

    std::string uniqueName;
};

namespace std
{
    template<>
    struct hash<UniqueName>
    {
        auto operator()(const UniqueName& name) const -> size_t {
            return name.hash();
        }
    };
}

struct ShaderDesc
{
    std::string source;
    std::unordered_map<std::string, std::string> variables;
};

struct PipelineDesc
{
    struct Program
    {
        std::optional<UniqueName> vert;
        std::optional<UniqueName> geom;
        std::optional<UniqueName> tese;
        std::optional<UniqueName> tesc;
        std::optional<UniqueName> frag;
    };
};

template<typename T>
struct VariantGroup
{
    std::vector<size_t> flagTypes;
    std::string baseName;
    std::unordered_map<UniqueName, T> variants;
};

struct CompileResult
{
    template<typename T>
    using SingleOrVariant = std::variant<T, VariantGroup<T>>;

    FlagTable flagTable;

    std::unordered_map<std::string, SingleOrVariant<ShaderDesc>> shaders;
    std::unordered_map<std::string, SingleOrVariant<PipelineDesc>> pipelines;
};
