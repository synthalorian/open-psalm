#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../src/app.hpp"
#include "../src/cli.hpp"
#include "../src/theme.hpp"
#include <string>
#include <vector>
#include <cstring>

// ============================================================
// CLI argument parsing tests
// ============================================================

TEST_CASE("parseArgs returns defaults for no arguments", "[cli]") {
    char* argv[] = {(char*)"open-psalm"};
    CliOptions opts = parseArgs(1, argv);
    REQUIRE(opts.translation_code.empty());
    REQUIRE(opts.book.empty());
    REQUIRE(opts.chapter == 0);
    REQUIRE(opts.verse == 0);
}

TEST_CASE("parseArgs parses --translation flag", "[cli]") {
    char* argv[] = {(char*)"open-psalm", (char*)"--translation", (char*)"ASV"};
    CliOptions opts = parseArgs(3, argv);
    REQUIRE(opts.translation_code == "ASV");
    REQUIRE(opts.book.empty());
}

TEST_CASE("parseArgs parses --book flag", "[cli]") {
    char* argv[] = {(char*)"open-psalm", (char*)"--book", (char*)"Psalms"};
    CliOptions opts = parseArgs(3, argv);
    REQUIRE(opts.book == "Psalms");
}

TEST_CASE("parseArgs parses --chapter flag", "[cli]") {
    char* argv[] = {(char*)"open-psalm", (char*)"--chapter", (char*)"23"};
    CliOptions opts = parseArgs(3, argv);
    REQUIRE(opts.chapter == 23);
}

TEST_CASE("parseArgs parses --verse flag", "[cli]") {
    char* argv[] = {(char*)"open-psalm", (char*)"--verse", (char*)"16"};
    CliOptions opts = parseArgs(3, argv);
    REQUIRE(opts.verse == 16);
}

TEST_CASE("parseArgs parses all flags together", "[cli]") {
    char* argv[] = {(char*)"open-psalm", (char*)"--translation", (char*)"YLT",
                    (char*)"--book", (char*)"John", (char*)"--chapter", (char*)"3",
                    (char*)"--verse", (char*)"16"};
    CliOptions opts = parseArgs(9, argv);
    REQUIRE(opts.translation_code == "YLT");
    REQUIRE(opts.book == "John");
    REQUIRE(opts.chapter == 3);
    REQUIRE(opts.verse == 16);
}

TEST_CASE("parseArgs clamps chapter to minimum of 1", "[cli]") {
    char* argv[] = {(char*)"open-psalm", (char*)"--chapter", (char*)"-5"};
    CliOptions opts = parseArgs(3, argv);
    REQUIRE(opts.chapter == 1);
}

TEST_CASE("parseArgs clamps verse to minimum of 1", "[cli]") {
    char* argv[] = {(char*)"open-psalm", (char*)"--verse", (char*)"0"};
    CliOptions opts = parseArgs(3, argv);
    REQUIRE(opts.verse == 1);
}

TEST_CASE("parseArgs handles --verse without --chapter", "[cli]") {
    char* argv[] = {(char*)"open-psalm", (char*)"--book", (char*)"Psalms",
                    (char*)"--verse", (char*)"1"};
    CliOptions opts = parseArgs(5, argv);
    REQUIRE(opts.book == "Psalms");
    REQUIRE(opts.verse == 1);
    REQUIRE(opts.chapter == 0); // chapter not specified
}

TEST_CASE("parseArgs ignores unknown flags", "[cli]") {
    char* argv[] = {(char*)"open-psalm", (char*)"--unknown", (char*)"foo",
                    (char*)"--translation", (char*)"KJV"};
    CliOptions opts = parseArgs(5, argv);
    REQUIRE(opts.translation_code == "KJV");
}

TEST_CASE("parseArgs handles --translation without value", "[cli]") {
    char* argv[] = {(char*)"open-psalm", (char*)"--translation"};
    CliOptions opts = parseArgs(2, argv);
    // No value consumed since it's the last arg
    REQUIRE(opts.translation_code.empty());
}

// ============================================================
// Highlight search terms tests
// ============================================================

