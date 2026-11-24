#!/usr/bin/env python3
"""Build a metadata-only PS4 catalogue; never copy package/source URLs."""
import json
import sys
from pathlib import Path
from urllib.parse import urlparse

def safe_artwork(value):
    if not isinstance(value, str):
        return ""
    parsed = urlparse(value)
    if parsed.scheme != "https" or parsed.netloc != "cdn.orbispatches.com":
        return ""
    return value

def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: sanitize_catalog.py INPUT.json OUTPUT.json")
    source = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
    releases = source.get("releases")
    if not isinstance(releases, list):
        raise SystemExit("catalogue has no releases array")

    entries = []
    seen = set()
    for item in releases:
        if not isinstance(item, dict) or item.get("platform") != "ps4":
            continue
        entry_id = item.get("id")
        title_id = item.get("titleId")
        title = item.get("title")
        if not all(isinstance(v, str) and v.strip() for v in (entry_id, title_id, title)):
            continue
        if entry_id in seen:
            continue
        seen.add(entry_id)
        size = item.get("sizeBytes", 0)
        if not isinstance(size, int) or size < 0:
            size = 0
        version = item.get("version")
        if not isinstance(version, str):
            version = ""
        region = item.get("region")
        if not isinstance(region, str):
            region = ""
        publisher = item.get("publisher")
        if not isinstance(publisher, str):
            publisher = ""
        release_date = item.get("releaseDate")
        if not isinstance(release_date, str):
            release_date = ""
        game_id = item.get("gameId")
        if not isinstance(game_id, str):
            game_id = ""
        entries.append({
            "id": entry_id,
            "titleId": title_id,
            "title": title,
            "gameId": game_id,
            "region": region,
            "version": version,
            "sizeBytes": size,
            "publisher": publisher,
            "releaseDate": release_date,
            "cover": safe_artwork(item.get("cover")),
            "hero": safe_artwork(item.get("hero")),
        })

    output = {
        "schemaVersion": 1,
        "sourceRevision": source.get("revision", 0),
        "entryCount": len(entries),
        "entries": entries,
    }
    destination = Path(sys.argv[2])
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(json.dumps(output, ensure_ascii=False, separators=(",", ":")) + "\n",
                           encoding="utf-8")
    print(f"Sanitized {len(entries)} PS4 metadata entries -> {destination}")

if __name__ == "__main__":
    main()
