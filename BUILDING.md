# How to Build

## Windows 64-bit

```
mkdir build\win64
cd build\win64
cmake -G "Visual Studio 12 Win64" ..\..
```

Open the build\win64\VULKAN_SAMPLES.sln in the Visual Studio to build the samples.

## Windows 32-bit

```
mkdir build\win32
cd build\win32
cmake -G "Visual Studio 12" ..\..
```

Open the build\win32\VULKAN_SAMPLES.sln in the Visual Studio to build the samples.

## Linux Debug

```
cmake -H. -Bbuild/linux_debug -DCMAKE_BUILD_TYPE=Debug
cd build/linux_debug
make
```

## Linux Release

```
cmake -H. -Bbuild/linux_release -DCMAKE_BUILD_TYPE=Release
cd build/linux_release
make
```

## Android

Use the build files in the projects/android/ folder inside the sample's folder.

The output will be placed in the build/android/ folder in the base of the repository.