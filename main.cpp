#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <limits>

#include "json.hpp"

#include "picosha2.h"


namespace fs = std::filesystem;
using json = nlohmann::json;

// Represents one file discovered during a scan.
struct FileRecord {
    fs::path absolutePath;      // Full path to the file on disk.
    fs::path relativePath;      // Path relative to the scanned root folder.
    std::uintmax_t size;        // File size in bytes.
    std::string lastModifiedTime; // Human-readable last modified timestamp.
    std::string hash;           // SHA-256 hash of the file contents.
};

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

    for (const auto& entry : fs::recursive_directory_iterator(root)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        FileRecord record;
        record.absolutePath = entry.path();
        record.relativePath = fs::relative(entry.path(), root);
        record.size = entry.file_size();
        record.lastModifiedTime = format_last_modified(entry.last_write_time());
        record.hash = calculate_sha256(entry.path());

        files.push_back(record);
    }

    return files;
}

// Converts a FileRecord into a JSON object so it can be saved to disk.
json file_to_json(const FileRecord& file) {
    json jObj;
    jObj["relativePath"] = file.relativePath.string();
    jObj["size"] = file.size;
    jObj["lastModifiedTime"] = file.lastModifiedTime;
    jObj["hash"] = file.hash;
    return jObj;
}

// Rebuilds a FileRecord from JSON when a baseline is loaded.
FileRecord json_to_file_record(const json& obj) {
    FileRecord file;
    file.relativePath = fs::path(obj["relativePath"].get<std::string>());
    file.size = obj["size"].get<std::uintmax_t>();
    file.hash = obj["hash"].get<std::string>();
    file.lastModifiedTime = obj["lastModifiedTime"].get<std::string>();
    return file;
}

// Saves the current scan results to a JSON baseline file.
bool save_baseline(const fs::path& baselinePath, const std::vector<FileRecord>& files) {
    // Create the parent folder if it does not exist yet.
    if (!baselinePath.parent_path().empty()) {
        std::error_code ec;
        fs::create_directories(baselinePath.parent_path(), ec);
        if (ec) {
            std::cerr << "Unable to create baseline folder: " << baselinePath.parent_path() << "\n";
            return false;
        }
    }

    std::ofstream out(baselinePath, std::ios::trunc);
    if (!out) {
        return false;
    }

    json j = json::array();
    for (const auto& file : files) {
        j.push_back(file_to_json(file));
    }

    out << j.dump(4);
    return true;
}

// Compares the old snapshot and the new snapshot to find files that changed.
std::vector<ChangeResult> compare_scans(const std::vector<FileRecord>& baseline, const std::vector<FileRecord>& current) {
    std::vector<ChangeResult> results;

    for (const auto& oldFile : baseline) {
        bool found = false;

        for (const auto& newFile : current) {
            if (oldFile.relativePath == newFile.relativePath) {
                found = true;

                ChangeResult result;
                result.path = oldFile.relativePath.string();
                result.status = (oldFile.hash == newFile.hash) ? ChangeType::Unchanged : ChangeType::Modified;
                results.push_back(result);
                break;
            }
        }

        if (!found) {
            ChangeResult result;
            result.path = oldFile.relativePath.string();
            result.status = ChangeType::Deleted;
            results.push_back(result);
        }
    }

    for (const auto& newFile : current) {
        bool found = false;

        for (const auto& oldFile : baseline) {
            if (newFile.relativePath == oldFile.relativePath) {
                found = true;
                break;
            }
        }

        if (!found) {
            ChangeResult result;
            result.path = newFile.relativePath.string();
            result.status = ChangeType::New;
            results.push_back(result);
        }
    }

    return results;
}

