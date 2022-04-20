#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <atomic>

#include "SyntaxElements.h"
#include "TypeConfiguration.h"
#include "CompileResult.h"

class Compiler
{
public:
    Compiler() = default;

    auto compile(const std::vector<Stmt>& statements) -> CompileResult;

private:
    auto getIdentifierID(const Identifier& id) -> uint;
};
