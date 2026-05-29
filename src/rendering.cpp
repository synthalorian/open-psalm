#include "app.hpp"
#include <ftxui/dom/elements.hpp>
#include <algorithm>

using namespace ftxui;

// ============================================================
// Search term highlighting helper
// ============================================================
std::vector<Element> App::highlightSearchTerms(const std::string& content,
                                                const std::string& query,
                                                const Theme& t) {
    std::vector<Element> result;
    if (query.empty()) {
        result.push_back(ftxui::text(content));
        return result;
    }

    std::string lower_content = content;
    std::string lower_query = query;
    std::transform(lower_content.begin(), lower_content.end(), lower_content.begin(), ::tolower);
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);

    size_t pos = 0;
    size_t found;
    while ((found = lower_content.find(lower_query, pos)) != std::string::npos) {
        // Add text before match
        if (found > pos) {
            result.push_back(ftxui::text(content.substr(pos, found - pos)));
        }
        // Add highlighted match
        result.push_back(ftxui::text(content.substr(found, query.size())) |
                         bgcolor(t.secondary) | color(t.background) | bold);
        pos = found + query.size();
    }
    // Add remaining text after last match
    if (pos < content.size()) {
        result.push_back(ftxui::text(content.substr(pos)));
    }

    return result;
}

// ============================================================
// Rendering — all Element-producing methods
// ============================================================

Element App::renderHeader() {
    auto& t = ThemeManager::getAllThemes()[theme_index_];
    auto themes = ThemeManager::getAllThemes();

    std::string title = "  \xe2\x9c\xa6 OPEN PSALM \xe2\x9c\xa6  ";
    std::string theme_name = "  " + themes[theme_index_].name + "  ";
    std::string translation = "  " + db_->translationName() + "  ";

    auto title_el = text(title) | color(t.title) | bold;
    auto theme_el = text(theme_name) | color(t.accent);
    auto translation_el = text(translation) | color(t.highlight);

    return hbox({
        title_el,
        filler(),
        theme_el | borderLight | color(t.border),
        text(" "),
        translation_el,
    });
}

Element App::renderBookPanel() {
    auto& t = ThemeManager::getAllThemes()[theme_index_];

    auto header = text(" BOOKS ") | color(t.title) | bold | center;

    Elements book_entries;
    for (size_t i = 0; i < books_.size(); i++) {
        bool is_selected = (int)i == selected_book_;
        bool is_focused = focus_panel_ == 0 && is_selected;

        std::string prefix = is_selected ? " > " : "   ";
        auto entry = text(prefix + books_[i]);

        if (is_focused) {
            entry = entry | color(t.highlight) | bold | bgcolor(t.accent);
        } else if (is_selected) {
            entry = entry | color(t.highlight) | bold;
        } else {
            entry = entry | color(t.foreground);
        }
        book_entries.push_back(entry);
    }

    return vbox({
        header,
        separator() | color(t.border),
        vbox(std::move(book_entries)) | flex,
    });
}

Element App::renderChapterBar() {
    auto& t = ThemeManager::getAllThemes()[theme_index_];
    if (books_.empty()) return text("");

    chapters_ = db_->getChapters(books_[selected_book_]);

    Elements chapter_entries;
    for (int ch : chapters_) {
        bool active = ch == selected_chapter_;
        auto el = text(std::to_string(ch));
        if (active) {
            el = el | color(t.background) | bgcolor(t.highlight) | bold;
        } else {
            el = el | color(t.accent);
        }
        chapter_entries.push_back(el);
    }

    return hbox({
        text(" Ch.") | color(t.secondary),
        separator() | color(t.border),
        hbox(std::move(chapter_entries)) | flex | color(t.accent),
    });
}

