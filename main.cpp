#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "picosha2.h"

namespace fs = std::filesystem;

// Represents one file discovered during a scan.
struct FileRecord {
    fs::path absolutePath;      // Full path to the file on disk.
    fs::path relativePath;      // Path relative to the scanned root folder.
    std::uintmax_t size;        // File size in bytes.
    std::string lastModifiedTime; // Human-readable last modified timestamp.
    std::string hash;           // SHA-256 hash of the file contents.
};

struct ChangeResult{
    std::string path;
    std::string status;
}
;
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

// Saves the current scan results to a simple text baseline file.
bool save_baseline(const fs::path& baselinePath, const std::vector<FileRecord>& files) 
{
    std::ofstream out(baselinePath, std::ios::trunc);
    if (!out) {
        return false;
    }

    out << "relativePath|size|lastModifiedTime|hash\n";
    for (const auto& file : files) {
        out << file.relativePath.string() << '|'
            << file.size << '|'
            << file.lastModifiedTime << '|'
            << file.hash << '\n';
    }

    return true;
}

// Compares two scans and stores one result per file.
std::vector<ChangeResult> compare_scans(const std::vector<FileRecord>& baseline, const std::vector<FileRecord>& current) {
    std::vector<ChangeResult> results;

    for (const auto& oldFile : baseline) {
        bool found = false;

        for (const auto& newFile : current) {
            if (oldFile.relativePath == newFile.relativePath) {
                found = true;

                ChangeResult result;
                result.path = oldFile.relativePath.string();
                result.status = (oldFile.hash == newFile.hash) ? "unchanged" : "modified";
                results.push_back(result);
                break;
            }
        }

        if (!found) {
            ChangeResult result;
            result.path = oldFile.relativePath.string();
            result.status = "deleted";
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
            result.status = "new";
            results.push_back(result);
        }
    }

    return results;
}

// Loads a previously saved baseline from disk.
std::vector<FileRecord> load_baseline(const fs::path& baselinePath) {
    std::ifstream in(baselinePath);
    if (!in) {
        throw std::runtime_error("Cannot open baseline file: " + baselinePath.string());
    }

    std::vector<FileRecord> files;
    std::string line;
    std::getline(in, line); // header

    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }

        std::stringstream ss(line);
        std::string relativePath;
        std::string size;
        std::string lastModifiedTime;
        std::string hash;

        std::getline(ss, relativePath, '|');
        std::getline(ss, size, '|');
        std::getline(ss, lastModifiedTime, '|');
        std::getline(ss, hash, '|');

        FileRecord record;
        record.relativePath = fs::path(relativePath);
        record.size = static_cast<std::uintmax_t>(std::stoull(size));
        record.lastModifiedTime = lastModifiedTime;
        record.hash = hash;
        files.push_back(record);
    }

    return files;
}

// Prints the scan results to the console for easy inspection.
void print_files(const std::vector<FileRecord>& files) 
{
    std::cout << "Scanned files:\n";
    for (const auto& file : files) {
        std::cout << file.relativePath << " | " << file.size << " bytes\n";
    }
    std::cout << "Total files scanned: " << files.size() << "\n";
}

int main() 
{
    std::vector<ChangeResult> results;
    // The folder we want to monitor.
    const fs::path root = "C:/Users/tobit/TestFolder/sub";
    // Where the baseline will be stored.
    const fs::path baselinePath1 = "baseline1.txt";
    const fs::path baselinePath2 = "baseline2.txt";

    try {
        // Step 1: Scan the folder and collect file information.
        std::vector<FileRecord> files1 = scan_directory(root);
        print_files(files1);

        std::vector<FileRecord> files2 = scan_directory(root);
        print_files(files2);

        // Step 2: Save the current scan as a baseline.

        if (!save_baseline(baselinePath1, files1)) {
            std::cerr << "Failed to save 1 to baseline.\n";
            return 1;
        }

        std::cout << "Baseline 1 saved to " << baselinePath1 << "\n";

        if (!save_baseline(baselinePath2, files2)) {
            std::cerr << "Failed to save 2 to baseline.\n";
            return 1;
        }
        std::cout << "Baseline 2 saved to " << baselinePath2 << "\n";

        // Step 3: Load the baseline back to prove the data was stored correctly.
        std::vector<FileRecord> loadedFile1 = load_baseline(baselinePath1);
        std::cout << "Loaded " << loadedFile1.size() << " records from baseline 1.\n";

        std::vector<FileRecord> loadedFile2 = load_baseline(baselinePath2);
        std::cout << "Loaded " << loadedFile2.size() << " records from baseline 2.\n";

        std::cout << "\nCompare results:\n";
        results = compare_scans(loadedFile1, loadedFile2);

        for (const auto& result : results) {
            std::cout << result.path << " -> " << result.status << "\n";
        }

    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}

