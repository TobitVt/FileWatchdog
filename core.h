#pragma once

#include <chrono>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "database.h"
#include "file_record.h"

namespace fs = std::filesystem;

enum class ChangeType {
    Unchanged,
    Modified,
    New,
    Deleted
};

struct ChangeResult {
    std::string path;
    ChangeType status;
};

struct ScanOutcome {
    std::vector<FileRecord> files;
};

struct CompareOutcome {
    std::vector<FileRecord> baselineRecords;
    std::vector<FileRecord> currentRecords;
    std::vector<ChangeResult> changes;
};

std::string change_type_to_string(ChangeType type);
std::string calculate_sha256(const fs::path& filepath);
std::string format_last_modified(const fs::file_time_type& time);

std::vector<FileRecord> scan_directory(
    const fs::path& root,
    std::function<bool(const fs::path& current, std::size_t count)> onProgress = nullptr);

bool save_baseline(Database& db, const std::string& baselineName, const std::vector<FileRecord>& files);
std::vector<ChangeResult> compare_scans(const std::vector<FileRecord>& baseline, const std::vector<FileRecord>& current);
std::vector<FileRecord> load_baseline(Database& db, const std::string& baselineName);

ScanOutcome run_create(Database& db, const fs::path& root, const std::string& baselineName);
CompareOutcome run_compare(Database& db, const fs::path& root, const std::string& baselineName);