Element App::renderContent() {
    auto& t = ThemeManager::getAllThemes()[theme_index_];
    if (current_verses_.empty()) {
        return text(" No verses found.") | color(t.secondary);
    }

    Elements verse_lines;
    for (size_t i = 0; i < current_verses_.size(); i++) {
        auto& vr = current_verses_[i];

        std::string verse_num = std::to_string(vr.verse);

        if (vr.verse == 1) {
            std::string heading = books_[selected_book_] + " " + std::to_string(selected_chapter_);
            verse_lines.push_back(text(""));
            verse_lines.push_back(text(heading) | color(t.title) | bold | underlined | color(t.highlight));
            verse_lines.push_back(text(""));
        }

        // Syntax highlighting: color quoted text in verse
        std::vector<std::pair<std::string, ftxui::Color>> colored_parts;
        std::string remaining = vr.text;
        while (!remaining.empty()) {
            // Check for quoted text ("..." or '...')
            size_t quote_start = std::string::npos;
            char quote_char = '"';
            auto dq = remaining.find('"');
            auto sq = remaining.find('\'');
            if (dq != std::string::npos && (sq == std::string::npos || dq < sq)) {
                quote_start = dq;
                quote_char = '"';
            } else if (sq != std::string::npos) {
                quote_start = sq;
                quote_char = '\'';
            }
            
            if (quote_start != std::string::npos) {
                // Text before quote
                if (quote_start > 0) {
                    colored_parts.push_back({remaining.substr(0, quote_start), t.foreground});
                }
                remaining = remaining.substr(quote_start);
                
                // Find closing quote
                size_t close = remaining.find(quote_char, 1);
                if (close != std::string::npos) {
                    colored_parts.push_back({remaining.substr(0, close + 1), t.accent});
                    remaining = remaining.substr(close + 1);
                } else {
                    colored_parts.push_back({remaining, t.accent});
                    remaining.clear();
                }
            } else {
                // Check for parentheses (red-letter words of Christ highlight)
                size_t paren_open = remaining.find('(');
                if (paren_open != std::string::npos) {
                    if (paren_open > 0) {
                        colored_parts.push_back({remaining.substr(0, paren_open), t.foreground});
                    }
                    remaining = remaining.substr(paren_open);
                    size_t paren_close = remaining.find(')');
                    if (paren_close != std::string::npos) {
                        colored_parts.push_back({remaining.substr(0, paren_close + 1), t.secondary});
                        remaining = remaining.substr(paren_close + 1);
                    } else {
                        colored_parts.push_back({remaining, t.foreground});
                        remaining.clear();
                    }
                } else {
                    colored_parts.push_back({remaining, t.foreground});
                    remaining.clear();
                }
            }
        }

        bool is_current_line = (int)i == content_scroll_ && focus_panel_ == 1;

        // If search results are active and this verse is among them, highlight search terms
        bool has_search_highlight = !search_query_.empty() && !search_results_.empty();
        
        if (is_current_line) {
            // Current verse line: use colored parts with highlight theme
            Elements parts;
            parts.push_back(text(" \u25b6 " + verse_num) | color(t.highlight) | bold | size(WIDTH, EQUAL, 6));
            for (auto& [text_part, _] : colored_parts) {
                if (has_search_highlight) {
                    // Split text part at search term boundaries for highlighting
                    auto highlighted = highlightSearchTerms(text_part, search_query_, t);
                    for (auto& sub_el : highlighted) {
                        parts.push_back(sub_el | color(t.highlight) | bold);
                    }
                } else {
                    parts.push_back(text(text_part) | color(t.highlight) | bold);
                }
            }
            auto el = hbox(std::move(parts)) | bgcolor(t.accent);
            verse_lines.push_back(el);
        } else {
            // Normal verse line: colored parts with theme colors
            Elements parts;
            parts.push_back(text(verse_num) | color(t.secondary) | bold | size(WIDTH, EQUAL, 4));
            parts.push_back(text(" ") | color(t.foreground));
            for (auto& [text_part, color_part] : colored_parts) {
                if (has_search_highlight) {
                    // Split text part at search term boundaries for highlighting
                    auto highlighted = highlightSearchTerms(text_part, search_query_, t);
                    for (auto& sub_el : highlighted) {
                        parts.push_back(sub_el | color(color_part));
                    }
                } else {
                    parts.push_back(text(text_part) | color(color_part));
                }
            }
            verse_lines.push_back(hbox(std::move(parts)) | flex);
        }
    }

    return vbox(std::move(verse_lines)) | vscroll_indicator |
           focusPositionRelative(0, content_scroll_ / (float)std::max(1, (int)current_verses_.size())) | flex;
}

