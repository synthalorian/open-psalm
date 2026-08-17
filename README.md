# Open Psalm

![Status](https://img.shields.io/badge/status-active-brightgreen)
![C++20](https://img.shields.io/badge/standard-C%2B%2B20-blue)
![License](https://img.shields.io/badge/license-MIT-green)

> A fast, keyboard-driven Bible reader for the terminal.

Open Psalm is a C++ TUI (Terminal User Interface) application for reading and studying Scripture. It features vim-style navigation, full-text search across multiple translations, Strong's concordance, cross-references, reading plans, verse export, and 11 built-in themes — all running locally in your terminal.

---

## Features

- **10 Bible translations** — KJV, ASV, YLT, BBE, WEB, Geneva (1599), Webster, DRC, CPDV, and KJVA (with Apocrypha)
- **Parallel view** — read two translations side-by-side
- **Full-text search** — FTS5-powered search across any translation or all translations at once
- **Strong's Concordance** — look up original Hebrew and Greek words with definitions
- **Cross-references** — see related verses for any passage
- **Reading plans** — daily reading schedule with progress tracking
- **Bookmarks** — save verses with optional notes
- **Verse export** — copy verse text, `$REF` format, Bible Gateway URLs, or save to file
- **11 color themes** — Synthwave '84, Sunset, Chrome, Matrix, Dracula, Nord, Gruvbox, Catppuccin Mocha, Tokyo Night, Monokai, Sepia
- **Vim-style navigation** — `j`/`k`, `g`/`G`, `[`/`]`, `/`, `?`, and more

---

## Installation

### Quick Install

```bash
git clone https://github.com/synthalorian/open-psalm
cd open-psalm
./install.sh
```

This builds the project and installs the `open-psalm` binary to `~/.local/bin/`. After installation, run:

```bash
open-psalm
```

### Dependencies

| Distribution | Command |
|--------------|---------|
| Arch Linux | `sudo pacman -S cmake gcc sqlite3 nlohmann-json` |
| Debian / Ubuntu | `sudo apt install cmake g++ libsqlite3-dev nlohmann-json3-dev` |
| Fedora | `sudo dnf install cmake gcc-c++ sqlite-devel nlohmann-json-devel` |

### Manual Build

```bash
git clone https://github.com/synthalorian/open-psalm
cd open-psalm

# Generate Bible databases (one-time)
python3 data/create_db.py

# Build
mkdir -p build && cd build
cmake ..
make -j$(nproc)

# Run
./open-psalm
```

### Quick Start

```bash
# Start at a specific passage
./open-psalm --translation KJV --book John --chapter 3 --verse 16

# Open to a random verse
./open-psalm --random

# Show verse of the day
./open-psalm --daily

# Print version
./open-psalm --version
```

---

## CLI Options

| Flag | Description |
|------|-------------|
| `--translation <code>` | Start with a specific translation |
| `--book <name>` | Jump to a specific book |
| `--chapter <n>` | Jump to a specific chapter |
| `--verse <n>` | Jump to a specific verse |
| `--random` | Open to a random verse |
| `--daily` | Show verse of the day and exit |
| `--help` | Show help with available translations |
| `--version` | Print version |

---

## Keybindings

### Navigation
| Key | Action |
|-----|--------|
| `j` / `k` | Scroll down / up |
| `h` / `l` | Switch panels (Books / Content) |
| `Tab` | Cycle focus between panels |
| `[` / `]` | Previous / Next chapter |
| `g` / `G` | Top / Bottom of chapter |
| `1`–`9` | Jump to chapter 1–9 |
| `Space` / `b` | Page down / Page up |
| `Enter` | Open book / Focus content |

### Study
| Key | Action |
|-----|--------|
| `t` | Cycle translation |
| `p` / `P` | Parallel view / Cycle second translation |
| `r` | Reading plan for today |
| `u` / `Ctrl+r` | Go back / Go forward in history |
| `Ctrl+g` | Go to verse (`Book Ch:Vs`) |
| `J` | Jump to book (type to filter) |
| `x` | Cross-references for current verse |
| `s` | Strong's concordance lookup |
| `m` / `M` | Bookmark verse / View bookmarks |
| `Ctrl+n` | Edit note on bookmark |
| `d` (bookmarks) | Delete bookmark |

### Search
| Key | Action |
|-----|--------|
| `/` | Search current translation |
| `C` | Cross-translation search (all DBs) |
| `n` / `N` | Next / Previous search result |
| `Up` / `Down` | Search history navigation |
| `Enter` | Jump to selected result |

### Export
| Key | Action |
|-----|--------|
| `y` | Yank (copy) verse text |
| `Ctrl+y` | Copy as `$REF` format (e.g. `John 3:16 KJV`) |
| `Ctrl+u` | Copy Bible Gateway URL |
| `E` | Export verse to TXT file |

### Appearance
| Key | Action |
|-----|--------|
| `T` | Cycle themes |
| `?` | Toggle help screen |
| `q` / `Esc` | Quit / Close overlay |

---

## Themes

| Theme | Description |
|-------|-------------|
| **Synthwave '84** | Neon magenta/cyan on deep purple (default) |
| **Sunset** | Warm orange/pink gradient |
| **Chrome** | Clean glass-morphism, light background |
| **Matrix** | Green phosphor on black |
| **Dracula** | Dark purple/pink Dracula palette |
| **Nord** | Arctic blue-grey |
| **Gruvbox** | Warm retro ochre |
| **Catppuccin Mocha** | Soft dark pastel |
| **Tokyo Night** | Deep blue with cyan accents |
| **Monokai** | High-contrast developer-friendly |
| **Sepia** | Warm paper-toned light theme |

---

## Tech Stack

| Layer | Technology |
|-------|-----------|
| **Language** | C++20 |
| **TUI Framework** | FTXUI 5.0 |
| **Database** | SQLite3 with FTS5 full-text search |
| **Data Format** | JSON source → SQLite databases |
| **Build System** | CMake 3.16+ |
| **Test Framework** | Catch2 |

---

## Development

### Running Tests

```bash
cd build
cmake .. -DBUILD_TESTS=ON
make -j$(nproc)
./open-psalm-tests
# or
ctest --output-on-failure
```

The test suite includes 32 test cases covering:
- Database connectivity and query accuracy
- CLI argument parsing
- Search highlighting with edge cases
- Book, chapter, and verse count verification across translations

### Building Database Files

```bash
# Full download and build
python3 data/create_db.py

# Download translations only
python3 data/download_translations.py

# Download extras (Strong's, cross-references)
python3 data/download_extras.py
```

---

## License

[MIT](LICENSE)

---

*Built with C++20, FTXUI, and SQLite3.*
---

## ☕ Support the Developer

If this project saved you time, solved a problem, or just made your day a little more neon, you can fuel the next one:

[![Buy Me A Coffee](https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png)](https://buymeacoffee.com/synthalorian)
