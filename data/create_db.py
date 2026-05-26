#!/usr/bin/env python3
"""Create SQLite databases for Bible translations from flat JSON files.
Replicates the schema from the C++ Database::createSchema() method.
"""
import json
import sqlite3
import os
import sys

DATA_DIR = os.path.dirname(os.path.abspath(__file__))

NT_BOOKS = [
    "Matthew", "Mark", "Luke", "John", "Acts", "Romans",
    "1 Corinthians", "2 Corinthians", "Galatians", "Ephesians",
    "Philippians", "Colossians", "1 Thessalonians", "2 Thessalonians",
    "1 Timothy", "2 Timothy", "Titus", "Philemon", "Hebrews",
    "James", "1 Peter", "2 Peter", "1 John", "2 John", "3 John",
    "Jude", "Revelation"
]

TRANSLATIONS = [
    ("en_asv.json", "bible_asv.db", "ASV", "American Standard Version (1901)"),
    ("en_ylt.json", "bible_ylt.db", "YLT", "Young's Literal Translation (1898)"),
    ("en_bbe.json", "bible_bbe.db", "BBE", "Bible in Basic English (1965)"),
    ("en_web.json", "bible_web.db", "WEB", "World English Bible"),
    ("en_geneva1599.json", "bible_geneva1599.db", "Geneva", "Geneva Bible (1599)"),
    ("en_webster.json", "bible_webster.db", "Webster", "Webster's Bible (1833)"),
    ("en_drc.json", "bible_drc.db", "DRC", "Douay-Rheims (Challoner)"),
    ("en_cpdv.json", "bible_cpdv.db", "CPDV", "Catholic Public Domain Version"),
    ("en_kjv.json", "bible_kjv.db", "KJV", "King James Version (1769)"),
]


def create_database(json_path, db_path, code, name):
    """Create a SQLite database from a flat JSON Bible file."""
    print(f"\n=== Creating {code}: {name} ===")

    # Load JSON
    print(f"  Loading {json_path}...")
    with open(os.path.join(DATA_DIR, json_path), "r", encoding="utf-8") as f:
        data = json.load(f)
    print(f"  Loaded {len(data)} verses")

    # Remove old db
    full_db_path = os.path.join(DATA_DIR, db_path)
    if os.path.exists(full_db_path):
        os.remove(full_db_path)
        print(f"  Removed old {db_path}")

    # Open database
    conn = sqlite3.connect(full_db_path)
    conn.execute("PRAGMA journal_mode=WAL")
    conn.execute("PRAGMA synchronous=NORMAL")
    c = conn.cursor()

    # Create schema
    print("  Creating schema...")
    c.executescript("""
        CREATE TABLE IF NOT EXISTS books (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT UNIQUE NOT NULL,
            testament TEXT NOT NULL DEFAULT 'old'
        );
        CREATE TABLE IF NOT EXISTS verses (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            book_id INTEGER NOT NULL,
            chapter INTEGER NOT NULL,
            verse INTEGER NOT NULL,
            text TEXT NOT NULL,
            FOREIGN KEY (book_id) REFERENCES books(id)
        );
        CREATE INDEX IF NOT EXISTS idx_verses_book_chapter
            ON verses(book_id, chapter);
        CREATE VIRTUAL TABLE IF NOT EXISTS verses_fts USING fts5(
            text,
            content='verses',
            content_rowid='id'
        );
    """)

    # Collect unique books in order of appearance using rsplit
    ordered_books = []
    for key in data:
        parts = key.rsplit(' ', 1)
        if len(parts) != 2:
            continue
        book_name = parts[0]
        if book_name not in ordered_books:
            ordered_books.append(book_name)

    print(f"  Found {len(ordered_books)} books")

    # Insert books
    print("  Inserting books...")
    for book in ordered_books:
        testament = "new" if book in NT_BOOKS else "old"
        c.execute("INSERT INTO books (name, testament) VALUES (?, ?)", (book, testament))

    # Get book IDs
    book_ids = {}
    for row in c.execute("SELECT id, name FROM books"):
        book_ids[row[1]] = row[0]

    # Insert verses
    print("  Inserting verses...")
    count = 0
    verse_data = []

    for key, text in data.items():
        # Parse "BookName C:V" using rsplit
        parts = key.rsplit(' ', 1)
        if len(parts) != 2:
            continue
        book_name = parts[0]
        ch_v = parts[1].split(':')
        if len(ch_v) != 2:
            continue
        try:
            chapter = int(ch_v[0])
            verse = int(ch_v[1])
        except ValueError:
            continue

        if book_name not in book_ids:
            continue

        verse_data.append((book_ids[book_name], chapter, verse, text))
        count += 1

        if count % 10000 == 0:
            print(f"    Prepared {count} verses...")

    c.executemany(
        "INSERT INTO verses (book_id, chapter, verse, text) VALUES (?, ?, ?, ?)",
        verse_data
    )
    print(f"  Inserted {count} verses")

    # Populate FTS index
    print("  Building FTS index...")
    c.execute("INSERT INTO verses_fts(rowid, text) SELECT id, text FROM verses")

    # Save
    conn.commit()
    db_size = os.path.getsize(full_db_path)
    print(f"  Database created: {full_db_path} ({db_size:,} bytes)")

    # Verify
    c.execute("SELECT COUNT(*) FROM verses")
    v_count = c.fetchone()[0]
    c.execute("SELECT COUNT(*) FROM books")
    b_count = c.fetchone()[0]
    c.execute("SELECT COUNT(*) FROM verses_fts")
    fts_count = c.fetchone()[0]
    print(f"  Verification: {b_count} books, {v_count} verses, {fts_count} FTS entries")

    conn.close()
    return v_count


def main():
    print("Creating Bible translation databases...")
    results = {}
    for json_file, db_file, code, name in TRANSLATIONS:
        try:
            count = create_database(json_file, db_file, code, name)
            results[code] = count
        except Exception as e:
            print(f"  ERROR creating {code}: {e}")

    print("\n=== Summary ===")
    for code, count in results.items():
        print(f"  {code}: {count} verses")
    print("Done!")


if __name__ == "__main__":
    main()
