# Multithreaded File Search Engine (C++)

A high-performance C++ file indexing and search tool with a Qt GUI.

## Features
- Multithreaded indexing with a custom thread pool
- Safe concurrent merging using mutexes
- Interactive search (CLI and GUI)
- Benchmark mode to measure scalability
- Skips non-text and large files for real-world performance
- Responsive Qt GUI with background indexing

## Tech Stack
- C++17
- CMake
- Qt6
- std::filesystem, std::thread, mutex

## Build
```bash
cmake -S . -B build
cmake --build build
```
## Run
```bash
./build/search_engine_gui
```

