# TUI System Monitor

TUI: [cpp-tui](https://github.com/jonoton/cpp-tui), vendored in
`ThirdParty/cpptui.hpp`.

TUI System Monitor is a C++17 application that displays Linux system
statistics and running processes in a terminal interface.

## Build

Requirements: Linux, a C++17 compiler, and CMake 3.20 or newer.

```bash
cmake -S . -B build
cmake --build build
```

```bash
./build/tsm
```
