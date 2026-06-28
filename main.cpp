#include <iostream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <vector>
#include <sstream>

using namespace std;

namespace fs = filesystem;

struct FileRecord {
    fs::path absolutePath;
    fs::path relativePath;
    int size;
    string lastModifiedTime;
    string hash;
};

int main() {

    // Given a selected root folder, print file paths relative to that folder
    fs::path root = "C:/Users/tobit/TestFolder/sub";

    vector<FileRecord> files;


    // Check if directory exists before iterating to avoid runtime crashes
    if (!fs::exists(root) || !fs::is_directory(root)) {
        cout << "Error: Path does not exist or is not a directory." << endl;
        return 1;
    }

    // print out each file inside chosen directory
    for (const auto & entry : fs::directory_iterator(root))
    {
        // skip current if its not a normal file
        if (!entry.is_regular_file())
            continue;

        //for every item inside folder, get path, size, last modified time
        FileRecord new_file;

        // get relative path for each file
        fs::path relative = fs::relative(entry.path(), root);

        // get file size
        int fSize = entry.file_size();

        //get last modified time
        auto ftime = entry.last_write_time();

        auto sctp = chrono::time_point_cast<chrono::system_clock::duration>(
            ftime - fs::file_time_type::clock::now() + chrono::system_clock::now());

        time_t cftime = chrono::system_clock::to_time_t(sctp);

        ostringstream modTime;
        modTime << std::put_time(std::localtime(&cftime), "%Y-%m-%d %H:%M:%S");

        new_file.absolutePath = entry.path();
        new_file.relativePath = relative;
        new_file.size = fSize;
        new_file.lastModifiedTime = modTime.str();
        new_file.hash = "";

        files.push_back(new_file);
    };
    for (const auto& f : files) 
    {
        std::cout << f.relativePath << " | " << f.size << " bytes\n";
    }

    std::cout << "\nTotal files scanned: " << files.size() << "\n";

    return 0;
}
 