Element App::renderSearchOverlay() {
    auto& t = ThemeManager::getAllThemes()[theme_index_];

    Elements results;
    results.push_back(text(" Search: " + search_query_ + (search_query_.empty() ? "" : "_")) | color(t.highlight) | bold);
    results.push_back(separator() | color(t.border));

    if (search_query_.empty()) {
        results.push_back(text(" Type to search. Press Enter to jump to a result.") | color(t.secondary));
    } else if (search_results_.empty()) {
        results.push_back(text(" No results found.") | color(t.secondary));
    } else {
        results.push_back(text(" Found " + std::to_string(search_results_.size()) +
                               " results (n/N to cycle, Enter to jump):") | color(t.accent));
        results.push_back(separator() | color(t.border));

        int start = std::max(0, search_result_index_ - 5);
        int end = std::min((int)search_results_.size(), start + 12);

        for (int i = start; i < end; i++) {
            auto& r = search_results_[i];
            bool active = i == search_result_index_;
            std::string ref = r.book + " " + std::to_string(r.chapter) + ":" + std::to_string(r.verse);

            auto el = text("  " + ref) | color(t.foreground);
            if (active) {
                el = text(" > " + ref) | color(t.highlight) | bold;
            }

            std::string snippet = r.text.substr(0, std::min((size_t)60, r.text.size()));
            if (r.text.size() > 60) snippet += "...";

            results.push_back(el);
            results.push_back(text("    " + snippet) | color(t.secondary));
        }

        if ((int)search_results_.size() > end) {
            results.push_back(text("  ... and " + std::to_string(search_results_.size() - end) + " more") | color(t.secondary));
        }
    }

    return vbox(std::move(results)) | flex;
}

Element App::renderHelpOverlay() {
    auto& t = ThemeManager::getAllThemes()[theme_index_];

    auto title = text(" KEYBOARD SHORTCUTS ") | color(t.title) | bold | center;

    // Grouped shortcuts
    struct ShortcutGroup {
        std::string group_name;
        std::vector<std::pair<std::string, std::string>> shortcuts;
    };

    std::vector<ShortcutGroup> groups = {
        {"Navigation", {
            {"j / k",          "Scroll down / up"},
            {"h / l",          "Switch panels (Books / Content)"},
            {"Tab",            "Cycle focus between panels"},
            {"[ / ]",          "Previous / Next chapter"},
            {"g / G",          "Top / Bottom of chapter"},
            {"1-9",            "Jump to chapter 1-9"},
            {"Space / b",      "Page down / Page up"},
            {"Enter",          "Open book / Focus content"},
        }},
        {"Study Features", {
            {"t",              "Cycle translation"},
            {"p / P",          "Parallel view / Cycle second translation"},
            {"r",              "Reading plan for today"},
            {"u / Ctrl+r",     "Go back / Go forward in history"},
            {"Ctrl+g",         "Go to verse (Book Ch:Vs)"},
            {"J",              "Jump to book (type to filter)"},
            {"x",              "Cross-references for current verse"},
        }},
        {"Search", {
            {"/",              "Search current translation"},
            {"C",              "Cross-translation search (all DBs)"},
            {"n / N",          "Next / Previous search result"},
            {"Up/Down",        "Search history navigation"},
            {"Enter",          "Jump to selected result"},
        }},
        {"Reference Tools", {
            {"s",              "Strong's concordance lookup"},
            {"m / M",          "Bookmark verse / View bookmarks"},
            {"Ctrl+n",         "Edit note on bookmark"},
            {"d (bookmarks)",  "Delete bookmark"},
        }},
        {"Export & Share", {
            {"y",              "Yank (copy) verse text"},
            {"Ctrl+y",         "Copy as $REF format (John 3:16 KJV)"},
            {"Ctrl+u",         "Copy Bible Gateway URL"},
            {"E",              "Export verse to TXT file"},
        }},
        {"Appearance & Misc", {
            {"T",              "Cycle themes"},
            {"?",              "Toggle this help screen"},
            {"q / Esc",        "Quit / Close overlay"},
        }},
    };

    auto section = color(t.accent) | bold;

    Elements lines;
    lines.push_back(separator() | color(t.border));

    for (auto& group : groups) {
        // Group header
        lines.push_back(text("  " + group.group_name + ":") | section);
        for (auto& [key, desc] : group.shortcuts) {
            auto k = text("    " + key) | color(t.highlight) | bold | size(WIDTH, GREATER_THAN, 22);
            auto d = text(desc) | color(t.foreground);
            lines.push_back(hbox({k, d}));
        }
        lines.push_back(text(""));
    }

    lines.push_back(separator() | color(t.border));
    lines.push_back(text(" Press ? to close") | color(t.secondary) | center);

    auto inner = vbox(std::move(lines)) | flex | center;
    return vbox({title, inner});
}

