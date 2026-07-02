#include <iostream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <vector>
#include <sstream>
#include <fstream>

#include "picosha2.h"

using namespace std;

namespace fs = filesystem;

struct FileRecord {
    fs::path absolutePath;
    fs::path relativePath;
    int size;
    string lastModifiedTime;
    string hash;
};

std::string calculate_sha256(const std::string& filepath)
{
    std::ifstream file(filepath, std::ios::binary);
    if (!file)
        throw std::runtime_error("Cannot open file: " + filepath);

    std::vector<unsigned char> buffer(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    std::string hash = picosha2::hash256_hex_string(buffer.begin(), buffer.end());
    return hash;
}

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
        new_file.hash = calculate_sha256(entry.path().string());

        files.push_back(new_file);
    };

    // for each file, get file name, get file contents size in bytes
    for (const auto& f : files) 
    {
        cout << f.relativePath << " | " << f.size << " bytes\n";

        ifstream file(f.absolutePath, ios::binary);

        // Ensure the file opened successfully
        if (!file.is_open()) {
            cout << "Error: Could not open the file." << std::endl;
        }
 
        // 2. Prepare a buffer to store the data
        const size_t bufferSize = 1024;
        vector<char> buffer(bufferSize);

        // 3. Read data from the file
        // This attempts to read up to 1024 bytes into the vector's memory
        file.read(buffer.data(), buffer.size());

        // 4. Confirm how many bytes were actually read
        streamsize bytesRead = file.gcount();

        // Output the results
        cout << "Successfully read " << bytesRead << " bytes from the file." << std::endl;

        // test hash:
        string ogHash = f.hash;

        cout << "\nOriginal hash: " << ogHash << endl;
        cout << "alter file now please, press enter when done" << endl;
        cin.get();


        string newHash = calculate_sha256(f.absolutePath.string());
        cout << "\nnew hash: " << newHash << endl;

        if (ogHash == newHash)
        {
            cout << "\nfile unchanged" << endl;
        }
        else
        {
            cout << "\nfile modified" << endl;
        }


        
        file.close();
    }

    std::cout << "\nTotal files scanned: " << files.size() << "\n";

    return 0;
}
 
