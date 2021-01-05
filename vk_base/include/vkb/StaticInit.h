#pragma once

#include <vector>
#include <functional>

namespace vkb
{
    /**
     * @brief Initialization helper for static Vulkan objects
     *
     * Create a static StaticInit object and pass initialization- and
     * destruction functions that are executed as soon as Vulkan has been
     * initialized (i.e. `VulkanBase::init()` has been called).
     *
     * Example:
     *
     *     class MyClass
     *     {
     *         static inline vk::Buffer staticMember;
     *
     *         static inline StaticInit _init{
     *             []() {
     *                 staticMember = ...
     *             },
     *             []() {
     *                 vkDestroyBuffer(staticMember);
     *             }
     *         };
     *     };
     */
    class StaticInit
    {
    public:
        explicit StaticInit(std::function<void()> init);
        StaticInit(std::function<void()> init, std::function<void()> destroy);

        static void executeStaticInitializers();
        static void executeStaticDestructors();

    private:
        static inline bool isInitialized{ false };
        static inline std::vector<std::function<void(void)>> onInitCallbacks;
        static inline std::vector<std::function<void(void)>> onDestroyCallbacks;
    };
} // namespace vkb
