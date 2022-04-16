#pragma once

#include <stdexcept>

class PipelineLanguageCompilerError : public std::exception
{
public:
    auto what() const noexcept -> const char* override {
        return "";
    }
};

class ParseError : public PipelineLanguageCompilerError
{
public:
};
