#include "app.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/dom/elements.hpp>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <ctime>
#include <filesystem>
namespace fs = std::filesystem;

using namespace ftxui;

// Reading plan: approximate chapter-per-day through the Bible in ~365 days
static const std::vector<std::pair<const char*, int>> PLAN_BOOKS = {
    {"Genesis", 50}, {"Exodus", 40}, {"Leviticus", 27}, {"Numbers", 36},
    {"Deuteronomy", 34}, {"Joshua", 24}, {"Judges", 21}, {"Ruth", 4},
    {"1 Samuel", 31}, {"2 Samuel", 24}, {"1 Kings", 22}, {"2 Kings", 25},
    {"1 Chronicles", 29}, {"2 Chronicles", 36}, {"Ezra", 10}, {"Nehemiah", 13},
    {"Esther", 10}, {"Job", 42}, {"Psalms", 150}, {"Proverbs", 31},
    {"Ecclesiastes", 12}, {"Song of Solomon", 8}, {"Isaiah", 66},
    {"Jeremiah", 52}, {"Lamentations", 5}, {"Ezekiel", 48}, {"Daniel", 12},
    {"Hosea", 14}, {"Joel", 3}, {"Amos", 9}, {"Obadiah", 1}, {"Jonah", 4},
    {"Micah", 7}, {"Nahum", 3}, {"Habakkuk", 3}, {"Zephaniah", 3},
    {"Haggai", 2}, {"Zechariah", 14}, {"Malachi", 4},
    {"Matthew", 28}, {"Mark", 16}, {"Luke", 24}, {"John", 21},
    {"Acts", 28}, {"Romans", 16}, {"1 Corinthians", 16}, {"2 Corinthians", 13},
    {"Galatians", 6}, {"Ephesians", 6}, {"Philippians", 4}, {"Colossians", 4},
    {"1 Thessalonians", 5}, {"2 Thessalonians", 3}, {"1 Timothy", 6},
    {"2 Timothy", 4}, {"Titus", 3}, {"Philemon", 1}, {"Hebrews", 13},
    {"James", 5}, {"1 Peter", 5}, {"2 Peter", 3}, {"1 John", 5},
    {"2 John", 1}, {"3 John", 1}, {"Jude", 1}, {"Revelation", 22},
};

App::App(std::unique_ptr<Database> db, const std::vector<TranslationInfo>& translations,
         const CliOptions& cli)
    : db_(std::move(db)), translation_list_(translations), screen_(ScreenInteractive::Fullscreen()) {

    books_ = db_->getBooks();

    // Load saved config (theme, last position, translation)
    loadConfig();

    // Restore translation from config
    if (!config_.translation_code.empty()) {
        for (size_t i = 0; i < translation_list_.size(); i++) {
            if (translation_list_[i].code == config_.translation_code) {
                current_translation_ = i;
                break;
            }
        }
    }

    if (!books_.empty()) {
        // If config had a saved position, jump to it
        if (!config_.last_book.empty()) {
            for (size_t i = 0; i < books_.size(); i++) {
                if (books_[i] == config_.last_book) {
                    selected_book_ = i;
                    selected_chapter_ = config_.last_chapter;
                    break;
                }
            }
        }
        loadChapter();
    }

    // Restore theme from config
    theme_index_ = config_.theme_index;

    // CLI theme override
    if (!cli.theme.empty()) {
        auto themes = ThemeManager::getAllThemes();
        for (size_t i = 0; i < themes.size(); i++) {
            std::string tn = themes[i].name;
            // Case-insensitive comparison
            std::transform(tn.begin(), tn.end(), tn.begin(), ::tolower);
            std::string clt = cli.theme;
            std::transform(clt.begin(), clt.end(), clt.begin(), ::tolower);
            if (tn == clt) {
                theme_index_ = (int)i;
                break;
            }
        }
    }

    // Generate reading plan
    generatePlan();

    // Compute verse of the day
    verse_of_day_ = computeVerseOfDay();

    // Open Strong's concordance database
    {
        fs::path strongs_path = fs::current_path() / "data" / "strongs.db";
        if (!fs::exists(strongs_path)) {
            try {
                fs::path exe_p = fs::read_symlink("/proc/self/exe").parent_path().parent_path();
                strongs_path = exe_p / "data" / "strongs.db";
            } catch (...) {}
        }
        if (fs::exists(strongs_path)) {
            strongs_db_ = std::make_unique<Database>(strongs_path.string(), "STR", "Strong's Concordance");
        }
    }

    // Open cross-references database
    {
        fs::path xref_path = fs::current_path() / "data" / "cross_references.db";
        if (!fs::exists(xref_path)) {
            try {
                fs::path exe_p = fs::read_symlink("/proc/self/exe").parent_path().parent_path();
                xref_path = exe_p / "data" / "cross_references.db";
            } catch (...) {}
        }
        if (fs::exists(xref_path)) {
            cross_refs_db_ = std::make_unique<Database>(xref_path.string(), "XREF", "Cross-References");
        }
    }

    // Pre-cache cross-search DBs for fast switching
    {
        for (size_t ti = 0; ti < translation_list_.size(); ti++) {
            if ((int)ti == current_translation_) {
                cross_search_dbs_.push_back(nullptr); // current translation uses db_ directly
                continue;
            }
            fs::path exe_p;
            try { exe_p = fs::read_symlink("/proc/self/exe").parent_path().parent_path(); }
            catch (...) { exe_p = fs::current_path(); }
            fs::path db_path = exe_p / "data" / translation_list_[ti].db_file;
            if (fs::exists(db_path)) {
                auto cached = std::make_unique<Database>(db_path.string(),
                    translation_list_[ti].code, translation_list_[ti].name);
                if (cached->isOpen()) {
                    cross_search_dbs_.push_back(std::move(cached));
                } else {
                    cross_search_dbs_.push_back(nullptr);
                }
            } else {
                cross_search_dbs_.push_back(nullptr);
            }
        }
    }

    // Open parallel translation DB if configured
    if (config_.parallel_enabled) {
        for (size_t i = 0; i < translation_list_.size(); i++) {
            if (translation_list_[i].code == config_.parallel_second_translation &&
                (int)i != current_translation_) {
                fs::path exe_path;
                try { exe_path = fs::read_symlink("/proc/self/exe").parent_path().parent_path(); }
                catch (...) { exe_path = fs::current_path(); }
                fs::path pdb_path = exe_path / "data" / translation_list_[i].db_file;
                if (fs::exists(pdb_path)) {
                    parallel_db_ = std::make_unique<Database>(pdb_path.string(),
                                                              translation_list_[i].code,
                                                              translation_list_[i].name);
                    if (parallel_db_->isOpen()) {
                        parallel_book_ = selected_book_;
                        parallel_chapter_ = selected_chapter_;
                        loadParallelChapter();
                    }
                }
                break;
            }
        }
    }

    // Check for reading reminder
    reading_reminder_shown_ = (reading_plan_day_ > 0);

    // Use a simple Menu component for book navigation state
    book_menu_ = Menu(&books_, &selected_book_);

    // Main content renderer
    content_renderer_ = Renderer([this] {
        auto& t = ThemeManager::getAllThemes()[theme_index_];

        // Show verse of day on first render
        if (!dotd_shown_) {
            return renderVerseOfDay() | flex;
        }

        // Show reading reminder if applicable (after VOTD)
        if (reading_reminder_shown_) {
            return renderReadingReminder() | flex;
        }

        if (help_mode_) {
            return renderHelpOverlay() | flex;
        }
        if (bookmark_mode_) {
            return renderBookmarkOverlay() | flex;
        }
        if (jump_mode_) {
            return renderJumpOverlay() | flex;
        }
        if (plan_mode_) {
            return renderReadingPlan() | flex;
        }
        if (strongs_mode_) {
            return renderStrongsOverlay() | flex;
        }
        if (cross_ref_mode_) {
            return renderCrossReferenceOverlay() | flex;
        }
        if (goto_verse_mode_) {
            return renderGotoVerseOverlay() | flex;
        }
        if (cross_search_mode_) {
            return renderCrossSearchOverlay() | flex;
        }
        if (search_mode_) {
            return renderSearchOverlay() | flex;
        }

        if (parallel_mode_ && parallel_db_ && parallel_db_->isOpen()) {
            auto header = renderChapterBar();
            auto parallel = renderParallelContent();
            return vbox({header | color(t.highlight), separator(), parallel | color(t.foreground)}) | flex;
        }

        auto header = renderChapterBar();
        auto verses = renderContent();
        return vbox({header | color(t.highlight), separator(), verses | color(t.foreground)}) | flex;
    });

    // Main layout
    main_container_ = Container::Horizontal({
        book_menu_,
        content_renderer_,
    });

    focus_panel_ = 0;

    // Apply CLI overrides
    if (!cli.book.empty()) {
        for (size_t i = 0; i < books_.size(); i++) {
            std::string bl = books_[i];
            std::transform(bl.begin(), bl.end(), bl.begin(), ::tolower);
            std::string cl = cli.book;
            std::transform(cl.begin(), cl.end(), cl.begin(), ::tolower);
            if (bl.find(cl) != std::string::npos || cl.find(bl) != std::string::npos) {
                selected_book_ = (int)i;
                config_.last_book = books_[i];
                if (cli.chapter > 0) {
                    selected_chapter_ = cli.chapter;
                    config_.last_chapter = cli.chapter;
                }
                break;
            }
        }
        loadChapter();
        content_scroll_ = std::max(0, (int)current_verses_.size() - 1);
        if (cli.verse > 0) {
            content_scroll_ = std::min(cli.verse - 1, std::max(0, (int)current_verses_.size() - 1));
        }
    }

    // Auto-copy verse of day to clipboard
    autoCopyVotd();

    // Show welcome notification
    int total_chapters = 0;
    for (const auto& b : books_) {
        total_chapters += (int)db_->getChapters(b).size();
    }
    showNotification("Open Psalm :: " + std::to_string(books_.size()) + " books, " +
                     std::to_string(total_chapters) + " chapters | Press ? for help");
}

