# Requirements

These samples require a Vulkan compliant graphics driver.

To compile the layers, a collocated [glslang](https://github.com/KhronosGroup/glslang) repository,
and a collocated [Vulkan-LoaderAndValidationLayers](https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers)
repository are needed. The paths for the collocated repositories will look as follows:

    <path>/Vulkan-Samples/
    <path>/Vulkan-LoaderAndValidationLayers/
    <path>/glslang/

On Windows make sure that the &lt;path&gt; is no more than one folder deep to
avoid running into maximum path depth compilation issues.

## Windows

Windows 7+ with additional required software packages:

- Microsoft Visual Studio 2013 Professional.
  - Older versions may work, but this has not been tested.
- CMake (from http://www.cmake.org/download/).
  - Tell the installer to "Add CMake to the system PATH" environment variable.
- Vulkan SDK for Windows (from https://vulkan.lunarg.com).
  - Verify that the VK_SDK_PATH environment variable is set.

## Linux

A compatible Linux distribution.

- The samples have been tested with Ubuntu Linux 14.04 and later versions.
- Vulkan SDK for Linux (from https://vulkan.lunarg.com).

Samples using Xlib typically require the following packages.
```
sudo apt-get install libx11-dev
sudo apt-get install libxxf86vm-dev
sudo apt-get install libxrandr-dev
```

Samples using XCB typically require the following packages:
```
sudo apt-get install libxcb1-dev
sudo apt-get install libxcb-keysyms1-dev
sudo apt-get install libxcb-icccm4-dev
```

## Android

Android M or earlier.

- Android NDK Revision 11c or later (from https://github.com/android-ndk/ndk/wiki).
  - Older versions may work, but this has not been tested.
- Vulkan SDK for the host platform (from https://vulkan.lunarg.com).
  - Verify that the VK_SDK_PATH environment variable is set.

Android N or later.

- Android NDK Revision 12 beta 2 or later (from https://github.com/android-ndk/ndk/wiki).
- Read the Android Vulkan guides for more details (from http://developer.android.com/ndk/guides/graphics/index.html).
