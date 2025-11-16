#include "trc/material/FragmentShader.h"

#include "trc/material/TorchMaterialSettings.h"
#include "trc/material/shader/ShaderModuleCompiler.h"



namespace trc
{

FragmentModule::FragmentModule(const FragmentModuleCreateInfo& createInfo)
    :
    config(createInfo)
{
}

auto FragmentModule::makeCapabilityConfig() -> u_ptr<shader::CapabilityConfig>
{
    return makeFragmentCapabilityConfig();
}

auto FragmentModule::makeOutputs(shader::ShaderModuleBuilder& builder)
    -> shader::ShaderOutputInterface
{
    shader::ShaderOutputInterface output;

    // Ensure that every required parameter exists and has a value
    fillDefaultValues(builder);

    // Cast the emissive value (bool) to float.
    auto emissiveParam = builder.makeCast<bool>(getParameterValue(Out::emissive));
    setParameter(Out::emissive, builder.makeCast<float>(emissiveParam));

    if (!config.transparent)
    {
        auto storeOutput = [&](OutputParameter param,
                               shader::code::Value out,
                               std::string_view accessor="")
        {
            if (!accessor.empty()) {
                out = builder.makeMemberAccess(out, std::string{accessor});
            }
            output.makeStore(out, getParameterValue(param));
        };

        auto outNormal = builder.makeOutputLocation(0, vec3{});
        auto outAlbedo = builder.makeOutputLocation(1, vec4{});
        auto outMaterial = builder.makeOutputLocation(2, vec4{});

        storeOutput(Out::color, outAlbedo);
        storeOutput(Out::normal, outNormal);
        storeOutput(Out::specularFactor, outMaterial, "x");
        storeOutput(Out::roughness,      outMaterial, "y");
        storeOutput(Out::metallicness,   outMaterial, "z");
        storeOutput(Out::emissive,       outMaterial, "w");
    }
    else {
        builder.includeCode("material_utils/append_fragment.glsl", {
            { "nextFragmentListIndex",   FragmentCapability::kNextFragmentListIndex },
            { "maxFragmentListIndex",    FragmentCapability::kMaxFragmentListIndex },
            { "fragmentListHeadPointer", FragmentCapability::kFragmentListHeadPointerImage },
            { "fragmentList",            FragmentCapability::kFragmentListBuffer },
        });
        builder.includeCode("material_utils/shadow.glsl", {
            { "shadowMatrixBufferName", FragmentCapability::kShadowMatrices },
        });
        builder.includeCode("material_utils/lighting.glsl", {
            { "lightBufferName", FragmentCapability::kLightBuffer },
        });

        auto color = getParameterValue(Out::color);
        builder.annotateType(color, vec4{});

        auto alpha = builder.makeMemberAccess(color, "a");
        auto doLighting = builder.makeNot(emissiveParam);
        auto isVisible = builder.makeGreaterThan(alpha, builder.makeConstant(0.0f));
        auto cond = builder.makeAnd(isVisible, doLighting);

        color = builder.makeConditional(
            cond,
            // if true:
            builder.makeConstructor<vec4>(
                builder.makeExternalCall("calcLighting", {
                    builder.makeMemberAccess(color, "xyz"),
                    builder.makeCapabilityAccess(MaterialCapability::kVertexWorldPos),
                    getParameterValue(Out::normal),
                    builder.makeCapabilityAccess(MaterialCapability::kCameraWorldPos),
                    builder.makeExternalCall("MaterialParams", {
                        getParameterValue(Out::specularFactor),
                        getParameterValue(Out::roughness),
                        getParameterValue(Out::metallicness),
                    })
                }),
                builder.makeMemberAccess(color, "a")
            ),
            // if false:
            color
        );

        // TODO: Ideally, we only call this if `isVisible` evaluates to true,
        // though we don't have a mechanism for conditional output yet.
        output.makeBuiltinCall("appendFragment", { color });
    }

    builder.enableEarlyFragmentTest();

    return output;
}

auto FragmentModule::build(shader::ShaderModuleBuilder builder)
    -> shader::ShaderModule
{
    auto outputs = makeOutputs(builder);
    return shader::ShaderModuleCompiler{}.compile(std::move(outputs), std::move(builder));
}

auto FragmentModule::buildClosesthitShader(shader::ShaderModuleBuilder builder)
    -> shader::ShaderModule
{
    shader::ShaderOutputInterface out;

    fillDefaultValues(builder);
    out.makeStore(
        builder.makeCapabilityAccess(RayHitCapability::kOutColor),
        builder.makeConditional(
            getParameterValue(Out::emissive),
            // Use albedo if the material is emissive
            getParameterValue(Out::color),
            // Calculate full lighting if not emissive
            builder.makeExternalCall(
                "calcLighting", {
                    getParameterValue(Out::color),
                    builder.makeCapabilityAccess(MaterialCapability::kVertexWorldPos),
                    getParameterValue(Out::normal),
                    builder.makeCapabilityAccess(MaterialCapability::kCameraWorldPos),
                    builder.makeExternalCall("MaterialParams", {
                        getParameterValue(Out::specularFactor),
                        getParameterValue(Out::roughness),
                        getParameterValue(Out::metallicness),
                    })
                }
            )
        )
    );

    return shader::ShaderModuleCompiler{}.compile(out, std::move(builder));
}

void FragmentModule::fillDefaultValues(shader::ShaderModuleBuilder& builder)
{
    auto tryFill = [&](OutputParameter param, shader::Constant constant) {
        if (!tryGetParameterValue(param)) {
            setParameter(param, builder.makeConstant(constant));
        }
    };

    tryFill(Out::color,          vec4(1.0f));
    tryFill(Out::normal,         vec3(0, 0, 1));
    tryFill(Out::specularFactor, 1.0f);
    tryFill(Out::metallicness,   0.0f);
    tryFill(Out::roughness,      1.0f);
    tryFill(Out::emissive,       false);
}

} // namespace trc
