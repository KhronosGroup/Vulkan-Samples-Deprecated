@echo off

set BUILD_DIR="../../../../../../build/android/ant/layers/queue_muxer/"

call ndk-build NDK_LIBS_OUT=%BUILD_DIR%libs NDK_OUT=%BUILD_DIR%obj
