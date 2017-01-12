#!/bin/bash

mkdir -p ../Vulkan-LoaderAndValidationLayers/build/generated/include
python3 ../Vulkan-LoaderAndValidationLayers/scripts/lvl_genvk.py -registry ../Vulkan-LoaderAndValidationLayers/scripts/vk.xml vk_dispatch_table_helper.h -o ../Vulkan-LoaderAndValidationLayers/build/generated/include/

samples/apps/atw/projects/android/gradle/atw_cpu_dsp/gradlew -b samples/apps/atw/projects/android/gradle/atw_cpu_dsp/build.gradle --project-cache-dir build/android/gradle/apps/atw_cpu_dsp/.gradle build
samples/apps/atw/projects/android/gradle/atw_opengl/gradlew -b samples/apps/atw/projects/android/gradle/atw_opengl/build.gradle --project-cache-dir build/android/gradle/apps/atw_opengl/.gradle build
samples/apps/atw/projects/android/gradle/atw_vulkan/gradlew -b samples/apps/atw/projects/android/gradle/atw_vulkan/build.gradle --project-cache-dir build/android/gradle/apps/atw_vulkan/.gradle build
samples/apps/driverinfo/projects/android/gradle/driverinfo_opengl/gradlew -b samples/apps/driverinfo/projects/android/gradle/driverinfo_opengl/build.gradle --project-cache-dir build/android/gradle/apps/driverinfo_opengl/.gradle build
samples/apps/driverinfo/projects/android/gradle/driverinfo_vulkan/gradlew -b samples/apps/driverinfo/projects/android/gradle/driverinfo_vulkan/build.gradle --project-cache-dir build/android/gradle/apps/driverinfo_vulkan/.gradle build
samples/layers/glsl_shader/projects/android/gradle/gradlew -b samples/layers/glsl_shader/projects/android/gradle/build.gradle --project-cache-dir build/android/gradle/layers/glsl_shader/.gradle build
samples/layers/queue_muxer/projects/android/gradle/gradlew -b samples/layers/queue_muxer/projects/android/gradle/build.gradle --project-cache-dir build/android/gradle/layers/queue_muxer/.gradle build