Element App::renderJumpOverlay() {
    auto& t = ThemeManager::getAllThemes()[theme_index_];

    Elements lines;
    lines.push_back(text(" Jump to book: " + jump_query_ + (jump_query_.empty() ? "" : "_")) | color(t.highlight) | bold);
    lines.push_back(separator() | color(t.border));

    if (jump_query_.empty()) {
        lines.push_back(text(" Type the name of a book to jump to it.") | color(t.secondary));
    } else if (jump_results_.empty()) {
        lines.push_back(text(" No matching books found.") | color(t.secondary));
    } else {
        int max_show = 20;
        for (size_t i = 0; i < std::min((size_t)max_show, jump_results_.size()); i++) {
            int idx = jump_results_[i];
            bool active = i == 0;
            auto prefix = active ? " > " : "   ";
            auto el = text(prefix + books_[idx]) | color(t.foreground);
            if (active) el = el | color(t.highlight) | bold;
            lines.push_back(el);
        }
        if (jump_results_.size() > (size_t)max_show) {
            lines.push_back(text("  ... and " + std::to_string(jump_results_.size() - max_show) + " more") | color(t.secondary));
        }
        lines.push_back(separator() | color(t.border));
        lines.push_back(text(" Press Enter to jump, Esc to cancel") | color(t.secondary));
    }

    return vbox(std::move(lines)) | flex;
}

Element App::renderBookmarkOverlay() {
    auto& t = ThemeManager::getAllThemes()[theme_index_];

    auto title = text(" BOOKMARKS (" + std::to_string(bookmarks_.size()) + ") ") | color(t.title) | bold | center;

    Elements lines;
    lines.push_back(separator() | color(t.border));

    if (bookmarks_.empty()) {
        lines.push_back(text(" No bookmarks yet. Press 'm' on a verse to bookmark it.") | color(t.secondary) | center);
    } else {
        int start = std::max(0, bookmark_index_ - 5);
        int end = std::min((int)bookmarks_.size(), start + 15);

        for (int i = start; i < end; i++) {
            auto& bm = bookmarks_[i];
            bool active = i == bookmark_index_;

            std::string ref = bm.book + " " + std::to_string(bm.chapter) + ":" + std::to_string(bm.verse);
            std::string prefix = active ? " > " : "   ";

            lines.push_back(hbox({
                text(prefix + ref) | (active ? color(t.highlight) | bold : color(t.foreground)),
                filler(),
                text(bm.label) | color(t.secondary) | flex,
            }));

            if (active) {
                auto verses = db_->getVerses(bm.book, bm.chapter);
                for (auto& v : verses) {
                    if (v.verse == bm.verse) {
                        lines.push_back(text("    " + v.text.substr(0, std::min((size_t)70, v.text.size()))) | color(t.accent));
                        break;
                    }
                }
            }
        }

        if ((int)bookmarks_.size() > end) {
            lines.push_back(text("  ... and " + std::to_string(bookmarks_.size() - end) + " more") | color(t.secondary));
        }
    }

    lines.push_back(separator() | color(t.border));
    if (!bookmarks_.empty()) {
        lines.push_back(text(" Enter=jump, d=delete, Esc=close") | color(t.secondary) | center);
    } else {
        lines.push_back(text(" Esc to close") | color(t.secondary) | center);
    }

    return vbox({
        title,
        vbox(std::move(lines)) | flex,
    });
}

