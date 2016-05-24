
# Description

This layer implements queue multiplexing.

Some Vulkan drivers support only a single queue family with a single queue.
Applications that use more than one queue will not work with these Vulkan drivers.

This layer makes all Vulkan devices look like they have at least 16 queues per family.
There is virtually no impact on performance when the application only uses the queues that are exposed by the Vulkan driver.
If an application uses more queues than are exposed by the Vulkan driver,
then additional virtual queues are used that all map to the last physical queue.
When this happens there may be a noticeable impact on performance, but at least the application will work.
The impact on performance automatically goes away when new Vulkan drivers are
installed with support for more queues, even if an application continues to use this layer.

# Compilation

Either the [Vulkan SDK](https://lunarg.com/vulkan-sdk/) or a collocated
[Vulkan-LoaderAndValidationLayers](https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers)
repository right next to the Vulkan-Samples repository is needed to compile this layer.

### Windows

To compile on Windows use the regular CMake procedure.

### Linux

To compile on Linux use the regular CMake procedure.

### Android

To compile on Android use the project files in projects/android.

# Installation

### Windows 

Add a reference to VkLayer_queue_muxer.json to the registry key:

    HKEY_LOCAL_MACHINE\SOFTWARE\Khronos\Vulkan\ExplicitLayers

Alternatively, use the VK_LAYER_PATH environment variable to specify where the layer library resides.

### Linux

Place the VkLayer_queue_muxer.json and libVkLayer_queue_muxer.so in one of the following folders:

    /usr/share/vulkan/icd.d
    /etc/vulkan/icd.d
    $HOME/.local/share/vulkan/icd.d

Where $HOME is the current home directory of the application's user id; this path will be ignored for suid programs.

Alternatively, use the VK_LAYER_PATH environment variable to specify where the layer library resides.

### Android

On Android copy the libVkLayer_queue_muxer.so file to the application's lib folder:

    src/main/jniLibs/
        arm64-v8a/
            libVkLayer_queue_muxer.so
            ...
        armeabi-v7a/
            libVkLayer_queue_muxer.so
            ...

Recompile the application and use the jar tool to verify that the libraries are actually in the APK:

	jar -tf <filename>.apk

Alternatively, on a device with root access, the libVkLayer_queue_muxer.so can be placed in

	/data/local/debug/vulkan/

The Android loader queries layer and extension information directly from the respective libraries,
and does not use JSON manifest files as used by the Windows and Linux loaders.

# Activation

To enable the layer, the name of the layer ("VK_LAYER_OCULUS_queue_muxer") should be added
to the ppEnabledLayerNames member of VkInstanceCreateInfo when creating a VkInstance,
and the ppEnabledLayerNames member of VkDeviceCreateInfo when creating a VkDevice.

### Windows

Alternatively, on Windows the layer can be enabled for all applications by adding the layer name
("VK_LAYER_OCULUS_queue_muxer") to the VK_INSTANCE_LAYERS and VK_DEVICE_LAYERS environment variables.

    set VK_INSTANCE_LAYERS=VK_LAYER_OCULUS_queue_muxer
    set VK_DEVICE_LAYERS=VK_LAYER_OCULUS_queue_muxer

Multiple layers can be enabled simultaneously by separating them with colons.

    set VK_INSTANCE_LAYERS=VK_LAYER_OCULUS_queue_muxer:VK_LAYER_LUNARG_core_validation
    set VK_DEVICE_LAYERS=VK_LAYER_OCULUS_queue_muxer:VK_LAYER_LUNARG_core_validation

### Linux

Alternatively, on Linux the layer can be enabled for all applications by adding the layer name
("VK_LAYER_OCULUS_queue_muxer") to the VK_INSTANCE_LAYERS and VK_DEVICE_LAYERS environment variables.

    export VK_INSTANCE_LAYERS=VK_LAYER_OCULUS_queue_muxer
    export VK_DEVICE_LAYERS=VK_LAYER_OCULUS_queue_muxer

Multiple layers can be enabled simultaneously by separating them with colons.

    export VK_INSTANCE_LAYERS=VK_LAYER_OCULUS_queue_muxer:VK_LAYER_LUNARG_core_validation
    export VK_DEVICE_LAYERS=VK_LAYER_OCULUS_queue_muxer:VK_LAYER_LUNARG_core_validation

### Android

Alternatively on Android the layer can be enabled for all applications using
the debug.vulkan.layers system property:

	adb shell setprop debug.vulkan.layers VK_LAYER_OCULUS_queue_muxer

Multiple layers can be enabled simultaneously by separating them with colons.

	adb shell setprop debug.vulkan.layers VK_LAYER_OCULUS_queue_muxer:VK_LAYER_LUNARG_core_validation
