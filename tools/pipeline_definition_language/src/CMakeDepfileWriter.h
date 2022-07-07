#pragma once

#include "Writer.h"

class CMakeDepfileWriter : public Writer
{
public:
    CMakeDepfileWriter(const fs::path shaderInputDir, const fs::path& targetPath);

    void write(const CompileResult& result, std::ostream& os) override;
    void write(const CompileResult& result, std::ostream& os, std::ostream&) override;

private:
    const fs::path shaderInputDir;
    std::string target;
};
