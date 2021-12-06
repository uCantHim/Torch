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
    {
        // Initialize Torch
        trc::TorchStack torch = trc::initFull();

        // The things required to render something are
        //   1. A scene
        //   2. A camera
        trc::Scene scene;
        trc::Camera camera;

        // We create a draw configuration that tells the renderer what to
        // draw. We can create this object ourselves, but, since we will use
        // default settings anyway, we let a utility function do it for us.
        trc::DrawConfig drawConf = torch.makeDrawConfig(scene, camera);

        // Main loop
        while (torch.window->getSwapchain().isOpen())
        {
            // Poll system events
            trc::pollEvents();

            // Draw a frame
            torch.drawFrame(drawConf);
        }

        // End of scope, the TorchStack object gets destroyed
    }

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
auto& ar = *torch.assetRegistry;  // `torch` is the TorchStack object from above

// Load a geometry from a file. Only FBX format is supported at the moment.
trc::GeometryID geo = ar.add(trc::loadGeometry("my_geo.fbx").get());

// Add a material
trc::MaterialID mat = ar.add(trc::Material{ .color=vec4(0, 1, 0.4, 1) });
ar.updateMaterials();

// Create the drawable
trc::Drawable myDrawable(geo, mat);
```

### Scene graph architecture

TODO: write this section


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
