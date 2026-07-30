# TUI System Monitor

TUI: [cpp-tui](https://github.com/jonoton/cpp-tui), vendored in
`ThirdParty/cpptui.hpp`.

TUI System Monitor is a C++17 application that displays Linux system
statistics and running processes in a terminal interface.

## Build

Requirements: Linux, a C++17 compiler, CMake 3.20 or newer, and Git.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Run the application:

```bash
./build/tsm
```

Build and run the tests:

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```