Element App::renderStatusBar() {
    auto& t = ThemeManager::getAllThemes()[theme_index_];

    if (books_.empty()) return text(" No Bible data loaded.");

    std::string loc = books_[selected_book_] + " " + std::to_string(selected_chapter_);
    std::string verses_info = std::to_string(current_verses_.size()) + " verses";

    int total_books = (int)books_.size();
    int progress_pct = total_books > 0 ? (selected_book_ * 100) / total_books : 0;

    std::string progress_bar;
    int bar_width = 10;
    int filled = (progress_pct * bar_width) / 100;
    progress_bar = "[";
    for (int i = 0; i < bar_width; i++) {
        progress_bar += (i < filled) ? "#" : ".";
    }
    progress_bar += "]";

    auto progress_el = text(" " + progress_bar + " " + std::to_string(progress_pct) + "% ") | color(t.accent);

    // Mode indicators
    std::string modes;
    if (parallel_mode_) modes += " [PAR]";
    if (strongs_mode_) modes += " [STR]";
    if (cross_search_mode_) modes += " [X-SRCH]";
    if (goto_verse_mode_) modes += " [GOTO]";
    if (cross_ref_mode_) modes += " [XREF]";
    if (edit_note_mode_) modes += " [NOTE]";
    if (!plan_entries_.empty()) modes += " [PLAN]";
    // Verse of the day indicator in status bar
    std::string votd_label;
    if (!verse_of_day_.book.empty()) {
        votd_label = " [VOTD: " + verse_of_day_.book + " " +
                     std::to_string(verse_of_day_.chapter) + ":" +
                     std::to_string(verse_of_day_.verse) + "]";
    }

    std::string controls;
    if (focus_panel_ == 0) {
        controls = "[j/k] books  [l] focus content  [/] search";
    } else {
        controls = "[j/k] scroll  [h] focus books  [[/]] ch  [?] help";
    }

    if (notification_timer_ > 0) {
        notification_timer_--;
        controls = notification_;
    }

    return hbox({
        text(" " + loc) | bold | color(t.highlight),
        text("  |  ") | color(t.secondary),
        text(verses_info) | color(t.accent),
        progress_el,
        text(modes + votd_label) | bold | color(t.highlight),
        filler(),
        text(controls) | color(t.secondary),
    });
}

Element App::renderVerseOfDay() {
    auto& t = ThemeManager::getAllThemes()[theme_index_];

    if (verse_of_day_.book.empty()) {
        return text(" Verse of the Day unavailable.") | color(t.secondary) | center;
    }

    std::string ref = verse_of_day_.book + " " +
                      std::to_string(verse_of_day_.chapter) + ":" +
                      std::to_string(verse_of_day_.verse);

    std::string translation = db_->translationName();

    return vbox({
        text("") | size(HEIGHT, EQUAL, 3),
        text(" \xe2\x9c\xa6 VERSE OF THE DAY \xe2\x9c\xa6 ") | color(t.title) | bold | center | underlined,
        text("") | size(HEIGHT, EQUAL, 1),
        text("  \"" + verse_of_day_.text + "\"") | color(t.highlight) | bold | center,
        text("") | size(HEIGHT, EQUAL, 1),
        text("  -- " + ref + " (" + translation + ")") | color(t.accent) | center,
        text("") | size(HEIGHT, EQUAL, 1),
        separator() | color(t.border),
        text(" Press any key to continue") | color(t.secondary) | center,
    }) | flex | center;
}