App::~App() {
    saveConfig();
}

void App::generatePlan() {
    plan_entries_.clear();
    for (const auto& [book, chapters] : PLAN_BOOKS) {
        if (chapters <= 2) {
            // Short books: one day
            NavPosition entry;
            entry.book = book;
            entry.chapter = 1;
            plan_entries_.push_back(entry);
        } else {
            // Longer books: one chapter per day
            for (int ch = 1; ch <= chapters; ch++) {
                NavPosition entry;
                entry.book = book;
                entry.chapter = ch;
                plan_entries_.push_back(entry);
            }
        }
    }

    // Clamp saved day in case plan changed
    reading_plan_day_ = std::min(reading_plan_day_, (int)plan_entries_.size() - 1);
    if (reading_plan_day_ < 0) reading_plan_day_ = 0;
}

void App::togglePlan() {
    plan_mode_ = !plan_mode_;
    if (plan_mode_) {
        showNotification("Reading Plan: Day " + std::to_string(reading_plan_day_ + 1) +
                         " of " + std::to_string(plan_entries_.size()));
    }
}

void App::switchTranslation(int delta) {
    if (translation_list_.empty()) return;

    int new_idx = (current_translation_ + delta) % (int)translation_list_.size();
    if (new_idx < 0) new_idx += (int)translation_list_.size();
    if (new_idx == current_translation_) return;

    const auto& info = translation_list_[new_idx];

    // Determine db path relative to current working directory or exe
    fs::path data_dir = fs::current_path() / "data";
    fs::path db_path = data_dir / info.db_file;

    // Fallback: check relative to project root
    if (!fs::exists(db_path)) {
        fs::path exe_path;
        try {
            exe_path = fs::read_symlink("/proc/self/exe").parent_path().parent_path();
        } catch (...) {
            exe_path = fs::current_path();
        }
        db_path = exe_path / "data" / info.db_file;
    }

    if (!fs::exists(db_path)) {
        showNotification("Translation '" + info.code + "' not found at " + db_path.string());
        return;
    }

    // Open new database
    auto new_db = std::make_unique<Database>(db_path.string(), info.code, info.name);
    if (!new_db->isOpen()) {
        showNotification("Failed to open translation: " + info.code);
        return;
    }

    int old_idx = current_translation_;
    current_translation_ = new_idx;
    db_ = std::move(new_db);

    // Update cross-search cache: swap cached DB between old and new slots
    if (old_idx >= 0 && old_idx < (int)cross_search_dbs_.size() &&
        new_idx >= 0 && new_idx < (int)cross_search_dbs_.size()) {
        // Old current slot gets the cached DB from the new translation
        // New current slot becomes nullptr (uses db_ directly)
        cross_search_dbs_[old_idx] = std::move(cross_search_dbs_[new_idx]);
        cross_search_dbs_[new_idx] = nullptr;
    }

    // Reload books list
    books_ = db_->getBooks();
    if (!books_.empty()) {
        selected_book_ = std::min(selected_book_, (int)books_.size() - 1);
        loadChapter();
    }
    content_scroll_ = 0;

    showNotification("Switched to " + info.name);
}

VerseRef App::computeVerseOfDay() {
    std::time_t now = std::time(nullptr);
    std::tm* tm = std::localtime(&now);
    int day_of_year = tm->tm_yday; // 0-365

    if (books_.empty()) return {};

    // Use day of year to deterministically pick a verse
    int book_idx = day_of_year % books_.size();
    std::string book = books_[book_idx];

    auto chapters = db_->getChapters(book);
    if (chapters.empty()) return {};

    int ch_idx = (day_of_year / books_.size()) % chapters.size();
    int chapter = chapters[ch_idx];

    auto verses = db_->getVerses(book, chapter);
    if (verses.empty()) return {};

    int v_idx = (day_of_year / (books_.size() * (int)chapters.size() + 1)) % verses.size();

    return verses[v_idx];
}

