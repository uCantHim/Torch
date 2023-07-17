#include "trc/material/FragmentShader.h"



namespace trc
{

void FragmentModule::setParameter(Parameter param, code::Value value)
{
    const size_t index = static_cast<size_t>(param);
    parameters[index] = value;
}

auto FragmentModule::build(ShaderModuleBuilder builder, bool transparent) -> ShaderModule
{
    // Ensure that every required parameter exists and has a value
    fillDefaultValues(builder);

    // Cast the emissive value (bool) to float.
    auto emissiveParam = *parameters[static_cast<size_t>(Parameter::eEmissive)];
    setParameter(Parameter::eEmissive, builder.makeCast<float>(emissiveParam));

    if (!transparent)
    {
        static constexpr std::array<std::string_view, FragmentModule::kNumParams> paramAccessors{{
            "", "", "x", "y", "z", "w",
        }};

        auto storeOutput = [this, &builder](Parameter param, code::Value out) {
            const size_t index = static_cast<size_t>(param);
            assert(parameters[index].has_value());

            if (!paramAccessors[index].empty()) {
                out = builder.makeMemberAccess(out, std::string(paramAccessors[index]));
            }
            output.makeStore(out, *parameters[index]);
        };

        auto outNormal = builder.makeOutputLocation(0, vec3{});
        auto outAlbedo = builder.makeOutputLocation(1, vec4{});
        auto outMaterial = builder.makeOutputLocation(2, vec4{});

        storeOutput(Parameter::eColor, outAlbedo);
        storeOutput(Parameter::eNormal, outNormal);
        storeOutput(Parameter::eSpecularFactor, outMaterial);
        storeOutput(Parameter::eMetallicness, outMaterial);
        storeOutput(Parameter::eRoughness, outMaterial);
        storeOutput(Parameter::eEmissive, outMaterial);
    }
    else {
        auto getParam = [this](Parameter param) {
            assert(parameters[static_cast<size_t>(param)].has_value());
            return *parameters[static_cast<size_t>(param)];
        };

        builder.includeCode(util::Pathlet("material_utils/append_fragment.glsl"), {
            { "nextFragmentListIndex",   FragmentCapability::kNextFragmentListIndex },
            { "maxFragmentListIndex",    FragmentCapability::kMaxFragmentListIndex },
            { "fragmentListHeadPointer", FragmentCapability::kFragmentListHeadPointerImage },
            { "fragmentList",            FragmentCapability::kFragmentListBuffer },
        });
        builder.includeCode(util::Pathlet("material_utils/shadow.glsl"), {
            { "shadowMatrixBufferName", FragmentCapability::kShadowMatrices },
        });
        builder.includeCode(util::Pathlet("material_utils/lighting.glsl"), {
            { "lightBufferName", FragmentCapability::kLightBuffer },
        });

        auto color = getParam(Parameter::eColor);
        builder.annotateType(color, vec4{});

        auto alpha = builder.makeMemberAccess(color, "a");
        auto doLighting = builder.makeNot(builder.makeCast<bool>(getParam(Parameter::eEmissive)));
        auto isVisible = builder.makeGreaterThan(alpha, builder.makeConstant(0.0f));
        auto cond = builder.makeAnd(isVisible, doLighting);

        color = builder.makeConditional(
            cond,
            // if true:
            builder.makeConstructor<vec4>(
                builder.makeExternalCall("calcLighting", {
                    builder.makeMemberAccess(color, "xyz"),
                    builder.makeCapabilityAccess(FragmentCapability::kVertexWorldPos),
                    getParam(Parameter::eNormal),
                    builder.makeCapabilityAccess(FragmentCapability::kCameraWorldPos),
                    builder.makeExternalCall("MaterialParams", {
                        getParam(Parameter::eSpecularFactor),
                        getParam(Parameter::eRoughness),
                        getParam(Parameter::eMetallicness),
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

    return ShaderModuleCompiler{}.compile(output, std::move(builder));
}

void FragmentModule::fillDefaultValues(ShaderModuleBuilder& builder)
{
    auto tryFill = [&](Parameter param, Constant constant) {
        const size_t index = static_cast<size_t>(param);
        if (!parameters[index]) {
            parameters[index] = builder.makeConstant(constant);
        }
    };

    tryFill(Parameter::eColor,          vec4(1.0f));
    tryFill(Parameter::eNormal,         vec3(0, 0, 1));
    tryFill(Parameter::eSpecularFactor, 1.0f);
    tryFill(Parameter::eMetallicness,   0.0f);
    tryFill(Parameter::eRoughness,      1.0f);
    tryFill(Parameter::eEmissive,       false);
}

} // namespace trc
