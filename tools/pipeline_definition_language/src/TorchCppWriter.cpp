#include "TorchCppWriter.h"

#include <fstream>
#include <sstream>
#include <iostream>

#include <ShaderDocument.h>

#include "CompileResult.h"
#include "Exceptions.h"



auto capitalize(std::string str)
{
    if (!str.empty() && str[0] >= 'a' && str[0] <= 'z') {
        str[0] -= 32;
    }

    return str;
}



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



TorchCppWriter::TorchCppWriter(ErrorReporter& errorReporter, TorchCppWriterCreateInfo info)
    :
    config(std::move(info)),
    errorReporter(&errorReporter)
{
}

template<typename T>
void TorchCppWriter::write(const auto& map, std::ostream& os)
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

void TorchCppWriter::write(const CompileResult& result, std::ostream& os)
{
    flagTable = &result.flagTable;

    writeIncludes(os);
    writeBanner("Flag Types", os);
    writeFlags(result, os);

    writeBanner("Shaders", os);
    write<ShaderDesc>(result.shaders, os);
    writeBanner("Programs", os);
    write<ProgramDesc>(result.programs, os);
    writeBanner("Pipelines", os);
    write<PipelineDesc>(result.pipelines, os);
}

void TorchCppWriter::writeIncludes(std::ostream& os)
{
    os << "#include <array>\n"
       << "#include <filesystem>\n"
       << "#include <array>\n"
       << "namespace fs = std::filesystem;\n"
       << "\n"
       << "#include \"FlagCombination.h\"\n"
       << "using namespace trc;\n";
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

auto TorchCppWriter::openInputFile(const std::string& filename) -> std::ifstream
{
    fs::path path{ config.baseInputDir / filename };
    std::ifstream file{ path };
    if (!file.is_open()) {
        throw InternalLogicError("Unable to open file " + path.string() + " for reading");
    }

    return file;
}

auto TorchCppWriter::openOutputFile(const std::string& filename) -> std::ofstream
{
    fs::path path{ config.baseOutputDir / filename };
    std::ofstream file{ path };
    if (!file.is_open()) {
        throw InternalLogicError("Unable to open file " + path.string() + " for writing");
    }

    return file;
}

auto TorchCppWriter::compileShader(const ShaderDesc& shader) -> std::string
{
    std::ifstream source = openInputFile(shader.source);

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
    os << nl;
    for (const auto& flag : result.flagTable.makeFlagDescriptions())
    {
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