Element App::renderReadingPlan() {
    auto& t = ThemeManager::getAllThemes()[theme_index_];

    if (plan_entries_.empty()) {
        return text(" No reading plan available.") | color(t.secondary) | center;
    }

    int day = reading_plan_day_;
    if (day >= (int)plan_entries_.size()) {
        day = plan_entries_.size() - 1;
    }

    // Today's passage
    auto& entry = plan_entries_[day];

    // Get the verses for today's passage
    auto verses = db_->getVerses(entry.book, entry.chapter);

    Elements lines;
    lines.push_back(text(" READING PLAN ") | color(t.title) | bold | center);
    lines.push_back(separator() | color(t.border));

    // Reading progress
    int total_days = (int)plan_entries_.size();
    int days_completed = day + 1;
    int pct = (days_completed * 100) / std::max(1, total_days);
    int bar_width = 25;
    int filled = (pct * bar_width) / 100;
    std::string progress_bar = "[";
    for (int i = 0; i < bar_width; i++) {
        progress_bar += (i < filled) ? "\xe2\x96\x88" : "\xe2\x96\x91";
    }
    progress_bar += "]";

    lines.push_back(text(" Day " + std::to_string(day + 1) + " of " +
                         std::to_string(total_days)) | color(t.highlight) | bold | center);
    lines.push_back(text(" " + progress_bar + " " + std::to_string(pct) + "% complete") | color(t.accent) | center);

    std::string passage = entry.book + " " + std::to_string(entry.chapter);
    lines.push_back(text(" " + passage + " ") | color(t.accent) | bold | center);
    lines.push_back(separator() | color(t.border));

    if (verses.empty()) {
        lines.push_back(text(" No verses found for this passage.") | color(t.secondary));
    } else {
        int max_show = std::min((int)verses.size(), 15);
        for (int i = 0; i < max_show; i++) {
            auto& v = verses[i];
            lines.push_back(hbox({
                text(" " + std::to_string(v.verse) + " ") | color(t.secondary) | bold | size(WIDTH, EQUAL, 5),
                text(v.text) | color(t.foreground) | flex,
            }));
        }
        if ((int)verses.size() > max_show) {
            lines.push_back(text(" ... " + std::to_string(verses.size() - max_show) + " more verses") | color(t.secondary));
        }
    }

    lines.push_back(separator() | color(t.border));
    lines.push_back(text(" [n] Next day  [p] Prev day  [Space] Jump to passage  [r/?] Close") | color(t.secondary) | center);

    return vbox(std::move(lines)) | flex;
}

Element App::renderParallelContent() {
    auto& t = ThemeManager::getAllThemes()[theme_index_];

    if (current_verses_.empty() && parallel_verses_.empty()) {
        return text(" No verses to display.") | color(t.secondary);
    }

    // Left panel: primary translation
    Elements left_lines;
    left_lines.push_back(text(" " + db_->translationCode() + " ") | color(t.title) | bold | center);
    left_lines.push_back(separator() | color(t.border));

    for (size_t i = 0; i < current_verses_.size(); i++) {
        auto& vr = current_verses_[i];
        std::string verse_num = std::to_string(vr.verse);
        bool is_current = (int)i == content_scroll_;
        auto el = hbox({
            text(verse_num) | color(t.secondary) | size(WIDTH, EQUAL, 3),
            text(" ") | color(t.foreground),
            text(vr.text) | color(t.foreground) | flex,
        });
        if (is_current) {
            el = hbox({
                text(" \u25b6 " + verse_num) | color(t.highlight) | bold | size(WIDTH, EQUAL, 5),
                text(vr.text) | color(t.highlight) | flex,
            }) | bgcolor(t.accent);
        }
        left_lines.push_back(el);
    }

    // Right panel: second translation
    Elements right_lines;
    std::string parallel_code = parallel_db_ ? parallel_db_->translationCode() : "?";
    right_lines.push_back(text(" " + parallel_code + " ") | color(t.title) | bold | center);
    right_lines.push_back(separator() | color(t.border));

    for (size_t i = 0; i < parallel_verses_.size(); i++) {
        auto& vr = parallel_verses_[i];
        std::string verse_num = std::to_string(vr.verse);
        auto el = hbox({
            text(verse_num) | color(t.secondary) | size(WIDTH, EQUAL, 3),
            text(" ") | color(t.foreground),
            text(vr.text) | color(t.foreground) | flex,
        });
        if ((int)i == parallel_scroll_) {
            el = hbox({
                text(" \u25b6 " + verse_num) | color(t.highlight) | bold | size(WIDTH, EQUAL, 5),
                text(vr.text) | color(t.highlight) | flex,
            }) | bgcolor(t.accent);
        }
        right_lines.push_back(el);
    }

    auto left_panel = vbox(std::move(left_lines)) | vscroll_indicator | flex | border | color(t.border);
    auto right_panel = vbox(std::move(right_lines)) | vscroll_indicator | flex | border | color(t.border);

    return hbox({left_panel, right_panel}) | flex;
}

