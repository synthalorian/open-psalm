#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include "cli.hpp"
#include "database.hpp"
#include "app.hpp"
#include "theme.hpp"

namespace fs = std::filesystem;

static constexpr const char* VERSION = "1.0.0";

// ──────────────────────────────────────────────
// ASCII banner
// ──────────────────────────────────────────────
static void printBanner() {
    std::cout << "\n";
    std::cout << "  ╔═══════════════════════════════════════╗\n";
    std::cout << "  ║        ◈  OPEN  PSALM  ◈              ║\n";
    std::cout << "  ║    The Word, blazing in your terminal  ║\n";
    std::cout << "  ╚═══════════════════════════════════════╝\n";
    std::cout << "\n";
}

// ──────────────────────────────────────────────
// Usage / help
// ──────────────────────────────────────────────
static void printUsage(const char* prog, const std::vector<TranslationInfo>& translations) {
    printBanner();
    std::cout << "  Usage:  " << prog << " [options]\n";
    std::cout << "          open-psalm [options]\n";
    std::cout << "\n";
    std::cout << "  NAVIGATION:\n";
    std::cout << "    --book <name>        Jump to a specific book\n";
    std::cout << "    --chapter <n>        Jump to a specific chapter\n";
    std::cout << "    --verse <n>          Jump to a specific verse\n";
    std::cout << "\n";
    std::cout << "  TRANSLATION:\n";
    std::cout << "    --translation <code> Start with a specific translation\n";
    std::cout << "\n";
    std::cout << "  THEMES:\n";
    std::cout << "    --theme <name>       Start with a specific theme\n";
    std::cout << "    --list-themes        List all available themes\n";
    std::cout << "    --themes             Alias for --list-themes\n";
    std::cout << "\n";
    std::cout << "  QUICK ACCESS:\n";
    std::cout << "    --daily              Open to today's verse\n";
    std::cout << "    --votd               Alias for --daily\n";
    std::cout << "    --random             Open to a random chapter\n";
    std::cout << "\n";
    std::cout << "  OTHER:\n";
    std::cout << "    --version, -v        Show version info\n";
    std::cout << "    --help               Show this help\n";
    std::cout << "\n";
    std::cout << "  TRANSLATIONS:\n";
    for (auto& t : translations) {
        std::cout << "    " << t.code << "  " << t.name << "\n";
    }
    std::cout << "\n";
    std::cout << "  KEYBOARD (inside TUI):\n";
    std::cout << "    ?      Help            /   Search\n";
    std::cout << "    j/k    Navigate        b   Bookmarks\n";
    std::cout << "    q      Quit            T   Cycle theme\n";
    std::cout << "    g      Jump to book    s   Toggle Strong's\n";
    std::cout << "    [ ]    Prev/next book  H L Change translation\n";
    std::cout << "\n";
}

// ──────────────────────────────────────────────
// List themes
// ──────────────────────────────────────────────
static void printThemes() {
    printBanner();
    auto themes = ThemeManager::getAllThemes();
    std::cout << "  Available themes (" << themes.size() << "):\n\n";
    for (size_t i = 0; i < themes.size(); i++) {
        auto& t = themes[i];
        std::cout << "  " << (i + 1) << ".  " << t.name << "\n";
    }
    std::cout << "\n  Use: open-psalm --theme \"Theme Name\"\n";
    std::cout << "\n";
}

// ──────────────────────────────────────────────
// Find a random book/chapter
// ──────────────────────────────────────────────
static void randomVerse(Database* db, std::string& out_book, int& out_chapter) {
    auto books = db->getBooks();
    if (books.empty()) return;

    // Seed rand if not yet
    static bool seeded = false;
    if (!seeded) {
        std::srand(static_cast<unsigned>(std::time(nullptr)));
        seeded = true;
    }

    int idx = std::rand() % books.size();
    out_book = books[idx];
    auto chapters = db->getChapters(out_book);
    if (!chapters.empty()) {
        out_chapter = chapters[std::rand() % chapters.size()];
    } else {
        out_chapter = 1;
    }
}

