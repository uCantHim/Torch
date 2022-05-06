#include "TorchCppWriter.h"

#include "Util.h"
#include "StringUtil.h"
#include "PipelineDataWriter.h"



template<typename T>
auto TorchCppWriter::makeGroupInfo(const VariantGroup<T>& group) -> VariantGroupRepr
{
    std::string flagTypeName = makeFlagsType(group);

    return {
        .combinedFlagType=std::move(flagTypeName),
        .storageName=group.baseName + "Storage",
    };
}

template<typename T>
auto TorchCppWriter::makeGroupFlagUsingDecl(const VariantGroup<T>& group) -> std::string
{
    std::stringstream ss;
    ss << "using " << makeFlagsType(group) << " = se::FlagCombination<";
    ++nl;
    for (size_t type : group.flagTypes) {
        ss << nl << makeFlagBitsType(flagTable->getFlagType(type)) << ",";
    }
    ss.seekp(-1, std::ios_base::end);  // Remove trailing ',' character
    ss << --nl << ">;";

    return ss.str();
}

template<typename T>
auto TorchCppWriter::makeFlagsType(const VariantGroup<T>& group) -> std::string
{
    return capitalize(group.baseName) + "TypeFlags";
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
    return "\"" + shader.target + (shader.isIncludeFile ? "" : ".spv") + "\"";
}

template<>
inline void TorchCppWriter::writeSingleStorageInit(
    const std::string& name,
    const ShaderDesc& shader,
    std::ostream& os)
{
    os << makeStoredType<ShaderDesc>() << " " << name << " = " << makeValue(shader) << ";";

    if (shader.isIncludeFile) {
        config.writePlainData(compileShader(shader), shader.target);
    }
    else {
        config.generateShader(compileShader(shader), shader.target);
    }
}

template<>
inline void TorchCppWriter::writeVariantStorageInit(
    const UniqueName& name,
    const ShaderDesc& shader,
    std::ostream& os)
{
    // Construct unique file path based on the source path
    fs::path outFilePath = shader.target;
    outFilePath.replace_extension(name.getUniqueExtension() + outFilePath.extension().string());

    if (shader.isIncludeFile)
    {
        os << makeStoredType<ShaderDesc>() << "{ " << "\"" << outFilePath << "\"" << " }";
        config.writePlainData(compileShader(shader), outFilePath);
    }
    else {
        os << makeStoredType<ShaderDesc>() << "{ " << "\"" << outFilePath << ".spv\"" << " }";
        config.generateShader(compileShader(shader), outFilePath);
    }
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
