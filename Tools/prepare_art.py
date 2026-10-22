"""Extract the 16 real silhouettes, not an assumed uniform grid, then build Paper2D atlases.

Run with Python 3, Pillow, numpy and scipy. Originals are never modified.
"""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import statistics

import numpy as np
from PIL import Image
from scipy import ndimage

ROOT = Path(__file__).resolve().parents[1]
CELL = 256
BASELINE = 240
DIRECTIONS = ["south", "west", "east", "north"]


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def extract_frames(path: Path) -> list[Image.Image]:
    rgba = np.array(Image.open(path).convert("RGBA"))
    labels, _ = ndimage.label(rgba[:, :, 3] >= 32)
    sizes = np.bincount(labels.ravel())
    major = [int(i) for i in np.argsort(sizes[1:])[-16:] + 1]
    if len(major) != 16 or min(sizes[i] for i in major) < 1000:
        raise ValueError(f"Expected 16 complete sprite silhouettes: {path}")
    if sum(sizes[1:] >= 1000) != 16:
        raise ValueError(f"Ambiguous touching or additional silhouettes: {path}")
    objects = ndimage.find_objects(labels)
    # A generated sheet need not have exact cell boundaries. Group actual bodies into four rows.
    centers = {i: ((objects[i-1][1].start + objects[i-1][1].stop) / 2,
                   (objects[i-1][0].start + objects[i-1][0].stop) / 2) for i in major}
    vertical = sorted(major, key=lambda i: centers[i][1])
    order = []
    for row in range(4):
        order.extend(sorted(vertical[row*4:row*4+4], key=lambda i: centers[i][0]))
    # Assign disconnected sparkles/whiskers to the closest body. Ignore practically invisible
    # alpha noise, including bright RGB hidden under alpha 1–5 in the image generator's output.
    _, nearest = ndimage.distance_transform_edt(~np.isin(labels, major), return_indices=True)
    owner = labels[tuple(nearest)]
    result = []
    for identity in order:
        mask = (owner == identity) & (rgba[:, :, 3] >= 16)
        ys, xs = np.nonzero(mask)
        box = (int(xs.min()), int(ys.min()), int(xs.max())+1, int(ys.max())+1)
        pixels = rgba[box[1]:box[3], box[0]:box[2]].copy()
        local = mask[box[1]:box[3], box[0]:box[2]]
        pixels[~local] = 0
        result.append(Image.fromarray(pixels))
    return result


def prepare_clip(spec: dict) -> dict:
    original = ROOT / spec["source"]
    frames = extract_frames(original)
    # Most clips start with a standing pose. Seated poses intentionally remain shorter.
    standing = statistics.median(frames[r*4].height for r in range(4))
    factor = spec["height"] / standing
    factor = min(factor, 224 / max(f.width for f in frames), 224 / max(f.height for f in frames))
    atlas = Image.new("RGBA", (CELL*4, CELL*4), (0, 0, 0, 0))
    for index, source in enumerate(frames):
        size = (max(1, round(source.width * factor)), max(1, round(source.height * factor)))
        frame = source.resize(size, Image.Resampling.NEAREST)
        x = (CELL - frame.width) // 2
        y = BASELINE - frame.height
        if x < 8 or y < 8:
            raise ValueError(f"Clipping risk: {original}, frame {index}")
        atlas.paste(frame, ((index % 4)*CELL + x, (index // 4)*CELL + y))
    relative = f"ArtSource/Prepared/{spec['actor']}/{spec['action']}.png"
    output = ROOT / relative
    output.parent.mkdir(parents=True, exist_ok=True)
    atlas.save(output, optimize=True)
    return {
        "actor": spec["actor"], "action": spec["action"], "atlas": relative,
        "destination": f"/Game/GeneratedArt/{spec['actor']}/{spec['action']}",
        "texture_name": f"T_{spec['actor']}_{spec['action']}",
        "fps": spec["fps"], "loop": spec["loop"], "cell": CELL,
        "pivot_x": CELL // 2, "pivot_y": BASELINE, "pixels_per_unit": 2.0,
        "source": spec["source"], "source_sha256": sha(original), "atlas_sha256": sha(output),
    }


def main() -> None:
    specs = json.loads((ROOT / "ArtSource/catalog.json").read_text())
    manifest = {"schema": 1, "directions": DIRECTIONS,
                "clips": [prepare_clip(s) for s in specs["clips"]], "concepts": []}
    for concept in specs["concepts"]:
        manifest["concepts"].append({**concept, "source_sha256": sha(ROOT/concept["source"])})
    target = ROOT / "ArtSource/manifest.json"
    target.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"Prepared {len(manifest['clips'])} atlases / {16*len(manifest['clips'])} frames and {len(manifest['concepts'])} design boards")


if __name__ == "__main__":
    main()
