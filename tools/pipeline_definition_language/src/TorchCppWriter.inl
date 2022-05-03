#include "TorchCppWriter.h"

#include "Util.h"
#include "PipelineDataWriter.h"



template<typename T>
auto TorchCppWriter::makeGroupInfo(const VariantGroup<T>& group) -> VariantGroupRepr
{
    std::string flagTypeName = group.baseName + "TypeFlags";

    std::stringstream ss;
    ss << "using " << flagTypeName << " = FlagCombination<";
    ++nl;
    for (size_t type : group.flagTypes) {
        ss << nl << makeFlagBitsType(flagTable->getFlagType(type)) << ",";
    }
    ss.seekp(-1, std::ios_base::end);  // Remove trailing ',' character
    ss << --nl << ">;";

    return {
        .combinedFlagType=std::move(flagTypeName),
        .usingDecl=ss.str(),
        .storageName=group.baseName + "Storage",
    };
}

template<typename T>
void TorchCppWriter::writeSingle(const std::string& name, const T& value, std::ostream& os)
{
    writeSingleStorageInit(name, value, os);
    os << nl;
    writeGetterFunctionHead<T>(name, os);
    os << nl << "{"
       << ++nl << "return " << name << ";"
       << --nl << "}";
}

template<typename T>
void TorchCppWriter::writeGroup(const VariantGroup<T>& group, std::ostream& os)
{
    auto groupInfo = makeGroupInfo(group);

    os << groupInfo.usingDecl << nl;
    os << "std::array<" << makeStoredType<T>() << ", " << groupInfo.combinedFlagType << "::size()> "
       << groupInfo.storageName << "{";
    ++nl;
    for (const auto& [name, variant] : group.variants)
    {
        os << nl;
        writeVariantStorageInit(name, variant, os);
        os << ",";
    }
    os << --nl << "};" << nl;

    writeGetterFunction(group, os);
}

template<typename T>
void TorchCppWriter::writeGetterFunctionHead(const std::string& name, std::ostream& os)
{
    os << "auto " << makeGetterFunctionName(name) << "() -> const " << makeStoredType<T>() << "&";
}

template<typename T>
void TorchCppWriter::writeGetterFunctionHead(const VariantGroup<T>& group, std::ostream& os)
{
    VariantGroupRepr groupInfo = makeGroupInfo(group);

    os << "auto " << makeGetterFunctionName(group.baseName) << "("
       << "const " << groupInfo.combinedFlagType << "& flags"
       << ") -> " << makeStoredType<T>() << "&";
}

template<typename T>
void TorchCppWriter::writeGetterFunction(const VariantGroup<T>& group, std::ostream& os)
{
    VariantGroupRepr groupInfo = makeGroupInfo(group);

    writeGetterFunctionHead(group, os);
    os << nl << "{" << ++nl
       << "return " << groupInfo.storageName << "[flags.toIndex()];"
       << --nl << "}" << nl;
}

template<typename T>
auto TorchCppWriter::makeValue(const ObjectReference<T>& ref) -> std::string
{
    return std::visit(VariantVisitor{
        [&](const UniqueName& name) { return makeReferenceCall(name); },
        [&](const T& value) { return makeValue(value); },
    }, ref);
}

template<typename T>
void TorchCppWriter::writeSingleStorageInit(
    const std::string& name,
    const T& value,
    std::ostream& os)
{
    os << makeStoredType<T>() << " " << name << " = " << makeValue(value) << ";";
}



//////////////////////////
//  Shader type writer  //
//////////////////////////

template<typename T>
auto TorchCppWriter::makeStoredType() -> std::string
{
    if constexpr (std::same_as<T, ShaderDesc>) {
        return "fs::path";
    }
    else if constexpr (std::same_as<T, ProgramDesc>) {
        return "ProgramDefinitionData";
    }
    else if constexpr (std::same_as<T, PipelineDesc>) {
        return "PipelineTemplate";
    }
}

template<>
inline auto TorchCppWriter::makeValue(const ShaderDesc& shader) -> std::string
{
    return "\"" + shader.source + ".spv\"";
}

template<>
inline void TorchCppWriter::writeVariantStorageInit(
    const UniqueName& name,
    const ShaderDesc& shader,
    std::ostream& os)
{
    const std::string& shaderFile = name.getUniqueName();

    os << makeStoredType<ShaderDesc>() << "{ " << "\"" << shaderFile << ".spv\"" << " }";

    auto code = compileShader(shader);
    auto outFile = openOutputFile(shaderFile);
    outFile << code;
}



///////////////////////////
//  Program type writer  //
///////////////////////////

template<>
inline auto TorchCppWriter::makeValue(const ProgramDesc& program) -> std::string
{
    std::stringstream ss;
    auto writeStage = [this, &ss](const char* stage, const ObjectReference<ShaderDesc>& ref) {
        ss << nl << "{ vk::ShaderStageFlagBits::e" << stage << ", { loadShader(";
        std::visit(VariantVisitor{
            [&](const UniqueName& name) {
                ss << makeReferenceCall(name);
            },
            [&](const ShaderDesc& shader) {
                ss << makeValue(shader);
            },
        }, ref);
        ss << "), {} },";
    };

    ss << "ProgramDefinitionData{" << ++nl
       << ".stages={";
    ++nl;
    if (program.vert.has_value()) {
        writeStage("Vertex", program.vert.value());
    }
    if (program.tesc.has_value()) {
        writeStage("TessellationControl", program.tesc.value());
    }
    if (program.tese.has_value()) {
        writeStage("TessellationEvaluation", program.tese.value());
    }
    if (program.geom.has_value()) {
        writeStage("Geometry", program.geom.value());
    }
    if (program.frag.has_value()) {
        writeStage("Fragment", program.frag.value());
    }
    ss << --nl << "}" << --nl << "}";

    return ss.str();
}

template<>
inline void TorchCppWriter::writeVariantStorageInit(
    const UniqueName&,
    const ProgramDesc& program,
    std::ostream& os)
{
    os << makeValue(program);
}



////////////////////////////
//  Pipeline type writer  //
////////////////////////////

template<>
inline auto TorchCppWriter::makeValue(const PipelineDesc& pipeline) -> std::string
{
    std::stringstream ss;
    ss << makeStoredType<PipelineDesc>() << "("
       << ++nl << makeValue(pipeline.program) << ","
       << nl << makePipelineDefinitionDataInit(pipeline, nl)
       << --nl << ")";

    return ss.str();
}

template<>
inline void TorchCppWriter::writeVariantStorageInit(
    const UniqueName&,
    const PipelineDesc& pipeline,
    std::ostream& os)
{
    os << makeValue(pipeline);
}
