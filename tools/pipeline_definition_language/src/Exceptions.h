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

class CompilerError : PipelineLanguageCompilerError
{
public:
};

class IOError : PipelineLanguageCompilerError
{
public:
};

class InternalLogicError : public PipelineLanguageCompilerError
{
public:
    InternalLogicError(std::string message) : message(std::move(message)) {}

    std::string message;
};
