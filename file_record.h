#ifndef FILE_RECORD_H
#define FILE_RECORD_H

#include <cstdint>
#include <filesystem>
#include <string>

// Represents one file discovered during a scan.
struct FileRecord {
    std::filesystem::path absolutePath;      // Full path to the file on disk.
    std::filesystem::path relativePath;      // Path relative to the scanned root folder.
    std::uintmax_t size;                    // File size in bytes.
    std::string lastModifiedTime;           // Human-readable last modified timestamp.
    std::string hash;                       // SHA-256 hash of the file contents.
};

#endif
