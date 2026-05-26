#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../src/database.hpp"
#include "../src/types.hpp"
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

// Helper to find test db path
static std::string find_db(const std::string& name) {
    std::vector<fs::path> candidates = {
        fs::current_path() / "data" / name,
        fs::current_path() / ".." / "data" / name,
        fs::path(__FILE__).parent_path().parent_path() / "data" / name,
    };
    for (const auto& p : candidates) {
        if (fs::exists(p)) return p.string();
    }
    return candidates[0].string();
}

TEST_CASE("Database opens KJV", "[database]") {
    auto path = find_db("bible_kjv.db");
    REQUIRE(fs::exists(path));

    Database db(path, "KJV", "King James Version (1769)");
    REQUIRE(db.isOpen());
    REQUIRE(db.translationCode() == "KJV");
    REQUIRE(db.translationName() == "King James Version (1769)");
}

TEST_CASE("Database getBooks returns 66 books", "[database]") {
    auto path = find_db("bible_kjv.db");
    REQUIRE(fs::exists(path));

    Database db(path, "KJV", "King James Version (1769)");
    auto books = db.getBooks();

    REQUIRE(books.size() == 66);
    REQUIRE(books[0] == "Genesis");
    REQUIRE(books[38] == "Malachi");
    REQUIRE(books[39] == "Matthew");
    REQUIRE(books.back() == "Revelation");
}

TEST_CASE("Database getChapters for known book", "[database]") {
    auto path = find_db("bible_kjv.db");
    Database db(path, "KJV", "KJV");

    auto genesis_chs = db.getChapters("Genesis");
    REQUIRE(!genesis_chs.empty());
    REQUIRE(genesis_chs.size() == 50);
    REQUIRE(genesis_chs[0] == 1);
    REQUIRE(genesis_chs.back() == 50);

    auto psalms_chs = db.getChapters("Psalms");
    REQUIRE(psalms_chs.size() == 150);
}

TEST_CASE("Database getVerses returns correct count", "[database]") {
    auto path = find_db("bible_kjv.db");
    Database db(path, "KJV", "KJV");

    // Genesis 1 has 31 verses
    auto verses = db.getVerses("Genesis", 1);
    REQUIRE(!verses.empty());
    REQUIRE(verses.size() == 31);
    REQUIRE(verses[0].chapter == 1);
    REQUIRE(verses[0].verse == 1);
    REQUIRE(verses[0].book == "Genesis");
    REQUIRE(!verses[0].text.empty());

    // Psalm 119 has 176 verses
    auto psalm119 = db.getVerses("Psalms", 119);
    REQUIRE(psalm119.size() == 176);
}

TEST_CASE("Database getVerses respects book/chapter", "[database]") {
    auto path = find_db("bible_kjv.db");
    Database db(path, "KJV", "KJV");

    auto verses = db.getVerses("John", 3);
    REQUIRE(!verses.empty());
    REQUIRE(verses[0].verse == 1);
    // John 3:16
    bool found_john316 = false;
    for (const auto& v : verses) {
        if (v.verse == 16) {
            found_john316 = true;
            REQUIRE(v.text.find("God") != std::string::npos);
            break;
        }
    }
    REQUIRE(found_john316);
}

TEST_CASE("Database search returns results", "[database]") {
    auto path = find_db("bible_kjv.db");
    Database db(path, "KJV", "KJV");

    auto results = db.search("faith*");
    REQUIRE(!results.empty());
    // Should have some results with "faith"
    REQUIRE(results.size() > 5);
    REQUIRE(!results[0].book.empty());
    REQUIRE(results[0].chapter > 0);
}

TEST_CASE("Database search respects limits", "[database]") {
    auto path = find_db("bible_kjv.db");
    Database db(path, "KJV", "KJV");

    // Common word should return many results, capped at 100
    auto results = db.search("the*");
    REQUIRE(!results.empty());
    REQUIRE(results.size() <= 100);
}

TEST_CASE("Database handles missing book gracefully", "[database]") {
    auto path = find_db("bible_kjv.db");
    Database db(path, "KJV", "KJV");

    auto chapters = db.getChapters("NonExistentBook");
    REQUIRE(chapters.empty());

    auto verses = db.getVerses("NonExistentBook", 1);
    REQUIRE(verses.empty());
}

TEST_CASE("All translations return same book count", "[database]") {
    auto kjv_path = find_db("bible_kjv.db");
    auto asv_path = find_db("bible_asv.db");
    auto ylt_path = find_db("bible_ylt.db");
    auto bbe_path = find_db("bible_bbe.db");
    auto web_path = find_db("bible_web.db");

    REQUIRE(fs::exists(kjv_path));
    REQUIRE(fs::exists(asv_path));
    REQUIRE(fs::exists(ylt_path));
    REQUIRE(fs::exists(bbe_path));
    REQUIRE(fs::exists(web_path));

    Database kjv(kjv_path, "KJV", "KJV");
    Database asv(asv_path, "ASV", "ASV");
    Database ylt(ylt_path, "YLT", "YLT");
    Database bbe(bbe_path, "BBE", "BBE");
    Database web(web_path, "WEB", "WEB");

    REQUIRE(kjv.getBooks().size() == 66);
    REQUIRE(asv.getBooks().size() == 66);
    REQUIRE(ylt.getBooks().size() == 66);
    REQUIRE(bbe.getBooks().size() == 66);
    REQUIRE(web.getBooks().size() == 66);
}

TEST_CASE("Verse count consistency between translations", "[database]") {
    auto kjv_path = find_db("bible_kjv.db");
    auto asv_path = find_db("bible_asv.db");

    Database kjv(kjv_path, "KJV", "KJV");
    Database asv(asv_path, "ASV", "ASV");

    // John 3 should have 36 verses in both KJV and ASV
    auto kjv_verses = kjv.getVerses("John", 3);
    auto asv_verses = asv.getVerses("John", 3);
    REQUIRE(kjv_verses.size() == asv_verses.size());

    // First verse of John should be the same reference
    REQUIRE(kjv_verses[0].verse == 1);
    REQUIRE(asv_verses[0].verse == 1);
}

TEST_CASE("FTS5 search works on all translations", "[database]") {
    auto paths = {
        find_db("bible_kjv.db"),
        find_db("bible_asv.db"),
        find_db("bible_ylt.db"),
        find_db("bible_bbe.db"),
        find_db("bible_web.db"),
    };

    for (const auto& path : paths) {
        if (!fs::exists(path)) continue;
        Database db(path, "test", "Test");
        auto results = db.search("love*");
        REQUIRE(!results.empty());
        REQUIRE(results.size() >= 10);
    }
}

TEST_CASE("Database handles empty search gracefully", "[database]") {
    auto path = find_db("bible_kjv.db");
    Database db(path, "KJV", "KJV");

    auto results = db.search("xyznonexistent12345*");
    REQUIRE(results.empty());
}
