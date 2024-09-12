#include "trc/core/DeviceTask.h"

#include <cassert>

#include <trc_util/Assert.h>

#include "trc/core/Frame.h"



namespace trc
{

DeviceExecutionContext::DeviceExecutionContext(
    Frame& _frame,
    const s_ptr<DependencyRegion>& depRegion,
    const s_ptr<ResourceStorage>& resources)
    :
    parentFrame(_frame),
    dependencyRegion(depRegion),
    resourceStorage(resources)
{
    assert(depRegion != nullptr);
    assert(resources != nullptr);
}

auto DeviceExecutionContext::frame() -> Frame&
{
    return parentFrame;
}

auto DeviceExecutionContext::device() -> const Device&
{
    return parentFrame.getDevice();
}

auto DeviceExecutionContext::resources() -> ResourceStorage&
{
    return *resourceStorage;
}

auto DeviceExecutionContext::deps() -> DependencyRegion&
{
    return *dependencyRegion;
}

auto DeviceExecutionContext::overrideResources(s_ptr<ResourceStorage> newStorage) const
    -> DeviceExecutionContext
{
    assert_arg(newStorage != nullptr);
    return DeviceExecutionContext{
        parentFrame,
        dependencyRegion,
        newStorage
    };
}

} // namespace trc
