#include "TorchCppWriter.h"



TorchCppWriter::TorchCppWriter(const TorchCppWriterCreateInfo&)
{
}

void TorchCppWriter::write(const CompileResult& result, std::ostream& os)
{
    writeFlags(result, os);
}

void TorchCppWriter::writeFlags(const CompileResult& result, std::ostream& os)
{
    for (const auto& flag : result.flagTypes)
    {
        os << "enum class " << flag.flagName << "FlagBits\n"
            << "{\n";
        for (const auto& bit : flag.flagBits)
        {
            os << "    " << bit << ",\n";
        }
        os << "};\n";
    }
}
