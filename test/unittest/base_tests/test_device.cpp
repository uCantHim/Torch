#include <gtest/gtest.h>

#include <trc/base/Device.h>
#include <trc/base/PhysicalDevice.h>
#include <trc/base/VulkanInstance.h>

class DeviceTest : public testing::Test
{
protected:
    DeviceTest()
        :
        instance([]{
            trc::log::setLogLevel(trc::log::LogLevel::eWarning);
            return trc::VulkanInstance{};
        }())
    {}

    trc::VulkanInstance instance;
};

TEST_F(DeviceTest, CreateDeviceWithoutSurface)
{
    auto devices = instance.queryPhysicalDevices();
    for (const auto& dev : devices)
    {
        auto device = dev.makeLogicalDevice();
    }
}

TEST_F(DeviceTest, CreateDeviceWithSurface)
{
    if (auto surface = instance.makeSurface({}))
    {
        auto devices = instance.queryPhysicalDevices(surface->getVulkanSurface());
        for (const auto& dev : devices)
        {
            auto device = dev.makeLogicalDevice();
        }
    }
}
