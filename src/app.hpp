#pragma once
#include <memory>
#include <string>
#include <vector>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include "database.hpp"
#include "theme.hpp"

struct TranslationInfo {
    std::string code;
    std::string name;
    std::string db_file;
};

struct CliOptions {
    std::string translation_code;
    std::string book;
    int chapter = 0;
    int verse = 0;
    std::string theme;
    bool list_themes = false;
    bool random = false;
    bool daily = false;
    bool version = false;
};

class App {
public:
    App(std::unique_ptr<Database> db, const std::vector<TranslationInfo>& translations,
        const CliOptions& cli = {});
    ~App();
    void run();

private:
    // Core
    std::unique_ptr<Database> db_;
    ftxui::ScreenInteractive screen_;

    // Translation management
    std::vector<TranslationInfo> translation_list_;
    int current_translation_ = 0;

    // State
    std::vector<std::string> books_;
    int selected_book_ = 0;
    int selected_chapter_ = 0;
    int focus_panel_ = 0; // 0=books, 1=content
    int content_scroll_ = 0;
    int theme_index_ = 0;

    // Navigation history
    std::vector<NavPosition> nav_back_;
    std::vector<NavPosition> nav_forward_;

    // Search
    bool search_mode_ = false;
    std::string search_query_;
    std::vector<VerseRef> search_results_;
    int search_result_index_ = 0;
    std::vector<std::string> search_history_;
    int search_history_index_ = -1; // points past end; Up arrow decrements

    // Cross-translation search
    bool cross_search_mode_ = false;
    std::vector<std::pair<std::string, VerseRef>> cross_search_results_;
    std::vector<std::unique_ptr<Database>> cross_search_dbs_; // cached DBs for all translations

    // Strong's numbers
    std::unique_ptr<Database> strongs_db_;
    bool strongs_mode_ = false;
    std::string strongs_query_;
    std::string strongs_result_;

    // Cross-references database (separate from translation DBs)
    std::unique_ptr<Database> cross_refs_db_;

    // Bookmark annotation editing
    bool edit_note_mode_ = false;
    std::string edit_note_text_;
    int edit_note_index_ = -1;

    // Help overlay
    bool help_mode_ = false;

    // Jump-to-book
    bool jump_mode_ = false;
    std::string jump_query_;
    std::vector<int> jump_results_;

    // Bookmark mode
    bool bookmark_mode_ = false;
    int bookmark_index_ = 0;
    std::vector<Bookmark> bookmarks_;

    // Notification
    std::string notification_;
    int notification_timer_ = 0;

    // Reading reminder
    bool reading_reminder_shown_ = false;

    // Config
    AppConfig config_;

    // Cached content
    std::vector<VerseRef> current_verses_;
    std::vector<int> chapters_;

    // Verse of the Day
    VerseRef verse_of_day_;
    bool dotd_shown_ = false;

    // Reading plan
    int reading_plan_day_ = 0;
    bool plan_mode_ = false;
    std::vector<NavPosition> plan_entries_;

    // Parallel/bilingual view
    bool parallel_mode_ = false;
    std::unique_ptr<Database> parallel_db_;
    int parallel_book_ = 0;
    int parallel_chapter_ = 1;
    int parallel_scroll_ = 0;
    std::vector<VerseRef> parallel_verses_;

    // Components
    ftxui::Component book_menu_;
    ftxui::Component content_renderer_;
    ftxui::Component main_container_;

    ftxui::Component buildComponent();

    // Internal methods
    void navigateBook(int delta);
    void navigateChapter(int delta);
    void navigateContent(int delta);
    void navigateSearchResult(int delta);
    void loadChapter();
    void setTheme(int index);
    void toggleSearch();
    void toggleHelp();
    void toggleJumpMode();
    void toggleBookmarkMode();
    void pushNavHistory();
    void goBack();
    void goForward();
    void jumpToLocation(const std::string& book, int chapter, int verse = 1);
    void copyCurrentVerse();
    void saveBookmark();
    void saveConfig();
    void loadConfig();
    void showNotification(const std::string& msg);
    void switchTranslation(int delta);
    VerseRef computeVerseOfDay();
    void generatePlan();
    void togglePlan();
    void toggleParallelMode();
    void cycleParallelTranslation();
    void loadParallelChapter();
    void toggleCrossSearch();
    void toggleStrongsMode();
    void lookupStrongs(const std::string& query);
    void toggleEditNote(int index);
    void saveEditNote();

    // Goto-verse mode
    bool goto_verse_mode_ = false;
    std::string goto_verse_query_;
    int goto_verse_input_stage_ = 0; // 0=typing book, 1=typing chapter, 2=typing verse
    std::string goto_verse_book_;
    std::string goto_verse_chapter_str_;
    std::string goto_verse_verse_str_;
    void toggleGotoVerse();

    // VOTD auto-copy
    void autoCopyVotd();

    // $REF format copy
    void copyRefFormat();

    // Cross-reference overlay
    bool cross_ref_mode_ = false;
    std::vector<CrossReference> current_cross_refs_;
    int cross_ref_index_ = 0;
    void toggleCrossReferences();

    // Export
    void exportVerse();
    void copyBibleGatewayUrl();

public:
    // Static helpers (visible for testing)
    static std::vector<ftxui::Element> highlightSearchTerms(const std::string& content,
                                                           const std::string& query,
                                                           const Theme& t);

private:
    // Rendering
    ftxui::Element renderHeader();
    ftxui::Element renderBookPanel();
    ftxui::Element renderReadingPlan();
    ftxui::Element renderChapterBar();
    ftxui::Element renderContent();
    ftxui::Element renderParallelContent();
    ftxui::Element renderSearchOverlay();
    ftxui::Element renderCrossSearchOverlay();
    ftxui::Element renderHelpOverlay();
    ftxui::Element renderJumpOverlay();
    ftxui::Element renderBookmarkOverlay();
    ftxui::Element renderStatusBar();
    ftxui::Element renderVerseOfDay();
    ftxui::Element renderReadingReminder();
    ftxui::Element renderStrongsOverlay();
    ftxui::Element renderCrossReferenceOverlay();
    ftxui::Element renderGotoVerseOverlay();
};
