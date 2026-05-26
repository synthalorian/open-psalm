#!/usr/bin/env python3
"""Download and convert Bible translations to flat 'Book Chapter:Verse': 'text' JSON format."""
import json
import os
import sys
import base64
import urllib.request
import urllib.error

DATA_DIR = os.path.dirname(os.path.abspath(__file__))


def download_blob(owner, repo, sha):
    """Download a git blob from GitHub API and return decoded content."""
    url = f"https://api.github.com/repos/{owner}/{repo}/git/blobs/{sha}"
    req = urllib.request.Request(url, headers={"User-Agent": "open-psalm"})
    try:
        with urllib.request.urlopen(req, timeout=120) as resp:
            data = json.load(resp)
            if "content" in data:
                return base64.b64decode(data["content"]).decode("utf-8")
            else:
                print(f"  ERROR: {data.get('message', 'Unknown error')}")
                return None
    except Exception as e:
        print(f"  ERROR downloading blob: {e}")
        return None


def download_url(url):
    """Download content from a URL."""
    req = urllib.request.Request(url, headers={"User-Agent": "open-psalm"})
    try:
        with urllib.request.urlopen(req, timeout=60) as resp:
            return resp.read().decode("utf-8")
    except Exception as e:
        print(f"  ERROR downloading {url}: {e}")
        return None


def extract_verse_text(val):
    """Extract verse text from either a string or a dict with 'text' key."""
    if isinstance(val, str):
        return val
    elif isinstance(val, dict):
        # Handle {"verse": N, "text": "..."} format
        return val.get("text", str(val))
    return str(val)


def convert_scrollmapper_to_flat(content, translation_name):
    """Convert scrollmapper format to flat key-value JSON.
    Format: {translation, books: [{name, chapters: [{chapter: N, verses: [...]}]}]}
    """
    data = json.loads(content)
    books = data.get("books", [])
    flat = {}
    for book in books:
        name = book.get("name", "")
        chapters = book.get("chapters", [])
        for ch_obj in chapters:
            if isinstance(ch_obj, dict):
                ci = ch_obj.get("chapter", 1)
                verses = ch_obj.get("verses", [])
                for vi, verse in enumerate(verses, 1):
                    key = f"{name} {ci}:{vi}"
                    flat[key] = extract_verse_text(verse)
            elif isinstance(ch_obj, list):
                for ci, chapter in enumerate(ch_obj, 1):
                    for vi, verse in enumerate(chapter, 1):
                        key = f"{name} {ci}:{vi}"
                        flat[key] = extract_verse_text(verse)
    print(f"  {translation_name}: {len(flat)} verses from {len(books)} books")
    return flat


def convert_bbe_to_flat(content):
    """Convert BBE format [{abbrev, name, chapters: [[verses]]}] to flat."""
    if content.startswith('\ufeff'):
        content = content[1:]
    data = json.loads(content)
    flat = {}
    for book in data:
        name = book.get("name", book.get("abbrev", ""))
        chapters = book.get("chapters", [])
        for ci, chapter in enumerate(chapters, 1):
            for vi, verse_text in enumerate(chapter, 1):
                key = f"{name} {ci}:{vi}"
                flat[key] = str(verse_text) if verse_text is not None else ""
    print(f"  BBE: {len(flat)} verses from {len(data)} books")
    return flat


def save_flat(data, filename):
    """Save flat JSON to file."""
    path = os.path.join(DATA_DIR, filename)
    with open(path, "w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, indent=None, separators=(',', ':'))
    print(f"  Saved {path} ({os.path.getsize(path):,} bytes)")


def verify_flat(filename):
    """Verify that a flat JSON file has the correct format."""
    path = os.path.join(DATA_DIR, filename)
    with open(path, "r", encoding="utf-8") as f:
        data = json.load(f)
    keys = list(data.keys())
    print(f"  Verified {filename}: {len(keys)} keys")
    if keys:
        val = data[keys[0]]
        print(f"    Sample key: {repr(keys[0])}")
        print(f"    Sample value type: {type(val).__name__}")
        print(f"    Sample value: {str(val)[:80]}")
        # Use split-based parsing
        k = keys[0]
        parts = k.rsplit(' ', 1)
        if len(parts) == 2:
            book = parts[0]
            ch_v = parts[1].split(':')
            if len(ch_v) == 2:
                print(f"    Parsed: book={repr(book)} ch={ch_v[0]} v={ch_v[1]}")
    return len(keys)


def main():
    results = {}

    # 1. ASV from scrollmapper
    print("\n=== Downloading ASV ===")
    content = download_blob("scrollmapper", "bible_databases", "16813f9098ec47b15f318b32c876f30ce1eb48cb")
    if content:
        flat = convert_scrollmapper_to_flat(content, "ASV")
        save_flat(flat, "en_asv.json")
        verify_flat("en_asv.json")
        results["ASV"] = len(flat)
    else:
        print("  FAILED to download ASV")

    # 2. YLT from scrollmapper
    print("\n=== Downloading YLT ===")
    content = download_blob("scrollmapper", "bible_databases", "7ceffe6737687b33255df0266b243407925e5ada")
    if content:
        flat = convert_scrollmapper_to_flat(content, "YLT")
        save_flat(flat, "en_ylt.json")
        verify_flat("en_ylt.json")
        results["YLT"] = len(flat)
    else:
        print("  FAILED to download YLT")

    # 3. BBE from thiagobodruk/bible
    print("\n=== Downloading BBE ===")
    content = download_url("https://raw.githubusercontent.com/thiagobodruk/bible/master/json/en_bbe.json")
    if content:
        flat = convert_bbe_to_flat(content)
        save_flat(flat, "en_bbe.json")
        verify_flat("en_bbe.json")
        results["BBE"] = len(flat)
    else:
        print("  FAILED to download BBE")

    # Summary
    print("\n=== Summary ===")
    for name, count in results.items():
        print(f"  {name}: {count} verses")
    if not results:
        print("No translations downloaded successfully.")
        sys.exit(1)


if __name__ == "__main__":
    main()
