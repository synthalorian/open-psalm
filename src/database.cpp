#include "database.hpp"
#include <iostream>

Database::Database(const std::string& db_path,
                   const std::string& translation_code,
                   const std::string& translation_name)
    : translation_code_(translation_code), translation_name_(translation_name) {

    if (sqlite3_open(db_path.c_str(), &db_) != SQLITE_OK) {
        std::cerr << "Failed to open database (" << translation_code
                  << "): " << sqlite3_errmsg(db_) << "\n";
        db_ = nullptr;
        return;
    }

    // Enable WAL mode
    exec("PRAGMA journal_mode=WAL;");
    exec("PRAGMA synchronous=NORMAL;");

    // Verify the database has data
    sqlite3_stmt* stmt;
    std::string check = "SELECT COUNT(*) FROM books";
    if (sqlite3_prepare_v2(db_, check.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_step(stmt);
        int count = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        if (count == 0) {
            std::cerr << "Warning: Database '" << translation_code
                      << "' is empty. Run data/create_db.py to populate it.\n";
        }
    }
}

Database::~Database() {
    if (db_) sqlite3_close(db_);
}

void Database::exec(const std::string& sql) {
    char* err = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
        std::cerr << "SQL error (" << translation_code_ << "): " << err << "\n";
        sqlite3_free(err);
    }
}

std::vector<std::string> Database::getBooks() {
    std::vector<std::string> books;
    sqlite3_stmt* stmt;
    std::string sql = "SELECT name FROM books ORDER BY id ASC";

    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            books.push_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
        }
        sqlite3_finalize(stmt);
    }
    return books;
}

std::vector<int> Database::getChapters(const std::string& book) {
    std::vector<int> chapters;
    sqlite3_stmt* stmt;

    std::string sql = "SELECT DISTINCT chapter FROM verses v "
                       "JOIN books b ON v.book_id = b.id "
                       "WHERE b.name = ? ORDER BY chapter";

    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, book.c_str(), -1, SQLITE_STATIC);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            chapters.push_back(sqlite3_column_int(stmt, 0));
        }
        sqlite3_finalize(stmt);
    }
    return chapters;
}

std::vector<VerseRef> Database::getVerses(const std::string& book, int chapter) {
    std::vector<VerseRef> verses;
    sqlite3_stmt* stmt;

    std::string sql = "SELECT v.chapter, v.verse, v.text FROM verses v "
                       "JOIN books b ON v.book_id = b.id "
                       "WHERE b.name = ? AND v.chapter = ? "
                       "ORDER BY v.verse";

    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, book.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, chapter);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            VerseRef vr;
            vr.book = book;
            vr.chapter = sqlite3_column_int(stmt, 0);
            vr.verse = sqlite3_column_int(stmt, 1);
            vr.text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            verses.push_back(std::move(vr));
        }
        sqlite3_finalize(stmt);
    }
    return verses;
}

std::vector<VerseRef> Database::search(const std::string& query) {
    std::vector<VerseRef> results;
    sqlite3_stmt* stmt;

    std::string sql = "SELECT b.name, v.chapter, v.verse, v.text FROM verses_fts "
                       "JOIN verses v ON verses_fts.rowid = v.id "
                       "JOIN books b ON v.book_id = b.id "
                       "WHERE verses_fts MATCH ? "
                       "ORDER BY rank LIMIT 100";

    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, query.c_str(), -1, SQLITE_STATIC);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            VerseRef vr;
            vr.book = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            vr.chapter = sqlite3_column_int(stmt, 1);
            vr.verse = sqlite3_column_int(stmt, 2);
            vr.text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            results.push_back(std::move(vr));
        }
        sqlite3_finalize(stmt);
    }
    return results;
}

std::string Database::lookupStrongs(const std::string& strongs_id) {
    if (!db_) return "";
    sqlite3_stmt* stmt;
    std::string sql = "SELECT definition FROM strongs WHERE id = ?";
    
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, strongs_id.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* def = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            std::string result = def ? def : "";
            sqlite3_finalize(stmt);
            return result;
        }
        sqlite3_finalize(stmt);
    }
    return "";
}

std::vector<std::pair<std::string, std::string>> Database::searchStrongs(const std::string& query) {
    std::vector<std::pair<std::string, std::string>> results;
    if (!db_) return results;
    
    sqlite3_stmt* stmt;
    std::string sql = "SELECT id, definition FROM strongs WHERE "
                       "id LIKE ? OR word LIKE ? OR definition LIKE ? "
                       "LIMIT 50";
    
    std::string like_query = "%" + query + "%";
    
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, like_query.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, like_query.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, like_query.c_str(), -1, SQLITE_STATIC);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            const char* def = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            results.push_back({id ? id : "", def ? def : ""});
        }
        sqlite3_finalize(stmt);
    }
    return results;
}

std::vector<CrossReference> Database::getCrossReferences(const std::string& book, int chapter, int verse) {
    std::vector<CrossReference> results;
    if (!db_) return results;
    
    // Check if cross_references table exists in this DB
    sqlite3_stmt* check;
    if (sqlite3_prepare_v2(db_, "SELECT name FROM sqlite_master WHERE type='table' AND name='cross_references'", -1, &check, nullptr) == SQLITE_OK) {
        bool has_table = (sqlite3_step(check) == SQLITE_ROW);
        sqlite3_finalize(check);
        if (!has_table) return results;
    }
    
    sqlite3_stmt* stmt;
    std::string sql = "SELECT to_book, to_chapter, to_verse_start, to_verse_end, votes "
                       "FROM cross_references "
                       "WHERE from_book = ? AND from_chapter = ? AND from_verse = ? "
                       "ORDER BY votes DESC LIMIT 20";
    
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, book.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, chapter);
        sqlite3_bind_int(stmt, 3, verse);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            CrossReference cr;
            const char* tb = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            cr.to_book = tb ? tb : "";
            cr.to_chapter = sqlite3_column_int(stmt, 1);
            cr.to_verse_start = sqlite3_column_int(stmt, 2);
            cr.to_verse_end = sqlite3_column_int(stmt, 3);
            cr.votes = sqlite3_column_int(stmt, 4);
            results.push_back(std::move(cr));
        }
        sqlite3_finalize(stmt);
    }
    return results;
}


