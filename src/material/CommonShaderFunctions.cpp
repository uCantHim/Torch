#include "trc/material/CommonShaderFunctions.h"

#include "trc/material/FragmentShader.h"



namespace trc
{
    TangentToWorldspace::TangentToWorldspace()
        : ShaderFunction("TangentspaceToWorldspace", FunctionType{ { vec3{} }, vec3{} })
    {
    }

    void TangentToWorldspace::build(ShaderModuleBuilder& builder, std::vector<code::Value> args)
    {
        auto movedInterval = builder.makeSub(
            builder.makeMul(args[0], builder.makeConstant(2.0f)),
            builder.makeConstant(1.0f)
        );
        builder.makeReturn(
            builder.makeMul(
                builder.makeCapabilityAccess(FragmentCapability::kTangentToWorldSpaceMatrix),
                movedInterval
            )
        );
    }
} // namespace trc
