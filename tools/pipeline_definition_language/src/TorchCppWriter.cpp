#include "TorchCppWriter.h"

#include <fstream>
#include <iostream>
#include <set>
#include <sstream>

#include <ShaderDocument.h>

#include "CompileResult.h"
#include "Exceptions.h"



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
    writeDynamicInitCreateInfoStruct(result, os);
    os << nl << nl;
    writeDynamicInitFunctionHead(os);
    os << ";";

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

    // Write at the end when all init callback names have been collected
    os << nl << nl;
    writeDynamicInitFunctionDef(os);
    os << nl;

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
       << "\n"
       << "#include \"FlagCombination.h\"\n"
        ;
}

void TorchCppWriter::writeSourceIncludes(std::ostream& os)
{
    os << "#include <array>\n"
       << "\n"
       << "#include \"PipelineCompilerUtils.cpp\"\n"
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

namespace std
{
    template<>
    struct hash<pair<string, string>>
    {
        auto operator()(const pair<string, string>& pair) const -> size_t {
            return hash<string>{}(pair.first + pair.second);
        }
    };
}

auto TorchCppWriter::collectDynamicInitCreateInfoMembers(const CompileResult& result)
    -> std::set<std::pair<std::string, std::string>>
{
    std::set<std::pair<std::string, std::string>> members;

    /** Collect all push constant default values from a layout description */
    auto collectPushConstants = [&members](auto&& layout){
        for (const auto& [stage, pcs] : layout.pushConstantsPerStage)
        {
            for (const auto& pc : pcs)
            {
                if (pc.defaultValueName.has_value()) {
                    members.emplace("trc::PushConstantDefaultValue", pc.defaultValueName.value());
                }
            }
        }
    };

    // Collect push constants
    for (const auto& [_, layout] : result.layouts)
    {
        std::visit(VariantVisitor{
            [&](const LayoutDesc& layout){ collectPushConstants(layout); },
            [&](const VariantGroup<LayoutDesc>& group){
                for (const auto& [_, layout] : group.variants) {
                    collectPushConstants(layout);
                }
            },
        }, layout);
    }

    return members;
}

auto TorchCppWriter::makeDynamicInitCreateInfoName() const -> std::string
{
    return capitalize(config.compiledFileName) + "CreateInfo";
}

void TorchCppWriter::writeDynamicInitCreateInfoStruct(
    const CompileResult& result,
    std::ostream& os)
{
    const auto members = collectDynamicInitCreateInfoMembers(result);

    os << "struct " << makeDynamicInitCreateInfoName()
       << nl << "{";

    // Write constructor head
    os << (++nl)++ << makeDynamicInitCreateInfoName() << "(";
    for (const auto& [type, member] : members) {
        os << nl << "const " << type << "& _" << member << ",";
    }
    // Special member which is always present
    os << nl << "fs::path _shaderInputDir = \"" << config.shaderOutputDir.string() << "\""
       << ")" << nl << ":";

    // Write constructor initialization
    for (const auto& [_, member] : members) {
        os << nl << member << "(_" << member << "),";
    }
    os << nl << "shaderInputDir(std::move(_shaderInputDir))";  // Special member
    os << --nl << "{}" << nl;

    // Write member definitions
    for (const auto& [type, member] : members) {
        os << nl << type << " " << member << ";";
    }
    os << nl << "fs::path" << " " << "shaderInputDir" << ";";  // Special member
    os << --nl << "};";
}

void TorchCppWriter::writeDynamicInitFunctionHead(std::ostream& os)
{
    os << "void init" << capitalize(config.compiledFileName)
       << "(const " << makeDynamicInitCreateInfoName() << "& info)";
}

void TorchCppWriter::writeDynamicInitFunctionDef(std::ostream& os)
{
    writeDynamicInitFunctionHead(os);
    os << nl++ << "{";
    for (const auto& funcName : initFunctionNames) {
        os << nl << funcName << "(info);";
    }
    os << --nl << "}";
}

auto TorchCppWriter::getOutputType(const ShaderDesc& shader) -> ShaderOutputType
{
    if (shader.outputType) {
        return shader.outputType.value();
    }
    return config.defaultShaderOutput;
}

auto TorchCppWriter::getAdditionalFileExt(const ShaderDesc& shader) -> std::string
{
    switch (getOutputType(shader))
    {
    case ShaderOutputType::eGlsl:
        return "";
    case ShaderOutputType::eSpirv:
        return ".spv";
    }
    throw std::logic_error("Invalid enum value in switch");
}

auto TorchCppWriter::openShaderFile(const std::string& filename) -> std::ifstream
{
    fs::path path{ config.shaderInputDir / filename };
    std::ifstream file{ path };
    if (!file.is_open()) {
        throw IOError("Unable to open file " + path.string() + " for reading");
    }

    return file;
}

auto TorchCppWriter::compileShader(const ShaderDesc& shader) -> std::string
{
    std::ifstream source = openShaderFile(shader.source);

    shader_edit::ShaderDocument doc(source);
    for (const auto& [name, val] : shader.variables) {
        doc.set(name, val);
    }

    try {
        return doc.compile();
    }
    catch (const shader_edit::CompileError& err) {
        throw InternalLogicError(err.what());
    }
}

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