Element App::renderCrossSearchOverlay() {
    auto& t = ThemeManager::getAllThemes()[theme_index_];

    Elements results;
    results.push_back(text(" Cross-Search [all translations]: " + search_query_ + (search_query_.empty() ? "" : "_")) | color(t.highlight) | bold);
    results.push_back(separator() | color(t.border));

    if (search_query_.empty()) {
        results.push_back(text(" Type to search across all translations.") | color(t.secondary));
    } else if (cross_search_results_.empty()) {
        results.push_back(text(" No results found across any translation.") | color(t.secondary));
    } else {
        results.push_back(text(" Found " + std::to_string(cross_search_results_.size()) +
                               " results (n/N to cycle, Enter to jump):") | color(t.accent));
        results.push_back(separator() | color(t.border));

        int start = std::max(0, search_result_index_ - 5);
        int end = std::min((int)cross_search_results_.size(), start + 12);

        for (int i = start; i < end; i++) {
            auto& r = cross_search_results_[i];
            bool active = i == search_result_index_;
            std::string ref = r.first + " | " + r.second.book + " " +
                              std::to_string(r.second.chapter) + ":" + std::to_string(r.second.verse);

            auto el = text("  " + ref) | color(t.foreground);
            if (active) {
                el = text(" > " + ref) | color(t.highlight) | bold;
            }
            results.push_back(el);
            std::string snippet = r.second.text.substr(0, std::min((size_t)50, r.second.text.size()));
            if (r.second.text.size() > 50) snippet += "...";
            results.push_back(text("    " + snippet) | color(t.secondary));
        }

        if ((int)cross_search_results_.size() > end) {
            results.push_back(text("  ... and " + std::to_string(cross_search_results_.size() - end) + " more") | color(t.secondary));
        }
    }

    return vbox(std::move(results)) | flex;
}

Element App::renderReadingReminder() {
    auto& t = ThemeManager::getAllThemes()[theme_index_];

    if (plan_entries_.empty()) {
        return text(" No reading plan available.") | color(t.secondary) | center;
    }

    int day = std::min(reading_plan_day_, (int)plan_entries_.size() - 1);
    auto& entry = plan_entries_[day];

    std::string passage = entry.book + " " + std::to_string(entry.chapter);

    return vbox({
        text("") | size(HEIGHT, EQUAL, 5),
        text(" \xe2\x9c\xa6 DAILY READING REMINDER \xe2\x9c\xa6 ") | color(t.title) | bold | center | underlined,
        text("") | size(HEIGHT, EQUAL, 1),
        text(" Your reading for today:") | color(t.accent) | center,
        text(" Day " + std::to_string(day + 1) + " of " + std::to_string(plan_entries_.size())) | color(t.secondary) | center,
        text("") | size(HEIGHT, EQUAL, 1),
        text(" \xe2\x87\x92  " + passage + " ") | color(t.highlight) | bold | center,
        text("") | size(HEIGHT, EQUAL, 1),
        text(" Press 'r' to open the reading plan") | color(t.secondary) | center,
        text(" Press any key to dismiss") | color(t.secondary) | center,
    }) | flex | center;
}

Element App::renderStrongsOverlay() {
    auto& t = ThemeManager::getAllThemes()[theme_index_];

    Elements lines;
    lines.push_back(text(" STRONG'S CONCORDANCE ") | color(t.title) | bold | center);
    lines.push_back(separator() | color(t.border));

    lines.push_back(text(" Enter Strong's number (e.g., H7225, G1234) or search term:") | color(t.secondary));
    lines.push_back(text(" > " + strongs_query_ + (strongs_query_.empty() ? "" : "_")) | color(t.highlight) | bold);
    lines.push_back(separator() | color(t.border));

    if (!strongs_result_.empty()) {
        lines.push_back(text(" " + strongs_query_ + " ") | color(t.accent) | bold);
        lines.push_back(text(""));
        lines.push_back(text(" " + strongs_result_) | color(t.foreground) | flex);
    } else if (!strongs_query_.empty()) {
        lines.push_back(text(" No results found.") | color(t.secondary));
    } else {
        lines.push_back(text(" Start typing a Strong's number or word to search.") | color(t.secondary));
    }

    lines.push_back(separator() | color(t.border));
    lines.push_back(text(" Esc to close") | color(t.secondary) | center);

    return vbox(std::move(lines)) | flex;
}

