#include "TorchCppWriter.h"

#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <unordered_set>

#include "CompileResult.h"
#include "Exceptions.h"
#include "shader_tools/ShaderDocument.h"



auto LineWriter::operator++() -> LineWriter&
{
    ++indent;
    return *this;
}

auto LineWriter::operator++(int) -> LineWriter
{
    auto res = *this;
    ++*this;
    return res;
}

auto LineWriter::operator--() -> LineWriter&
{
    if (indent > 0) --indent;
    return *this;
}

auto LineWriter::operator--(int) -> LineWriter
{
    auto res = *this;
    --*this;
    return res;
}

auto operator<<(std::ostream& os, const LineWriter& nl) -> std::ostream&
{
    os << "\n";
    for (size_t i = 0; i < nl.indent; i++) os << "    ";
    return os;
}



TorchCppWriter::TorchCppWriter(
    ErrorReporter& errorReporter,
    TorchCppWriterCreateInfo info)
    :
    config(std::move(info)),
    errorReporter(&errorReporter)
{
}

void TorchCppWriter::write(const CompileResult& result, std::ostream& os)
{
    write(result, os, os);
}

void TorchCppWriter::write(const CompileResult& result, std::ostream& header, std::ostream& source)
{
    meta = result.meta;
    flagTable = &result.flagTable;

    writeHeaderIncludes(header);
    writeSourceIncludes(source);

    writeHeader(result, header);
    writeSource(result, source);
}

void TorchCppWriter::writeHeader(const CompileResult& result, std::ostream& os)
{
    if (meta.enclosingNamespace.has_value()) {
        os << nl << "namespace " << meta.enclosingNamespace.value() << nl << "{";
    }

    // Write dynamic initialization stuff
    os << nl << nl;

    writeBanner("Flag Types", os);
    writeFlags(result, os);

    writeBanner("Shaders", os);
    writeHeader<ShaderDesc>(result.shaders, os);
    writeBanner("Programs", os);
    writeHeader<ProgramDesc>(result.programs, os);
    writeBanner("Layouts", os);
    writeHeader<LayoutDesc>(result.layouts, os);
    writeBanner("Pipelines", os);
    writeHeader<PipelineDesc>(result.pipelines, os);
    writeBanner("ComputePipelines", os);
    writeHeader<ComputePipelineDesc>(result.computePipelines, os);

    if (meta.enclosingNamespace.has_value()) {
        os << nl << "} // namespace " << meta.enclosingNamespace.value();
    }
}

void TorchCppWriter::writeSource(const CompileResult& result, std::ostream& os)
{
    os << nl;
    writeStaticData(os);
    os << nl;

    if (meta.enclosingNamespace.has_value()) {
        os << nl << "namespace " << meta.enclosingNamespace.value() << nl << "{";
    }

    writeBanner("Shaders", os);
    writeSource<ShaderDesc>(result.shaders, os);
    writeBanner("Programs", os);
    writeSource<ProgramDesc>(result.programs, os);
    writeBanner("Layouts", os);
    writeSource<LayoutDesc>(result.layouts, os);
    writeBanner("Pipelines", os);
    writeSource<PipelineDesc>(result.pipelines, os);
    writeBanner("ComputePipelines", os);
    writeSource<ComputePipelineDesc>(result.computePipelines, os);

    if (meta.enclosingNamespace.has_value()) {
        os << nl << "} // namespace " << meta.enclosingNamespace.value();
    }
}

template<typename T>
void TorchCppWriter::writeHeader(const auto& map, std::ostream& os)
{
    // Write group flag using declarations
    for (const auto& [name, value] : map)
    {
        if (std::holds_alternative<VariantGroup<T>>(value)) {
            os << nl << makeGroupFlagUsingDecl(std::get<VariantGroup<T>>(value));
        }
    }

    // Write getter function headers
    for (const auto& [name, value] : map)
    {
        os << nl;
        if (std::holds_alternative<VariantGroup<T>>(value)) {
            writeGetterFunctionHead(std::get<VariantGroup<T>>(value), os);
        }
        else {
            writeGetterFunctionHead<T>(name, os);
        }
        os << ";" << nl;
    }
}

