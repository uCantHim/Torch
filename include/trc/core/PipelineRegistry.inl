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
    recreateAll();
}

template<typename T>
void PipelineStorage<T>::notifyNewPipeline(
    Pipeline::ID id,
    FactoryType& factory)
{
    assert(id == pipelines.size());
    pipelines.emplace_back(createPipeline(factory));
}

template<typename T>
auto PipelineStorage<T>::get(Pipeline::ID pipeline) -> Pipeline&
{
    assert(pipeline < pipelines.size());
    return *pipelines.at(pipeline);
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
auto PipelineRegistry<T>::registerPipeline(const PipelineTemplate& _template) -> Pipeline::ID
{
    if (_template.getLayout() >= layoutFactories.size())
    {
        throw Exception(
            "[In PipelineRegistry<>::registerPipeline]: No pipeline layout with the specified "
            "ID \"" + std::to_string(_template.getLayout()) + "\" exists in the registry!"
        );
    }

    return _registerPipelineFactory({
        _template,
        _template.getLayout(),
        _template.getRenderPass()
    });
}

template<RenderConfigType T>
auto PipelineRegistry<T>::registerPipeline(const ComputePipelineTemplate& _template) -> Pipeline::ID
{
    if (_template.getLayout() >= layoutFactories.size())
    {
        throw Exception(
            "[In PipelineRegistry<>::registerPipeline]: No pipeline layout with the specified "
            "ID \"" + std::to_string(_template.getLayout()) + "\" exists in the registry!"
        );
    }

    _registerPipelineFactory({ _template, _template.getLayout() });
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
    if (id < factories.size())
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
auto trc::PipelineRegistry<T>::StorageAccessInterface::invokePipelineFactory(
    Pipeline::ID id,
    const Instance& instance,
    T& renderConfig)
    -> Pipeline
{
    std::scoped_lock lock(factoryLock);
    return factories.at(id).create(instance, renderConfig);
}

template<RenderConfigType T>
auto trc::PipelineRegistry<T>::StorageAccessInterface::invokeLayoutFactory(
    PipelineLayout::ID id,
    const Instance& instance,
    T& renderConfig)
    -> PipelineLayout
{
    std::scoped_lock lock(layoutFactoryLock);
    return layoutFactories.at(id).create(instance, renderConfig);
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

    // Create copy because it will be modified
    PipelineDefinitionData def = t.getPipelineData();
    const ProgramDefinitionData& shader = t.getProgramData();

    // Create a program from the shader code
    auto program = shader.makeProgram(device);

    auto [renderPass, subPass] = renderConfig.getRenderPass(renderPassName);
    auto pipeline = device->createGraphicsPipelineUnique(
        {},
        vk::GraphicsPipelineCreateInfo(
            {},
            program.getStageCreateInfo(),
            &def.vertexInput,
            &def.inputAssembly,
            &def.tessellation,
            &def.viewport,
            &def.rasterization,
            &def.multisampling,
            &def.depthStencil,
            &def.colorBlending,
            &def.dynamicState,
            *layout,
            renderPass, subPass,
            vk::Pipeline(), 0
        )
    ).value;

    return Pipeline{ layout, std::move(pipeline), vk::PipelineBindPoint::eGraphics };
}

template<RenderConfigType T>
auto PipelineRegistry<T>::PipelineFactory::create(
    ComputePipelineTemplate& t,
    const Instance& instance,
    T&,
    PipelineLayout& layout
    ) const -> Pipeline
{
    const auto& device = instance.getDevice();

    auto shaderModule = vkb::createShaderModule(device, t.getShaderCode());
    auto pipeline = device->createComputePipelineUnique(
        {},
        vk::ComputePipelineCreateInfo(
            {},
            vk::PipelineShaderStageCreateInfo(
                {}, vk::ShaderStageFlagBits::eCompute,
                *shaderModule,
                t.getEntryPoint().c_str()
            ),
            *layout
        )
    ).value;

    return Pipeline{ layout, std::move(pipeline), vk::PipelineBindPoint::eGraphics };
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
    std::vector<vk::DescriptorSetLayout> descLayouts;
    for (const auto& desc : _template.getDescriptors())
    {
        descLayouts.emplace_back(renderConfig.getDescriptor(desc.name).getDescriptorSetLayout());
    }

    std::vector<vk::PushConstantRange> pushConstantRanges;
    for (const auto& pc : _template.getPushConstants())
    {
        pushConstantRanges.emplace_back(pc.range);
    }

    vk::PipelineLayoutCreateInfo createInfo{ {}, descLayouts, pushConstantRanges };
    PipelineLayout layout{ instance.getDevice()->createPipelineLayoutUnique(createInfo) };

    // Add static descriptors and default push constant values to the layout
    for (ui32 i = 0; i < _template.getDescriptors().size(); i++)
    {
        const auto& desc = _template.getDescriptors().at(i);
        if (desc.isStatic) {
            layout.addStaticDescriptorSet(i, renderConfig.getDescriptor(desc.name));
        }
    }

    for (const auto& push : _template.getPushConstants())
    {
        if (push.defaultValue.has_value()) {
            push.defaultValue.value().setAsDefault(layout, push.range);
        }
    }

    return layout;
}

template<RenderConfigType T>
auto PipelineRegistry<T>::LayoutFactory::clone() const -> PipelineLayoutTemplate
{
    return _template;
}

} // namespace trc
