#include "trc/material/FragmentShader.h"



namespace trc
{

constexpr std::array<BasicType, FragmentModule::kNumParams> paramTypes{{
    vec4{},
    vec3{},
    float{},
    float{},
    float{},
    bool{},
}};

void FragmentModule::setParameter(Parameter param, code::Value value)
{
    const size_t index = static_cast<size_t>(param);

    auto& p = parameters[index];
    if (!p) {
        p = output.addParameter(paramTypes[index]);
    }
    output.setParameter(*p, value);
}

auto FragmentModule::build(ShaderModuleBuilder builder, bool transparent) -> ShaderModule
{
    // Ensure that every required parameter exists and has a value
    fillDefaultValues(builder);

    // Cast the emissive value (bool) to float.
    auto emissiveParam = *parameters[static_cast<size_t>(Parameter::eEmissive)];
    output.setParameter(
        emissiveParam,
        builder.makeExternalCall("float", { output.getParameter(emissiveParam) })
    );

    if (!transparent)
    {
        static constexpr std::array<const char*, FragmentModule::kNumParams> paramAccessors{{
            "", "", "[0]", "[1]", "[2]", "[3]",
        }};

        auto linkOutput = [this](Parameter param, OutputID out) {
            const size_t index = static_cast<size_t>(param);
            assert(parameters[index].has_value());

            output.linkOutput(*parameters[index], out, paramAccessors[index]);
        };

        auto outNormal = output.addOutput(0, vec3{});
        auto outAlbedo = output.addOutput(1, vec4{});
        auto outMaterial = output.addOutput(2, vec4{});

        linkOutput(Parameter::eColor, outAlbedo);
        linkOutput(Parameter::eNormal, outNormal);
        linkOutput(Parameter::eSpecularFactor, outMaterial);
        linkOutput(Parameter::eMetallicness, outMaterial);
        linkOutput(Parameter::eRoughness, outMaterial);
        linkOutput(Parameter::eEmissive, outMaterial);
    }
    else {
        auto getParam = [this](Parameter param) {
            assert(parameters[static_cast<size_t>(param)].has_value());
            return output.getParameter(*parameters[static_cast<size_t>(param)]);
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

        {
            //auto ifBlock = builder.makeIfStatement(getParam(Parameter::eEmissive));
            //builder.startBlock(ifBlock);
            auto lightedColor = builder.makeExternalCall("calcLighting", {
                builder.makeMemberAccess(color, "xyz"),
                builder.makeCapabilityAccess(FragmentCapability::kVertexWorldPos),
                getParam(Parameter::eNormal),
                builder.makeCapabilityAccess(FragmentCapability::kCameraWorldPos),
                builder.makeExternalCall("MaterialParams", {
                    getParam(Parameter::eSpecularFactor),
                    getParam(Parameter::eRoughness),
                    getParam(Parameter::eMetallicness),
                })
            });
            builder.makeAssignment(builder.makeMemberAccess(color, "rgb"), lightedColor);
            //builder.endBlock();
        }

        builder.makeExternalCallStatement("appendFragment", { color });
    }

    builder.enableEarlyFragmentTest();

    return ShaderModuleCompiler{}.compile(output, std::move(builder));
}

void FragmentModule::fillDefaultValues(ShaderModuleBuilder& builder)
{
    auto tryFill = [&](Parameter param, Constant constant) {
        assert(constant.getType() == paramTypes[static_cast<size_t>(param)]);

        const size_t index = static_cast<size_t>(param);
        if (!parameters[index])
        {
            parameters[index] = output.addParameter(paramTypes[index]);
            output.setParameter(*parameters[index], builder.makeConstant(constant));
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
