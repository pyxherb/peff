# PEFF - Portable Extended Freestanding Foundations

PEFF is a exception-free common library for C++ projects.

## Build

### CMake

To build PEFF with CMake, use commands like following (we use `bash` as an example):

```bash
cmake -S . -B build
cmake --build build --config <Your config>
```

And to install CMake, the command should be like following:

```bash
sudo cmake --install build --config <Your config>
```

For **Windows** you may have to use the **administrator privileged command prompt** instead of the `sudo` usage.
