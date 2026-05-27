# Build and Run Commands

## Configure (Debug, C23)

```powershell
cmake -S . -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
```

## Build

```powershell
cmake --build cmake-build-debug
```

## Run testbed

```powershell
.\cmake-build-debug\bin\testbed.exe
```

## Run Tests

```powershell
ctest --test-dir cmake-build-debug --output-on-failure
```

**Notes**:
- During configuration a local cstdlib checkout is preferred and added as a subdirectory (producing the shared cstdlib library alongside the Quark DLL).
- Two GLSL shaders are compiled to SPIR-V at build time using `glslc` from the installed Vulkan SDK.
