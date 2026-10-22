"""Check that committed sources and the engine's declared art inputs agree."""
import hashlib
import json
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]


def main():
    manifest = json.loads((ROOT / "ArtSource/manifest.json").read_text())
    assert manifest["directions"] == ["south", "west", "east", "north"]
    identities = set()
    for clip in manifest["clips"]:
        identity = (clip["actor"], clip["action"])
        assert identity not in identities, identity
        identities.add(identity)
        for key, digest in [("source", "source_sha256"), ("atlas", "atlas_sha256")]:
            path = ROOT / clip[key]
            assert hashlib.sha256(path.read_bytes()).hexdigest() == clip[digest], str(path)
        im = Image.open(ROOT / clip["atlas"])
        assert im.mode == "RGBA" and im.size == (1024, 1024)
        for row in range(4):
            for col in range(4):
                a = im.crop((col*256, row*256, (col+1)*256, (row+1)*256)).getchannel("A")
                box = a.getbbox()
                assert box and box[0] >= 8 and box[1] >= 8 and box[2] <= 248 and box[3] <= 240, (identity, row, col, box)
                assert a.getextrema()[0] == 0
        expected = f"/Game/GeneratedArt/{clip['actor']}/{clip['action']}"
        assert clip["destination"] == expected
        assert clip["texture_name"] == f"T_{clip['actor']}_{clip['action']}"
        assert clip["pivot_x"] == 128 and clip["pivot_y"] == 240
    assert identities == {("hero", a) for a in ["idle", "walk", "combat", "spell", "sit", "hurt", "dying"]} | {
        (a, "idle") for a in ["mousse", "roncinet", "luciolle"]}
    assert {c["id"] for c in manifest["concepts"]} == {"hero", "brother", "mousse", "roncinet", "luciolle"}
    for concept in manifest["concepts"]:
        path = ROOT / concept["source"]
        assert hashlib.sha256(path.read_bytes()).hexdigest() == concept["source_sha256"]
        Image.open(path).verify()
        assert concept["texture_name"] == "T_" + concept["id"]
    config = (ROOT / "Config/DefaultGame.ini").read_text()
    assert '+DirectoriesToAlwaysCook=(Path="/Game/GeneratedArt")' in config
    modules = json.loads((ROOT / "FinalTower.uproject").read_text())["Modules"]
    assert any(m["Name"] == "FinalTowerEditor" and m["Type"] == "Editor" for m in modules)
    print("Art: 15 source images, 10 padded atlases, 160 complete frames and engine paths verified")


if __name__ == "__main__":
    main()