ftxui::Component App::buildComponent() {
    auto component = CatchEvent(main_container_, [this](Event event) {
        auto& t = ThemeManager::getAllThemes()[theme_index_];

        // Quit
        if (event == Event::Character("q")) {
            if (search_mode_) { toggleSearch(); return true; }
            if (help_mode_) { toggleHelp(); return true; }
            if (jump_mode_) { toggleJumpMode(); return true; }
            if (bookmark_mode_) { toggleBookmarkMode(); return true; }
            if (plan_mode_) { togglePlan(); return true; }
            screen_.ExitLoopClosure()();
            return true;
        }

        // Theme cycling (T)
        if (event == Event::Character("T")) {
            auto themes = ThemeManager::getAllThemes();
            theme_index_ = (theme_index_ + 1) % themes.size();
            showNotification("Theme: " + themes[theme_index_].name);
            return true;
        }

        // Translation cycling (t) — only when VOTD is dismissed
        if (event == Event::Character("t") && dotd_shown_) {
            switchTranslation(1);
            return true;
        }

        // Any key dismisses verse of day overlay
        if (!dotd_shown_) {
            dotd_shown_ = true;
            return true;
        }

        // Dismiss reading reminder on any key
        if (reading_reminder_shown_) {
            reading_reminder_shown_ = false;
            return true;
        }

        // Help toggle (?)
        if (event == Event::Character("?") && !search_mode_ && !jump_mode_ && !bookmark_mode_ && !plan_mode_) {
            toggleHelp();
            return true;
        }

        // Bookmark mode (M - uppercase)
        if (event == Event::Character("M") && !search_mode_ && !help_mode_ && !jump_mode_ && !plan_mode_) {
            toggleBookmarkMode();
            return true;
        }

        // Jump mode (J)
        if (event == Event::Character("J") && !search_mode_ && !help_mode_ && !bookmark_mode_ && !plan_mode_) {
            toggleJumpMode();
            return true;
        }

        // Reading plan mode (r)
        if (event == Event::Character("r") && !search_mode_ && !help_mode_ && !jump_mode_ && !bookmark_mode_ && !strongs_mode_ && !cross_search_mode_) {
            togglePlan();
            return true;
        }

        // Parallel mode (p)
        if (event == Event::Character("p") && !search_mode_ && !help_mode_ && !jump_mode_ && !bookmark_mode_ && !plan_mode_ && !strongs_mode_ && !cross_search_mode_) {
            toggleParallelMode();
            return true;
        }

        // Cycle second translation in parallel mode (P)
        if (event == Event::Character("P") && !search_mode_ && !help_mode_ && !jump_mode_ && !bookmark_mode_ && !plan_mode_ && !strongs_mode_ && !cross_search_mode_) {
            cycleParallelTranslation();
            return true;
        }

        // Cross-translation search (C)
        if (event == Event::Character("C") && !help_mode_ && !jump_mode_ && !bookmark_mode_ && !plan_mode_ && !strongs_mode_ && !cross_ref_mode_) {
            toggleCrossSearch();
            return true;
        }

        // Strong's mode (s)
        if (event == Event::Character("s") && !search_mode_ && !help_mode_ && !jump_mode_ && !bookmark_mode_ && !plan_mode_ && !cross_search_mode_ && !cross_ref_mode_) {
            toggleStrongsMode();
            return true;
        }

        // === HELP MODE ===
        if (help_mode_) {
            return true;
        }

        // === EDIT NOTE MODE ===
        if (edit_note_mode_) {
            if (event == Event::Escape) {
                edit_note_mode_ = false;
                edit_note_index_ = -1;
                edit_note_text_.clear();
                return true;
            }
            if (event == Event::Backspace && !edit_note_text_.empty()) {
                edit_note_text_.pop_back();
                return true;
            }
            if (event == Event::Return) {
                saveEditNote();
                return true;
            }
            if (event.is_character()) {
                auto c = event.character();
                if (!c.empty() && c[0] >= 32 && c[0] < 127) {
                    edit_note_text_ += c[0];
                    return true;
                }
            }
            return true;
        }

        // === BOOKMARK MODE ===
        if (bookmark_mode_) {
            if (event == Event::Character("j") || event == Event::ArrowDown) {
                bookmark_index_ = std::min(bookmark_index_ + 1, (int)bookmarks_.size() - 1);
                return true;
            }
            if (event == Event::Character("k") || event == Event::ArrowUp) {
                bookmark_index_ = std::max(bookmark_index_ - 1, 0);
                return true;
            }
            if (event == Event::Return) {
                if (!bookmarks_.empty() && bookmark_index_ < (int)bookmarks_.size()) {
                    auto& bm = bookmarks_[bookmark_index_];
                    pushNavHistory();
                    jumpToLocation(bm.book, bm.chapter, bm.verse);
                }
                toggleBookmarkMode();
                return true;
            }
            if (event == Event::Character("d")) {
                if (!bookmarks_.empty() && bookmark_index_ < (int)bookmarks_.size()) {
                    bookmarks_.erase(bookmarks_.begin() + bookmark_index_);
                    bookmark_index_ = std::min(bookmark_index_, (int)bookmarks_.size() - 1);
                    showNotification("Bookmark deleted");
                }
                return true;
            }
            if (event == Event::Character("\x0e")) { // Ctrl+n
                if (!bookmarks_.empty() && bookmark_index_ < (int)bookmarks_.size()) {
                    toggleEditNote(bookmark_index_);
                }
                return true;
            }
            if (event == Event::Escape) {
                toggleBookmarkMode();
                return true;
            }
            return true;
        }

        // === READING PLAN MODE ===
        if (plan_mode_) {
            if (event == Event::Character("n") || event == Event::Character("j")) {
                reading_plan_day_ = std::min(reading_plan_day_ + 1, (int)plan_entries_.size() - 1);
                showNotification("Reading Plan: Day " + std::to_string(reading_plan_day_ + 1) +
                                 " of " + std::to_string(plan_entries_.size()));
                return true;
            }
            if (event == Event::Character("p") || event == Event::Character("k")) {
                reading_plan_day_ = std::max(reading_plan_day_ - 1, 0);
                showNotification("Reading Plan: Day " + std::to_string(reading_plan_day_ + 1) +
                                 " of " + std::to_string(plan_entries_.size()));
                return true;
            }
            if (event == Event::Character(" ") || event == Event::Return) {
                if (!plan_entries_.empty()) {
                    auto& entry = plan_entries_[reading_plan_day_];
                    pushNavHistory();
                    // Find the book in the current translation (case-insensitive)
                    int found_book = -1;
                    std::string target_lower = entry.book;
                    std::transform(target_lower.begin(), target_lower.end(), target_lower.begin(), ::tolower);
                    for (size_t i = 0; i < books_.size(); i++) {
                        std::string book_lower = books_[i];
                        std::transform(book_lower.begin(), book_lower.end(), book_lower.begin(), ::tolower);
                        if (book_lower == target_lower ||
                            book_lower.find(target_lower) != std::string::npos ||
                            target_lower.find(book_lower) != std::string::npos) {
                            found_book = (int)i;
                            break;
                        }
                    }
                    if (found_book >= 0) {
                        selected_book_ = found_book;
                        selected_chapter_ = entry.chapter;
                        loadChapter();
                        content_scroll_ = 0;
                        focus_panel_ = 1;
                        plan_mode_ = false;
                        showNotification("Reading: " + entry.book + " " + std::to_string(entry.chapter));
                    }
                }
                return true;
            }
            if (event == Event::Escape || event == Event::Character("q") || event == Event::Character("r")) {
                togglePlan();
                return true;
            }
            return true;
        }

        // === JUMP MODE ===
        if (jump_mode_) {
            if (event == Event::Escape) {
                toggleJumpMode();
                return true;
            }
            if (event == Event::Backspace && !jump_query_.empty()) {
                jump_query_.pop_back();
                if (jump_query_.empty()) {
                    jump_results_.clear();
                } else {
                    jump_results_.clear();
                    std::string lower_q = jump_query_;
                    std::transform(lower_q.begin(), lower_q.end(), lower_q.begin(), ::tolower);
                    for (size_t i = 0; i < books_.size(); i++) {
                        std::string book_lower = books_[i];
                        std::transform(book_lower.begin(), book_lower.end(), book_lower.begin(), ::tolower);
                        if (book_lower.find(lower_q) != std::string::npos) {
                            jump_results_.push_back(i);
                        }
                    }
                }
                return true;
            }
            if (event.is_character()) {
                auto c = event.character();
                if (!c.empty() && c[0] >= 32 && c[0] < 127) {
                    jump_query_ += c[0];
                    jump_results_.clear();
                    std::string lower_q = jump_query_;
                    std::transform(lower_q.begin(), lower_q.end(), lower_q.begin(), ::tolower);
                    for (size_t i = 0; i < books_.size(); i++) {
                        std::string book_lower = books_[i];
                        std::transform(book_lower.begin(), book_lower.end(), book_lower.begin(), ::tolower);
                        if (book_lower.find(lower_q) != std::string::npos) {
                            jump_results_.push_back(i);
                        }
                    }
                    return true;
                }
            }
            if (event == Event::Return && !jump_results_.empty()) {
                pushNavHistory();
                selected_book_ = jump_results_[0];
                selected_chapter_ = 1;
                auto chs = db_->getChapters(books_[selected_book_]);
                if (!chs.empty()) selected_chapter_ = chs[0];
                loadChapter();
                content_scroll_ = 0;
                focus_panel_ = 1;
                toggleJumpMode();
                showNotification("Jumped to " + books_[selected_book_] + " " + std::to_string(selected_chapter_));
                return true;
            }
            return true;
        }

        // Enter search mode with /
        if (event == Event::Character("/") && !search_mode_ && !help_mode_ && !jump_mode_ && !bookmark_mode_ && !plan_mode_ && !strongs_mode_ && !cross_search_mode_) {
            toggleSearch();
            return true;
        }

        // Back navigation (u)
        if (event == Event::Character("u") && !search_mode_ && !help_mode_ && !jump_mode_ && !bookmark_mode_ && !plan_mode_) {
            goBack();
            return true;
        }

        // Forward navigation (Ctrl+r = 0x12)
        if (event == Event::Character("\x12") && !search_mode_) {
            goForward();
            return true;
        }

        // Mark bookmark (m)
        if (event == Event::Character("m") && !search_mode_ && !help_mode_ && !jump_mode_ && !bookmark_mode_ && !plan_mode_ && !cross_ref_mode_) {
            saveBookmark();
            return true;
        }

        // Yank verse (y)
        if (event == Event::Character("y") && !search_mode_ && !help_mode_ && !jump_mode_ && !bookmark_mode_ && !plan_mode_ && !cross_ref_mode_ && focus_panel_ == 1) {
            copyCurrentVerse();
            return true;
        }

        // === STRONG'S MODE ===
        if (strongs_mode_) {
            if (event == Event::Escape) {
                toggleStrongsMode();
                return true;
            }
            if (event.is_character()) {
                auto c = event.character();
                if (!c.empty() && c[0] >= 32 && c[0] < 127) {
                    strongs_query_ += c[0];
                    lookupStrongs(strongs_query_);
                    return true;
                }
            }
            if (event == Event::Backspace && !strongs_query_.empty()) {
                strongs_query_.pop_back();
                if (strongs_query_.empty()) {
                    strongs_result_.clear();
                } else {
                    lookupStrongs(strongs_query_);
                }
                return true;
            }
            return true;
        }

        // === CROSS-SEARCH MODE ===
        // Helper lambda to run cross-search across all cached DBs
        auto runCrossSearch = [&]() {
            cross_search_results_.clear();
            if (search_query_.empty()) return;
            std::string fts_query = search_query_ + "*";
            // Current translation
            auto cur_results = db_->search(fts_query);
            for (auto& r : cur_results) {
                cross_search_results_.push_back({db_->translationCode(), r});
            }
            // Cached other translations
            for (size_t ti = 0; ti < cross_search_dbs_.size(); ti++) {
                if (!cross_search_dbs_[ti]) continue;
                if ((int)ti == current_translation_) continue;
                auto other_results = cross_search_dbs_[ti]->search(fts_query);
                for (auto& r : other_results) {
                    cross_search_results_.push_back({cross_search_dbs_[ti]->translationCode(), r});
                }
            }
            search_result_index_ = 0;
        };

        if (cross_search_mode_) {
            if (event == Event::Escape) {
                toggleCrossSearch();
                return true;
            }
            if (event.is_character()) {
                auto c = event.character();
                if (!c.empty() && c[0] >= 32 && c[0] < 127) {
                    search_query_ += c[0];
                    runCrossSearch();
                    return true;
                }
            }
            if (event == Event::Backspace && !search_query_.empty()) {
                search_query_.pop_back();
                runCrossSearch();
                search_result_index_ = 0;
                return true;
            }
            if (event == Event::Return) {
                if (!cross_search_results_.empty()) {
                    auto& r = cross_search_results_[search_result_index_];
                    // If result is from a different translation, switch to it
                    if (r.first != db_->translationCode()) {
                        for (size_t ti = 0; ti < translation_list_.size(); ti++) {
                            if (translation_list_[ti].code == r.first) {
                                // Switch translation
                                fs::path exe_p;
                                try { exe_p = fs::read_symlink("/proc/self/exe").parent_path().parent_path(); }
                                catch (...) { exe_p = fs::current_path(); }
                                fs::path tp = exe_p / "data" / translation_list_[ti].db_file;
                                if (fs::exists(tp)) {
                                    auto ndb = std::make_unique<Database>(tp.string(), translation_list_[ti].code, translation_list_[ti].name);
                                    if (ndb->isOpen()) {
                                        current_translation_ = ti;
                                        db_ = std::move(ndb);
                                        books_ = db_->getBooks();
                                    }
                                }
                                break;
                            }
                        }
                    }
                    pushNavHistory();
                    for (size_t i = 0; i < books_.size(); i++) {
                        if (books_[i] == r.second.book) {
                            selected_book_ = i;
                            selected_chapter_ = r.second.chapter;
                            loadChapter();
                            for (size_t vi = 0; vi < current_verses_.size(); vi++) {
                                if (current_verses_[vi].verse == r.second.verse) {
                                    content_scroll_ = vi;
                                    break;
                                }
                            }
                            break;
                        }
                    }
                }
                toggleCrossSearch();
                return true;
            }
            if (event == Event::Tab || event == Event::Character("n")) {
                if (!cross_search_results_.empty()) {
                    search_result_index_ = std::clamp(search_result_index_ + 1, 0, (int)cross_search_results_.size() - 1);
                    showNotification("Result " + std::to_string(search_result_index_ + 1) + "/" +
                                     std::to_string(cross_search_results_.size()));
                }
                return true;
            }
            if (event == Event::Character("N")) {
                if (!cross_search_results_.empty()) {
                    search_result_index_ = std::clamp(search_result_index_ - 1, 0, (int)cross_search_results_.size() - 1);
                    showNotification("Result " + std::to_string(search_result_index_ + 1) + "/" +
                                     std::to_string(cross_search_results_.size()));
                }
                return true;
            }
            return true;
        }

        // === SEARCH MODE ===
        if (search_mode_) {
            if (event == Event::Escape) {
                toggleSearch();
                return true;
            }
            if (event.is_character()) {
                auto c = event.character();
                if (!c.empty() && c[0] >= 32 && c[0] < 127) {
                    search_query_ += c[0];
                    search_results_ = db_->search(search_query_ + "*");
                    search_result_index_ = 0;
                    return true;
                }
            }
            if (event == Event::Backspace && !search_query_.empty()) {
                search_query_.pop_back();
                if (search_query_.empty()) {
                    search_results_.clear();
                } else {
                    search_results_ = db_->search(search_query_ + "*");
                }
                search_result_index_ = 0;
                return true;
            }
            if (event == Event::ArrowUp) {
                if (!search_history_.empty()) {
                    search_history_index_ = std::max(search_history_index_ - 1, 0);
                    search_query_ = search_history_[search_history_index_];
                    search_results_ = db_->search(search_query_ + "*");
                    search_result_index_ = 0;
                    showNotification("History: " + search_query_);
                }
                return true;
            }
            if (event == Event::ArrowDown) {
                if (!search_history_.empty()) {
                    search_history_index_ = std::min(search_history_index_ + 1, (int)search_history_.size());
                    if (search_history_index_ >= (int)search_history_.size()) {
                        // Past end — clear to allow typing a fresh query
                        search_query_.clear();
                        search_results_.clear();
                        showNotification("New search");
                    } else {
                        search_query_ = search_history_[search_history_index_];
                        search_results_ = db_->search(search_query_ + "*");
                        search_result_index_ = 0;
                        showNotification("History: " + search_query_);
                    }
                }
                return true;
            }
            if (event == Event::Return) {
                // Save to search history
                if (!search_query_.empty()) {
                    search_history_.push_back(search_query_);
                    search_history_index_ = (int)search_history_.size();
                }
                if (!search_results_.empty()) {
                    auto& r = search_results_[search_result_index_];
                    pushNavHistory();
                    for (size_t i = 0; i < books_.size(); i++) {
                        if (books_[i] == r.book) {
                            selected_book_ = i;
                            selected_chapter_ = r.chapter;
                            loadChapter();
                            for (size_t vi = 0; vi < current_verses_.size(); vi++) {
                                if (current_verses_[vi].verse == r.verse) {
                                    content_scroll_ = vi;
                                    break;
                                }
                            }
                            break;
                        }
                    }
                }
                toggleSearch();
                return true;
            }
            if (event == Event::Tab || event == Event::Character("n")) {
                if (!search_results_.empty()) {
                    navigateSearchResult(1);
                }
                return true;
            }
            if (event == Event::Character("N")) {
                if (!search_results_.empty()) {
                    navigateSearchResult(-1);
                }
                return true;
            }
            return true;
        }

        // === GOTO-VERSE MODE ===
        if (goto_verse_mode_) {
            if (event == Event::Escape) {
                toggleGotoVerse();
                return true;
            }
            if (event == Event::Tab || (event == Event::Character(" ") && !goto_verse_query_.empty())) {
                // Tab / Space: advance to next input stage
                if (goto_verse_input_stage_ == 0) {
                    goto_verse_book_ = goto_verse_query_;
                    goto_verse_query_.clear();
                    goto_verse_input_stage_ = 1;
                } else if (goto_verse_input_stage_ == 1) {
                    goto_verse_chapter_str_ = goto_verse_query_;
                    goto_verse_query_.clear();
                    goto_verse_input_stage_ = 2;
                }
                return true;
            }
            if (event == Event::Return) {
                // Execute the jump
                if (goto_verse_input_stage_ == 0 && !goto_verse_query_.empty()) {
                    goto_verse_book_ = goto_verse_query_;
                } else if (goto_verse_input_stage_ == 1) {
                    goto_verse_chapter_str_ = goto_verse_query_;
                } else if (goto_verse_input_stage_ == 2) {
                    goto_verse_verse_str_ = goto_verse_query_;
                }
                
                if (!goto_verse_book_.empty()) {
                    // Find book (case-insensitive)
                    std::string lower_q = goto_verse_book_;
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
                        pushNavHistory();
                        selected_book_ = found_idx;
                        int ch = 1;
                        int vs = 1;
                        try {
                            if (!goto_verse_chapter_str_.empty()) ch = std::max(1, std::stoi(goto_verse_chapter_str_));
                        } catch (...) { ch = 1; }
                        try {
                            if (!goto_verse_verse_str_.empty()) vs = std::max(1, std::stoi(goto_verse_verse_str_));
                        } catch (...) { vs = 1; }
                        selected_chapter_ = ch;
                        loadChapter();
                        content_scroll_ = std::min(vs - 1, std::max(0, (int)current_verses_.size() - 1));
                        focus_panel_ = 1;
                        showNotification("Goto: " + books_[selected_book_] + " " + std::to_string(ch) + ":" + std::to_string(vs));
                    } else {
                        showNotification("Book not found: " + goto_verse_book_);
                    }
                }
                toggleGotoVerse();
                return true;
            }
            if (event == Event::Backspace && !goto_verse_query_.empty()) {
                goto_verse_query_.pop_back();
                return true;
            }
            if (event.is_character()) {
                auto c = event.character();
                if (!c.empty() && c[0] >= 32 && c[0] < 127) {
                    goto_verse_query_ += c[0];
                    return true;
                }
            }
            return true;
        }

        // === CROSS-REFERENCE OVERLAY ===
        if (cross_ref_mode_) {
            if (event == Event::Escape || event == Event::Character("q")) {
                toggleCrossReferences();
                return true;
            }
            if (event == Event::Character("j") || event == Event::ArrowDown) {
                if (!current_cross_refs_.empty()) {
                    cross_ref_index_ = std::min(cross_ref_index_ + 1, (int)current_cross_refs_.size() - 1);
                }
                return true;
            }
            if (event == Event::Character("k") || event == Event::ArrowUp) {
                if (!current_cross_refs_.empty()) {
                    cross_ref_index_ = std::max(cross_ref_index_ - 1, 0);
                }
                return true;
            }
            if (event == Event::Return && !current_cross_refs_.empty()) {
                auto& cr = current_cross_refs_[cross_ref_index_];
                // Find matching book in current translation (case-insensitive)
                std::string target_lower = cr.to_book;
                std::transform(target_lower.begin(), target_lower.end(), target_lower.begin(), ::tolower);
                int found_book = -1;
                for (size_t bi = 0; bi < books_.size(); bi++) {
                    std::string bl = books_[bi];
                    std::transform(bl.begin(), bl.end(), bl.begin(), ::tolower);
                    if (bl.find(target_lower) != std::string::npos ||
                        target_lower.find(bl) != std::string::npos) {
                        found_book = (int)bi;
                        break;
                    }
                }
                if (found_book >= 0) {
                    pushNavHistory();
                    selected_book_ = found_book;
                    selected_chapter_ = cr.to_chapter;
                    loadChapter();
                    content_scroll_ = std::max(0, cr.to_verse_start - 1);
                    focus_panel_ = 1;
                    showNotification("Cross-ref: " + cr.to_book + " " +
                                     std::to_string(cr.to_chapter) + ":" + std::to_string(cr.to_verse_start));
                } else {
                    showNotification("Book not found: " + cr.to_book);
                }
                toggleCrossReferences();
                return true;
            }
            return true;
        }

        // === NORMAL MODE ===

        // Goto-verse (Ctrl+g)
        if (event == Event::Character("\x07") && !search_mode_ && !help_mode_ && !jump_mode_ && !bookmark_mode_ && !plan_mode_ && !strongs_mode_ && !cross_search_mode_ && !cross_ref_mode_) {
            toggleGotoVerse();
            return true;
        }

        // Cross-references for current verse (x)
        if (event == Event::Character("x") && focus_panel_ == 1 && !search_mode_ && !help_mode_ && !jump_mode_ && !bookmark_mode_ && !plan_mode_ && !strongs_mode_ && !cross_search_mode_) {
            cross_ref_index_ = 0;
            toggleCrossReferences();
            return true;
        }

        // Export current verse (E)
        if (event == Event::Character("E") && focus_panel_ == 1 && !search_mode_ && !help_mode_ && !jump_mode_ && !bookmark_mode_ && !plan_mode_) {
            exportVerse();
            return true;
        }

        // Copy $REF format (Ctrl+y)
        if (event == Event::Character("\x19") && focus_panel_ == 1 && !search_mode_ && !help_mode_) {
            copyRefFormat();
            return true;
        }

        // Copy Bible Gateway URL (Ctrl+u)
        if (event == Event::Character("\x15") && focus_panel_ == 1 && !search_mode_ && !help_mode_) {
            copyBibleGatewayUrl();
            return true;
        }

        // Panel switching
        if (event == Event::Character("h") && !goto_verse_mode_) {
            focus_panel_ = 0;
            return true;
        }
        if (event == Event::Character("l") && !goto_verse_mode_) {
            focus_panel_ = 1;
            return true;
        }
        if (event == Event::Tab && !goto_verse_mode_) {
            focus_panel_ = (focus_panel_ + 1) % 2;
            return true;
        }

        // Book panel navigation
        if (focus_panel_ == 0) {
            if (event == Event::Character("j") || event == Event::ArrowDown) {
                navigateBook(1);
                return true;
            }
            if (event == Event::Character("k") || event == Event::ArrowUp) {
                navigateBook(-1);
                return true;
            }
            if (event == Event::Character("G")) {
                pushNavHistory();
                selected_book_ = books_.size() - 1;
                selected_chapter_ = 1;
                auto chs = db_->getChapters(books_[selected_book_]);
                if (!chs.empty()) selected_chapter_ = chs[0];
                loadChapter();
                showNotification(books_[selected_book_]);
                return true;
            }
            if (event == Event::Return || event == Event::Character(" ")) {
                focus_panel_ = 1;
                return true;
            }
            if (event == Event::Character("[")) {
                pushNavHistory();
                navigateChapter(-1);
                return true;
            }
            if (event == Event::Character("]")) {
                pushNavHistory();
                navigateChapter(1);
                return true;
            }
        } else {
            // Content panel navigation
            if (event == Event::Character("j") || event == Event::ArrowDown) {
                navigateContent(1);
                return true;
            }
            if (event == Event::Character("k") || event == Event::ArrowUp) {
                navigateContent(-1);
                return true;
            }
            if (event == Event::PageDown || event == Event::Character(" ")) {
                navigateContent(10);
                return true;
            }
            if (event == Event::PageUp || event == Event::Character("b")) {
                navigateContent(-10);
                return true;
            }
            if (event == Event::Character("[")) {
                pushNavHistory();
                navigateChapter(-1);
                return true;
            }
            if (event == Event::Character("]")) {
                pushNavHistory();
                navigateChapter(1);
                return true;
            }
            if (event == Event::Character("g")) {
                content_scroll_ = 0;
                return true;
            }
            if (event == Event::Character("G")) {
                content_scroll_ = std::max(0, (int)current_verses_.size() - 1);
                return true;
            }
            // Number shortcuts for chapters 1-9
            if (event.is_character()) {
                auto c = event.character();
                if (!c.empty() && c[0] >= '1' && c[0] <= '9') {
                    pushNavHistory();
                    int ch = c[0] - '0';
                    auto chs = db_->getChapters(books_[selected_book_]);
                    if (!chs.empty()) {
                        int idx = std::min(ch - 1, (int)chs.size() - 1);
                        selected_chapter_ = chs[idx];
                        loadChapter();
                        content_scroll_ = 0;
                        showNotification(books_[selected_book_] + " " + std::to_string(selected_chapter_));
                    }
                    return true;
                }
            }
        }

        // === MOUSE SUPPORT ===
        if (event.is_mouse()) {
            auto& mouse = event.mouse();
            // Wheel scrolling
            if (mouse.button == Mouse::WheelUp) {
                if (focus_panel_ == 1) {
                    navigateContent(-3); // scroll 3 lines per wheel tick
                } else {
                    navigateBook(-1);
                }
                return true;
            }
            if (mouse.button == Mouse::WheelDown) {
                if (focus_panel_ == 1) {
                    navigateContent(3);
                } else {
                    navigateBook(1);
                }
                return true;
            }
            // Left click focuses the panel under the cursor
            if (mouse.button == Mouse::Left && mouse.motion == Mouse::Released) {
                // Book panel is roughly 25-30 chars wide (border included)
                if (mouse.x < 30) {
                    focus_panel_ = 0;
                } else {
                    focus_panel_ = 1;
                }
                return true;
            }
            return true;
        }

        return false;
    });

    // Build final component tree with rendered layout
    auto final_component = Renderer(component, [this, component] {
        auto& t = ThemeManager::getAllThemes()[theme_index_];

        // Left book panel
        auto book_panel = renderBookPanel() | size(WIDTH, GREATER_THAN, 20) | size(WIDTH, LESS_THAN, 30);

        // Right content panel
        auto content_panel = vbox({
            content_renderer_->Render() | flex,
        }) | flex | border | color(t.border);

        auto main = hbox({
            book_panel | border | color(t.border),
            content_panel | flex,
        });

        auto header = renderHeader();
        auto status = renderStatusBar();

        return ThemeManager::apply(t, vbox({
            header | color(t.title) | bold | center,
            separator() | color(t.border),
            main | flex,
            separator() | color(t.border),
            status | color(t.secondary),
        }));
    });

    return final_component;
}

