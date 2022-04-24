#include "TorchCppWriter.h"

#include "Util.h"



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
auto TorchCppWriter::makeStoredType() -> std::string
{
    if constexpr (std::same_as<T, ShaderDesc>) {
        return "fs::path";
    }
}

template<>
inline auto TorchCppWriter::makeValue(const ShaderDesc& shader) -> std::string
{
    return "\"" + shader.source + ".spv\"";
}

template<typename T>
void TorchCppWriter::writeSingle(const std::string& name, const T& value, std::ostream& os)
{
    os << makeStoredType<T>() << " " << name << " = " << makeValue(value) << ";";
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

template<>
inline void TorchCppWriter::writeVariantStorageInit(
    const UniqueName& name,
    const ShaderDesc& shader,
    std::ostream& os)
{
    const std::string shaderFile = name.getUniqueName();

    os << makeStoredType<ShaderDesc>() << "{ " << makeValue(shader) << " }";

    auto code = compileShader(shader);
    auto outFile = openOutputFile(shaderFile);
    outFile << code;
}

template<typename T>
void TorchCppWriter::writeGetterFunction(const VariantGroup<T>& group, std::ostream& os)
{
    VariantGroupRepr groupInfo = makeGroupInfo(group);

    os << "auto " << makeGetterFunctionName(group.baseName) << "("
       << "const " << groupInfo.combinedFlagType << "& flags"
       << ") -> " << makeStoredType<T>() << "&" << nl
       << "{" << ++nl
       << "return " << groupInfo.storageName << "[flags.toIndex()];"
       << --nl << "}" << nl;
}