// ──────────────────────────────────────────────
// Main
// ──────────────────────────────────────────────
int main(int argc, char** argv) {
    // Define available translations
    std::vector<TranslationInfo> translations = {
        {"KJV", "King James Version (1769)", "bible_kjv.db"},
        {"ASV", "American Standard Version (1901)", "bible_asv.db"},
        {"YLT", "Young's Literal Translation (1898)", "bible_ylt.db"},
        {"BBE", "Bible in Basic English (1965)", "bible_bbe.db"},
        {"WEB", "World English Bible", "bible_web.db"},
        {"Geneva", "Geneva Bible (1599)", "bible_geneva1599.db"},
        {"Webster", "Webster's Bible (1833)", "bible_webster.db"},
        {"DRC", "Douay-Rheims Catholic (1899)", "bible_drc.db"},
        {"CPDV", "Catholic Public Domain Version", "bible_cpdv.db"},
        {"KJVA", "KJV with Apocrypha (1769)", "bible_kjva.db"},
    };

    // Check for --help before anything else
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--help") {
            printUsage(argv[0], translations);
            return 0;
        }
    }

    // Parse CLI args
    CliOptions cli = parseArgs(argc, argv);

    // --version
    if (cli.version) {
        printBanner();
        std::cout << "  Version:  " << VERSION << "\n";
        std::cout << "  Built:    C++20 + FTXUI + SQLite3\n";
        std::cout << "\n";
        std::cout << "  \"Thy word is a lamp unto my feet,\n";
        std::cout << "   and a light unto my path.\"\n";
        std::cout << "   — Psalm 119:105 (KJV)\n";
        std::cout << "\n";
        return 0;
    }

    // --list-themes
    if (cli.list_themes) {
        printThemes();
        return 0;
    }

    // Determine paths relative to the executable
    fs::path project_root;
    try {
        project_root = fs::read_symlink("/proc/self/exe").parent_path().parent_path();
    } catch (...) {
        project_root = fs::current_path();
    }

    // Data directories to search
    std::vector<fs::path> search_paths = {
        fs::current_path() / "data",
        project_root / "data",
    };

    // Find the data directory
    fs::path data_dir;
    for (const auto& p : search_paths) {
        if (fs::exists(p)) {
            data_dir = p;
            break;
        }
    }

    if (data_dir.empty()) {
        std::cerr << "Error: data/ directory not found.\n";
        std::cerr << "Please run from the project root directory.\n";
        return 1;
    }

    // Determine which database to open initially
    int initial_translation_idx = 0;
    if (!cli.translation_code.empty()) {
        for (size_t i = 0; i < translations.size(); i++) {
            if (translations[i].code == cli.translation_code) {
                initial_translation_idx = (int)i;
                break;
            }
        }
    }

    const auto& db_info = translations[initial_translation_idx];
    fs::path db_file = data_dir / db_info.db_file;
    if (!fs::exists(db_file)) {
        if (initial_translation_idx != 0) {
            std::cerr << "Warning: Translation '" << cli.translation_code << "' not found.\n";
            std::cerr << "Falling back to " << translations[0].code << ".\n";
            initial_translation_idx = 0;
            db_file = data_dir / translations[0].db_file;
        }
    }

    if (!fs::exists(db_file)) {
        std::cerr << "Error: Bible database not found at " << db_file << "\n";
        std::cerr << "Please run 'python3 data/create_db.py' first.\n";
        return 1;
    }

    auto db = std::make_unique<Database>(db_file.string(),
                                         db_info.code,
                                         db_info.name);
    if (!db->isOpen()) {
        std::cerr << "Error: Failed to open Bible database.\n";
        return 1;
    }

    // --random: pick a random book/chapter and override CLI
    if (cli.random || cli.daily) {
        if (cli.random) {
            std::string rand_book;
            int rand_chapter = 1;
            randomVerse(db.get(), rand_book, rand_chapter);
            cli.book = rand_book;
            cli.chapter = rand_chapter;
        }
        // For --daily, just let the TUI show VOTD (it does this automatically)
    }

    // Print a quick splash banner before launching TUI
    printBanner();

    try {
        auto app = std::make_unique<App>(std::move(db), translations, cli);
        app->run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}