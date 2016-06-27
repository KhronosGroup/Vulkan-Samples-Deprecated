@echo off

set BUILD_DIR="../../../../../../build/android/ndk/layers/glsl_shader/"

call ndk-build NDK_LIBS_OUT=%BUILD_DIR%libs NDK_OUT=%BUILD_DIR%obj

adb push %BUILD_DIR%libs\armeabi-v7a\libVkLayer_glsl_shader.so /data/local/debug/vulkan/libVkLayer_glsl_shader-armeabi-v7a.so
adb push %BUILD_DIR%libs\arm64-v8a\libVkLayer_glsl_shader.so /data/local/debug/vulkan/libVkLayer_glsl_shader-arm64-v8a.so
adb push %BUILD_DIR%libs\x86\libVkLayer_glsl_shader.so /data/local/debug/vulkan/libVkLayer_glsl_shader-x86.so
adb push %BUILD_DIR%libs\x86_64\libVkLayer_glsl_shader.so /data/local/debug/vulkan/libVkLayer_glsl_shader-x86_64.so
