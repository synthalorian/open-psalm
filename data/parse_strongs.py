#!/usr/bin/env python3
"""Parse Strong's Dictionary XHTML (ol/li format) and create SQLite + JSON."""
import json
import sqlite3
import os
import re

DATA_DIR = os.path.dirname(os.path.abspath(__file__))
XHTML_PATH = os.path.join(DATA_DIR, "strongs.xhtml")
DB_PATH = os.path.join(DATA_DIR, "strongs.db")
JSON_PATH = os.path.join(DATA_DIR, "strongs.json")


def extract_entries(content, section_id, prefix):
    """Extract entries from a <section> with given id."""
    # Find the section
    section_pattern = re.compile(
        r'<section[^>]*id=[\'\"]' + section_id + r'[\'\"][^>]*>(.*?)</section>',
        re.DOTALL
    )
    section_match = section_pattern.search(content)
    if not section_match:
        print(f"  Section '{section_id}' not found!")
        return {}

    section = section_match.group(1)

    # Find all <li> entries
    li_pattern = re.compile(
        r'<li\s+value=[\'\"](\d+)[\'\"][^>]*>(.*?)</li>',
        re.DOTALL
    )

    entries = {}
    li_matches = li_pattern.findall(section)
    print(f"  Found {len(li_matches)} entries in section '{section_id}'")

    for value_str, li_content in li_matches:
        strongs_id = f"{prefix}{value_str}"

        # Extract heading (text before <span class="kjv_def">)
        # Find the first <i> tag for transliteration
        i_match = re.search(r'<i[^>]*>(.*?)</i>', li_content, re.DOTALL)
        heading = ""
        if i_match:
            heading = re.sub(r'<[^>]+>', '', i_match.group(1)).strip()

        # Extract definition from <span class="kjv_def">
        def_match = re.search(r'<span\s+class="kjv_def"[^>]*>(.*?)</span>', li_content, re.DOTALL)
        definition = ""
        if def_match:
            definition = re.sub(r'<[^>]+>', '', def_match.group(1)).strip()
            definition = re.sub(r'\s+', ' ', definition).strip()

        # If no kjv_def, try the full text
        if not definition:
            # Remove all tags, get remaining text
            clean = re.sub(r'<[^>]+>', ' ', li_content)
            definition = re.sub(r'\s+', ' ', clean).strip()

        if definition:
            entries[strongs_id] = {
                "word": heading if heading else "",
                "definition": definition[:600]  # Trim
            }

    return entries


def main():
    print("Parsing Strong's Dictionary XHTML...")

    if not os.path.exists(XHTML_PATH):
        print(f"Error: {XHTML_PATH} not found.")
        return

    with open(XHTML_PATH, "r", encoding="utf-8") as f:
        content = f.read()

    print(f"File size: {len(content):,} bytes")

    # Extract OT (Hebrew) entries
    print("\nExtracting Hebrew entries (OT)...")
    hebrew = extract_entries(content, "ot", "H")

    # Extract NT (Greek) entries
    print("Extracting Greek entries (NT)...")
    greek = extract_entries(content, "nt", "G")

    # Combine
    entries = {**hebrew, **greek}
    print(f"\nTotal entries: {len(entries)}")

    # Write JSON
    with open(JSON_PATH, "w", encoding="utf-8") as f:
        json.dump(entries, f, ensure_ascii=False, indent=1)
    print(f"Wrote JSON: {os.path.getsize(JSON_PATH):,} bytes")

    # Create SQLite database
    if os.path.exists(DB_PATH):
        os.remove(DB_PATH)

    conn = sqlite3.connect(DB_PATH)
    conn.execute("PRAGMA journal_mode=WAL")

    c = conn.cursor()
    c.execute("""
        CREATE TABLE IF NOT EXISTS strongs (
            id TEXT PRIMARY KEY,
            word TEXT,
            definition TEXT,
            language TEXT GENERATED ALWAYS AS (
                CASE WHEN substr(id, 1, 1) = 'H' THEN 'hebrew' ELSE 'greek' END
            ) STORED
        )
    """)
    c.execute("CREATE INDEX idx_strongs_lang ON strongs(language)")

    count = 0
    for sid, entry in entries.items():
        c.execute(
            "INSERT OR REPLACE INTO strongs (id, word, definition) VALUES (?, ?, ?)",
            (sid, entry["word"], entry["definition"])
        )
        count += 1

    conn.commit()

    # Verify
    c.execute("SELECT COUNT(*) FROM strongs")
    total = c.fetchone()[0]
    c.execute("SELECT language, COUNT(*) FROM strongs GROUP BY language")
    lang_counts = c.fetchall()
    conn.close()

    print(f"\nVerification:")
    print(f"  Total: {total}")
    for lang, cnt in lang_counts:
        print(f"  {lang}: {cnt}")
    print(f"  DB size: {os.path.getsize(DB_PATH):,} bytes")
    print("Done!")


if __name__ == "__main__":
    main()
