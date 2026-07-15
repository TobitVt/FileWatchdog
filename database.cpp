// database.cpp
#include "database.h"
#include "sqlite3.h"
#include <stdexcept>

Database::Database(const std::string& dbPath) {
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc) {
        throw std::runtime_error("Cannot open database: " + std::string(sqlite3_errmsg(db)));
    }
    
    // Create tables if they don't exist
    const char* createTableSQL = R"(
        CREATE TABLE IF NOT EXISTS baselines (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL UNIQUE,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            folder_path TEXT NOT NULL
        );
        
        CREATE TABLE IF NOT EXISTS baseline_files (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            baseline_id INTEGER NOT NULL,
            relative_path TEXT NOT NULL,
            size INTEGER NOT NULL,
            last_modified TEXT NOT NULL,
            hash TEXT NOT NULL,
            FOREIGN KEY (baseline_id) REFERENCES baselines(id)
        );
    )";
    
    char* err = nullptr;
    rc = sqlite3_exec(db, createTableSQL, nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::string error = err ? std::string(err) : "Unknown error";
        sqlite3_free(err);
        throw std::runtime_error("Cannot create tables: " + error);
    }
}

Database::~Database() {
    if (db) sqlite3_close(db);
}

bool Database::create_baseline(const std::string& name, const std::string& folderPath) {
    const char* sql = R"(
        INSERT INTO baselines (name, folder_path)
        VALUES (?, ?)
        ON CONFLICT(name) DO UPDATE SET folder_path = excluded.folder_path;
    )";
    sqlite3_stmt* stmt;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, folderPath.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

bool Database::save_files_to_baseline(const std::string& name, const std::vector<FileRecord>& files) {
    sqlite3_stmt* stmt;
    int baseline_id = -1;

    sqlite3_prepare_v2(db, "SELECT id FROM baselines WHERE name = ?;", -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        baseline_id = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    if (baseline_id == -1) return false;

    sqlite3_prepare_v2(db, "DELETE FROM baseline_files WHERE baseline_id = ?;", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, baseline_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    const char* insertSQL = "INSERT INTO baseline_files (baseline_id, relative_path, size, last_modified, hash) VALUES (?, ?, ?, ?, ?);";

    for (const auto& file : files) {
        sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, baseline_id);
        sqlite3_bind_text(stmt, 2, file.relativePath.string().c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int64(stmt, 3, file.size);
        sqlite3_bind_text(stmt, 4, file.lastModifiedTime.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 5, file.hash.c_str(), -1, SQLITE_STATIC);

        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (rc != SQLITE_DONE) {
            return false;
        }
    }

    return true;
}

std::vector<FileRecord> Database::load_baseline(const std::string& name) {
    std::vector<FileRecord> files;
    sqlite3_stmt* stmt;
    
    const char* sql = R"(
        SELECT f.relative_path, f.size, f.last_modified, f.hash
        FROM baseline_files f
        JOIN baselines b ON f.baseline_id = b.id
        WHERE b.name = ?;
    )";
    
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        FileRecord record;
        record.relativePath = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        record.size = sqlite3_column_int64(stmt, 1);
        record.lastModifiedTime = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        record.hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        files.push_back(record);
    }
    
    sqlite3_finalize(stmt);
    return files;
}

bool Database::baseline_exists(const std::string& name) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "SELECT id FROM baselines WHERE name = ?;", -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    
    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return exists;
}