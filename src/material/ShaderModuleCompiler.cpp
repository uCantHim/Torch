#include "trc/material/ShaderModuleCompiler.h"

#include "trc/material/ShaderCodeCompiler.h"
#include "trc/util/TorchDirectories.h"



namespace trc
{

ShaderModule::ShaderModule(
    std::string shaderCode,
    ShaderResources resourceInfo)
    :
    ShaderResources(std::move(resourceInfo)),
    shaderGlslCode(std::move(shaderCode))
{
}

auto ShaderModule::getGlslCode() const -> const std::string&
{
    return shaderGlslCode;
}



/**
 * The resource resolver used internally by the shader module compiler.
 *
 * Builds a ShaderResources object from capability/resource queries.
 */
class CapabilityConfigResourceResolver : public ResourceResolver
{
public:
    CapabilityConfigResourceResolver(
        const ShaderCapabilityConfig& conf,
        ShaderCodeBuilder& builder)
        :
        resources(conf, builder)
    {}

    auto resolveCapabilityAccess(Capability cap) -> code::Value override {
        return resources.queryCapability(cap);
    }

    auto resolveRuntimeConstantAccess(s_ptr<ShaderRuntimeConstant> c) -> code::Value override
    {
        auto [it, success] = existingRuntimeConstants.try_emplace(c);
        if (success)
        {
            auto value = resources.makeSpecConstant(c);
            it->second = value;
            return value;
        }

        return it->second;
    }

    auto buildResources() -> ShaderResources {
        return resources.compile();
    }

private:
    ShaderResourceInterface resources;

    // Used to de-duplicate creations of runtime constants.
    std::unordered_map<s_ptr<ShaderRuntimeConstant>, code::Value> existingRuntimeConstants;
};



auto ShaderModuleCompiler::compile(
    const ShaderOutputInterface& outputs,
    ShaderModuleBuilder builder,
    const ShaderCapabilityConfig& caps)
    -> ShaderModule
{
    // Create and build the main function
    auto main = builder.makeOrGetFunction("main", FunctionType{ {}, std::nullopt });
    builder.startBlock(main);
    outputs.buildStatements(builder);
    builder.endBlock();

    // Generate resource and function declarations
    auto resolver = std::make_unique<CapabilityConfigResourceResolver>(caps, builder);
    spirv::FileIncluder includer(
        util::getInternalShaderBinaryDirectory(),
        { util::getInternalShaderStorageDirectory() }
    );

    auto includedCode = builder.compileIncludedCode(includer, *resolver);
    auto typeDeclCode = builder.compileTypeDecls();
    auto functionDeclCode = builder.compileFunctionDecls(*resolver);  // Compiles all code
    auto resources = resolver->buildResources();  // Finalizes definitions of resources
                                                  // required by the compiled shader code

    // Build shader file
    std::stringstream ss;
    ss << "#version 460\n";

    // Write additional module settings
    ss << compileSettings(builder.getSettings()) << "\n";

    // Write type definitions
    ss << std::move(typeDeclCode) << "\n";

    // Write resources
    ss << resources.getGlslCode() << "\n";
    ss << builder.compileOutputLocations() << "\n";

    // Write additional includes
    ss << includedCode << "\n";

    // Write function definitions
    // This also writes the main function.
    ss << std::move(functionDeclCode);

    return { ss.str(), resources };
}

auto ShaderModuleCompiler::compileSettings(const ShaderModuleBuilder::Settings& settings)
    -> std::string
{
    std::string result;
    if (settings.earlyFragmentTests) {
        result += "layout (early_fragment_tests) in;\n";
    }

    return result;
}

} // namespace trc