Element App::renderCrossReferenceOverlay() {
    auto& t = ThemeManager::getAllThemes()[theme_index_];

    Elements lines;
    lines.push_back(text(" \xe2\x86\x94 CROSS-REFERENCES ") | color(t.title) | bold | center);
    lines.push_back(separator() | color(t.border));

    if (current_cross_refs_.empty()) {
        lines.push_back(text(" No cross-references found for this verse.") | color(t.secondary) | center);
    } else {
        lines.push_back(text(" Related passages (" + std::to_string(current_cross_refs_.size()) + "):") | color(t.accent));
        lines.push_back(separator() | color(t.border));

        int max_show = std::min((size_t)20, current_cross_refs_.size());
        int start = std::max(0, std::min(cross_ref_index_ - 5, (int)current_cross_refs_.size() - max_show));

        for (int i = start; i < start + max_show; i++) {
            if (i >= (int)current_cross_refs_.size()) break;
            auto& cr = current_cross_refs_[i];
            bool active = i == cross_ref_index_;

            std::string prefix = active ? " > " : "   ";
            std::string ref = prefix + cr.to_book + " " + std::to_string(cr.to_chapter) + ":" + std::to_string(cr.to_verse_start);
            if (cr.to_verse_end > cr.to_verse_start) {
                ref += "-" + std::to_string(cr.to_verse_end);
            }
            std::string votes_str = " (" + std::to_string(cr.votes) + ")";

            auto ref_el = text(ref) | bold;
            if (active) {
                ref_el = ref_el | color(t.highlight) | bgcolor(t.accent);
            } else {
                ref_el = ref_el | color(t.foreground);
            }

            lines.push_back(hbox({
                ref_el,
                filler(),
                text(votes_str) | color(t.secondary),
            }));
        }

        // Show keyboard hints
        lines.push_back(separator() | color(t.border));
        lines.push_back(text(" j/k navigate  Enter=jump  x/Esc=close") | color(t.secondary) | center);

        if (current_cross_refs_.size() > 20) {
            lines.push_back(text("  ... and " + std::to_string(current_cross_refs_.size() - 20) + " more") | color(t.secondary));
        }
    }

    return vbox(std::move(lines)) | flex;
}

Element App::renderGotoVerseOverlay() {
    auto& t = ThemeManager::getAllThemes()[theme_index_];

    std::string stage_label;
    switch (goto_verse_input_stage_) {
        case 0: stage_label = "Book"; break;
        case 1: stage_label = "Chapter"; break;
        case 2: stage_label = "Verse"; break;
        default: stage_label = "?";
    }

    Elements lines;
    lines.push_back(text(" \xe2\x87\xa1 GO TO VERSE ") | color(t.title) | bold | center);
    lines.push_back(separator() | color(t.border));

    // Show input so far
    std::string input_so_far;
    if (!goto_verse_book_.empty()) input_so_far += goto_verse_book_;
    if (!goto_verse_chapter_str_.empty()) input_so_far += " " + goto_verse_chapter_str_;
    if (!goto_verse_verse_str_.empty()) input_so_far += ":" + goto_verse_verse_str_;
    if (!input_so_far.empty()) {
        lines.push_back(text("  " + input_so_far) | color(t.accent) | bold);
    }

    lines.push_back(text("  " + stage_label + ": " + goto_verse_query_ + (goto_verse_query_.empty() ? "" : "_")) | color(t.highlight) | bold);
    lines.push_back(separator() | color(t.border));

    // Show matching book if in stage 0
    if (goto_verse_input_stage_ == 0 && !goto_verse_query_.empty()) {
        std::string lower_q = goto_verse_query_;
        std::transform(lower_q.begin(), lower_q.end(), lower_q.begin(), ::tolower);
        int found_idx = -1;
        for (size_t i = 0; i < books_.size(); i++) {
            std::string bl = books_[i];
            std::transform(bl.begin(), bl.end(), bl.begin(), ::tolower);
            if (bl.find(lower_q) != std::string::npos) {
                found_idx = (int)i;
                break;
            }
        }
        if (found_idx >= 0) {
            lines.push_back(text("  \u2192 " + books_[found_idx]) | color(t.accent));
        } else {
            lines.push_back(text("  No matching book") | color(t.secondary));
        }
    }

    lines.push_back(separator() | color(t.border));
    lines.push_back(text(" Tab/Space: next field | Enter: jump | Esc: cancel") | color(t.secondary) | center);

    return vbox(std::move(lines)) | flex;
}