template<typename T>
void TorchCppWriter::writeSource(const auto& map, std::ostream& os)
{
    for (const auto& [name, value] : map)
    {
        os << nl;
        if (std::holds_alternative<VariantGroup<T>>(value)) {
            writeGroup(std::get<VariantGroup<T>>(value), os);
        }
        else {
            writeSingle(name, std::get<T>(value), os);
        }
        os << nl;
    }
}

void TorchCppWriter::writeHeaderIncludes(std::ostream& os)
{
    os << "#include <filesystem>\n"
       << "namespace fs = std::filesystem;\n"
       << "\n"
       << "#include <trc/core/PipelineTemplate.h>\n"
       << "#include <trc/core/PipelineLayoutTemplate.h>\n"
       << "#include <trc/core/PipelineRegistry.h>\n"
       << "#include <trc/ShaderPath.h>\n"
       << "\n"
       << "#include \"trc/FlagCombination.h\"\n"
        ;
}

void TorchCppWriter::writeSourceIncludes(std::ostream& os)
{
    os << "#include <array>\n"
       << "\n"
       << "#include <trc/PipelineDefinitions.h>\n"
       << "#include <trc/ShaderLoader.h>\n"
        ;
}

void TorchCppWriter::writeBanner(const std::string& msg, std::ostream& os)
{
    const size_t borderSize = 2 + 3 + msg.size() + 3 + 2;
    os << nl << nl
       << std::string(borderSize, '/') << nl
       << "//   " << msg << "   //" << nl
       << std::string(borderSize, '/') << nl;
}

void TorchCppWriter::writeStaticData(std::ostream& os)
{
    os << "static trc::ShaderLoader shaderLoader("
       // Shader source (input) directories:
       << ++nl << "{ " << config.shaderInputDir << ", " << config.shaderOutputDir << " },"
       // Shader binary (output) directory:
       <<   nl << config.shaderOutputDir;

    // Optional path to a shader database
    if (config.shaderDatabasePath) {
        os << "," << nl << config.shaderDatabasePath.value();
    }

    os << --nl << ");";
}



template<>
struct std::hash<std::pair<std::string, std::string>>
{
    auto operator()(const pair<string, string>& pair) const -> size_t {
        return hash<string>{}(pair.first + pair.second);
    }
};

void TorchCppWriter::writeFlags(const CompileResult& result, std::ostream& os)
{
    // HACK: Hard-coded 'standard library' flag types that shall not be written
    // to the generated file.
    static std::unordered_set<std::string> hack{
        "Bool",
        "Format",
        "VertexInputRate", "PrimitiveTopology",
        "PolygonFillMode", "CullMode", "FaceWinding",
        "BlendFactor", "BlendOp", "ColorComponent",
        "ShaderTargetType",
        "DescriptorType",
    };

    os << nl;
    for (const auto& flag : result.flagTable.makeFlagDescriptions())
    {
        if (hack.contains(flag.flagName)) continue;

        os << "enum class " << makeFlagBitsType(flag.flagName) << "\n"
           << "{";
        nl++;
        for (const auto& bit : flag.flagBits)
        {
            os << nl << bit << " = " << flagTable->getRef(flag.flagName, bit).flagBitId << ",";
        }
        os << nl << nl << "eMaxEnum = " << flag.flagBits.size() << ",";
        os << --nl << "};" << nl;
    }
}

auto TorchCppWriter::makeFlagsType(const UniqueName& name) -> std::string
{
    return capitalize(name.getBaseName()) + "TypeFlags";
}

auto TorchCppWriter::makeGetterFunctionName(const std::string& name) -> std::string
{
    return "get" + capitalize(name);
}

auto TorchCppWriter::makeReferenceCall(const UniqueName& name) -> std::string
{
    if (!name.hasFlags()) {
        return makeGetterFunctionName(name.getBaseName()) + "()";
    }

    std::stringstream ss;
    ss << makeGetterFunctionName(name.getBaseName()) << "(";
    ss << makeFlagsType(name) << "{}";
    for (auto flag : name.getFlags())
    {
        auto [type, bit] = flagTable->getFlagBit(flag);
        ss << " | " << makeFlagBitsType(type) << "::" << bit;
    }
    ss << ")";

    return ss.str();
}

auto TorchCppWriter::makeFlagBitsType(const std::string& flagName) -> std::string
{
    return flagName + "FlagBits";
}