void App::run() {
    auto final_component = buildComponent();
    screen_.Loop(final_component);
}

void App::loadChapter() {
    if (books_.empty() || selected_book_ >= (int)books_.size()) return;
    current_verses_ = db_->getVerses(books_[selected_book_], selected_chapter_);
    chapters_ = db_->getChapters(books_[selected_book_]);
    // Sync parallel view if active
    if (parallel_mode_ && parallel_db_ && parallel_db_->isOpen()) {
        parallel_book_ = selected_book_;
        parallel_chapter_ = selected_chapter_;
        loadParallelChapter();
    }
}

void App::setTheme(int index) {
    auto themes = ThemeManager::getAllThemes();
    theme_index_ = index % themes.size();
    showNotification("Theme: " + themes[theme_index_].name);
}

void App::autoCopyVotd() {
    if (verse_of_day_.book.empty() || verse_of_day_.text.empty()) return;

    std::string ref = verse_of_day_.book + " " +
                      std::to_string(verse_of_day_.chapter) + ":" +
                      std::to_string(verse_of_day_.verse);
    std::string full = ref + " (Auto VOTD)\n" + verse_of_day_.text;

    // Copy to clipboard
    fs::path tmp_path = fs::temp_directory_path() / "open-psalm-votd.txt";
    {
        std::ofstream tmp_file(tmp_path);
        if (tmp_file.is_open()) {
            tmp_file << full;
            tmp_file.close();
        }
    }

    FILE* pipe = popen("which xclip xsel 2>/dev/null | head -1", "r");
    if (pipe) {
        char buf[128] = {0};
        if (fgets(buf, sizeof(buf), pipe)) {
            std::string clipboard_tool = buf;
            clipboard_tool.erase(std::remove(clipboard_tool.begin(), clipboard_tool.end(), '\n'), clipboard_tool.end());
            if (!clipboard_tool.empty()) {
                std::string cmd = "cat " + tmp_path.string() + " | " + clipboard_tool + " -selection clipboard 2>/dev/null";
                FILE* out = popen(cmd.c_str(), "r");
                if (out) pclose(out);
            }
        }
        pclose(pipe);
    }
}

