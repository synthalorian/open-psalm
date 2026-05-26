#pragma once
#include <sqlite3.h>
#include <string>
#include <vector>
#include "types.hpp"

class Database {
public:
    Database(const std::string& db_path,
             const std::string& translation_code = "KJV",
             const std::string& translation_name = "King James Version (1769)");
    ~Database();

    std::vector<std::string> getBooks();
    std::vector<int> getChapters(const std::string& book);
    std::vector<VerseRef> getVerses(const std::string& book, int chapter);
    std::vector<VerseRef> search(const std::string& query);

    // Strong's support
    std::string lookupStrongs(const std::string& strongs_id);
    // Returns pairs of (strongs_id, language)
    std::vector<std::pair<std::string, std::string>> searchStrongs(const std::string& query);

    // Cross-reference support
    // NOTE: Ensure the database has a cross_references table before calling this.
    std::vector<CrossReference> getCrossReferences(const std::string& book, int chapter, int verse);

    std::string translationCode() const { return translation_code_; }
    std::string translationName() const { return translation_name_; }
    bool isOpen() const { return db_ != nullptr; }

private:
    sqlite3* db_ = nullptr;
    std::string translation_code_;
    std::string translation_name_;

    void exec(const std::string& sql);
};
