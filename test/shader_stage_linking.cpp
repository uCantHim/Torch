#include <trc/material/FragmentShader.h>
#include <trc/material/ShaderStageInputLinker.h>
#include <trc/material/VertexShader.h>
#include <trc/material/shader/ShaderTypeChecker.h>
#include <trc/material/shader/ShaderModuleCompiler.h>
#include <trc/material/shader/ShaderFunction.h>
#include <trc/material/shader/ShaderProgram.h>
#include <trc/Torch.h>
#include <trc/TorchRenderStages.h>
#include <trc/DrawablePipelines.h>

using namespace trc::basic_types;

class VertexModuleBuilder
{
public:
    static auto makeCapabilityConfig()
    {
        return trc::VertexModule::makeCapabilityConfig({ .animated=false });
    }
};

int main()
{
    // Create the vertex module
    trc::VertexModule vertStage{ { .animated=false } };
    trc::shader::ShaderModuleBuilder vertBuilder{ VertexModuleBuilder::makeCapabilityConfig() };
    auto vertOutputs = vertStage.makeOutputs(vertBuilder);

    // Create the fragment module
    trc::shader::ShaderModuleBuilder fragBuilder{ trc::makeFragmentCapabilityConfig() };

    trc::FragmentModule fragStage{ { .transparent=false } };
    fragStage.setParameter(
        trc::FragmentModule::Out::color,
        fragBuilder.makeConstructor<vec4>(
            fragBuilder.makeCapabilityAccess(trc::MaterialCapability::kVertexNormal),
            fragBuilder.makeConstant(1.0f)
        )
    );
    fragStage.setParameter(
        trc::FragmentModule::Out::roughness,
        fragBuilder.makeConstant(0.1f)
    );
    auto fragOutputs = fragStage.makeOutputs(fragBuilder);

    // Link the stages
    auto res = trc::linkShaderStageInputs({
        {
            vk::ShaderStageFlagBits::eVertex,
            trc::ModuleLinkInfo{ &vertBuilder, &vertBuilder.getResourceInterface(), &vertOutputs, }
        },
        {
            vk::ShaderStageFlagBits::eFragment,
            trc::ModuleLinkInfo{ nullptr, &fragBuilder.getResourceInterface(), nullptr, },
        },
    });
    if (!res)
    {
        std::cout << "Link error(s) occurred in the following stages:\n";
        for (const auto& [stage, inputs] : res.error().unresolvedInputs)
        {
            std::cout << " - " << vk::to_string(stage) << ":\n";
            for (const auto& input : inputs)
            {
                std::cout << "    Requested capability " << input.capability.getName()
                          << " is not provided by any prior shader stage.\n";
            }
        }
        return 1;
    }

    // Compile stages to modules
    auto fragModule = trc::shader::ShaderModuleCompiler{}.compile(fragOutputs, fragBuilder);
    auto vertModule = trc::shader::ShaderModuleCompiler{}.compile(vertOutputs, vertBuilder);

    // Link shader modules to a program
    auto programLinkResult = trc::shader::linkShaderProgram(
        {
            { vk::ShaderStageFlagBits::eVertex, vertModule },
            { vk::ShaderStageFlagBits::eFragment, fragModule },
        },
        { .inputLocationMapping=res->locationMap }
    );
    if (!programLinkResult)
    {
        std::cout << "Error when linking shader program.\n";
        return 1;
    }

    trc::shader::ShaderProgramData& program = *programLinkResult;
    // trc::shader::ShaderProgramRuntime _runtime{ program };
    assert(program.glslCode.size() == 2);
    assert(program.glslCode.contains(vk::ShaderStageFlagBits::eVertex));
    assert(program.glslCode.contains(vk::ShaderStageFlagBits::eFragment));

    std::cout << "+++ VERTEX SHADER +++\n\n"
              << program.glslCode.at(vk::ShaderStageFlagBits::eVertex)
              << "\n\n";
    std::cout << "+++ FRAGMENT SHADER +++\n\n"
              << program.glslCode.at(vk::ShaderStageFlagBits::eFragment)
              << "\n\n";

    auto [tmpl, rp] = trc::PipelineRegistry::cloneGraphicsPipeline(
        trc::pipelines::getDrawableBasePipeline(trc::pipelines::DrawableBasePipelineTypeFlags{})
    );
    //trc::PipelineDefinitionData pipeline;
    auto matProg = trc::makeMaterialProgram(program, tmpl.getPipelineData(), rp);
    if (!matProg)
    {
        std::cout << "Error when compiling shader program: " << matProg.error().what() << "\n";
        return 1;
    }

    trc::MaterialRuntime runtime{ program, **matProg };

    auto torch = trc::initFull();
    auto scene = std::make_shared<trc::Scene>();
    auto camera = std::make_shared<trc::Camera>();
    camera->makeOrthogonal(-1, 1, -1, 1, -1, 1);

    auto geo = torch->getAssetManager().create(trc::makePlaneGeo());
    //auto plane = scene->makeDrawable({
    //    .geo = torch->getAssetManager().create(trc::makePlaneGeo()),
    //    .mat = torch->getAssetManager().create<trc::Material>(),
    //});

    auto vp = torch->makeFullscreenViewport(camera, scene);
    auto frame = torch->getRenderPipeline().makeFrame();
    frame->spawnTask(trc::stages::gBuffer, [&](vk::CommandBuffer cmdBuf, trc::DeviceExecutionContext& ctx) {
        //auto& p = ctx.resources().getPipeline(runtime.getPipeline());
        //p.bind(cmdBuf, ctx.resources());
        runtime.bind(cmdBuf, ctx);

        auto hnd = geo.getDeviceDataHandle();
        hnd.bindVertices(cmdBuf, 0);
        cmdBuf.drawIndexed(hnd.getIndexCount(), 1, 0, 0, 0);
    });

    torch->drawFrame(std::move(frame));

    while (torch->getWindow().isOpen())
    {
        trc::pollEvents();
    }
    trc::terminate();

    return 0;
}
