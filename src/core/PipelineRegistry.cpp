#include "trc/core/PipelineRegistry.h"

#include <cassert>

#include "trc/core/ResourceConfig.h"
#include "trc_util/TypeUtils.h"



namespace trc
{

InvalidPipelineType::InvalidPipelineType(
    std::source_location loc,
    const std::string& whatHappened,
    vk::PipelineBindPoint expected,
    vk::PipelineBindPoint actual)
    :
    Exception(
        "[In " + std::string(loc.function_name()) + "]: " + whatHappened
        + ": Expected pipeline of type " + vk::to_string(expected)
        + ", but got " + vk::to_string(actual)
    )
{
}



PipelineStorage::PipelineStorage(
    typename PipelineRegistry::StorageAccessInterface interface,
    const Instance& instance,
    ResourceConfig& resourceConfig)
    :
    registry(interface),
    instance(instance),
    resourceConfig(&resourceConfig)
{
}

auto PipelineStorage::get(Pipeline::ID pipeline) -> Pipeline&
{
    if (!pipelines.contains(pipeline))
    {
        assert(resourceConfig != nullptr);

        auto& layout = getLayout(registry.getPipelineLayout(pipeline));
        pipelines.try_emplace(
            pipeline,
            registry.invokePipelineFactory(pipeline, instance, *resourceConfig, layout)
        );
    }

    return pipelines.at(pipeline);
}

auto PipelineStorage::getLayout(PipelineLayout::ID id) -> PipelineLayout&
{
    if (!layouts.contains(id))
    {
        assert(resourceConfig != nullptr);
        layouts.try_emplace(id, registry.invokeLayoutFactory(id, instance, *resourceConfig));
    }

    return layouts.at(id);
}

void PipelineStorage::clear()
{
    pipelines.clear();
    layouts.clear();
}

auto PipelineStorage::createPipeline(FactoryType& factory) -> u_ptr<Pipeline>
{
    PipelineLayout& layout = getLayout(factory.getLayout());
    assert(layout);

    return std::make_unique<Pipeline>(factory.create(instance, *resourceConfig, layout));
}



//////////////////////////////////
//      Pipeline registry       //
//////////////////////////////////

auto PipelineRegistry::registerPipelineLayout(PipelineLayoutTemplate _template)
    -> PipelineLayout::ID
{
    const auto id = _allocPipelineLayoutId();
    assert(id < layoutFactories.size());

    std::scoped_lock lock(layoutFactoryLock);
    layoutFactories.emplace(layoutFactories.begin() + id, std::move(_template));

    return id;
}

auto PipelineRegistry::clonePipelineLayout(PipelineLayout::ID id) -> PipelineLayoutTemplate
{
    if (id >= layoutFactories.size())
    {
        throw Exception(
            "[In PipelineRegistry<>::clonePipelineLayout]: No pipeline layout with the specified "
            "ID \"" + std::to_string(id) + "\" exists in the registry!"
        );
    }

    return layoutFactories.at(id).clone();
}

auto PipelineRegistry::registerPipeline(
    PipelineTemplate _template,
    PipelineLayout::ID layout,
    const RenderPassDefinition& renderPass
    ) -> Pipeline::ID
{
    if (layout >= layoutFactories.size())
    {
        throw Exception(
            "[In PipelineRegistry<>::registerPipeline]: No pipeline layout with the specified "
            "ID \"" + std::to_string(layout) + "\" exists in the registry!"
        );
    }

    return std::visit(
        [&](auto&& info){
            return _registerPipelineFactory({ std::move(_template), layout, info });
        },
        renderPass
    );
}

auto PipelineRegistry::registerPipeline(
    ComputePipelineTemplate _template,
    PipelineLayout::ID layout
    ) -> Pipeline::ID
{
    if (layout >= layoutFactories.size())
    {
        throw Exception(
            "[In PipelineRegistry<>::registerPipeline]: No pipeline layout with the specified "
            "ID \"" + std::to_string(layout) + "\" exists in the registry!"
        );
    }

    return _registerPipelineFactory({ std::move(_template), layout });
}

inline auto PipelineRegistry::_registerPipelineFactory(PipelineFactory newFactory) -> Pipeline::ID
{
    const Pipeline::ID id{ _allocPipelineId() };
    assert(id < factories.size());

    std::scoped_lock lock(factoryLock);

    // Create a new factory
    *factories.emplace(factories.begin() + id, std::move(newFactory));

    return id;
}

auto PipelineRegistry::cloneGraphicsPipeline(Pipeline::ID id)
    -> std::pair<PipelineTemplate, RenderPassDefinition>
{
    try {
        std::scoped_lock lock(factoryLock);
        auto [t, i] = std::get<PipelineFactory::GraphicsPipelineInfo>(factories.at(id).clone());
        return { std::move(t), std::move(i) };
    }
    catch (const std::out_of_range&)
    {
        throw Exception(
            "[In PipelineRegistry<>::cloneGraphicsPipeline]: Pipeline with ID \""
            + std::to_string(id) + "\" does not exist in the registry!"
        );
    }
    catch (const std::bad_variant_access&)
    {
        throw InvalidPipelineType(
            std::source_location::current(),
            "Unable to clone pipeline with ID " + std::to_string(id),
            vk::PipelineBindPoint::eGraphics, vk::PipelineBindPoint::eCompute
        );
    }
}

auto PipelineRegistry::cloneComputePipeline(Pipeline::ID id) -> ComputePipelineTemplate
{
    try {
        std::scoped_lock lock(factoryLock);
        return std::get<ComputePipelineTemplate>(factories.at(id).clone());
    }
    catch (const std::out_of_range&)
    {
        throw Exception(
            "[In PipelineRegistry<>::cloneComputePipeline]: Pipeline with ID \""
            + std::to_string(id) + "\" does not exist in the registry!"
        );
    }
    catch (const std::bad_variant_access&)
    {
        throw InvalidPipelineType(
            std::source_location::current(),
            "Unable to clone pipeline with ID " + std::to_string(id),
            vk::PipelineBindPoint::eCompute, vk::PipelineBindPoint::eGraphics
        );
    }
}

auto PipelineRegistry::getPipelineLayout(Pipeline::ID id) -> PipelineLayout::ID
{
    if (id >= factories.size())
    {
        throw Exception(
            "[In PipelineRegistry<>::getPipelineLayout]: Pipeline with ID \""
            + std::to_string(id) + "\" does not exist in the registry!"
        );
    }

    std::scoped_lock lock(factoryLock);
    return factories.at(id).getLayout();
}

auto PipelineRegistry::getPipelineRenderPass(Pipeline::ID id)
    -> std::optional<RenderPassDefinition>
{
    if (id >= factories.size())
    {
        throw Exception(
            "[In PipelineRegistry<>::getPipelineLayout]: Pipeline with ID \""
            + std::to_string(id) + "\" does not exist in the registry!"
        );
    }

    std::scoped_lock lock(factoryLock);
    if (auto info = factories.at(id).getRenderPassCompatInfo()) {
        return *info;
    }
    return std::nullopt;
}

auto PipelineRegistry::makeStorage(const Instance& instance, ResourceConfig& resourceConfig)
    -> std::unique_ptr<PipelineStorage>
{
    return u_ptr<PipelineStorage>{
        new PipelineStorage(StorageAccessInterface{}, instance, resourceConfig)
    };
}

auto PipelineRegistry::_allocPipelineLayoutId() -> PipelineLayout::ID
{
    const PipelineLayout::ID id{ pipelineLayoutIdPool.generate() };

    std::scoped_lock lock(layoutFactoryLock);
    if (layoutFactories.size() <= id) {
        layoutFactories.resize(id + 1);
    }

    return id;
}

auto PipelineRegistry::_allocPipelineId() -> Pipeline::ID
{
    const Pipeline::ID id{ pipelineIdPool.generate() };

    std::scoped_lock lock(factoryLock);
    if (factories.size() <= id) {
        factories.resize(id + 1);
    }

    return id;
}



////////////////////////////////////////
//      Storage access interface      //
////////////////////////////////////////

auto trc::PipelineRegistry::StorageAccessInterface::getPipelineLayout(Pipeline::ID id)
    -> PipelineLayout::ID
{
    return PipelineRegistry::getPipelineLayout(id);
}

auto trc::PipelineRegistry::StorageAccessInterface::invokePipelineFactory(
    Pipeline::ID id,
    const Instance& instance,
    ResourceConfig& resourceConfig,
    PipelineLayout& layout)
    -> Pipeline
{
    std::scoped_lock lock(PipelineRegistry::factoryLock);
    assert(id < PipelineRegistry::factories.size());

    return PipelineRegistry::factories.at(id).create(instance, resourceConfig, layout);
}

auto trc::PipelineRegistry::StorageAccessInterface::invokeLayoutFactory(
    PipelineLayout::ID id,
    const Instance& instance,
    ResourceConfig& resourceConfig)
    -> PipelineLayout
{
    std::scoped_lock lock(PipelineRegistry::layoutFactoryLock);
    assert(id < PipelineRegistry::layoutFactories.size());

    return PipelineRegistry::layoutFactories.at(id).create(instance, resourceConfig);
}



////////////////////////////////////////
//      Pipeline/-layout factory      //
////////////////////////////////////////

PipelineRegistry::PipelineFactory::PipelineFactory(
    PipelineTemplate t,
    PipelineLayout::ID layout,
    RenderPassName rp)
    :
    layoutId(layout),
    _template(GraphicsPipelineInfo{
        .tmpl=std::move(t),
        .renderPassCompatInfo=std::move(rp)
    })
{
}

PipelineRegistry::PipelineFactory::PipelineFactory(
    PipelineTemplate t,
    PipelineLayout::ID layout,
    RenderPassCompatInfo rp)
    :
    layoutId(layout),
    _template(GraphicsPipelineInfo{
        .tmpl=std::move(t),
        .renderPassCompatInfo=std::move(rp)
    })
{
}

PipelineRegistry::PipelineFactory::PipelineFactory(
    ComputePipelineTemplate t,
    PipelineLayout::ID layout)
    :
    layoutId(layout),
    _template(std::move(t))
{
}

auto trc::PipelineRegistry::PipelineFactory::getLayout() const -> PipelineLayout::ID
{
    return layoutId;
}

auto trc::PipelineRegistry::PipelineFactory::getRenderPassCompatInfo() const
    -> const std::variant<RenderPassName, RenderPassCompatInfo>*
{
    if (!std::holds_alternative<GraphicsPipelineInfo>(_template)) {
        return nullptr;
    }

    return &std::get<GraphicsPipelineInfo>(_template).renderPassCompatInfo;
}

auto PipelineRegistry::PipelineFactory::create(
    const Instance& instance,
    ResourceConfig& resourceConfig,
    PipelineLayout& layout)
    -> Pipeline
{
    // Create either a graphics pipeline or a compute pipeline
    return std::visit(util::VariantVisitor{
        [&, this](GraphicsPipelineInfo& info)
        {
            // Obtain the render pass compatibility information. Either resolve
            // a render pass reference via the render config, or retrieve the
            // direct information object from our own storage.
            const auto& rpCompat = std::visit(util::VariantVisitor{
                [&](const RenderPassName& ref){
                    try {
                        return resourceConfig.getRenderPass(ref);
                    }
                    catch (const Exception&) {
                        throw std::runtime_error(
                            "[In PipelineFactory::create]: Failed to create a pipeline because its"
                            " render pass compatibility information was specified as a reference"
                            " name \"" + ref + "\", but that reference has no render pass"
                            " registered at the render pass registry."
                        );
                    }
                },
                [](const RenderPassCompatInfo& def){ return def; },
            }, info.renderPassCompatInfo);

            return std::visit(
                [&](auto& compatInfo){ return create(info.tmpl, compatInfo, instance, layout); },
                rpCompat
            );
        },
        [&](ComputePipelineTemplate& t){
            return create(t, instance, layout);
        },
    }, _template);
}

auto PipelineRegistry::PipelineFactory::clone() const
    -> std::variant<GraphicsPipelineInfo, ComputePipelineTemplate>
{
    return _template;
}

/** Internal function */
auto PipelineRegistry::PipelineFactory::create(
    PipelineTemplate& t,
    const RenderPassInfo& compatInfo,
    const Instance& instance,
    PipelineLayout& layout
    ) -> Pipeline
{
    const auto& device = instance.getDevice();
    const auto& [renderPass, subPass] = compatInfo;
    return makeGraphicsPipeline(device, t, layout, renderPass, subPass);
}

/** Internal function */
auto PipelineRegistry::PipelineFactory::create(
    PipelineTemplate& p,
    const DynamicRenderingInfo& compatInfo,
    const Instance& instance,
    PipelineLayout& layout) -> Pipeline
{
    vk::PipelineRenderingCreateInfo renderingInfo{
        compatInfo.viewMask,
        compatInfo.colorAttachmentFormats,
        compatInfo.depthAttachmentFormat,
        compatInfo.stencilAttachmentFormat
    };

    return makeGraphicsPipeline(instance.getDevice(), p, layout, renderingInfo);
}

/** Internal function */
auto PipelineRegistry::PipelineFactory::create(
    ComputePipelineTemplate& t,
    const Instance& instance,
    PipelineLayout& layout
    ) -> Pipeline
{
    return makeComputePipeline(instance.getDevice(), t, layout);
}



PipelineRegistry::LayoutFactory::LayoutFactory(PipelineLayoutTemplate t)
    : _template(std::move(t))
{
}

auto PipelineRegistry::LayoutFactory::create(
    const Instance& instance,
    ResourceConfig& resourceConfig)
    -> PipelineLayout
{
    return makePipelineLayout(instance.getDevice(), _template, resourceConfig);
}

auto PipelineRegistry::LayoutFactory::clone() const -> PipelineLayoutTemplate
{
    return _template;
}


} // namespace trc
