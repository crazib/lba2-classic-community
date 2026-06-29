#!/usr/bin/env python3

import os
import struct
import sys
import tempfile
import unittest


ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "scripts", "dev"))

import data_patch
import hqr_inspect
import scene_zones
import script_compile


def scene_blob():
    actor = {
        "flags": 0,
        "model": 7,
        "body": 2,
        "animation": 0,
        "sprite": -1,
        "x": 100,
        "y": 200,
        "z": 300,
        "hit_force": 0,
        "option_flags": 0,
        "beta": 512,
        "srot": 1280,
        "move": 0,
        "info": [0, 0, 0, 0],
        "bonus": 0,
        "color": 3,
        "anim_3ds_num": None,
        "anim_3ds_fps": None,
        "armour": 1,
        "life": 50,
        "track": b"\0",
        "life_script": b"\0",
    }
    raw = bytearray()
    raw.extend(struct.pack("<7b", 1, 2, 3, 0, 0, 1, 0))
    raw.extend(struct.pack("<24h", *([0] * 24)))
    raw.extend(struct.pack("<b", 0))
    raw.extend(struct.pack("<3h", 0, 0, 0))
    raw.extend(struct.pack("<h", 1))
    raw.extend(b"\0")
    raw.extend(struct.pack("<h", 1))
    raw.extend(b"\0")
    raw.extend(struct.pack("<h", 2))
    raw.extend(scene_zones.encode_actor(actor))
    raw.extend(struct.pack("<Ih", 0, 0))
    raw.extend(struct.pack("<hI", 0, 0))
    return bytes(raw)


class InjectLifeScriptTest(unittest.TestCase):
    def test_injects_scene_actor_zero_life_script_idempotently(self):
        with tempfile.TemporaryDirectory() as tmp:
            scene_path = os.path.join(tmp, "SCENE.HQR")
            source_path = os.path.join(tmp, "twinsen_life.ls2s")
            data_patch.write_hqr_blobs(
                scene_path,
                [
                    data_patch.stored_hqr_blob(b"max scene"),
                    data_patch.stored_hqr_blob(b"other scene"),
                    data_patch.stored_hqr_blob(scene_blob()),
                ],
            )
            with open(source_path, "w") as f:
                f.write("MESSAGE 123\nEND\n")

            op = {
                "file": "SCENE.HQR",
                "entry": 2,
                "actor": 0,
                "source": "twinsen_life.ls2s",
                "_manifest_dir": tmp,
            }
            self.assertTrue(data_patch.op_inject_life_script(op, tmp, True))
            self.assertFalse(data_patch.op_inject_life_script(op, tmp, True))

            ents, archive = hqr_inspect.entries(scene_path)
            raw = hqr_inspect.decompress_entry(
                archive, ents[2][1], ents[2][2], ents[2][3], ents[2][4]
            )
            layout = script_compile.scene_script_layout(raw, 1)
            _tracks, lifes = data_patch.scene_script_bytes(raw, layout)
            self.assertEqual(lifes[0], bytes([25, 123, 0, 0]))
            self.assertEqual(lifes[1], b"\0")

    def test_rejects_actor_out_of_range(self):
        with tempfile.TemporaryDirectory() as tmp:
            scene_path = os.path.join(tmp, "SCENE.HQR")
            source_path = os.path.join(tmp, "life.ls2s")
            data_patch.write_hqr_blobs(
                scene_path,
                [
                    data_patch.stored_hqr_blob(b"max scene"),
                    data_patch.stored_hqr_blob(scene_blob()),
                ],
            )
            with open(source_path, "w") as f:
                f.write("END\n")

            with self.assertRaises(ValueError):
                data_patch.op_inject_life_script(
                    {
                        "file": "SCENE.HQR",
                        "entry": 1,
                        "actor": 2,
                        "source": "life.ls2s",
                        "_manifest_dir": tmp,
                    },
                    tmp,
                    True,
                )


if __name__ == "__main__":
    unittest.main()
