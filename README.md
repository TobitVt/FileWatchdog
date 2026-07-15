# FileWatchdog (File Integrity Monitoring Tool - C++)

## Overview

FileWatchdog is a C++ console-based prototype for a file integrity monitoring system. It scans directories, extracts file metadata, and builds structured records that can later be used to detect changes in files over time.

This project is part of my learning path toward building a full cybersecurity-focused desktop application using Qt and C++.

---

## Purpose

The goal of this project is to simulate the core idea behind file integrity monitoring systems used in cybersecurity:

- Detect modified files
- Track new files
- Identify deleted files (future step)
- Maintain a trusted baseline of file states

---

## Current Features

This version implements the core file integrity monitoring workflow:

- Recursive directory scanning using `std::filesystem`
- Extraction of file metadata:
  - Absolute and relative paths
  - File size
  - Last modified timestamp
- SHA-256 hashing for cryptographic file verification
- Baseline persistence:
  - Save scan results to JSON format
  - Load previously saved baselines
  - Structured, human-readable JSON output
  - Automatically create parent folders for baseline files
- File comparison engine:
  - Compare current scan against baseline
  - Classify files as unchanged, modified, new, or deleted
  - Type-safe status using `ChangeType` enum
  - Print a simple summary of counts for each change category
- Command-line interface:
  - `create` mode to generate a baseline
  - `compare` mode to compare a folder against an existing baseline
  - `help` for usage information
- Modular function design for future extensibility
- SQLite database storage for baselines (replacing JSON)

---

## Planned Features

Future development will include:

- Support for multiple independent baselines
- Qt6 GUI dashboard with:
  - Folder selection
  - Scan and baseline creation buttons
  - Results displayed in a table with color-coding
  - Summary statistics and progress bar
- Export capabilities:
  - CSV export for scan results
  - JSON export for reports
- Advanced features:
  - Ignore rules for file types and folders
  - Error handling for locked/unreadable files
  - Performance optimizations for large folder hierarchies
- Automated testing suite

---

## Technologies Used

- **Language**: C++17
- **Build System**: CMake 3.16+
- **Standard Library**: `filesystem`, `chrono`, `vector`, `iostream`, `fstream`
- **Cryptography**: picosha2 (SHA-256 hashing)
- **Data Serialization**: nlohmann/json (single-header JSON library)
- **Compiler**: MSVC, MinGW, or GCC with C++17 support
- **Console-based output** (current version; Qt6 GUI planned)

---

## Key Concepts Practiced

- File system traversal with recursive directory iteration
- Data modeling using structs and enums
- Cryptographic hashing for integrity verification
- Comparison algorithms and change detection logic
- Type-safe status representation using `enum class`
- Modular function design and separation of concerns
- File I/O operations (text and structured formats)
- CMake-based project configuration
- Error handling and resource management

---

## How to Build & Run

### Requirements
- C++17 or later
- CMake 3.16 or later
- A compiler supporting `std::filesystem` (MSVC / MinGW / GCC)

### Build using CMake
```bash
cmake -S . -B build
cmake --build build
```

### Run
```bash
./build/FileIntegrityMonitor.exe help
./build/FileIntegrityMonitor.exe create "C:/path/to/folder" baseline.json
./build/FileIntegrityMonitor.exe compare "C:/path/to/folder" baseline.json
```

### Example Output
```
Scanned files:
docs/readme.txt | 1200 bytes
docs/guide.txt | 850 bytes
Total files scanned: 2

Compare results:
docs/readme.txt -> unchanged
docs/guide.txt -> modified
docs/newfile.txt -> new
docs/oldfile.txt -> deleted
Summary: unchanged=1, modified=1, new=1, deleted=1
```

The baseline is automatically saved as JSON for easy inspection and version control.

---

## Project Status

This project has completed the core logic layer:
- File scanning and hashing
- Change detection and classification
- Modular function architecture

Next phases will focus on:
- Refactoring into separate header/implementation files
- Qt6 GUI for desktop application interface
- Additional robustness and performance features

The current CLI layer is a stepping stone for the future GUI and will later be mirrored by buttons, dialogs, and a results table in the desktop app.

**Core principle**: The scanning and comparison logic remains independent of persistence and UI layers, ensuring clean architecture for future extensions.

---

### Author

- Tobit Vervat