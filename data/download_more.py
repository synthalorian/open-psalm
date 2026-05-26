#!/usr/bin/env python3
"""Download additional public domain Bible translations from scrollmapper CSV format.
Converts to flat JSON format (key=Book C:V -> text) for compatibility with create_db.py.
"""
import csv
import json
import os
import io
import urllib.request

DATA_DIR = os.path.dirname(os.path.abspath(__file__))

TRANSLATIONS = [
    ("Geneva1599", "Geneva Bible (1599)"),
    ("Webster", "Webster's Bible (1833)"),
    ("DRC", "Douay-Rheims (Challoner)"),
    ("CPDV", "Catholic Public Domain Version"),
]

BASE_URL = "https://raw.githubusercontent.com/scrollmapper/bible_databases/master/formats/csv/"


def download_and_convert(code, name):
    """Download CSV and convert to flat JSON format."""
    print(f"\\n=== {code}: {name} ===")
    csv_url = BASE_URL + code + ".csv"

    print(f"  Downloading {csv_url}...")
    try:
        req = urllib.request.Request(csv_url, headers={"User-Agent": "Mozilla/5.0"})
        response = urllib.request.urlopen(req, timeout=60)
        raw = response.read().decode("utf-8")
        print(f"  Downloaded {len(raw):,} bytes")
    except Exception as e:
        print(f"  ERROR downloading: {e}")
        return 0

    # Parse CSV
    reader = csv.reader(io.StringIO(raw))
    verse_count = 0
    data = {}

    for row in reader:
        if len(row) < 4:
            continue
        book = row[0].strip()
        chapter = row[1].strip()
        verse = row[2].strip()
        text = row[3].strip().strip('"')

        # Skip header
        if book.lower() == "book":
            continue

        key = f"{book} {chapter}:{verse}"
        data[key] = text
        verse_count += 1

    # Write flat JSON
    json_filename = f"en_{code.lower()}.json"
    json_path = os.path.join(DATA_DIR, json_filename)
    with open(json_path, "w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, indent=1)

    print(f"  Wrote {verse_count} verses to {json_filename}")
    return verse_count


def main():
    print("Downloading additional Bible translations...")
    results = {}
    for code, name in TRANSLATIONS:
        try:
            count = download_and_convert(code, name)
            results[code] = count
        except Exception as e:
            print(f"  ERROR processing {code}: {e}")

    print("\\n=== Summary ===")
    for code, count in results.items():
        print(f"  {code}: {count:,} verses")
    print("Done!")


if __name__ == "__main__":
    main()
