#!/usr/bin/env python3
"""
compose_views.py - put the MAME screenshot and the Blender renders of each view side by side, one image per view.

  python compose_views.py <views.json> <render folder> <out folder> <suffix> [<suffix> ...]
Each view gets <out folder>\\<name>_compare.png: screenshot, then <name>_<suffix>.png for every suffix, with labels.
Renders are mirrored left-right: the game's view space is left-handed (camera right x up = forward, see docs/formats.md),
so a Blender camera shows the scene mirrored.
"""
import sys, os, json
from PIL import Image, ImageDraw, ImageOps

W, H = 496, 384
LABELS = {"pc": "PC (blue)", "arcade": "arcade (orange)", "textured": "arcade, textured + MAME colors"}


def main(views_file, renders, out, suffixes):
    os.makedirs(out, exist_ok=True)
    for view in json.load(open(views_file)):
        tiles = [("MAME screenshot", Image.open(view["screenshot"]).convert("RGB").resize((W, H)))]
        for s in suffixes:
            path = os.path.join(renders, f"{view['name']}_{s}.png")
            if os.path.exists(path):
                tiles.append((LABELS.get(s, s), ImageOps.mirror(Image.open(path).convert("RGB").resize((W, H)))))
        img = Image.new("RGB", (W * len(tiles), H + 20), (20, 20, 20))
        d = ImageDraw.Draw(img)
        for i, (label, tile) in enumerate(tiles):
            img.paste(tile, (W * i, 20))
            d.text((W * i + 6, 4), f"{view['name']}: {label}", fill=(255, 255, 255))
        img.save(os.path.join(out, f"{view['name']}_compare.png"))
    print("composed", len(json.load(open(views_file))), "views")


if __name__ == "__main__":
    if len(sys.argv) < 5:
        print(__doc__); sys.exit(1)
    main(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4:])
