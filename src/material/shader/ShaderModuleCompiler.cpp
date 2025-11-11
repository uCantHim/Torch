#include "trc/material/shader/ShaderModuleCompiler.h"

#include "trc/util/TorchDirectories.h"



namespace trc::shader
{

auto ShaderModuleCompiler::compile(
    const ShaderOutputInterface& outputs,
    ShaderModuleBuilder builder)
    -> ShaderModule
{
    // Create and build the main function
    auto main = builder.makeOrGetFunction("main", FunctionType{ {}, std::nullopt });
    builder.startBlock(main);
    outputs.buildStatements(builder);
    builder.endBlock();

    // Generate resource and function declarations
    spirv::FileIncluder includer{
        {
            util::getInternalShaderBinaryDirectory(),
            util::getInternalShaderStorageDirectory(),
        }
    };

    const auto includedCode = builder.compileIncludedCode(includer);
    const auto typeDeclCode = builder.compileTypeDecls();
    const auto functionDeclCode = builder.compileFunctionDecls(); // Compiles all code.

    // Build the shader file
    std::stringstream ss;

    // Write module settings and version
    ss << builder.compileSettings() << "\n";

    // Write type definitions
    ss << typeDeclCode << "\n";

    // Write resources
    ss << builder.compileInputResources() << "\n";
    ss << builder.compileOutputLocations() << "\n";

    // Write additional includes
    ss << includedCode << "\n";

    // Write function definitions
    // This also writes the main function.
    ss << functionDeclCode;

    return { shader_edit::ShaderDocument{ ss.str() }, builder.getResourceInterface() };
}

} // namespace trc::shader
