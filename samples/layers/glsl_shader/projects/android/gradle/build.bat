@echo off

set BUILD_DIR="../../../../../../build/android/gradle/layers/glsl_shader/"

call gradlew --project-cache-dir %BUILD_DIR%.gradle build_glslang build

call adb push %BUILD_DIR%build\outputs\native\debug\all\lib\armeabi-v7a\libVkLayer_glsl_shader.so /data/local/debug/vulkan/libVkLayer_glsl_shader-armeabi-v7a.so
call adb push %BUILD_DIR%build\outputs\native\debug\all\lib\arm64-v8a\libVkLayer_glsl_shader.so /data/local/debug/vulkan/libVkLayer_glsl_shader-arm64-v8a.so
call adb push %BUILD_DIR%build\outputs\native\debug\all\lib\x86\libVkLayer_glsl_shader.so /data/local/debug/vulkan/libVkLayer_glsl_shader-x86.so
call adb push %BUILD_DIR%build\outputs\native\debug\all\lib\x86_64\libVkLayer_glsl_shader.so /data/local/debug/vulkan/libVkLayer_glsl_shader-x86_64.so
