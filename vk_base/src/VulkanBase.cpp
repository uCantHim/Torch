#include "VulkanBase.h"



void vkb::init(const VulkanBaseInitInfo& info)
{
    if (info.startEventThread) {
        EventThread::start();
    }

    // Init GLFW first
    if (glfwInit() == GLFW_FALSE)
    {
        const char* errorMsg{ nullptr };
        glfwGetError(&errorMsg);
        throw std::runtime_error("Initialization of GLFW failed: " + std::string(errorMsg));
    }
    if constexpr (vkb::enableVerboseLogging) {
        std::cout << "GLFW initialized successfully\n";
    }
}

void vkb::terminate()
{
    vkb::EventThread::terminate();
    glfwTerminate();
}
