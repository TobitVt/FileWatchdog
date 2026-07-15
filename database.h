// database.h
#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include "file_record.h"

struct sqlite3;

class Database {
private:
    sqlite3* db;

public:
    Database(const std::string& dbPath);
    ~Database();

    bool create_baseline(const std::string& name, const std::string& folderPath);
    bool save_files_to_baseline(const std::string& name, const std::vector<FileRecord>& files);
    std::vector<FileRecord> load_baseline(const std::string& name);
    bool baseline_exists(const std::string& name);
};

#endif