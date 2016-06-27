
# Layers

Additional Vulkan functionality may be provided by layers and extensions.
While a Vulkan extension can add or modify Vulkan commands, a Vulkan layer only
deals with existing Vulkan commands. A layer is implemented as a separate
shared object or dynamically linked library. An extensions is typically
implemented as part of the graphics driver. However, an extension can also
be implemented as part of a layer object.

Layers are typically used for validation. To avoid driver overhead,
implementations of the Vulkan API perform minimal to no validation.
Incorrect use of the API will often result in a crash. Validation
layers can be used during development to make sure the Vulkan API
is used correctly. Khronos provides a set of validation layers here:
[Vulkan-LoaderAndValidationLayers](https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers).
Layers can also be used to privide performance telemetry and analysis.
In other cases, a layer can be used to make an instance or device appear
differently to an application.

The set of layers to enable is specified when an application creates an
instance/device. These layers are able to intercept any Vulkan command
dispatched to that instance/device or any of its child objects.
A layer is inserted into the call chain of any Vulkan commands the layer is
interested in. Multiple layers can be active at the same time and in some
cases the ordering of the layers is important.

More information about layers can be found here [LoaderAndLayerInterface](https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers/blob/master/loader/LoaderAndLayerInterface.md).

# Layer Compilation

To compile the layers, a collocated [glslang](https://github.com/KhronosGroup/glslang) repository,
and either the [Vulkan SDK](https://lunarg.com/vulkan-sdk/) or a collocated
[Vulkan-LoaderAndValidationLayers](https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers)
repository are needed. The paths for the collocated repositories will look as follows:

    <path>/Vulkan-Samples/
    <path>/Vulkan-LoaderAndValidationLayers/
    <path>/glslang/

On Windows make sure that the &lt;path&gt; is no more than one folder deep to
avoid running into maximum path depth compilation issues.

### Windows

To compile on Windows use the regular CMake procedure.

### Linux

To compile on Linux use the regular CMake procedure.

### Android

To compile on Android use the project files in projects/android.

# Layer Installation

### Windows 

Add a reference to VkLayer_&lt;name&gt;.json to one of the registry keys:

    HKEY_LOCAL_MACHINE\SOFTWARE\Khronos\Vulkan\ExplicitLayers
    HKEY_LOCAL_MACHINE\SOFTWARE\Khronos\Vulkan\ImplicitLayers

Each key name must be a full path to the JSON manifest file.
The Vulkan loader opens the JSON manifest file specified by the key name.
The value of the key is a DWORD data set to 0.

Implicit layers are enabled automatically, whereas explicit layers must be enabled explicitly by
the application.

Alternatively, use the VK_LAYER_PATH environment variable to specify where the layer library resides.

### Linux

Place the VkLayer_&lt;name&gt;.json and libVkLayer_&lt;name&gt; .so in one of the following folders:

    /usr/share/vulkan/explicit_layer.d
    /usr/share/vulkan/implicit_layer.d
    /etc/vulkan/explicit_layer.d
    /etc/vulkan/implicit_layer.d
    \$HOME/.local/share/vulkan/explicit_layer.d
    \$HOME/.local/share/vulkan/implicit_layer.d

Where $HOME is the current home directory of the application's user id; this path will be ignored for suid programs.

Implicit layers are enabled automatically, whereas explicit layers must be enabled explicitly by
the application.

Alternatively, use the VK_LAYER_PATH environment variable to specify where the layer library resides.

### Android

On Android copy the libVkLayer_&lt;name&gt; .so file to the application's lib folder:

    src/main/jniLibs/
        arm64-v8a/
            libVkLayer_<name>.so
            ...
        armeabi-v7a/
            libVkLayer_<name>.so
            ...

Recompile the application and use the jar tool to verify that the libraries are actually in the APK:

	jar -tf <filename>.apk

The output of the jar tool would look similar to the following:

    > jar -tf atw_vulkan-all-debug.apk
    AndroidManifest.xml
    classes.dex
    lib/arm64-v8a/libatw_vulkan.so
    lib/arm64-v8a/libVkLayer_queue_muxer.so
    lib/armeabi-v7a/libatw_vulkan.so
    lib/armeabi-v7a/libVkLayer_queue_muxer.so
    lib/x86/libatw_vulkan.so
    lib/x86/libVkLayer_queue_muxer.so
    lib/x86_64/libatw_vulkan.so
    lib/x86_64/libVkLayer_queue_muxer.so
    META-INF/MANIFEST.MF
    META-INF/CERT.SF
    META-INF/CERT.RSA

Notice the libVkLayer_queue_muxer.so right next to the libatw_vulkan.so in the APK.

Alternatively, on a device with root access, the libVkLayer_&lt;name&gt; .so can be placed in

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

Multiple layers can be enabled simultaneously by separating them with semi-colons.

    set VK_INSTANCE_LAYERS=VK_LAYER_OCULUS_queue_muxer;VK_LAYER_LUNARG_core_validation
    set VK_DEVICE_LAYERS=VK_LAYER_OCULUS_queue_muxer;VK_LAYER_LUNARG_core_validation

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
the <i>debug.vulkan.layers</i> system property:

	adb shell setprop debug.vulkan.layers VK_LAYER_<COMPANY>_<name>

Multiple layers can be enabled simultaneously by separating them with colons.

	adb shell setprop debug.vulkan.layers VK_LAYER_OCULUS_queue_muxer:VK_LAYER_LUNARG_core_validation
