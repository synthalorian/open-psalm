#pragma once
#include "app.hpp"
#include <string>
#include <vector>
#include <cstdlib>
#include <algorithm>
#include <cstring>

// Parse CLI arguments into CliOptions
inline CliOptions parseArgs(int argc, char** argv) {
    CliOptions opts;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--translation" && i + 1 < argc) {
            opts.translation_code = argv[++i];
        } else if (arg == "--book" && i + 1 < argc) {
            opts.book = argv[++i];
        } else if (arg == "--chapter" && i + 1 < argc) {
            opts.chapter = std::max(1, std::atoi(argv[++i]));
        } else if (arg == "--verse" && i + 1 < argc) {
            opts.verse = std::max(1, std::atoi(argv[++i]));
        } else if (arg == "--theme" && i + 1 < argc) {
            opts.theme = argv[++i];
        } else if (arg == "--list-themes" || arg == "--themes") {
            opts.list_themes = true;
        } else if (arg == "--random") {
            opts.random = true;
        } else if (arg == "--daily" || arg == "--votd") {
            opts.daily = true;
        } else if (arg == "--version" || arg == "-v") {
            opts.version = true;
        }
    }
    return opts;
}