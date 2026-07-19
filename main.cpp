#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <functional>
#include <unordered_map>

#include "picosha2.h"

#include "database.h"
#include "file_record.h"
#include "sqlite3.h"

namespace fs = std::filesystem;

enum class ChangeType {
    Unchanged,
    Modified,
    New,
    Deleted
};

struct ChangeResult{
    std::string path;      // File path that changed.
    ChangeType status;     // What happened to that file.
};

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
std::vector<FileRecord> scan_directory(const fs::path& root) {
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
    }

    return files;
}

// Plain-data outcome of a create/compare run — no I/O, safe for Qt to consume.
struct ScanOutcome {
    std::vector<FileRecord> files;
};

struct CompareOutcome {
    std::vector<FileRecord> baselineRecords;
    std::vector<FileRecord> currentRecords;
    std::vector<ChangeResult> changes;
};

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

// Prints the scan results to the console for easy inspection.
void print_files(const std::vector<FileRecord>& files) {
    std::cout << "Scanned files:\n";
    for (const auto& file : files) {
        std::cout << file.relativePath << " | " << file.size << " bytes\n";
    }
    std::cout << "Total files scanned: " << files.size() << "\n";
}

// Prints the contents of a loaded baseline for inspection.
void print_loaded_baseline(const std::vector<FileRecord>& files, const std::string& label) {
    std::cout << "Loaded " << files.size() << " records from " << label << ":\n";
    for (const auto& file : files) {
        std::cout << file.relativePath.string() << " | " << file.size << " bytes | "
                  << file.hash << "\n";
    }
}

// Prints a quick summary of how many files were unchanged, modified, new, or deleted.
void print_change_summary(const std::vector<ChangeResult>& results) {
    std::size_t unchanged = 0;
    std::size_t modified = 0;
    std::size_t newFiles = 0;
    std::size_t deleted = 0;

    for (const auto& result : results) {
        switch (result.status) {
            case ChangeType::Unchanged: ++unchanged; break;
            case ChangeType::Modified: ++modified; break;
            case ChangeType::New: ++newFiles; break;
            case ChangeType::Deleted: ++deleted; break;
        }
    }

    std::cout << "Summary: unchanged=" << unchanged
              << ", modified=" << modified
              << ", new=" << newFiles
              << ", deleted=" << deleted << "\n";
}

// Shows how to use the command-line interface.
void print_usage(const char* programName) {
    std::cout << "Usage:\n";
    std::cout << "  " << programName << " create <root-folder> [baseline-file]\n";
    std::cout << "  " << programName << " compare <root-folder> <baseline-file>\n";
    std::cout << "  " << programName << " help\n";
}

// Creates a new baseline from the current contents of a folder.
int run_create_mode(const fs::path& root, const std::string& baselineName) {
    try {
        Database db("file_integrity.db");
        ScanOutcome outcome = run_create(db, root, baselineName);

        print_files(outcome.files);
        std::cout << "Baseline saved to database as '" << baselineName << "'.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }
}

// Compares the current folder contents against a saved baseline.
int run_compare_mode(const fs::path& root, const std::string& baselineName) {
    try {
        Database db("file_integrity.db");
        CompareOutcome outcome = run_compare(db, root, baselineName);

        print_files(outcome.currentRecords);
        std::cout << "Compared against baseline: " << baselineName << "\n";

        for (const auto& result : outcome.changes) {
            std::cout << result.path << " -> " << change_type_to_string(result.status) << "\n";
        }

        print_change_summary(outcome.changes);
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }
}
// Entry point for the program. Supports CLI usage or the older interactive prompt.
int main(int argc, char* argv[]) {
    if (argc > 1) {
        std::string mode = argv[1];
        if (mode == "help" || mode == "--help" || mode == "-h") {
            print_usage(argv[0]);
            return 0;
        }

        if (mode == "create") {
            if (argc < 3) {
                print_usage(argv[0]);
                return 1;
            }

            fs::path root = argv[2];
            std::string baselineName = (argc >= 4) ? argv[3] : "default_baseline";
            return run_create_mode(root, baselineName);
        }

        if (mode == "compare") {
            if (argc < 3) {
                print_usage(argv[0]);
                return 1;
            }

            fs::path root = argv[2];
            std::string baselineName = (argc >= 4) ? argv[3] : "default_baseline";
            return run_compare_mode(root, baselineName);
        }

        print_usage(argv[0]);
        return 1;
    }

    std::vector<ChangeResult> results;
    fs::path root = fs::current_path();
    std::string baselineName1 = "baseline1";
    std::string baselineName2 = "baseline2";

    std::cout << "Please provide the root folder to scan (press Enter for current directory): ";
    std::string rootInput;
    std::getline(std::cin, rootInput);
    if (!rootInput.empty()) {
        root = fs::path(rootInput);
    }

    std::cout << "What should the first baseline be called? (press Enter for baseline1): ";
    std::string baseline1Input;
    std::getline(std::cin, baseline1Input);
    if (!baseline1Input.empty()) {
        baselineName1 = baseline1Input;
    }

    try {
        Database db("file_integrity.db");
        std::vector<FileRecord> files1 = scan_directory(root);
        print_files(files1);

        if (!save_baseline(db, baselineName1, files1)) {
            std::cerr << "Failed to save the first baseline.\n";
            return 1;
        }
        std::cout << "Baseline 1 saved to database as '" << baselineName1 << "'.\n";

        std::cout << "Please alter files in " << root << " now, then press Enter when done.\n";
        std::cin.get();

        std::cout << "What should the second baseline be called? (press Enter for baseline2): ";
        std::string baseline2Input;
        std::getline(std::cin, baseline2Input);
        if (!baseline2Input.empty()) {
            baselineName2 = baseline2Input;
        }

        std::vector<FileRecord> files2 = scan_directory(root);
        print_files(files2);

        if (!save_baseline(db, baselineName2, files2)) {
            std::cerr << "Failed to save the second baseline.\n";
            return 1;
        }
        std::cout << "Baseline 2 saved to database as '" << baselineName2 << "'.\n";

        std::vector<FileRecord> loadedFile1 = load_baseline(db, baselineName1);
        print_loaded_baseline(loadedFile1, "baseline 1");

        std::vector<FileRecord> loadedFile2 = load_baseline(db, baselineName2);
        print_loaded_baseline(loadedFile2, "baseline 2");

        std::cout << "\nCompare results:\n";
        results = compare_scans(loadedFile1, loadedFile2);

        for (const auto& result : results) {
            std::cout << result.path << " -> " << change_type_to_string(result.status) << "\n";
        }

        print_change_summary(results);
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}

