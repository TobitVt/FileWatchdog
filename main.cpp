#include <iostream>
#include <limits>

#include "core.h"

namespace fs = std::filesystem;

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