void App::toggleSearch() {
    search_mode_ = !search_mode_;
    if (search_mode_) {
        search_history_index_ = (int)search_history_.size(); // start past end
    } else {
        search_query_.clear();
        search_results_.clear();
        search_result_index_ = 0;
    }
}

void App::toggleHelp() {
    help_mode_ = !help_mode_;
}

void App::toggleJumpMode() {
    jump_mode_ = !jump_mode_;
    if (jump_mode_) {
        jump_query_.clear();
        jump_results_.clear();
    }
}

void App::toggleBookmarkMode() {
    bookmark_mode_ = !bookmark_mode_;
    bookmark_index_ = 0;
}

void App::copyCurrentVerse() {
    if (current_verses_.empty() || content_scroll_ < 0 ||
        content_scroll_ >= (int)current_verses_.size()) {
        showNotification("No verse to copy");
        return;
    }

    auto& vr = current_verses_[content_scroll_];
    std::string ref = vr.book + " " + std::to_string(vr.chapter) + ":" + std::to_string(vr.verse);
    std::string full = ref + " " + vr.text;

    // Try to copy to clipboard via xclip or xsel
    fs::path tmp_path = fs::temp_directory_path() / "open-psalm-yank.txt";
    {
        std::ofstream tmp_file(tmp_path);
        if (tmp_file.is_open()) {
            tmp_file << full;
            tmp_file.close();
        }
    }

    // Detect clipboard tool and pipe the temp file
    FILE* pipe = popen("which xclip xsel 2>/dev/null | head -1", "r");
    if (pipe) {
        char buf[128] = {0};
        if (fgets(buf, sizeof(buf), pipe)) {
            std::string clipboard_tool = buf;
            clipboard_tool.erase(std::remove(clipboard_tool.begin(), clipboard_tool.end(), '\n'), clipboard_tool.end());
            if (!clipboard_tool.empty()) {
                std::string cmd = "cat " + tmp_path.string() + " | " + clipboard_tool + " -selection clipboard 2>/dev/null";
                FILE* out = popen(cmd.c_str(), "r");
                if (out) pclose(out);
            }
        }
        pclose(pipe);
    }

    // Also save to yank file
    const char* home_env = std::getenv("HOME");
    std::string home = home_env ? home_env : ".";
    std::string yank_path = home + "/.config/open-psalm/yanks.txt";
    std::ofstream yank_file(yank_path, std::ios::app);
    if (yank_file.is_open()) {
        yank_file << full << "\n";
        yank_file.close();
    }

    showNotification("Yanked: " + ref + " \u2713");
}

