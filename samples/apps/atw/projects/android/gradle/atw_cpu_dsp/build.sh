#!/bin/bash

set BUILD_DIR="../../../../../../../build/android/gradle/apps/atw_cpu_dsp/"

gradlew --project-cache-dir ${BUILD_DIR}.gradle build
adb push ${BUILD_DIR}build/libs/armeabi-v7a/atw_cpu_dsp /data/local
adb shell chmod 777 /data/local/atw_cpu_dsp
adb shell mkdir -p /sdcard/atw/images/
adb shell rm /sdcard/atw/images/*
adb shell /data/local/atw_cpu_dsp
adb pull /sdcard/atw/images/ ${BUILD_DIR}images/
