Torch
=====

10/10 - the objectively best 3D graphics engine

Table of Contents
-----------------

1. [Installation](#installation)
2. [Example](#example)
3. [Overview](#overview)


<a name="installation"></a>
Installation
------------

You'll need the following libraries and tools:

 - Vulkan

 - GLFW

 - GLM

 - Freetype

 - Protobuf

 - libPNG

I'm working on reducing the need for these external dependencies.

Optional dependencies that enable additional features:

 - FBX SDK for asset import from FBX files. Installing this one can be a pain. Set the CMake option `TORCH_USE_FBX_SDK`
to enable/disable integration of this functionality.

 - assimp for asset import from its supported file types. The build automatically detects and uses assimp installations.

### Arch Linux

    # pacman -S vulkan-devel glfw-x11 glm freetype2 libpng protobuf nlohmann-json

Replace `glfw-x11` with `glfw-wayland` for Wayland-based desktop environments.

You can find a heavily outdated version of the FBX SDK on the [AUR](https://aur.archlinux.org/packages/fbx-sdk/).

### Ubuntu

    # apt install libvulkan-dev libglfw3-dev libglm-dev libfreetype6-dev libpng-dev libprotobuf-dev nlohmann-json3-dev

### MacOS

I know that a Vulkan -> Metal compatibility layer exists, but I haven't tried how that works - mainly because I don't
own a Mac.

### Windows

I'll work on the Windows build in those moments when I want to feel pain.

### CMake options

 - `TORCH_BUILD_TEST`: Control if tests should be built. Default is `OFF`.

 - `TORCH_DEBUG`: Control if Torch should be built in debug configuration. Default is `OFF`.

 - `TORCH_INTEGRATE_IMGUI`: Build with ImGui support.

### Installation

After installing the dependencies, the installation is straight-forward:

    git clone https://github.com/uCantHim/torch.git
    cd torch
    mkdir build && cd build
    cmake ..
    cmake --build .


<a name="example"></a>
Small Example
-------------

Draw a triangle:

```c++
#include <trc/Torch.h>

int main()
{
    // Initialize Torch
    trc::init();

    // Create a default Torch setup
    trc::TorchStack torch;

    // The things required to render something are
    //   1. A scene
    //   2. A camera
    trc::Scene scene;
    trc::Camera camera;
    camera.makeOrthogonal(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);

    auto& assets = torch.getAssetManager();

    trc::GeometryID geo = assets.create(trc::makeTriangleGeo());
    trc::MaterialID mat = assets.create(trc::MaterialData{
        .color={ 0.392f, 0.624f, 0.82f },
        .doPerformLighting=false,
    });
    trc::Drawable myDrawable(geo, mat, scene);

    // Main loop
    while (torch.getWindow().isOpen())
    {
        // Poll system events
        trc::pollEvents();

        // Draw a frame
        torch.drawFrame(camera, scene);
    }

    // Call this after you've destroyed all Torch/Vulkan resources.
    trc::terminate();

    return 0;
}
```


<a name="overview"></a>
Overview
--------

### Events

Torch provides a cool event system. The easiest way to register an event handler for a specific type of event is the
`trc::on` function template:

```c++
#include <trc/base/event/Event.h>

trc::on<trc::KeyPressEvent>([](const trc::KeyPressEvent& e) {
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

trc::on<MyEventType>([](const auto& e) {
    std::cout << e.description << ": " << e.number << "\n";
});

trc::fire<MyEventType>({ 42, "The answer" });
```