void App::saveBookmark() {
    if (current_verses_.empty() || content_scroll_ < 0 ||
        content_scroll_ >= (int)current_verses_.size()) {
        showNotification("No verse to bookmark");
        return;
    }

    auto& vr = current_verses_[content_scroll_];
    Bookmark bm;
    bm.book = vr.book;
    bm.chapter = vr.chapter;
    bm.verse = vr.verse;
    // Truncate text for label
    bm.label = vr.text.substr(0, std::min((size_t)50, vr.text.size()));
    if (vr.text.size() > 50) bm.label += "...";

    // Check for duplicate
    for (auto& existing : bookmarks_) {
        if (existing.book == bm.book && existing.chapter == bm.chapter && existing.verse == bm.verse) {
            showNotification("Bookmark already exists");
            return;
        }
    }

    bookmarks_.push_back(bm);
    showNotification("Bookmarked: " + bm.book + " " + std::to_string(bm.chapter) + ":" + std::to_string(bm.verse));
}

void App::showNotification(const std::string& msg) {
    notification_ = msg;
    notification_timer_ = 100;
}

void App::toggleParallelMode() {
    parallel_mode_ = !parallel_mode_;
    if (parallel_mode_) {
        // Ensure we have a second translation loaded
        if (!parallel_db_ || !parallel_db_->isOpen()) {
            // Try the first available translation that isn't current
            for (size_t i = 0; i < translation_list_.size(); i++) {
                if ((int)i != current_translation_) {
                    fs::path exe_p;
                    try { exe_p = fs::read_symlink("/proc/self/exe").parent_path().parent_path(); }
                    catch (...) { exe_p = fs::current_path(); }
                    fs::path pdb = exe_p / "data" / translation_list_[i].db_file;
                    if (fs::exists(pdb)) {
                        parallel_db_ = std::make_unique<Database>(pdb.string(),
                                                                  translation_list_[i].code,
                                                                  translation_list_[i].name);
                        if (parallel_db_->isOpen()) {
                            config_.parallel_second_translation = translation_list_[i].code;
                            showNotification("Parallel: " + db_->translationCode() + " + " + parallel_db_->translationCode());
                            break;
                        }
                    }
                }
            }
        }
        if (parallel_db_ && parallel_db_->isOpen()) {
            parallel_book_ = selected_book_;
            parallel_chapter_ = selected_chapter_;
            loadParallelChapter();
        } else {
            showNotification("No second translation available");
            parallel_mode_ = false;
        }
    } else {
        showNotification("Parallel view closed");
    }
}

