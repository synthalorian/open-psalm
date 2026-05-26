#pragma once
#include <string>
#include <vector>
#include <ftxui/dom/elements.hpp>

struct VerseRef {
    std::string book;
    int chapter;
    int verse;
    std::string text;
};

struct Theme {
    std::string name;
    ftxui::Color background;
    ftxui::Color foreground;
    ftxui::Color accent;
    ftxui::Color highlight;
    ftxui::Color secondary;
    ftxui::Color border;
    ftxui::Color title;
};

struct Bookmark {
    std::string book;
    int chapter;
    int verse;
    std::string label;
    std::string note; // annotation text
};

struct NavPosition {
    std::string book;
    int chapter;
    int verse;
};

struct CrossReference {
    std::string to_book;
    int to_chapter;
    int to_verse_start;
    int to_verse_end;
    int votes;
};

struct AppConfig {
    std::string last_book;
    int last_chapter = 1;
    int theme_index = 0;
    std::string translation_code = "KJV";
    int reading_plan_day = 0;
    std::vector<Bookmark> bookmarks;
    std::vector<std::string> search_history;
    bool parallel_enabled = false;
    std::string parallel_second_translation = "WEB";
    bool votd_autocopy = false;
    bool cross_references_enabled = true;
    std::string last_export_dir = "~/.config/open-psalm/exports";
};
