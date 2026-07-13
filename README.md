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
- File comparison engine:
  - Compare current scan against baseline
  - Classify files as unchanged, modified, new, or deleted
  - Type-safe status using `ChangeType` enum
- Modular function design for future extensibility

---

## Planned Features

Future development will include:

- SQLite database storage for baselines (replacing JSON)
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
- JSON serialization and deserialization
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
./build/FileIntegrityMonitor.exe
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
```

The baseline is automatically saved as JSON for easy inspection and version control.

---

## Project Status

This project has completed the core logic layer:
- File scanning and hashing
- JSON-based baseline persistence
- Change detection and classification
- Modular function architecture

Next phases will focus on:
- Refactoring into separate header/implementation files
- SQLite integration for database-backed storage
- Qt6 GUI for desktop application interface
- Additional robustness and performance features

**Core principle**: The scanning and comparison logic remains independent of persistence and UI layers, ensuring clean architecture for future extensions.

---

### Author

- Tobit Vervat