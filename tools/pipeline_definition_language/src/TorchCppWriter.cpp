#include "TorchCppWriter.h"

#include <fstream>
#include <sstream>
#include <iostream>

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
    --indent;
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

    writeBanner("Flag Types", os);
    writeFlags(result, os);

    writeBanner("Shaders", os);
    writeHeader<ShaderDesc>(result.shaders, os);
    writeBanner("Programs", os);
    writeHeader<ProgramDesc>(result.programs, os);
    writeBanner("Pipelines", os);
    writeHeader<PipelineDesc>(result.pipelines, os);

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
    writeBanner("Pipelines", os);
    writeSource<PipelineDesc>(result.pipelines, os);

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
       << "#include <trc/core/PipelineTemplate.h>"
       << "\n"
       << "#include \"FlagCombination.h\"\n"
        ;
}

void TorchCppWriter::writeSourceIncludes(std::ostream& os)
{
    os << "#include <array>\n"
       << "\n"
       << "#include <trc/PipelineDefinitions.h>"
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
    static std::unordered_set<std::string> hack{
        "Bool",
        "Format",
        "VertexInputRate", "PrimitiveTopology",
        "PolygonFillMode", "CullMode", "FaceWinding",
        "BlendFactor", "BlendOp", "ColorComponent",
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
    for (size_t i = 0; auto flag : name.getFlags())
    {
        auto [type, bit] = flagTable->getFlagBit(flag);
        ss << makeFlagBitsType(type) << "::" << bit;
        if (++i < name.getFlags().size()) ss << " | ";
    }
    ss << ")";

    return ss.str();
}

auto TorchCppWriter::makeFlagBitsType(const std::string& flagName) -> std::string
{
    return flagName + "FlagBits";
}
