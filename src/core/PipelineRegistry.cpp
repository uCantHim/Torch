#include "trc/core/PipelineRegistry.h"

// #include "trc/base/ShaderProgram.h"



namespace trc
{

auto registerPipeline(PipelineTemplate t,
                             PipelineLayout::ID layout,
                             RenderPassName renderPass)
    -> Pipeline::ID
{
    return PipelineRegistry::registerPipeline(std::move(t), layout, std::move(renderPass));
}

auto registerPipeline(ComputePipelineTemplate t,
                             PipelineLayout::ID layout)
    -> Pipeline::ID
{
    return PipelineRegistry::registerPipeline(std::move(t), layout);
}



PipelineStorage::PipelineStorage(
    typename PipelineRegistry::StorageAccessInterface interface,
    const Instance& instance,
    RenderConfig& renderConfig)
    :
    registry(interface),
    instance(instance),
    renderConfig(&renderConfig)
{
}

void PipelineStorage::notifyNewPipeline(
    Pipeline::ID,
    FactoryType&)
{
}

auto PipelineStorage::get(Pipeline::ID pipeline) -> Pipeline&
{
    if (pipeline >= pipelines.size()) {
        pipelines.resize(pipeline + 1);
    }

    if (pipelines.at(pipeline) == nullptr)
    {
        auto& layout = getLayout(registry.getPipelineLayout(pipeline));
        pipelines.at(pipeline) = std::make_unique<Pipeline>(
            registry.invokePipelineFactory(pipeline, instance, *renderConfig, layout)
        );
    }
    return *pipelines.at(pipeline);
}

auto PipelineStorage::getLayout(PipelineLayout::ID id) -> PipelineLayout&
{
    if (layouts.size() <= id || !layouts.at(id))
    {
        layouts.resize(std::max(static_cast<size_t>(id + 1), layouts.size()));
        layouts.at(id) = std::make_unique<PipelineLayout>(
            registry.invokeLayoutFactory(id, instance, *renderConfig)
        );
    }

    return *layouts.at(id);
}

void PipelineStorage::recreateAll()
{
    pipelines.clear();
    registry.foreachFactory([this](auto& factory) {
        pipelines.emplace_back(createPipeline(factory));
    });
}

auto PipelineStorage::createPipeline(FactoryType& factory) -> u_ptr<Pipeline>
{
    PipelineLayout& layout = getLayout(factory.getLayout());
    assert(layout);

    return std::make_unique<Pipeline>(factory.create(instance, *renderConfig, layout));
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
    RenderPassName renderPass
    ) -> Pipeline::ID
{
    if (layout >= layoutFactories.size())
    {
        throw Exception(
            "[In PipelineRegistry<>::registerPipeline]: No pipeline layout with the specified "
            "ID \"" + std::to_string(layout) + "\" exists in the registry!"
        );
    }

    return _registerPipelineFactory({ std::move(_template), layout, std::move(renderPass) });
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

    std::scoped_lock lock(factoryLock, storageLock);

    // Create a new factory
    auto& factory = *factories.emplace(factories.begin() + id, std::move(newFactory));

    // Notiy existing storages
    for (auto storage : storages) {
        storage->notifyNewPipeline(id, factory);
    }

    return id;
}

auto PipelineRegistry::cloneGraphicsPipeline(Pipeline::ID id) -> PipelineTemplate
{
    try {
        std::scoped_lock lock(factoryLock);
        return std::get<PipelineTemplate>(factories.at(id).clone());
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
        throw Exception(
            "[In PipelineRegistry<>::cloneGraphicsPipeline]: Pipeline with ID \""
            + std::to_string(id) + "\" is not a graphics pipeline!"
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
        throw Exception(
            "[In PipelineRegistry<>::cloneComputePipeline]: Pipeline with ID \""
            + std::to_string(id) + "\" is not a compute pipeline!"
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

auto PipelineRegistry::getPipelineRenderPass(Pipeline::ID id) -> RenderPassName
{
    if (id >= factories.size())
    {
        throw Exception(
            "[In PipelineRegistry<>::getPipelineLayout]: Pipeline with ID \""
            + std::to_string(id) + "\" does not exist in the registry!"
        );
    }

    std::scoped_lock lock(factoryLock);
    return factories.at(id).getRenderPassName();
}

auto PipelineRegistry::makeStorage(const Instance& instance, RenderConfig& renderConfig)
    -> std::unique_ptr<PipelineStorage>
{
    u_ptr<PipelineStorage> result{
        new PipelineStorage(StorageAccessInterface{}, instance, renderConfig)
    };

    std::lock_guard lock(storageLock);
    storages.push_back(result.get());

    return result;
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
    RenderConfig& renderConfig,
    PipelineLayout& layout)
    -> Pipeline
{
    std::scoped_lock lock(PipelineRegistry::factoryLock);
    assert(id < PipelineRegistry::factories.size());

    return PipelineRegistry::factories.at(id).create(instance, renderConfig, layout);
}

auto trc::PipelineRegistry::StorageAccessInterface::invokeLayoutFactory(
    PipelineLayout::ID id,
    const Instance& instance,
    RenderConfig& renderConfig)
    -> PipelineLayout
{
    std::scoped_lock lock(PipelineRegistry::layoutFactoryLock);
    assert(id < PipelineRegistry::layoutFactories.size());

    return PipelineRegistry::layoutFactories.at(id).create(instance, renderConfig);
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
    renderPassName(std::move(rp)),
    _template(std::move(t))
{
}

PipelineRegistry::PipelineFactory::PipelineFactory(
    ComputePipelineTemplate t,
    PipelineLayout::ID layout)
    :
    layoutId(layout),
    renderPassName({}),
    _template(std::move(t))
{
}

auto trc::PipelineRegistry::PipelineFactory::getLayout() const -> PipelineLayout::ID
{
    return layoutId;
}

auto trc::PipelineRegistry::PipelineFactory::getRenderPassName() const -> const RenderPassName&
{
    return renderPassName;
}

auto PipelineRegistry::PipelineFactory::create(
    const Instance& instance,
    RenderConfig& renderConfig,
    PipelineLayout& layout)
    -> Pipeline
{
    return std::visit(
        [&](auto& t) { return create(t, instance, renderConfig, layout); },
        _template
    );
}

auto PipelineRegistry::PipelineFactory::create(
    PipelineTemplate& t,
    const Instance& instance,
    RenderConfig& renderConfig,
    PipelineLayout& layout
    ) const -> Pipeline
{
    const auto& device = instance.getDevice();
    auto [renderPass, subPass] = renderConfig.getRenderPass(renderPassName);

    return makeGraphicsPipeline(device, t, layout, renderPass, subPass);
}

auto PipelineRegistry::PipelineFactory::create(
    ComputePipelineTemplate& t,
    const Instance& instance,
    RenderConfig&,
    PipelineLayout& layout
    ) const -> Pipeline
{
    return makeComputePipeline(instance.getDevice(), t, layout);
}

auto PipelineRegistry::PipelineFactory::clone() const
    -> std::variant<PipelineTemplate, ComputePipelineTemplate>
{
    return _template;
}



PipelineRegistry::LayoutFactory::LayoutFactory(PipelineLayoutTemplate t)
    : _template(std::move(t))
{
}

auto PipelineRegistry::LayoutFactory::create(
    const Instance& instance,
    RenderConfig& renderConfig)
    -> PipelineLayout
{
    return makePipelineLayout(instance.getDevice(), _template, renderConfig);
}

auto PipelineRegistry::LayoutFactory::clone() const -> PipelineLayoutTemplate
{
    return _template;
}


} // namespace trc
