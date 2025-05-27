#include "trc/material/shader/ShaderModuleCompiler.h"

#include "trc/material/shader/DefaultResourceResolver.h"
#include "trc/material/shader/ShaderCodeCompiler.h"
#include "trc/util/TorchDirectories.h"



namespace trc::shader
{

auto ShaderModuleCompiler::compile(
    const ShaderOutputInterface& outputs,
    ShaderModuleBuilder builder,
    const CapabilityConfig& caps)
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
    ShaderResourceInterfaceBuilder resourceBuilder{ caps, builder };
    CapabilityConfigResourceResolver resolver{ resourceBuilder };

    const auto includedCode = builder.compileIncludedCode(includer, resolver);
    const auto typeDeclCode = builder.compileTypeDecls();
    const auto functionDeclCode = builder.compileFunctionDecls(resolver); // Compiles all code.
    const auto resources = resourceBuilder.compile();

    // Build the shader file
    std::stringstream ss;

    // Write module settings and version
    ss << builder.compileSettings() << "\n";

    // Write type definitions
    ss << typeDeclCode << "\n";

    // Write resources
    ss << resources.getGlslCode() << "\n";
    ss << builder.compileOutputLocations() << "\n";

    // Write additional includes
    ss << includedCode << "\n";

    // Write function definitions
    // This also writes the main function.
    ss << functionDeclCode;

    return { shader_edit::ShaderDocument{ ss.str() }, resources };
}

} // namespace trc::shader
