#pragma once

#include <string>
#include <optional>

#include "Writer.h"

struct TorchCppWriterCreateInfo
{
    std::optional<std::string> enclosingNamespace;
};

class TorchCppWriter : public Writer
{
public:
    explicit TorchCppWriter(const TorchCppWriterCreateInfo& info = {});

    void write(const CompileResult& result, std::ostream& os) override;

private:
    void writeFlags(const CompileResult& result, std::ostream& os);
};
