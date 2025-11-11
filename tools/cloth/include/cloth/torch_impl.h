#pragma once

#include <trc/material/FragmentShader.h>
#include <trc/material/TorchMaterialSettings.h>
#include <trc/material/VertexShader.h>

#include "backend_config.h"
#include "builtins.h"

namespace trc
{
    class GlPosition : public shader::ShaderFunction
    {
    public:
        GlPosition()
            :
            ShaderFunction(
                "calcGlPosition",
                shader::FunctionType{ { vec3{} }, vec4{} }
            )
        {}

        void build(shader::ShaderModuleBuilder& builder, const std::vector<code::Value>& args) override
        {
            auto viewproj = builder.makeMul(
                builder.makeCapabilityAccess(VertexCapability::kProjMatrix),
                builder.makeCapabilityAccess(VertexCapability::kViewMatrix)
            );
            builder.makeReturn(builder.makeMul(
                viewproj,
                builder.makeConstructor<vec4>(args[0], builder.makeConstant(1.0f))
            ));
        }
    };

    class ApplyAnimation : public shader::ShaderFunction
    {
    public:
        ApplyAnimation()
            :
            ShaderFunction(
                "applyAnimationTransform",
                shader::FunctionType{ { vec4{} }, vec4{} }
            )
        {}

        void build(shader::ShaderModuleBuilder& builder, const std::vector<code::Value>& args) override
        {
            auto anim = builder.makeCapabilityAccess(VertexCapability::kAnimIndex);
            auto keyframes = builder.makeCapabilityAccess(VertexCapability::kAnimKeyframes);
            auto weight = builder.makeCapabilityAccess(VertexCapability::kAnimFrameWeight);
            auto res = builder.makeExternalCall("applyAnimation", { anim, args[0], keyframes, weight });
            builder.makeReturn(res);
        }
    };

    class NormalToWorldspace : public shader::ShaderFunction
    {
    public:
        NormalToWorldspace()
            :
            ShaderFunction("normalToWorldspace", shader::FunctionType{ { vec4{} }, vec3{} })
        {}

        void build(shader::ShaderModuleBuilder& builder, const std::vector<code::Value>& args) override
        {
            auto model = builder.makeCapabilityAccess(VertexCapability::kModelMatrix);
            auto tiModel = builder.makeExternalCall(
                "transpose",
                { builder.makeExternalCall("inverse", {model}) }
            );
            auto normal = builder.makeMemberAccess(builder.makeMul(tiModel, args[0]), "xyz");
            builder.makeReturn(builder.makeExternalCall("normalize", { normal }));
        }
    };
}

namespace cloth
{
    /**
     * Uses the deferred fragment shader implementation as the default backend.
     */
    class TorchImpl : public BackendConfig
    {
    public:
        auto makeBuilder() -> std::unique_ptr<trc::shader::ShaderModuleBuilder> override
        {
            return std::make_unique<trc::shader::ShaderModuleBuilder>(
                trc::makeFragmentCapabilityConfig()
            );
        }

        auto makeOutputConfig() -> std::unique_ptr<cloth::ShaderOutputImpl> override
        {
            return std::make_unique<DeferredFragmentShaderImpl>();
        }

        auto getBuiltins() -> BuiltinProvider& override
        {
            return builtins;
        }

        struct DeferredFragmentShaderImpl : cloth::ShaderOutputImpl
        {
            void setParameter(const std::string& outputName,
                              trc::shader::code::Value value) override
            {
                using Param = trc::FragmentModule::Parameter;
                static const std::unordered_map<std::string, Param> map{
                    { "color", Param::eColor },
                    { "normal", Param::eNormal },
                    { "specularFactor", Param::eSpecularFactor },
                    { "metallicness", Param::eMetallicness },
                    { "roughness", Param::eRoughness },
                    { "emissive", Param::eEmissive },
                };

                if (map.contains(outputName)) {
                    frag.setParameter(map.at(outputName), value);
                }
            }

            auto buildShaderOutputs(trc::shader::ShaderModuleBuilder& builder)
                -> trc::shader::ShaderOutputInterface override
            {
                const bool transparent = false;
                return frag.buildOutputs(builder, transparent);
            }

            trc::FragmentModule frag;
        };

        BuiltinProvider builtins = makeFragmentBuiltinProvider();
    };

