#pragma once

#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;

#include "Writer.h"

class ErrorReporter;

struct LineWriter
{
    auto operator++() -> LineWriter&;
    auto operator++(int) -> LineWriter;
    auto operator--() -> LineWriter&;
    auto operator--(int) -> LineWriter;

    size_t indent{ 0 };
};

auto operator<<(std::ostream& os, const LineWriter& nl) -> std::ostream&;

struct TorchCppWriterCreateInfo
{
    fs::path baseInputDir{ "." };
    fs::path baseOutputDir{ "." };

    std::optional<std::string> enclosingNamespace{ "test" };
};

class TorchCppWriter : public Writer
{
public:
    explicit TorchCppWriter(ErrorReporter& errorReporter, TorchCppWriterCreateInfo info = {});

    void write(const CompileResult& result, std::ostream& os) override;
    void write(const CompileResult& result, std::ostream& header, std::ostream& src) override;

private:
    struct VariantGroupRepr
    {
        std::string combinedFlagType;
        std::string storageName;
    };

    struct TypeRepr
    {
        std::string typeName;
        std::string initializer;
    };

    void writeHeader(const CompileResult& result, std::ostream& os);
    void writeSource(const CompileResult& result, std::ostream& os);
    template<typename T>
    void writeHeader(const auto& map, std::ostream& os);
    template<typename T>
    void writeSource(const auto& map, std::ostream& os);

    static void writeHeaderIncludes(std::ostream& os);
    static void writeSourceIncludes(std::ostream& os);
    void writeBanner(const std::string& msg, std::ostream& os);

    void error(std::string message);

    auto openInputFile(const std::string& filename) -> std::ifstream;
    auto openOutputFile(const std::string& filename) -> std::ofstream;
    auto compileShader(const ShaderDesc& shader) -> std::string;

    template<typename T>
    auto makeGroupInfo(const VariantGroup<T>& group) -> VariantGroupRepr;
    template<typename T>
    auto makeGroupFlagUsingDecl(const VariantGroup<T>& group) -> std::string;
    template<typename T>
    auto makeFlagsType(const VariantGroup<T>& group) -> std::string;
    auto makeFlagBitsType(const std::string& flagName) -> std::string;

    auto makeGetterFunctionName(const std::string& name) -> std::string;
    auto makeReferenceCall(const UniqueName& name) -> std::string;

    void writeFlags(const CompileResult& result, std::ostream& os);
    template<typename T>
    void writeSingle(const std::string& name, const T& value, std::ostream& os);
    template<typename T>
    void writeGroup(const VariantGroup<T>& group, std::ostream& os);
    template<typename T>
    void writeGetterFunctionHead(const std::string& name, std::ostream& os);
    template<typename T>
    void writeGetterFunctionHead(const VariantGroup<T>& group, std::ostream& os);
    template<typename T>
    void writeGetterFunction(const VariantGroup<T>& group, std::ostream& os);


    //////////////////////////////////////////
    //  Implementations for specific types  //
    //////////////////////////////////////////

    template<typename T>
    auto makeValue(const ObjectReference<T>& ref) -> std::string;
    template<typename T>
    void writeSingleStorageInit(const std::string& name, const T& value, std::ostream& os);

    template<typename T>
    auto makeStoredType() -> std::string;
    template<typename T>
    auto makeValue(const T& value) -> std::string;
    template<typename T>
    void writeVariantStorageInit(const UniqueName& name, const T& val, std::ostream& os);

    TorchCppWriterCreateInfo config;
    ErrorReporter* errorReporter;
    LineWriter nl;

    const FlagTable* flagTable;
};

#include "TorchCppWriter.inl"
