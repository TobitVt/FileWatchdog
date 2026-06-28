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

## Current Features (Prototype Stage)

This version implements the first core building blocks:

- Recursive directory scanning using `std::filesystem`
- Extraction of file metadata:
  - Absolute path
  - Relative path
  - File size
  - Last modified timestamp
- Storage of file data in structured `FileRecord` objects
- Collection of file records using `std::vector`

---

## Planned Features

Future development will include:

- SHA-256 hashing for file integrity verification
- Baseline storage system (JSON / SQLite)
- Change detection (new / modified / deleted files)
- Qt GUI dashboard
- File comparison engine
- Export scan reports (CSV / JSON)

---

## Technologies Used

- C++
- C++ Standard Library (`filesystem`, `chrono`, `vector`)
- STL data structures
- Console-based output (current version)

---

## Key Concepts Practiced

- File system traversal
- Data modeling using structs
- Vector-based data storage
- Path manipulation (absolute vs relative paths)
- Basic system design for security tools

---

## How to Build & Run

### Requirements
- C++17 or later
- A compiler supporting `std::filesystem` (MSVC / MinGW / GCC)

### Build (example using g++)
- g++ main.cpp -o FileWatchdog

### Run:
- ./FileWatchdog

### example output
Path: C:/TestFolder/sub/a.txt
File size: 1200 bytes
Last modified time: 2026-06-25 14:32

Path: C:/TestFolder/sub/b.txt
File size: 850 bytes
Last modified time: 2026-06-25 14:33

Total files scanned: 12

---

### Notes

This project is an early-stage prototype. The focus is currently on building the scanning engine and data model before introducing GUI and persistence layers.

Core principle:
Core logic must remain independent of the UI layer.

---

### Author

- Tobit Vervat