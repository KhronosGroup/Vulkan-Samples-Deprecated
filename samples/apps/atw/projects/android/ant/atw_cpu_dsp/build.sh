#!/bin/bash

set BUILD_DIR="../../../../../../../build/android/ant/apps/atw_cpu_dsp/"

ndk-build NDK_LIBS_OUT=${BUILD_DIR}libs NDK_OUT=${BUILD_DIR}obj
adb push ${BUILD_DIR}build/libs/armeabi-v7a/atw_cpu_dsp /data/local
adb shell chmod 777 /data/local/atw_cpu_dsp
adb shell mkdir -p /sdcard/atw/images/
adb shell rm /sdcard/atw/images/*
adb shell /data/local/atw_cpu_dsp
adb pull /sdcard/atw/images/ ${BUILD_DIR}images/
