# Open Psalm

**"The Word, blazing fast in your terminal."**

A high-performance TUI (Terminal User Interface) Bible reader for Linux. Vim-style keybindings, 11 neon themes, full-text search across 10 Bible translations, Strong's concordance, cross-references, reading plans, and verse export — all in your terminal.

---

## Features

### Reading
- **10 Bible translations** — KJV, ASV, YLT, BBE, WEB, Geneva (1599), Webster, DRC, CPDV, KJVA with Apocrypha
- **Parallel view** — read two translations side-by-side
- **Reading plans** — daily reading schedule with progress tracking
- **Verse of the Day** — randomly selected verse on launch
- **Navigation history** — go backward/forward through your reading

### Study Tools
- **Strong's Concordance** — lookup original Hebrew/Greek words with definitions
- **Cross-references** — see related verses for any passage
- **Full-text search** — FTS5-powered across any translation
- **Cross-translation search** — search all databases at once
- **Bookmarks** — save verses with optional notes

### Export & Share
- **Yank verse** — copy verse text to clipboard
- **$REF format** — copy as "John 3:16 (KJV)"
- **Bible Gateway URL** — copy direct link
- **Export to file** — save verse to TXT

### Appearance
- **11 themes** — Synthwave '84, Sunset, Chrome, Matrix, Dracula, Nord, Gruvbox, Catppuccin Mocha, Tokyo Night, Monokai, Sepia
- **Vim-style navigation** — j/k, g/G, [, ], /, ? and more
- **Dual-pane layout** — book list + chapter content

### Bible Data
- **66 canonical books** across all standard translations
- **Apocrypha included** in KJVA translation
- **Verse-accurate** — verified against source texts
- **Scripture indexed by book, chapter, and verse**
- **FTS5 full-text search** with stemming

---

## Installation

### Dependencies

```bash
# Arch Linux
sudo pacman -S cmake gcc sqlite3 nlohmann-json

# Debian / Ubuntu
sudo apt install cmake g++ libsqlite3-dev nlohmann-json3-dev

# Fedora
sudo dnf install cmake gcc-c++ sqlite-devel nlohmann-json-devel
```

### Build

```bash
git clone https://github.com/synthalorian/open-psalm
cd open-psalm

# Generate the Bible databases (one-time)
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
# Start with a specific translation and passage
./open-psalm --translation KJV --book John --chapter 3 --verse 16

# Start with a random verse
./open-psalm --random

# Show verse of the day and exit
./open-psalm --daily

# Print version
./open-psalm --version
```

---

## CLI Options

| Flag | Description |
|------|-------------|
| `--translation <code>` | Start with a specific translation (KJV, ASV, YLT, BBE, WEB, Geneva, Webster, DRC, CPDV, KJVA) |
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
| `1`-`9` | Jump to chapter 1-9 |
| `Space` / `b` | Page down / Page up |
| `Enter` | Open book / Focus content |

### Study Features
| Key | Action |
|-----|--------|
| `t` | Cycle translation |
| `p` / `P` | Parallel view / Cycle second translation |
| `r` | Reading plan for today |
| `u` / `Ctrl+r` | Go back / Go forward in history |
| `Ctrl+g` | Go to verse (Book Ch:Vs) |
| `J` | Jump to book (type to filter) |
| `x` | Cross-references for current verse |

### Search
| Key | Action |
|-----|--------|
| `/` | Search current translation |
| `C` | Cross-translation search (all DBs) |
| `n` / `N` | Next / Previous search result |
| `Up` / `Down` | Search history navigation |
| `Enter` | Jump to selected result |

### Reference Tools
| Key | Action |
|-----|--------|
| `s` | Strong's concordance lookup |
| `m` / `M` | Bookmark verse / View bookmarks |
| `Ctrl+n` | Edit note on bookmark |
| `d` (bookmarks) | Delete bookmark |

### Export & Share
| Key | Action |
|-----|--------|
| `y` | Yank (copy) verse text |
| `Ctrl+y` | Copy as `$REF` format (John 3:16 KJV) |
| `Ctrl+u` | Copy Bible Gateway URL |
| `E` | Export verse to TXT file |

### Appearance & Misc
| Key | Action |
|-----|--------|
| `T` | Cycle themes |
| `?` | Toggle help screen |
| `q` / `Esc` | Quit / Close overlay |

---

## Themes

| Theme | Vibe |
|-------|------|
| **Synthwave '84** | Neon magenta/cyan on deep purple — the default |
| **Sunset** | Warm orange/pink gradient |
| **Chrome** | Clean glass-morphism, light background |
| **Matrix** | Green phosphor on black |
| **Dracula** | Dark purple/pink Dracula palette |
| **Nord** | Arctic blue-grey |
| **Gruvbox** | Warm retro ochre |
| **Catppuccin Mocha** | Soft dark pastel |
| **Tokyo Night** | Deep blue with cyan accents |
| **Monokai** | High-contrast dev-friendly |
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

The test suite covers:
- Database connectivity and query accuracy (12 test cases)
- CLI argument parsing (9 test cases)
- Search highlighting with edge cases (11 test cases)
- Verifies all 66 books across translations
- Verifies chapter/verse counts (Genesis 50, Psalm 119:176, John 3:16)

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

MIT

---

## Credits

**Created by:** synth (synthalorian) with assistance from synthclaw 🎹🦞

*"The Word, blazing fast in your terminal."*