void App::cycleParallelTranslation() {
    if (!parallel_mode_) return;

    // Find next translation that isn't current
    int start = 0;
    for (size_t i = 0; i < translation_list_.size(); i++) {
        if (parallel_db_ && translation_list_[i].code == parallel_db_->translationCode()) {
            start = i + 1;
            break;
        }
    }

    for (int pass = 0; pass < 2; pass++) {
        for (size_t i = (pass == 0 ? start : 0); i < translation_list_.size(); i++) {
            if ((int)i != current_translation_) {
                fs::path exe_p;
                try { exe_p = fs::read_symlink("/proc/self/exe").parent_path().parent_path(); }
                catch (...) { exe_p = fs::current_path(); }
                fs::path pdb = exe_p / "data" / translation_list_[i].db_file;
                if (fs::exists(pdb)) {
                    parallel_db_ = std::make_unique<Database>(pdb.string(),
                                                              translation_list_[i].code,
                                                              translation_list_[i].name);
                    if (parallel_db_->isOpen()) {
                        config_.parallel_second_translation = translation_list_[i].code;
                        loadParallelChapter();
                        showNotification("Parallel: " + db_->translationCode() + " + " + parallel_db_->translationCode());
                        return;
                    }
                }
            }
        }
    }
}

void App::loadParallelChapter() {
    if (!parallel_db_ || !parallel_db_->isOpen()) return;
    if (books_.empty() || parallel_book_ >= (int)books_.size()) return;

    parallel_verses_ = parallel_db_->getVerses(books_[parallel_book_], parallel_chapter_);
}

void App::toggleCrossSearch() {
    cross_search_mode_ = !cross_search_mode_;
    if (cross_search_mode_) {
        search_query_.clear();
        cross_search_results_.clear();
        search_result_index_ = 0;
        showNotification("Cross-translation search — type to search all DBs");
    }
}

void App::toggleStrongsMode() {
    strongs_mode_ = !strongs_mode_;
    if (strongs_mode_) {
        strongs_query_.clear();
        strongs_result_.clear();
        showNotification("Strong's Concordance — type a number (e.g., H7225) or word");
    }
}

