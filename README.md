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

You'll need the following libraries and tools:

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

### MacOS

As far as I know, Apple doesn't support Vulkan :( If they do at some point and I forget to update this readme, you can
google how the packages are called in homebrew and install them analogously to the other operating systems. Then proceed
with the installation instructions below.

### Windows

I'm not a masochist, so I don't provide a script or a similar facility to set up the environment for Windows. Life is too
short. You can either get a proper operating system or adapt the CMakeLists.txt to find your libraries.

### CMake options

 - `TORCH_BUILD_TEST`: Control if tests should be built. Default is `OFF`.

 - `TORCH_DEBUG`: Control if Torch should be built in debug configuration. Default is `OFF`.

 - `TORCH_USE_FBX_SDK`: Control if Torch should be built with the FBX SDK. If disabled, certain mesh-import
functionality that is based on the FBX SDK will not be available. I added this option because installing that sdk can be
a huge pain.

### Installation

After installing the dependencies, the installation is straight-forward:

    git clone https://github.com/uCantHim/Torch.git
    cd Torch
    ./build.sh

The `build.sh` script does the following (do this if you don't have bash):

    mkdir build
    cd build/
    cmake ..
    cmake --build . --parallel 8


<a name="example"></a>
Small Example
-------------

```c++
#include <trc/Torch.h>

int main()
{
    // Initialize Torch and create a renderer. Do have to do this before
    // you use any other functionality of Torch.
    auto renderer = trc::init();

    // The two things required to render something are
    //   1. A scene
    //   2. A camera
    // The scene is created as a unique_ptr instead of value here so we can
    // easily delete it at the end. That's not necessary for the camera
    // since it doesn't allocate any Vulkan resources.
    auto scene = std::make_unique<trc::Scene>();
    trc::Camera camera;

    // Main loop
    while (vkb::getSwapchain().isOpen())
    {
        // Poll system events. The weird namespace name is explained below.
        vkb::pollEvents();

        // Use the renderer to draw the scene
        renderer->drawFrame(*scene, camera);
    }

    // Destroy the Torch resources
    renderer.reset();
    scene.reset();

    // Call this after you've destroyed all Torch/Vulkan resources.
    trc::terminate();

    return 0;
}
```


<a name="overview"></a>
Overview
--------

### Drawable

`trc::Drawable` represents the most general type of drawable object. It always has a geometry and a material. The full
process of creating a Drawable and the required assets:

```c++
// Load a geometry from a file. Only FBX format is supported at the moment.
trc::GeometryID geo = trc::AssetRegistry::addGeometry(trc::loadGeometry("my_geo.fbx").get());

// Add a material
trc::MaterialID mat = trc::AssetRegistry::addMaterial({ .color=vec4(0, 1, 0.4, 1) });
trc::AssetRegistry::updateMaterials();

// Create the drawable
trc::Drawable myDrawable(geo, mat);
```

### Scene graph architecture

TODO: write this section


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

```c++
#include <vkb/Event.h>

vkb::on<vkb::KeyPressEvent>([](const vkb::KeyPressEvent& e) {
    std::cout << "Key pressed: " << e.key << "\n";
});
```

It's possible to fire and listen to any type of event:

```c++
struct MyEventType
{
    int number;
    std::string description;
};

vkb::on<MyEventType>([](const auto& e) {
    std::cout << e.description << ": " << e.number << "\n";
});

vkb::fire<MyEventType>({ 42, "The answer" });
```

The `vkb::on` and `vkb::fire` functions are helpers on top of some underlying, less expressive structures. They are
roughly equivalent to the following:

```c++
// What vkb::on does:
vkb::EventHandler<MyEventType>::addListener([](const MyEventType& e) { ... });

// What vkb::fire does:
vkb::EventHandler<MyEventType>::notify({ 42, "The answer" });
```

`vkb::on` also wraps the result of `EventHandler<>::addListener` in a temporary object that can be used to create unique
listener handles in case you want to remove the listener later on:

```c++
{
    auto uniqueHandle = vkb::on<MyEventType>([](auto&&) { ... }).makeUnique();
} // uniqueHandle gets destroyed and the listener is removed from the event handler
```
