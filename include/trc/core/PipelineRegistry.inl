#include "PipelineRegistry.h"

#include <vkb/ShaderProgram.h>



namespace trc
{

template<typename T>
PipelineStorage<T>::PipelineStorage(
    typename PipelineRegistry<T>::StorageAccessInterface  interface,
    const Instance& instance,
    T& renderConfig)
    :
    registry(interface),
    instance(instance),
    renderConfig(&renderConfig)
{
}

template<typename T>
void PipelineStorage<T>::notifyNewPipeline(
    Pipeline::ID,
    FactoryType&)
{
}

template<typename T>
auto PipelineStorage<T>::get(Pipeline::ID pipeline) -> Pipeline&
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

template<typename T>
auto PipelineStorage<T>::getLayout(PipelineLayout::ID id) -> PipelineLayout&
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

template<typename T>
void PipelineStorage<T>::recreateAll()
{
    pipelines.clear();
    registry.foreachFactory([this](auto& factory) {
        pipelines.emplace_back(createPipeline(factory));
    });
}

template<typename T>
auto PipelineStorage<T>::createPipeline(FactoryType& factory) -> u_ptr<Pipeline>
{
    PipelineLayout& layout = getLayout(factory.getLayout());
    assert(layout);

    return std::make_unique<Pipeline>(factory.create(instance, *renderConfig, layout));
}



//////////////////////////////////
//      Pipeline registry       //
//////////////////////////////////

template<RenderConfigType T>
auto PipelineRegistry<T>::registerPipelineLayout(PipelineLayoutTemplate _template)
    -> PipelineLayout::ID
{
    const auto id = _allocPipelineLayoutId();
    assert(id < layoutFactories.size());

    std::scoped_lock lock(layoutFactoryLock);
    layoutFactories.emplace(layoutFactories.begin() + id, std::move(_template));

    return id;
}

template<RenderConfigType T>
auto PipelineRegistry<T>::clonePipelineLayout(PipelineLayout::ID id) -> PipelineLayoutTemplate
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

template<RenderConfigType T>
auto PipelineRegistry<T>::registerPipeline(
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

template<RenderConfigType T>
auto PipelineRegistry<T>::registerPipeline(
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

    _registerPipelineFactory({ std::move(_template), layout });
}

template<RenderConfigType T>
inline auto PipelineRegistry<T>::_registerPipelineFactory(PipelineFactory newFactory) -> Pipeline::ID
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

template<RenderConfigType T>
auto PipelineRegistry<T>::cloneGraphicsPipeline(Pipeline::ID id) -> PipelineTemplate
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

template<RenderConfigType T>
auto PipelineRegistry<T>::cloneComputePipeline(Pipeline::ID id) -> ComputePipelineTemplate
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

template<RenderConfigType T>
auto PipelineRegistry<T>::getPipelineLayout(Pipeline::ID id) -> PipelineLayout::ID
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

template<RenderConfigType T>
auto PipelineRegistry<T>::createStorage(const Instance& instance, T& renderConfig)
    -> std::unique_ptr<PipelineStorage<T>>
{
    u_ptr<PipelineStorage<T>> result{
        new PipelineStorage<T>(
            StorageAccessInterface{ PipelineRegistry<T>{} },
            instance,
            renderConfig
        )
    };

    std::lock_guard lock(storageLock);
    storages.push_back(result.get());

    return result;
}

template<RenderConfigType T>
auto PipelineRegistry<T>::_allocPipelineLayoutId() -> PipelineLayout::ID
{
    const PipelineLayout::ID id{ pipelineLayoutIdPool.generate() };

    std::scoped_lock lock(layoutFactoryLock);
    if (layoutFactories.size() <= id) {
        layoutFactories.resize(id + 1);
    }

    return id;
}

template<RenderConfigType T>
auto PipelineRegistry<T>::_allocPipelineId() -> Pipeline::ID
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

template<RenderConfigType T>
trc::PipelineRegistry<T>::StorageAccessInterface::StorageAccessInterface(PipelineRegistry reg)
    : registry(reg)
{
}

template<RenderConfigType T>
auto trc::PipelineRegistry<T>::StorageAccessInterface::getPipelineLayout(Pipeline::ID id)
    -> PipelineLayout::ID
{
    return registry.getPipelineLayout(id);
}

template<RenderConfigType T>
auto trc::PipelineRegistry<T>::StorageAccessInterface::invokePipelineFactory(
    Pipeline::ID id,
    const Instance& instance,
    T& renderConfig,
    PipelineLayout& layout)
    -> Pipeline
{
    std::scoped_lock lock(registry.factoryLock);
    assert(id < registry.factories.size());

    return registry.factories.at(id).create(instance, renderConfig, layout);
}

template<RenderConfigType T>
auto trc::PipelineRegistry<T>::StorageAccessInterface::invokeLayoutFactory(
    PipelineLayout::ID id,
    const Instance& instance,
    T& renderConfig)
    -> PipelineLayout
{
    std::scoped_lock lock(registry.layoutFactoryLock);
    assert(id < registry.layoutFactories.size());

    return registry.layoutFactories.at(id).create(instance, renderConfig);
}



////////////////////////////////////////
//      Pipeline/-layout factory      //
////////////////////////////////////////

template<RenderConfigType T>
PipelineRegistry<T>::PipelineFactory::PipelineFactory(
    PipelineTemplate t,
    PipelineLayout::ID layout,
    RenderPassName rp)
    :
    layoutId(layout),
    renderPassName(std::move(rp)),
    _template(std::move(t))
{
}

template<RenderConfigType T>
PipelineRegistry<T>::PipelineFactory::PipelineFactory(
    ComputePipelineTemplate t,
    PipelineLayout::ID layout)
    :
    layoutId(layout),
    renderPassName({}),
    _template(std::move(t))
{
}

template<RenderConfigType T>
auto trc::PipelineRegistry<T>::PipelineFactory::getLayout() const -> PipelineLayout::ID
{
    return layoutId;
}

template<RenderConfigType T>
auto trc::PipelineRegistry<T>::PipelineFactory::getRenderPassName() const -> const RenderPassName&
{
    return renderPassName;
}

template<RenderConfigType T>
auto PipelineRegistry<T>::PipelineFactory::create(
    const Instance& instance,
    T& renderConfig,
    PipelineLayout& layout)
    -> Pipeline
{
    return std::visit(
        [&](auto& t) { return create(t, instance, renderConfig, layout); },
        _template
    );
}

template<RenderConfigType T>
auto PipelineRegistry<T>::PipelineFactory::create(
    PipelineTemplate& t,
    const Instance& instance,
    T& renderConfig,
    PipelineLayout& layout
    ) const -> Pipeline
{
    const auto& device = instance.getDevice();
    auto [renderPass, subPass] = renderConfig.getRenderPass(renderPassName);

    return makeGraphicsPipeline(device, t, layout, renderPass, subPass);
}

template<RenderConfigType T>
auto PipelineRegistry<T>::PipelineFactory::create(
    ComputePipelineTemplate& t,
    const Instance& instance,
    T&,
    PipelineLayout& layout
    ) const -> Pipeline
{
    return makeComputePipeline(instance.getDevice(), t, layout);
}

template<RenderConfigType T>
auto PipelineRegistry<T>::PipelineFactory::clone() const
    -> std::variant<PipelineTemplate, ComputePipelineTemplate>
{
    return _template;
}



template<RenderConfigType T>
PipelineRegistry<T>::LayoutFactory::LayoutFactory(PipelineLayoutTemplate t)
    : _template(std::move(t))
{
}

template<RenderConfigType T>
auto PipelineRegistry<T>::LayoutFactory::create(
    const Instance& instance,
    T& renderConfig)
    -> PipelineLayout
{
    return makePipelineLayout(instance.getDevice(), _template, renderConfig);
}

template<RenderConfigType T>
auto PipelineRegistry<T>::LayoutFactory::clone() const -> PipelineLayoutTemplate
{
    return _template;
}

} // namespace trc
