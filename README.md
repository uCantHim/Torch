Torch
=====

10/10 best Vulkan 3D rendering engine

Table of Contents
-----------------

1. [Installation](#installation)
2. [Example](#example)
3. [Overview](#overview)
4. [Vulkan Base Library](#vkb)


<a name="installation"></a>
Installation
------------

You'll need the following libraries:

 - Vulkan

 - GLFW

 - GLM

 - DevIL

 - FBX SDK (Installing this one can be a pain because Autodesk is not a good company. There's a CMake option that
disables the use of this library)

 - glslangValidator (for shader compilation)

### Arch Linux

    $ sudo pacman -S vulkan-devel glfw3-x11 glm devil glslang

Replace `glfw-x11` with `glfw-wayland` for Wayland-based desktop environments.

You can find a heavily outdated version of the FBX SDK on the [AUR](https://aur.archlinux.org/packages/fbx-sdk/)

### Ubuntu

    $ sudo apt install libvulkan-dev libglfw3-dev libglm-dev libdevil-dev glslang-tools

### CMake options

 - `TORCH_BUILD_TEST`: Control if tests should be built. Default is `ON`.

 - `TORCH_DEBUG`: Control if Torch should be built in debug configuration. Default is `OFF`.

 - `TORCH_USE_FBX_SDK`: Control if Torch should be built with the FBX SDK. If disabled, certain mesh-import
functionality that is based on the FBX SDK will not be available. I added this option because installing that sdk can be
a huge pain.


<a name="example"></a>
Example
-------

    #include <trc/Renderer.h>

    int main()
    {
        // Initialize basic Vulkan functionality. You can find a detailed
        // explanation as to why this is necessary and about the weird
        // namespace name below.
        vkb::vulkanInit();

        // Create a renderer. This initializes most of Torch's internal
        // resources.
        trc::Renderer renderer;

        // The two things required to render something are
        //   1. A scene and
        //   2. A camera
        trc::Scene scene;
        trc::Camera camera;

        // Main loop
        while (true)
        {
            // Poll system events. Again, namespace explained below.
            vkb::pollEvent();

            // Use the renderer to draw the scene
            renderer.draw(scene, camera);
        }

        // You have to destroy all your Vulkan resources here. I could have used unique_ptrs
        // for the renderer, scene, and camera, and called reset on every one of them, but I
        // went for better readability instead.
        // After destroying all your Torch/Vulkan resouces, call:
        trc::terminate();

        return 0;
    }


<a name="overview"></a>
Overview
--------

### Drawable

`trc::Drawable` represents the most general type of drawable object. It always has a geometry and a material. The full
process of creating a Drawable and the required assets:

    // Load a geometry from a file. Only FBX format is supported at the moment.
    trc::GeometryID geo = trc::AssetRegistry::addGeometry(trc::loadGeometry("my_geo.fbx").get());

    // Add a material
    trc::MaterialID mat = trc::AssetRegistry::addMaterial({ .color=vec4(0, 1, 0.4, 1) });
    trc::AssetRegistry::updateMaterials();

    // Create the drawable
    trc::Drawable myDrawable(geo, mat);

### Scene graph architecture

TODO


<a name="vkb"></a>
vkb Aka Vulkan Base
-------------------

Vulkan Base is a library with basic Vulkan utilities that I have not yet put into its own repository. That's also why
the name is as original as MGK's lyrics.

It's based on GLFW for window creation and event handling.

### Setup

Most importantly, it contains classes that alleviate most of the tedious boilerplate work. Examples on how to use these
independently from Torch will be located in a separate documentation about Vulkan Base once I find both the time and
motivation to write one.

The call to `vkb::vulkanInit()` automatically initializes an instance, device, and swapchain. These are made available
globally and are used by Torch. If you want to do complex Vulkan stuff yourself, I suggest you create another device
just for yourself.

### Events

Vulkan Base provides an extremely cool event system. The easiest way to register an event handler for a specific type of
event is the `vkb::on` function template:

    #include <vkb/Event.h>

    vkb::on<vkb::KeyPressEvent>([](const vkb::KeyPressEvent& e) {
        std::cout << "Key pressed: " << e.key << "\n";
    });

It's possible to fire and listen to any type of event:

    struct MyEventType
    {
        int number;
        std::string description;
    };

    vkb::on<MyEventType>([](const auto& e) {
        std::cout << e.description << ": " << e.number << "\n";
    });

    vkb::fire<MyEventType>({ 42, "The answer" });

The `vkb::on` and `vkb::fire` functions are helpers on top of some underlying, less expressive structures. They are
roughly equivalent to the following:

    // What vkb::on does:
    vkb::EventHandler<MyEventType>::addListener([](const MyEventType& e) { ... });

    // What vkb::fire does:
    vkb::EventHandler<MyEventType>::notify({ 42, "The answer" });

`vkb::on` also wraps the result of `EventHandler<>::addListener` in a temporary object that can be used to create unique
listener handles in case you want to remove the listener later on:

    {
        auto uniqueHandle = vkb::on<MyEventType>([](auto&&) { ... }).makeUnique();
    } // uniqueHandle gets destroyed and the listener is removed from the event handler
