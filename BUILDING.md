# How to Build

## Windows 64-bit

```
mkdir build
cd build
cmake -G "Visual Studio 12 Win64" ..
```

Open the VULKAN_SAMPLES.sln in the Visual Studio to build the samples.

## Windows 32-bit

```
mkdir build32
cd build32
cmake -G "Visual Studio 12" ..
```

Open the VULKAN_SAMPLES.sln in the Visual Studio to build the samples.

## Linux Debug

```
cmake -H. -Bbuild/Debug -DCMAKE_BUILD_TYPE=Debug
cd build/Debug
make
```

## Linux Release

```
cmake -H. -Bbuild/Release -DCMAKE_BUILD_TYPE=Release
cd build/Release
make
```

## Android

Use the build files in the projects/android/ folder inside the sample's folder.
