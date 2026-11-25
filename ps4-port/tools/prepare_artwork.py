#!/usr/bin/env python3
"""Prepare real image assets for the native SDL2 PS4 interface.

Artwork is fetched only from the catalogue's cover-image URLs. Package URLs
are never read by this script and are never downloaded during the build.
"""
import argparse
import concurrent.futures
import io
import json
import os
import urllib.request
from PIL import Image, ImageOps

USER_AGENT = "BLACKBOX-PS4-Artwork/1.0"
MAX_WORKERS = 12

def fetch_cover(item, output_dir):
    title_id = "".join(c for c in str(item.get("titleId", "")) if c.isalnum() or c in "_-")
    url = item.get("cover") or item.get("coverFallback") or ""
    # Only the dedicated cover field is fetched; package/download URLs are never used.
    if not title_id or not url.startswith("https://"):
        return False
    target = os.path.join(output_dir, title_id + ".bmp")
    if os.path.isfile(target) and os.path.getsize(target) > 1024:
        return True
    try:
        req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
        with urllib.request.urlopen(req, timeout=18) as response:
            data = response.read(8 * 1024 * 1024 + 1)
        if len(data) > 8 * 1024 * 1024:
            return False
        image = Image.open(io.BytesIO(data)).convert("RGB")
        image = ImageOps.fit(image, (192, 192), method=Image.Resampling.LANCZOS)
        image.save(target, format="BMP")
        return True
    except Exception:
        print("BLACKBOX cover failed: %s (%s)" % (title_id, url))
        return False

def make_background(source, target):
    image = Image.open(source).convert("RGB")
    image = ImageOps.fit(image, (1920, 1080), method=Image.Resampling.LANCZOS)
    image.save(target, format="BMP")

def make_logo(source, target):
    image = Image.open(source).convert("RGBA")
    image.thumbnail((520, 150), Image.Resampling.LANCZOS)
    canvas = Image.new("RGBA", (560, 180), (9, 14, 24, 255))
    canvas.alpha_composite(image, ((560 - image.width) // 2, (180 - image.height) // 2))
    canvas.convert("RGB").save(target, format="BMP")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--catalog", required=True)
    parser.add_argument("--assets", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    os.makedirs(args.output, exist_ok=True)
    covers_dir = os.path.join(args.output, "covers")
    os.makedirs(covers_dir, exist_ok=True)
    make_background(os.path.join(args.assets, "blackbox-wallpaper.jpg"),
                    os.path.join(args.output, "ui-background.bmp"))
    make_logo(os.path.join(args.assets, "blackbox-logo.png"),
              os.path.join(args.output, "ui-logo.bmp"))

    with open(args.catalog, "r", encoding="utf-8") as handle:
        catalogue = json.load(handle)
    unique = {}
    for entry in catalogue.get("entries", []):
        key = entry.get("titleId")
        if key and key not in unique:
            unique[key] = entry
    items = list(unique.values())
    successes = 0
    with concurrent.futures.ThreadPoolExecutor(max_workers=MAX_WORKERS) as pool:
        futures = [pool.submit(fetch_cover, item, covers_dir) for item in items]
        for future in concurrent.futures.as_completed(futures):
            if future.result():
                successes += 1
    print("BLACKBOX artwork: %d/%d covers prepared" % (successes, len(items)))

if __name__ == "__main__":
    main()