    class TorchVertexShaderImpl : public BackendConfig
    {
    public:
        explicit TorchVertexShaderImpl(bool animated)
            : animated(animated)
        {}

        auto makeBuilder() -> std::unique_ptr<trc::shader::ShaderModuleBuilder> override
        {
            using namespace trc::basic_types;

            s_ptr caps = trc::VertexModule::makeCapabilityConfig();
            auto _builder = std::make_unique<trc::shader::ShaderModuleBuilder>(caps);
            auto& builder = *_builder;

            auto tbn = [&]() -> trc::code::Value {
                auto zero = builder.makeConstant(0.0f);

                auto normalObjspace = builder.makeCapabilityAccess(trc::VertexCapability::kNormal);
                auto tangentObjspace = builder.makeCapabilityAccess(trc::VertexCapability::kTangent);
                normalObjspace = builder.makeConstructor<vec4>(normalObjspace, zero);
                tangentObjspace = builder.makeConstructor<vec4>(tangentObjspace, zero);
                if (animated) {
                    normalObjspace = builder.makeCall<trc::ApplyAnimation>({ normalObjspace });
                    tangentObjspace = builder.makeCall<trc::ApplyAnimation>({ tangentObjspace });
                }

                auto normal = builder.makeCall<trc::NormalToWorldspace>({ normalObjspace });
                auto tangent = builder.makeCall<trc::NormalToWorldspace>({ tangentObjspace });
                auto bitangent = builder.makeExternalCall("cross", { normal, tangent });

                auto tbn = builder.makeConstructor<mat3>(tangent, bitangent, normal);

                return tbn;
            }();

            caps->linkCapability(
                trc::MaterialCapability::kVertexWorldPos,
                [&]() -> trc::code::Value
                {
                    auto objPos = builder.makeCapabilityAccess(trc::VertexCapability::kPosition);
                    auto modelMat = builder.makeCapabilityAccess(trc::VertexCapability::kModelMatrix);
                    auto objPos4 = builder.makeConstructor<glm::vec4>(objPos, builder.makeConstant(1.0f));
                    if (animated)
                    {
                        builder.includeCode("material_utils/animation.glsl", {
                            { "animationMetaDataDescriptorName", trc::VertexCapability::kAnimMetaBuffer },
                            { "animationDataDescriptorName", trc::VertexCapability::kAnimDataBuffer },
                            { "vertexBoneIndicesAttribName", trc::VertexCapability::kBoneIndices },
                            { "vertexBoneWeightsAttribName", trc::VertexCapability::kBoneWeights },
                        });
                        //objPos4 = builder.makeCall<ApplyAnimation>({ objPos4 });
                    }

                    auto worldPos = builder.makeMul(modelMat, objPos4);
                    return builder.makeMemberAccess(worldPos, "xyz");
                }(),
                {}
            );
            caps->linkCapability(
                trc::MaterialCapability::kTangentToWorldSpaceMatrix,
                tbn,
                {});
            caps->linkCapability(
                trc::MaterialCapability::kVertexUV,
                builder.makeCapabilityAccess(trc::VertexCapability::kUV),
                {});
            caps->linkCapability(
                trc::MaterialCapability::kVertexNormal,
                tbn,
                {});

            return _builder;
        }

        auto makeOutputConfig() -> std::unique_ptr<cloth::ShaderOutputImpl> override
        {
            return std::make_unique<ShaderImpl>();
        }

        auto getBuiltins() -> BuiltinProvider& override
        {
            return builtins;
        }

        struct ShaderImpl : cloth::ShaderOutputImpl
        {
            void setParameter(const std::string& outputName,
                              trc::shader::code::Value value) override
            {
                using Param = trc::FragmentModule::Parameter;
                static const std::unordered_map<std::string, Param> map{
                    { "position", Param::eColor },
                    { "normal", Param::eNormal },
                };

                std::cout << "-- Vertex shader impl: Setting parameter \"" << outputName << "\".\n";
                //if (map.contains(outputName)) {
                //    frag.setParameter(map.at(outputName), value);
                //}
            }

            auto buildShaderOutputs(trc::shader::ShaderModuleBuilder& builder)
                -> trc::shader::ShaderOutputInterface override
            {
                return vert.buildOutputs(builder, {});
            }

            trc::VertexModule vert{ true };
        };

        BuiltinProvider builtins = makeVertexBuiltinProvider();
        const bool animated;
    };
} // namespace cloth