void App::lookupStrongs(const std::string& query) {
    if (!strongs_db_ || !strongs_db_->isOpen()) {
        strongs_result_ = "Strong's database not available.";
        return;
    }

    strongs_result_.clear();

    // First try exact match as Strong's number
    std::string def = strongs_db_->lookupStrongs(query);
    if (!def.empty()) {
        strongs_result_ = def;
        return;
    }

    // Try as a search query
    auto results = strongs_db_->searchStrongs(query);
    if (results.empty()) {
        strongs_result_ = "No results found.";
    } else {
        std::string output;
        int count = std::min((int)results.size(), 10);
        for (int i = 0; i < count; i++) {
            if (i > 0) output += "\n";
            output += results[i].first + ": ";
            std::string def_text = results[i].second;
            if (def_text.size() > 100) {
                def_text = def_text.substr(0, 100) + "...";
            }
            output += def_text;
        }
        if ((int)results.size() > count) {
            output += "\n... and " + std::to_string(results.size() - count) + " more";
        }
        strongs_result_ = output;
    }
}

void App::toggleEditNote(int index) {
    if (index < 0 || index >= (int)bookmarks_.size()) return;
    edit_note_index_ = index;
    edit_note_text_ = bookmarks_[index].note;
    edit_note_mode_ = true;
    showNotification("Editing note for " + bookmarks_[index].book + " " +
                     std::to_string(bookmarks_[index].chapter) + ":" + std::to_string(bookmarks_[index].verse));
}

void App::saveEditNote() {
    if (edit_note_index_ >= 0 && edit_note_index_ < (int)bookmarks_.size()) {
        bookmarks_[edit_note_index_].note = edit_note_text_;
        showNotification("Note saved");
    }
    edit_note_mode_ = false;
    edit_note_index_ = -1;
    edit_note_text_.clear();
}

void App::toggleGotoVerse() {
    goto_verse_mode_ = !goto_verse_mode_;
    if (goto_verse_mode_) {
        goto_verse_query_.clear();
        goto_verse_book_.clear();
        goto_verse_chapter_str_.clear();
        goto_verse_verse_str_.clear();
        goto_verse_input_stage_ = 0;
    }
}

void App::toggleCrossReferences() {
    if (!cross_ref_mode_) {
        // Load cross-references for current verse from the cross-refs DB
        current_cross_refs_.clear();
        if (cross_refs_db_ && cross_refs_db_->isOpen() &&
            content_scroll_ >= 0 && content_scroll_ < (int)current_verses_.size()) {
            auto& vr = current_verses_[content_scroll_];
            current_cross_refs_ = cross_refs_db_->getCrossReferences(vr.book, vr.chapter, vr.verse);
        }
        cross_ref_mode_ = !current_cross_refs_.empty();
        if (cross_ref_mode_) {
            showNotification(std::to_string(current_cross_refs_.size()) + " cross-references found");
        } else if (cross_refs_db_ && cross_refs_db_->isOpen()) {
            showNotification("No cross-references for this verse");
        } else {
            showNotification("Cross-reference database not available");
        }
    } else {
        cross_ref_mode_ = false;
        current_cross_refs_.clear();
    }
}

void App::exportVerse() {
    if (current_verses_.empty() || content_scroll_ < 0 ||
        content_scroll_ >= (int)current_verses_.size()) {
        showNotification("No verse to export");
        return;
    }
    
    auto& vr = current_verses_[content_scroll_];
    std::string ref = vr.book + " " + std::to_string(vr.chapter) + ":" + std::to_string(vr.verse);
    std::string full = ref + " (" + db_->translationCode() + ")\n" + vr.text;
    
    // Save to exports directory
    const char* home_env = std::getenv("HOME");
    std::string home = home_env ? home_env : ".";
    std::string export_dir = home + "/.config/open-psalm/exports";
    
    try {
        fs::create_directories(export_dir);
    } catch (...) {}
    
    std::string filename = vr.book + "_" + std::to_string(vr.chapter) + "-" + std::to_string(vr.verse) + ".txt";
    std::replace(filename.begin(), filename.end(), ' ', '_');
    std::string path = export_dir + "/" + filename;
    
    {
        std::ofstream f(path);
        if (f.is_open()) {
            f << full << "\n";
            f.close();
            showNotification("Exported: " + path);
            return;
        }
    }
    showNotification("Export failed");
}

void App::copyRefFormat() {
    if (current_verses_.empty() || content_scroll_ < 0 ||
        content_scroll_ >= (int)current_verses_.size()) {
        showNotification("No verse to copy");
        return;
    }
    
    auto& vr = current_verses_[content_scroll_];
    std::string ref = vr.book + " " + std::to_string(vr.chapter) + ":" + std::to_string(vr.verse);
    std::string ref_format = ref + " (" + db_->translationCode() + ")";
    
    // Copy to clipboard
    fs::path tmp_path = fs::temp_directory_path() / "open-psalm-ref.txt";
    {
        std::ofstream tmp_file(tmp_path);
        if (tmp_file.is_open()) {
            tmp_file << ref_format;
            tmp_file.close();
        }
    }
    
    FILE* pipe = popen("which xclip xsel 2>/dev/null | head -1", "r");
    if (pipe) {
        char buf[128] = {0};
        if (fgets(buf, sizeof(buf), pipe)) {
            std::string clipboard_tool = buf;
            clipboard_tool.erase(std::remove(clipboard_tool.begin(), clipboard_tool.end(), '\n'), clipboard_tool.end());
            if (!clipboard_tool.empty()) {
                std::string cmd = "cat " + tmp_path.string() + " | " + clipboard_tool + " -selection clipboard 2>/dev/null";
                FILE* out = popen(cmd.c_str(), "r");
                if (out) pclose(out);
            }
        }
        pclose(pipe);
    }
    
    showNotification("Copied: " + ref_format);
}

void App::copyBibleGatewayUrl() {
    if (current_verses_.empty() || content_scroll_ < 0 ||
        content_scroll_ >= (int)current_verses_.size()) {
        showNotification("No verse to copy");
        return;
    }
    
    auto& vr = current_verses_[content_scroll_];
    std::string url = "https://www.biblegateway.com/passage/?search=" +
                      vr.book + "+" + std::to_string(vr.chapter) + "%3A" + std::to_string(vr.verse) +
                      "&version=" + db_->translationCode();
    
    fs::path tmp_path = fs::temp_directory_path() / "open-psalm-url.txt";
    {
        std::ofstream tmp_file(tmp_path);
        if (tmp_file.is_open()) {
            tmp_file << url;
            tmp_file.close();
        }
    }
    
    FILE* pipe = popen("which xclip xsel 2>/dev/null | head -1", "r");
    if (pipe) {
        char buf[128] = {0};
        if (fgets(buf, sizeof(buf), pipe)) {
            std::string clipboard_tool = buf;
            clipboard_tool.erase(std::remove(clipboard_tool.begin(), clipboard_tool.end(), '\n'), clipboard_tool.end());
            if (!clipboard_tool.empty()) {
                std::string cmd = "cat " + tmp_path.string() + " | " + clipboard_tool + " -selection clipboard 2>/dev/null";
                FILE* out = popen(cmd.c_str(), "r");
                if (out) pclose(out);
            }
        }
        pclose(pipe);
    }
    
    showNotification("Copied URL to clipboard");
}
