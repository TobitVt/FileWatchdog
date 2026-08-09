#include <iostream>

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
        Database db(default_database_path());
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
        Database db(default_database_path());
        if (!db.baseline_exists(baselineName)) {
            std::cerr << "Error: No baseline named '" << baselineName << "' exists. Run 'create' first.\n";
            return 1;
        }
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
// Entry point for the program.
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

    print_usage(argv[0]);
    return 1;
}
