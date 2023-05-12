#include "trc/assets/MaterialRegistry.h"

#include "material.pb.h"
#include "trc/DrawablePipelines.h"
#include "trc/assets/AssetManager.h"
#include "trc/drawable/DefaultDrawable.h"
#include "trc/material/TorchMaterialSettings.h"
#include "trc/ray_tracing/RayPipelineBuilder.h"



trc::AssetData<trc::Material>::AssetData(ShaderModule fragModule, bool transparent)
    :
    transparent(transparent)
{
    auto specialize = [this, &fragModule](const MaterialSpecializationInfo& info)
    {
        programs[MaterialKey{ info }] = linkMaterialProgram(
            makeMaterialSpecialization(fragModule, info),
            makeShaderDescriptorConfig()
        );
    };

    specialize(MaterialSpecializationInfo{ .animated=false });
    specialize(MaterialSpecializationInfo{ .animated=true });
}

void trc::AssetData<trc::Material>::serialize(std::ostream& os) const
{
    serial::Material mat;
    for (const auto& [key, program] : programs)
    {
        auto newSpec = mat.add_specializations();
        *newSpec->mutable_shader_program() = program.serialize();
        newSpec->set_animated(key.flags.has(MaterialKey::Flags::Animated::eTrue));
    }

    auto settings = mat.mutable_settings();
    settings->set_transparent(transparent);
    if (polygonMode) settings->set_polygon_mode(serial::PolygonMode(*polygonMode));
    if (lineWidth) settings->set_line_width(*lineWidth);
    if (cullMode)
    {
        serial::CullMode cm = serial::CullMode::NONE;
        if (*cullMode == vk::CullModeFlagBits::eFrontAndBack) cm = serial::CullMode::FRONT_AND_BACK;
        else if (*cullMode == vk::CullModeFlagBits::eFront) cm = serial::CullMode::FRONT;
        else if (*cullMode == vk::CullModeFlagBits::eBack) cm = serial::CullMode::BACK;
        settings->set_cull_mode(cm);
    }
    if (frontFace) settings->set_front_face_clockwise(*frontFace == vk::FrontFace::eClockwise);

    if (depthWrite) settings->set_depth_write(*depthWrite);
    if (depthTest) settings->set_depth_test(*depthTest);
    if (depthBiasConstantFactor) settings->set_depth_bias_constant_factor(*depthBiasConstantFactor);
    if (depthBiasSlopeFactor) settings->set_depth_bias_slope_factor(*depthBiasSlopeFactor);

    mat.SerializeToOstream(&os);
}

void trc::AssetData<trc::Material>::deserialize(std::istream& is)
{
    serial::Material mat;
    mat.ParseFromIstream(&is);

    // Parse settings
    const auto& settings = mat.settings();
    transparent = settings.transparent();
    if (settings.has_polygon_mode()) polygonMode = vk::PolygonMode(settings.polygon_mode());
    if (settings.has_line_width()) lineWidth = settings.line_width();
    if (settings.has_cull_mode()) cullMode = vk::CullModeFlags(settings.cull_mode());
    if (settings.has_front_face_clockwise())
    {
        frontFace = settings.front_face_clockwise() ? vk::FrontFace::eClockwise
                                                    : vk::FrontFace::eCounterClockwise;
    }
    if (settings.has_depth_write()) depthWrite = settings.depth_write();
    if (settings.has_depth_test()) depthTest = settings.depth_test();
    if (settings.has_depth_bias_constant_factor()) depthBiasConstantFactor = settings.depth_bias_constant_factor();
    if (settings.has_depth_bias_slope_factor()) depthBiasSlopeFactor = settings.depth_bias_slope_factor();

    // Parse specializations
    programs.clear();
    for (const auto& spec : mat.specializations())
    {
        MaterialProgramData program;
        program.deserialize(spec.shader_program(), *this);
        programs.try_emplace(
            MaterialSpecializationInfo{ .animated=spec.animated() },
            std::move(program)
        );
    }
}

auto trc::AssetData<trc::Material>::deserialize(const std::string& data)
    -> std::optional<s_ptr<ShaderRuntimeConstant>>
{
    try {
        // For now, we only have texture references as runtime constants.
        AssetReference<Texture> ref{ AssetPath(data) };
        textures.emplace_back(ref);
        return std::make_shared<RuntimeTextureIndex>(ref);
    }
    catch (const std::invalid_argument&) {
        // Happens if AssetPath constructor fails
        return std::nullopt;
    }
}

void trc::AssetData<trc::Material>::resolveReferences(AssetManager& assetManager)
{
    for (auto& ref : textures) {
        ref.resolve(assetManager);
    }
    textures.clear();
}



void trc::MaterialRegistry::update(vk::CommandBuffer, FrameRenderState&)
{
}

auto trc::MaterialRegistry::add(u_ptr<AssetSource<Material>> source) -> LocalID
{
    const LocalID id{ localIdPool.generate() };
    auto& mat = storage.emplace(id, Storage{
        .data=source->load(),
        .runtimePrograms={ nullptr }
    });

    // Create all runtime programs
    for (const auto& [key, program] : mat.data.programs)
    {
        const DrawablePipelineInfo info{
            .animated=key.flags.has(MaterialKey::Flags::Animated::eTrue),
            .transparent=mat.data.transparent
        };
        Pipeline::ID basePipeline = pipelines::getDrawableBasePipeline(info.toPipelineFlags());

        // Set some pipeline configuration parameters
        auto pipeline = PipelineRegistry::cloneGraphicsPipeline(basePipeline).getPipelineData();
        auto& d = mat.data;
        auto& r = pipeline.rasterization;
        if (mat.data.polygonMode) r.setPolygonMode(*mat.data.polygonMode);
        if (mat.data.lineWidth) r.setLineWidth(*mat.data.lineWidth);
        if (d.cullMode) r.setCullMode(*d.cullMode);
        if (d.frontFace) r.setFrontFace(*d.frontFace);
        if (d.depthWrite) pipeline.depthStencil.setDepthWriteEnable(*d.depthWrite);
        if (d.depthTest) pipeline.depthStencil.setDepthTestEnable(*d.depthTest);
        if (d.depthBiasConstantFactor) r.setDepthBiasConstantFactor(*d.depthBiasConstantFactor);
        if (d.depthBiasSlopeFactor) r.setDepthBiasSlopeFactor(*d.depthBiasSlopeFactor);

        // Create the runtime program
        mat.runtimePrograms.at(key.flags.toIndex()) = std::make_unique<MaterialShaderProgram>(
            program,
            pipeline,
            PipelineRegistry::getPipelineRenderPass(basePipeline)
        );
    }

    return id;
}

void trc::MaterialRegistry::remove(LocalID id)
{
    assert(storage.contains(id));

    storage.erase(id);
    localIdPool.free(id);
}

auto trc::MaterialRegistry::getHandle(LocalID id) -> Handle
{
    assert(storage.contains(id));

    return Handle{ storage.at(id) };
}
