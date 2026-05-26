#!/usr/bin/env python3
"""Download cross-references, KJVA (Apocrypha), and create SQLite databases."""

import json
import urllib.request
import sqlite3
import os

DATA_DIR = os.path.dirname(os.path.abspath(__file__))

def download(url, path):
    """Download a file if it doesn't already exist."""
    if os.path.exists(path):
        print(f"  Already exists: {path}")
        return True
    print(f"  Downloading {url}...")
    try:
        urllib.request.urlretrieve(url, path)
        size = os.path.getsize(path)
        print(f"  Saved {path} ({size} bytes)")
        return True
    except Exception as e:
        print(f"  Error: {e}")
        return False

def create_cross_references_db():
    """Create cross_references.db from scrollmapper JSON data."""
    db_path = os.path.join(DATA_DIR, "cross_references.db")
    
    # Download all 7 cross-reference JSON files
    json_files = []
    for i in range(7):
        url = f"https://raw.githubusercontent.com/scrollmapper/bible_databases/master/sources/extras/cross_references_{i}.json"
        local = os.path.join(DATA_DIR, f"cross_references_{i}.json")
        if download(url, local):
            json_files.append(local)
    
    if not json_files:
        print("  No cross-reference data downloaded.")
        return
    
    conn = sqlite3.connect(db_path)
    conn.execute("""
        CREATE TABLE IF NOT EXISTS cross_references (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            from_book TEXT NOT NULL,
            from_chapter INTEGER NOT NULL,
            from_verse INTEGER NOT NULL,
            to_book TEXT NOT NULL,
            to_chapter INTEGER NOT NULL,
            to_verse_start INTEGER NOT NULL,
            to_verse_end INTEGER NOT NULL,
            votes INTEGER DEFAULT 1
        )
    """)
    conn.execute("CREATE INDEX IF NOT EXISTS idx_xrefs_from ON cross_references(from_book, from_chapter, from_verse)")
    conn.execute("CREATE INDEX IF NOT EXISTS idx_xrefs_to ON cross_references(to_book, to_chapter)")
    
    total = 0
    for jf in json_files:
        try:
            with open(jf, 'r', encoding='utf-8') as f:
                data = json.load(f)
            refs = data.get('cross_references', [])
            for ref in refs:
                from_v = ref.get('from_verse', {})
                to_v_list = ref.get('to_verse', [])
                votes = ref.get('votes', 1)
                
                from_book = from_v.get('book', '')
                from_ch = from_v.get('chapter', 0)
                from_vs = from_v.get('verse', 0)
                
                for to_v in to_v_list:
                    conn.execute(
                        "INSERT INTO cross_references (from_book, from_chapter, from_verse, to_book, to_chapter, to_verse_start, to_verse_end, votes) VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
                        (from_book, from_ch, from_vs,
                         to_v.get('book', ''), to_v.get('chapter', 0),
                         to_v.get('verse_start', 0), to_v.get('verse_end', 0), votes)
                    )
                    total += 1
        except Exception as e:
            print(f"  Error processing {jf}: {e}")
    
    conn.commit()
    conn.close()
    print(f"  Created {db_path} with {total} cross-references.")

def create_kjva_db():
    """Create bible_kjva.db from scrollmapper KJVA.csv (KJV + Apocrypha)."""
    csv_path = os.path.join(DATA_DIR, "KJVA.csv")
    url = "https://raw.githubusercontent.com/scrollmapper/bible_databases/master/formats/csv/KJVA.csv"
    
    if not download(url, csv_path):
        print("  Failed to download KJVA.")
        return
    
    db_path = os.path.join(DATA_DIR, "bible_kjva.db")
    
    # Read CSV
    books = {}
    verses_data = []
    
    import csv
    with open(csv_path, 'r', encoding='utf-8') as f:
        reader = csv.reader(f)
        header = next(reader, None)
        for row in reader:
            if len(row) >= 4:
                book = row[0].strip()
                try:
                    chapter = int(row[1])
                    verse = int(row[2])
                except ValueError:
                    continue
                text = row[3].strip()
                verses_data.append((book, chapter, verse, text))
                if book not in books:
                    books[book] = len(books)
    
    conn = sqlite3.connect(db_path)
    conn.execute("PRAGMA journal_mode=WAL")
    conn.execute("PRAGMA synchronous=OFF")
    
    # Create books table
    conn.execute("""
        CREATE TABLE IF NOT EXISTS bible_kjva_books (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL
        )
    """)
    for name, idx in sorted(books.items(), key=lambda x: x[1]):
        conn.execute("INSERT OR IGNORE INTO bible_kjva_books (id, name) VALUES (?, ?)", (idx + 1, name))
    
    # Create verses table
    conn.execute("""
        CREATE TABLE IF NOT EXISTS bible_kjva_verses (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            book_id INTEGER,
            chapter INTEGER,
            verse INTEGER,
            text TEXT,
            FOREIGN KEY (book_id) REFERENCES bible_kjva_books(id)
        )
    """)
    
    book_id_map = {}
    cursor = conn.execute("SELECT id, name FROM bible_kjva_books")
    for row in cursor.fetchall():
        book_id_map[row[1]] = row[0]
    
    for book, ch, vs, txt in verses_data:
        bid = book_id_map.get(book, 1)
        conn.execute("INSERT INTO bible_kjva_verses (book_id, chapter, verse, text) VALUES (?, ?, ?, ?)",
                     (bid, ch, vs, txt))
    
    # Create FTS5 index
    conn.execute("""
        CREATE VIRTUAL TABLE IF NOT EXISTS bible_kjva_fts USING fts5(
            book, chapter, verse, text,
            content='bible_kjva_verses',
            content_rowid='id'
        )
    """)
    conn.execute("""
        INSERT INTO bible_kjva_fts (rowid, book, chapter, verse, text)
        SELECT v.id, b.name, v.chapter, v.verse, v.text
        FROM bible_kjva_verses v
        JOIN bible_kjva_books b ON v.book_id = b.id
    """)
    
    conn.commit()
    conn.close()
    print(f"  Created {db_path} with {len(books)} books, {len(verses_data)} verses.")

if __name__ == '__main__':
    print("=== Downloading KJVA (KJV + Apocrypha) ===")
    create_kjva_db()
    print()
    print("=== Creating Cross-References Database ===")
    create_cross_references_db()
    print()
    print("Done!")