TEST_CASE("highlightSearchTerms returns single element for empty query", "[search_highlight]") {
    Theme t;
    t.foreground = ftxui::Color::White;
    t.background = ftxui::Color::Black;
    t.secondary  = ftxui::Color::Yellow;
    t.highlight  = ftxui::Color::Cyan;

    auto result = App::highlightSearchTerms("Hello world", "", t);
    REQUIRE(result.size() == 1);
}

TEST_CASE("highlightSearchTerms returns single element for no match", "[search_highlight]") {
    Theme t;
    t.foreground = ftxui::Color::White;
    t.background = ftxui::Color::Black;
    t.secondary  = ftxui::Color::Yellow;
    t.highlight  = ftxui::Color::Cyan;

    auto result = App::highlightSearchTerms("Hello world", "xyz", t);
    // No matches, should return a single element with the full text
    REQUIRE(result.size() == 1);
}

TEST_CASE("highlightSearchTerms splits on single match", "[search_highlight]") {
    Theme t;
    t.foreground = ftxui::Color::White;
    t.background = ftxui::Color::Black;
    t.secondary  = ftxui::Color::Yellow;
    t.highlight  = ftxui::Color::Cyan;

    // "faith" in "walk by faith"
    auto result = App::highlightSearchTerms("walk by faith", "faith", t);
    // Should have: "walk by " (text), "faith" (highlighted)
    REQUIRE(result.size() == 2);
}

TEST_CASE("highlightSearchTerms handles match at start", "[search_highlight]") {
    Theme t;
    t.foreground = ftxui::Color::White;
    t.background = ftxui::Color::Black;
    t.secondary  = ftxui::Color::Yellow;
    t.highlight  = ftxui::Color::Cyan;

    auto result = App::highlightSearchTerms("faith is the substance", "faith", t);
    // Should have: "faith" (highlighted), " is the substance" (text)
    REQUIRE(result.size() == 2);
}

TEST_CASE("highlightSearchTerms handles match at end", "[search_highlight]") {
    Theme t;
    t.foreground = ftxui::Color::White;
    t.background = ftxui::Color::Black;
    t.secondary  = ftxui::Color::Yellow;
    t.highlight  = ftxui::Color::Cyan;

    auto result = App::highlightSearchTerms("for by grace", "grace", t);
    // Should have: "for by " (text), "grace" (highlighted)
    REQUIRE(result.size() == 2);
}

TEST_CASE("highlightSearchTerms handles multiple matches", "[search_highlight]") {
    Theme t;
    t.foreground = ftxui::Color::White;
    t.background = ftxui::Color::Black;
    t.secondary  = ftxui::Color::Yellow;
    t.highlight  = ftxui::Color::Cyan;

    auto result = App::highlightSearchTerms("love and love and love", "love", t);
    // Should have: "love" (h), " and " (t), "love" (h), " and " (t), "love" (h)
    REQUIRE(result.size() == 5);
}

TEST_CASE("highlightSearchTerms is case-insensitive", "[search_highlight]") {
    Theme t;
    t.foreground = ftxui::Color::White;
    t.background = ftxui::Color::Black;
    t.secondary  = ftxui::Color::Yellow;
    t.highlight  = ftxui::Color::Cyan;

    auto result = App::highlightSearchTerms("For GOD so loved", "god", t);
    // Should find "GOD" case-insensitively
    // Result: "For " (t), "GOD" (h), " so loved" (t)
    REQUIRE(result.size() == 3);
}

TEST_CASE("highlightSearchTerms handles overlapping matches correctly", "[search_highlight]") {
    Theme t;
    t.foreground = ftxui::Color::White;
    t.background = ftxui::Color::Black;
    t.secondary  = ftxui::Color::Yellow;
    t.highlight  = ftxui::Color::Cyan;

    auto result = App::highlightSearchTerms("aaa", "aa", t);
    // Should match "aa" at position 0, then resume at position 2
    // Result: "aa" (h), "a" (t)
    REQUIRE(result.size() == 2);
}

TEST_CASE("highlightSearchTerms matches full text exactly", "[search_highlight]") {
    Theme t;
    t.foreground = ftxui::Color::White;
    t.background = ftxui::Color::Black;
    t.secondary  = ftxui::Color::Yellow;
    t.highlight  = ftxui::Color::Cyan;

    auto result = App::highlightSearchTerms("Jesus", "Jesus", t);
    // Full match — just the highlighted element
    REQUIRE(result.size() == 1);
}
