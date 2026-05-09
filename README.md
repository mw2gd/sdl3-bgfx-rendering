# BgfxExample

A small SDL3 + bgfx sample that creates a resizable macOS window and redraws with bgfx during window size changes.

The CMake build fetches SDL3 and bgfx automatically with `FetchContent`, then builds `main.cpp` into the `BgfxExample` executable.

## Build

```sh
cmake -S . -B build
cmake --build build --target BgfxExample
```

## Run

```sh
./build/BgfxExample
```
