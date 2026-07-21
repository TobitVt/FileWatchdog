#include "core.h"
#include "picosha2.h"
#include "sqlite3.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

// Turns a change enum into a readable label for the console.
std::string change_type_to_string(ChangeType type) {
    switch (type) {
        case ChangeType::Unchanged: return "unchanged";
        case ChangeType::Modified: return "modified";
        case ChangeType::New: return "new";
        case ChangeType::Deleted: return "deleted";
    }
    return "unknown";
}


// Computes the SHA-256 hash for a file.
std::string calculate_sha256(const fs::path& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + filepath.string());
    }

    std::vector<unsigned char> buffer(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());

    return picosha2::hash256_hex_string(buffer.begin(), buffer.end());
}

// Converts a filesystem timestamp into a readable string.
std::string format_last_modified(const fs::file_time_type& time) {
    auto systemTime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        time - fs::file_time_type::clock::now() + std::chrono::system_clock::now());

    std::time_t cftime = std::chrono::system_clock::to_time_t(systemTime);
    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &cftime);
#else
    localtime_r(&cftime, &localTime);
#endif

    std::ostringstream oss;
    oss << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// Walks through a folder and collects metadata for every regular file it finds.
std::vector<FileRecord> scan_directory(const fs::path& root, std::function<bool(const fs::path& current, std::size_t count)> onProgress)
{
    if (!fs::exists(root) || !fs::is_directory(root)) {
        throw std::runtime_error("Path does not exist or is not a directory.");
    }

    std::vector<FileRecord> files;

    fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied);
    fs::recursive_directory_iterator end;

    for (; it != end; ++it) {
        const auto& entry = *it;

        // Skip symlinks explicitly — is_regular_file() follows them, which
        // can throw on broken links or silently hash link targets.
        if (entry.is_symlink()) {
            continue;
        }

        std::error_code ec;
        if (!entry.is_regular_file(ec) || ec) {
            continue;
        }

        try {
            FileRecord record;
            record.absolutePath = entry.path();
            record.relativePath = fs::relative(entry.path(), root);
            record.size = entry.file_size();
            record.lastModifiedTime = format_last_modified(entry.last_write_time());
            record.hash = calculate_sha256(entry.path());

            files.push_back(record);
        } catch (const std::exception& ex) {
            // Don't let one unreadable file abort the whole scan.
            std::cerr << "Skipping " << entry.path() << ": " << ex.what() << "\n";
            continue;
        }

        if (onProgress && !onProgress(entry.path(), files.size())) {
            break; // callback returned false → cancelled
        }
    }

    return files;
}


// Saves the current scan results to the SQLite database.
bool save_baseline(Database& db, const std::string& baselineName, const std::vector<FileRecord>& files) {
    if (!db.create_baseline(baselineName, "")) {
        return false;
    }

    return db.save_files_to_baseline(baselineName, files);
}

// Compares the old snapshot and the new snapshot to find files that changed.
std::vector<ChangeResult> compare_scans(const std::vector<FileRecord>& baseline, const std::vector<FileRecord>& current) {
    std::vector<ChangeResult> results;

    // Index the current scan by relative path for O(1) lookups.
    std::unordered_map<std::string, const FileRecord*> currentByPath;
    currentByPath.reserve(current.size());
    for (const auto& file : current) {
        currentByPath[file.relativePath.string()] = &file;
    }

    // Walk the baseline, checking each entry against the index.
    std::unordered_map<std::string, bool> seenInCurrent; // tracks which current entries got matched
    for (const auto& oldFile : baseline) {
        const std::string key = oldFile.relativePath.string();
        auto it = currentByPath.find(key);

        ChangeResult result;
        result.path = key;

        if (it == currentByPath.end()) {
            result.status = ChangeType::Deleted;
        } else {
            result.status = (oldFile.hash == it->second->hash) ? ChangeType::Unchanged : ChangeType::Modified;
            seenInCurrent[key] = true;
        }
        results.push_back(result);
    }

    // Anything in current that wasn't matched against baseline is new.
    for (const auto& newFile : current) {
        const std::string key = newFile.relativePath.string();
        if (seenInCurrent.find(key) == seenInCurrent.end()) {
            ChangeResult result;
            result.path = key;
            result.status = ChangeType::New;
            results.push_back(result);
        }
    }

    return results;
}

// Loads a previously saved baseline from the SQLite database.
std::vector<FileRecord> load_baseline(Database& db, const std::string& baselineName) {
    return db.load_baseline(baselineName);
}

// Pure logic: scan + save, no printing, no exit codes. Throws on failure.
ScanOutcome run_create(Database& db, const fs::path& root, const std::string& baselineName) {
    ScanOutcome outcome;
    outcome.files = scan_directory(root);
    if (!save_baseline(db, baselineName, outcome.files)) {
        throw std::runtime_error("Failed to save baseline '" + baselineName + "'.");
    }
    return outcome;
}

// Pure logic: load + scan + diff, no printing, no exit codes. Throws on failure.
CompareOutcome run_compare(Database& db, const fs::path& root, const std::string& baselineName) {
    CompareOutcome outcome;
    outcome.baselineRecords = load_baseline(db, baselineName);
    outcome.currentRecords = scan_directory(root);
    outcome.changes = compare_scans(outcome.baselineRecords, outcome.currentRecords);
    return outcome;
}