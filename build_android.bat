mkdir ..\Vulkan-LoaderAndValidationLayers\build\layers
python ..\Vulkan-LoaderAndValidationLayers\vk-generate.py Android dispatch-table-ops layer > "..\Vulkan-LoaderAndValidationLayers\build\layers\vk_dispatch_table_helper.h"

call %ANDROID_NDK_HOME%\ndk-build -C samples\apps\atw\projects\android\ndk\atw_cpu_dsp\ NDK_LIBS_OUT=..\..\..\..\..\..\..\build\android\ndk\apps\atw_cpu_dsp\libs NDK_OUT=..\..\..\..\..\..\..\build\android\ndk\apps\atw_cpu_dsp\obj
call %ANDROID_NDK_HOME%\ndk-build -C samples\apps\atw\projects\android\ndk\atw_opengl\ NDK_LIBS_OUT=..\..\..\..\..\..\..\build\android\ndk\apps\atw_opengl\libs NDK_OUT=..\..\..\..\..\..\..\build\android\ndk\apps\atw_opengl\obj
call %ANDROID_NDK_HOME%\ndk-build -C samples\apps\atw\projects\android\ndk\atw_vulkan\ NDK_LIBS_OUT=..\..\..\..\..\..\..\build\android\ndk\apps\atw_vulkan\libs NDK_OUT=..\..\..\..\..\..\..\build\android\ndk\apps\atw_vulkan\obj
call %ANDROID_NDK_HOME%\ndk-build -C samples\apps\driverinfo\projects\android\ndk\driverinfo_opengl\ NDK_LIBS_OUT=..\..\..\..\..\..\..\build\android\ndk\apps\driverinfo_opengl\libs NDK_OUT=..\..\..\..\..\..\..\build\android\ndk\apps\driverinfo_opengl\obj
call %ANDROID_NDK_HOME%\ndk-build -C samples\apps\driverinfo\projects\android\ndk\driverinfo_vulkan\ NDK_LIBS_OUT=..\..\..\..\..\..\..\build\android\ndk\apps\driverinfo_vulkan\libs NDK_OUT=..\..\..\..\..\..\..\build\android\ndk\apps\driverinfo_vulkan\obj
call %ANDROID_NDK_HOME%\ndk-build -C samples\layers\glsl_shader\projects\android\ndk\ NDK_LIBS_OUT=..\..\..\..\..\..\build\android\ndk\layers\glsl_shader\libs NDK_OUT=..\..\..\..\..\..\build\android\ndk\layers\glsl_shader\obj
call %ANDROID_NDK_HOME%\ndk-build -C samples\layers\queue_muxer\projects\android\ndk\ NDK_LIBS_OUT=..\..\..\..\..\..\build\android\ndk\layers\queue_muxer\libs NDK_OUT=..\..\..\..\..\..\build\android\ndk\layers\queue_muxer\obj

call samples\apps\atw\projects\android\gradle\atw_cpu_dsp\gradlew -b samples\apps\atw\projects\android\gradle\atw_cpu_dsp\build.gradle --project-cache-dir build\android\gradle\apps\atw_cpu_dsp\.gradle build
call samples\apps\atw\projects\android\gradle\atw_opengl\gradlew -b samples\apps\atw\projects\android\gradle\atw_opengl\build.gradle --project-cache-dir build\android\gradle\apps\atw_opengl\.gradle build
call samples\apps\atw\projects\android\gradle\atw_vulkan\gradlew -b samples\apps\atw\projects\android\gradle\atw_vulkan\build.gradle --project-cache-dir build\android\gradle\apps\atw_vulkan\.gradle build
call samples\apps\driverinfo\projects\android\gradle\driverinfo_opengl\gradlew -b samples\apps\driverinfo\projects\android\gradle\driverinfo_opengl\build.gradle --project-cache-dir build\android\gradle\apps\driverinfo_opengl\.gradle build
call samples\apps\driverinfo\projects\android\gradle\driverinfo_vulkan\gradlew -b samples\apps\driverinfo\projects\android\gradle\driverinfo_vulkan\build.gradle --project-cache-dir build\android\gradle\apps\driverinfo_vulkan\.gradle build
call samples\layers\glsl_shader\projects\android\gradle\gradlew -b samples\layers\glsl_shader\projects\android\gradle\build.gradle --project-cache-dir build\android\gradle\layers\glsl_shader\.gradle build
call samples\layers\queue_muxer\projects\android\gradle\gradlew -b samples\layers\queue_muxer\projects\android\gradle\build.gradle --project-cache-dir build\android\gradle\layers\queue_muxer\.gradle build
