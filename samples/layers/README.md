
# Layers

Additional Vulkan functionality may be provided by layers and extensions.
While a Vulkan extension can add or modify Vulkan commands, a Vulkan layer only
deals with existing Vulkan commands. A layer is implemented as a separate
shared object or dynamically linked library. An extensions is typically
implemented as part of the graphics driver, but an extension can also be
implemented as part of a layer object.

The set of layers to enable is specified when an application creates an
instance/device. These layers are able to intercept any Vulkan command
dispatched to that instance/device or any of its child objects.
A layer is inserted into the call chain of any Vulkan commands the layer is
interested in. Multiple layers can be active at the same time and in some
cases the ordering of the layers is important.

Layers are typically used for validation. To avoid driver overhead,
implementations of the Vulkan API perform minimal to no validation.
Incorrect use of the API will often result in a crash. Validation
layers can be used during development to make sure the Vulkan API
is used correctly. Khronos provides a set of validation layers here:
[Vulkan-LoaderAndValidationLayers](https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers)

Layers can also be used to privide performance telemetry and analysis.
In other cases, a layer can be used to make an instance or device appear
differently to an application.

# Layer Compilation

Either the [Vulkan SDK](https://lunarg.com/vulkan-sdk/) or a collocated
[Vulkan-LoaderAndValidationLayers](https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers)
repository right next to the Vulkan-Samples repository is needed.

### Windows

To compile on Windows use the regular CMake procedure.

### Linux

To compile on Linux use the regular CMake procedure.

### Android

To compile on Android use the project files in projects/android.

# Layer Installation

### Windows 

Add a reference to VkLayer_&lt;name&gt;.json to the registry key:

    HKEY_LOCAL_MACHINE\SOFTWARE\Khronos\Vulkan\ExplicitLayers

Alternatively, use the VK_LAYER_PATH environment variable to specify where the layer library resides.

### Linux

Place the VkLayer_&lt;name&gt;.json and libVkLayer_&lt;name&gt;.so in one of the following folders:

    /usr/share/vulkan/icd.d
    /etc/vulkan/icd.d
    $HOME/.local/share/vulkan/icd.d

Where $HOME is the current home directory of the application's user id; this path will be ignored for suid programs.

Alternatively, use the VK_LAYER_PATH environment variable to specify where the layer library resides.

### Android

On Android copy the libVkLayer_&lt;name&gt;.so file to the application's lib folder:

    src/main/jniLibs/
        arm64-v8a/
            libVkLayer_<name>.so
            ...
        armeabi-v7a/
            libVkLayer_<name>.so
            ...

Recompile the application and use the jar tool to verify that the libraries are actually in the APK:

	jar -tf <filename>.apk

Alternatively, on a device with root access, the libVkLayer_&lt;name&gt;.so can be placed in

	/data/local/debug/vulkan/

The Android loader queries layer and extension information directly from the respective libraries,
and does not use JSON manifest files as used by the Windows and Linux loaders.

# Layer Activation

To enable the layer, the name of the layer ("VK_LAYER_&lt;COMPANY&gt;_&lt;name&gt;") should be added
to the ppEnabledLayerNames member of VkInstanceCreateInfo when creating a VkInstance,
and the ppEnabledLayerNames member of VkDeviceCreateInfo when creating a VkDevice.

### Windows

Alternatively, on Windows the layer can be enabled for all applications by adding the layer name
("VK_LAYER_&lt;COMPANY&gt;_&lt;name&gt;") to the VK_INSTANCE_LAYERS and VK_DEVICE_LAYERS environment variables.

    set VK_INSTANCE_LAYERS=VK_LAYER_<COMPANY>_<name>
    set VK_DEVICE_LAYERS=VK_LAYER_<COMPANY>_<name>

Multiple layers can be enabled simultaneously by separating them with colons.

    set VK_INSTANCE_LAYERS=VK_LAYER_OCULUS_queue_muxer:VK_LAYER_LUNARG_core_validation
    set VK_DEVICE_LAYERS=VK_LAYER_OCULUS_queue_muxer:VK_LAYER_LUNARG_core_validation

### Linux

Alternatively, on Linux the layer can be enabled for all applications by adding the layer name
("VK_LAYER_&lt;COMPANY&gt;_&lt;name&gt;") to the VK_INSTANCE_LAYERS and VK_DEVICE_LAYERS environment variables.

    export VK_INSTANCE_LAYERS=VK_LAYER_<COMPANY>_<name>
    export VK_DEVICE_LAYERS=VK_LAYER_<COMPANY>_<name>

Multiple layers can be enabled simultaneously by separating them with colons.

    export VK_INSTANCE_LAYERS=VK_LAYER_OCULUS_queue_muxer:VK_LAYER_LUNARG_core_validation
    export VK_DEVICE_LAYERS=VK_LAYER_OCULUS_queue_muxer:VK_LAYER_LUNARG_core_validation

### Android

Alternatively on Android the layer can be enabled for all applications using
the debug.vulkan.layers system property:

	adb shell setprop debug.vulkan.layers VK_LAYER_<COMPANY>_<name>

Multiple layers can be enabled simultaneously by separating them with colons.

	adb shell setprop debug.vulkan.layers VK_LAYER_OCULUS_queue_muxer:VK_LAYER_LUNARG_core_validation
