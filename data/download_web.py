#!/usr/bin/env python3
"""Download WEB translation from TehShrike/world-english-bible repo.
Per-book JSON format: [{type, chapterNumber, verseNumber, value}, ...]
Converts to flat "Book Chapter:Verse": "text" format.
"""
import json
import os
import sys
import urllib.request

DATA_DIR = os.path.dirname(os.path.abspath(__file__))

# Map book name from TehShrike filenames to standard names
BOOK_NAME_MAP = {
    "genesis": "Genesis",
    "exodus": "Exodus",
    "leviticus": "Leviticus",
    "numbers": "Numbers",
    "deuteronomy": "Deuteronomy",
    "joshua": "Joshua",
    "judges": "Judges",
    "ruth": "Ruth",
    "1samuel": "1 Samuel",
    "2samuel": "2 Samuel",
    "1kings": "1 Kings",
    "2kings": "2 Kings",
    "1chronicles": "1 Chronicles",
    "2chronicles": "2 Chronicles",
    "ezra": "Ezra",
    "nehemiah": "Nehemiah",
    "esther": "Esther",
    "job": "Job",
    "psalms": "Psalms",
    "proverbs": "Proverbs",
    "ecclesiastes": "Ecclesiastes",
    "songofsolomon": "Song of Solomon",
    "isaiah": "Isaiah",
    "jeremiah": "Jeremiah",
    "lamentations": "Lamentations",
    "ezekiel": "Ezekiel",
    "daniel": "Daniel",
    "hosea": "Hosea",
    "joel": "Joel",
    "amos": "Amos",
    "obadiah": "Obadiah",
    "jonah": "Jonah",
    "micah": "Micah",
    "nahum": "Nahum",
    "habakkuk": "Habakkuk",
    "zephaniah": "Zephaniah",
    "haggai": "Haggai",
    "zechariah": "Zechariah",
    "malachi": "Malachi",
    "matthew": "Matthew",
    "mark": "Mark",
    "luke": "Luke",
    "john": "John",
    "acts": "Acts",
    "romans": "Romans",
    "1corinthians": "1 Corinthians",
    "2corinthians": "2 Corinthians",
    "galatians": "Galatians",
    "ephesians": "Ephesians",
    "philippians": "Philippians",
    "colossians": "Colossians",
    "1thessalonians": "1 Thessalonians",
    "2thessalonians": "2 Thessalonians",
    "1timothy": "1 Timothy",
    "2timothy": "2 Timothy",
    "titus": "Titus",
    "philemon": "Philemon",
    "hebrews": "Hebrews",
    "james": "James",
    "1peter": "1 Peter",
    "2peter": "2 Peter",
    "1john": "1 John",
    "2john": "2 John",
    "3john": "3 John",
    "jude": "Jude",
    "revelation": "Revelation",
}

def download_book(book_key):
    """Download a single book JSON from the WEB repo."""
    url = f"https://raw.githubusercontent.com/TehShrike/world-english-bible/master/json/{book_key}.json"
    req = urllib.request.Request(url, headers={"User-Agent": "open-psalm"})
    try:
        with urllib.request.urlopen(req, timeout=30) as resp:
            return json.loads(resp.read().decode("utf-8"))
    except Exception as e:
        print(f"  ERROR downloading {book_key}: {e}")
        return None

def convert_book_to_flat(book_data, book_name):
    """Convert a book's paragraph array to flat verse dict.
    Concatenates paragraph text segments for the same verse.
    """
    verses = {}  # key: (chapter, verse) -> text
    current_ch = None
    current_v = None
    current_text = []

    for item in book_data:
        t = item.get("type")
        if t in ("paragraph start", "paragraph end"):
            continue
        if t == "paragraph text":
            ch = item.get("chapterNumber")
            vs = item.get("verseNumber")
            val = item.get("value", "")

            if (ch, vs) != (current_ch, current_v):
                # Save previous verse if any
                if current_ch is not None and current_v is not None and current_text:
                    key = (current_ch, current_v)
                    verses[key] = " ".join(current_text).strip()
                current_ch, current_v = ch, vs
                current_text = []

            # Clean value - remove leading/trailing whitespace
            val = val.strip()
            if val:
                # Remove leading space before punctuation
                if current_text and val.startswith((";", ":", ",", ".")):
                    current_text[-1] = current_text[-1] + val
                else:
                    current_text.append(val)
            else:
                # Empty text might be a chapter heading
                pass

    # Don't forget the last verse
    if current_ch is not None and current_v is not None and current_text:
        key = (current_ch, current_v)
        verses[key] = " ".join(current_text).strip()

    # Convert to flat format
    flat = {}
    for (ch, vs), text in sorted(verses.items()):
        key = f"{book_name} {ch}:{vs}"
        flat[key] = text

    return flat

def main():
    print("=== Downloading WEB from TehShrike/world-english-bible ===\n")

    all_verses = {}
    book_count = 0

    for book_key, book_name in BOOK_NAME_MAP.items():
        print(f"  Downloading {book_name}...", end=" ", flush=True)
        data = download_book(book_key)
        if data is None:
            print("FAILED")
            continue

        flat = convert_book_to_flat(data, book_name)
        all_verses.update(flat)
        book_count += 1
        print(f"{len(flat)} verses")

    print(f"\n  Total: {book_count} books, {len(all_verses)} verses")

    # Save
    output_path = os.path.join(DATA_DIR, "en_web.json")
    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(all_verses, f, ensure_ascii=False, indent=None, separators=(",", ":"))
    size = os.path.getsize(output_path)
    print(f"  Saved {output_path} ({size:,} bytes)")

    # Verify
    keys = list(all_verses.keys())
    if keys:
        print(f"  Sample key: {repr(keys[0])}")
        print(f"  Sample value: {str(all_verses[keys[0]])[:100]}")
        # Verify parsing
        k = keys[0]
        parts = k.rsplit(" ", 1)
        if len(parts) == 2:
            cv = parts[1].split(":")
            print(f"  Parsed: book={repr(parts[0])} ch={cv[0]} v={cv[1]}")
        k2 = keys[len(keys)//2]
        parts = k2.rsplit(" ", 1)
        cv2 = parts[1].split(":")
        print(f"  Mid key: book={repr(parts[0])} ch={cv2[0]} v={cv2[1]}")

    return len(all_verses)


if __name__ == "__main__":
    count = main()
    print(f"\nDone! {count} verses downloaded.")