// Loads a previously saved baseline from disk.
std::vector<FileRecord> load_baseline(const fs::path& baselinePath) {
    std::ifstream in(baselinePath);
    if (!in) {
        throw std::runtime_error("Cannot open file: " + baselinePath.string());
    }

    json j;
    in >> j;

    std::vector<FileRecord> files;
    for (const auto& obj : j) {
        files.push_back(json_to_file_record(obj));
    }

    return files;
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
// Prints the contents of a loaded baseline for inspection.
void print_loaded_baseline(const std::vector<FileRecord>& files, const std::string& label) {
    std::cout << "Loaded " << files.size() << " records from " << label << ":\n";
    for (const auto& file : files) {
        std::cout << file_to_json(file).dump(4) << std::endl;
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
int run_create_mode(const fs::path& root, const fs::path& baselinePath) {
    try {
        std::vector<FileRecord> files = scan_directory(root);
        print_files(files);

        if (!save_baseline(baselinePath, files)) {
            std::cerr << "Failed to save baseline.\n";
            return 1;
        }

        std::cout << "Baseline saved to " << baselinePath << "\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }
}

// Compares the current folder contents against a saved baseline.
int run_compare_mode(const fs::path& root, const fs::path& baselinePath) {
    try {
        std::vector<FileRecord> baselineRecords = load_baseline(baselinePath);
        std::vector<FileRecord> currentRecords = scan_directory(root);

        print_files(currentRecords);
        std::cout << "Compared against baseline: " << baselinePath << "\n";

        std::vector<ChangeResult> results = compare_scans(baselineRecords, currentRecords);
        for (const auto& result : results) {
            std::cout << result.path << " -> " << change_type_to_string(result.status) << "\n";
        }

        print_change_summary(results);
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
            fs::path baselinePath = (argc >= 4) ? argv[3] : "baseline.json";
            return run_create_mode(root, baselinePath);
        }

        if (mode == "compare") {
            if (argc < 3) {
                print_usage(argv[0]);
                return 1;
            }

            fs::path root = argv[2];
            fs::path baselinePath = (argc >= 4) ? argv[3] : "baseline.json";
            return run_compare_mode(root, baselinePath);
        }

        print_usage(argv[0]);
        return 1;
    }

    std::vector<ChangeResult> results;
    fs::path root = fs::current_path();
    fs::path baselinePath1 = "baseline1.json";
    fs::path baselinePath2 = "baseline2.json";

    std::cout << "Please provide the root folder to scan (press Enter for current directory): ";
    std::string rootInput;
    std::getline(std::cin, rootInput);
    if (!rootInput.empty()) {
        root = fs::path(rootInput);
    }

    std::cout << "Where should the first baseline be saved? (press Enter for baseline1.json): ";
    std::string baseline1Input;
    std::getline(std::cin, baseline1Input);
    if (!baseline1Input.empty()) {
        baselinePath1 = fs::path(baseline1Input);
    }

    try {
        std::vector<FileRecord> files1 = scan_directory(root);
        print_files(files1);

        if (!save_baseline(baselinePath1, files1)) {
            std::cerr << "Failed to save the first baseline.\n";
            return 1;
        }
        std::cout << "Baseline 1 saved to " << baselinePath1 << "\n";

        std::cout << "Please alter files in " << root << " now, then press Enter when done.\n";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cin.get();

        std::cout << "Where should the second baseline be saved? (press Enter for baseline2.json): ";
        std::string baseline2Input;
        std::getline(std::cin, baseline2Input);
        if (!baseline2Input.empty()) {
            baselinePath2 = fs::path(baseline2Input);
        }

        std::vector<FileRecord> files2 = scan_directory(root);
        print_files(files2);

        if (!save_baseline(baselinePath2, files2)) {
            std::cerr << "Failed to save the second baseline.\n";
            return 1;
        }
        std::cout << "Baseline 2 saved to " << baselinePath2 << "\n";

        std::vector<FileRecord> loadedFile1 = load_baseline(baselinePath1);
        print_loaded_baseline(loadedFile1, "baseline 1");

        std::vector<FileRecord> loadedFile2 = load_baseline(baselinePath2);
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

