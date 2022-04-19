#pragma once

#include <stdexcept>

#include "ErrorReporter.h"

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

class TypeError : public PipelineLanguageCompilerError
{
public:
    TypeError(Token token, std::string message)
        : token(std::move(token)), message(std::move(message))
    {}

    Token token;
    std::string message;
};
