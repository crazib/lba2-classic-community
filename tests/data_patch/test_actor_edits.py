#!/usr/bin/env python3

import json
import os
import struct
import sys
import tempfile
import unittest


ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "scripts", "dev"))

import data_patch
import scene_zones


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


class ActorEditsTest(unittest.TestCase):
    def test_updates_and_appends_scene_actors_idempotently(self):
        with tempfile.TemporaryDirectory() as tmp:
            scene_path = os.path.join(tmp, "SCENE.HQR")
            edits_path = os.path.join(tmp, "actor_edits.json")
            data_patch.write_hqr_blobs(
                scene_path,
                [
                    data_patch.stored_hqr_blob(b"max scene"),
                    data_patch.stored_hqr_blob(scene_blob()),
                ],
            )
            edits = {
                "island": 1,
                "scenes": [
                    {
                        "scene": 0,
                        "actors": [
                            {
                                "actor": 2,
                                "added": True,
                                "model": 99,
                                "body": 1,
                                "animation": 0,
                                "x": 1,
                                "y": 2,
                                "z": 3,
                                "beta": 0,
                                "life": 255,
                                "armour": 0,
                                "move": 0,
                                "flags": 0,
                            },
                            {
                                "actor": 1,
                                "expect_body": 2,
                                "body": 4,
                                "expect_x": 100,
                                "x": 150,
                            },
                            {
                                "actor": 2,
                                "added": True,
                                "model": 7,
                                "body": 4,
                                "animation": 0,
                                "x": 400,
                                "y": 500,
                                "z": 600,
                                "beta": 1024,
                                "life": 50,
                                "armour": 0,
                                "move": 0,
                                "flags": 0,
                            },
                        ],
                    }
                ],
            }
            with open(edits_path, "w") as f:
                json.dump(edits, f)

            op = {
                "file": "SCENE.HQR",
                "source": "actor_edits.json",
                "_manifest_dir": tmp,
            }
            self.assertTrue(data_patch.op_apply_actor_edits(op, tmp, True))
            self.assertFalse(data_patch.op_apply_actor_edits(op, tmp, True))

            _ents, _data, _raw, scene = scene_zones.load_scene(scene_path, 0)
            self.assertEqual(scene["objects"], 3)
            self.assertEqual(scene["actors"][0]["body"], 4)
            self.assertEqual(scene["actors"][0]["x"], 150)
            self.assertEqual(scene["actors"][0]["track"], b"\0")
            self.assertEqual(scene["actors"][1]["x"], 400)
            self.assertEqual(scene["actors"][1]["model"], 7)
            self.assertEqual(scene["actors"][1]["life_script"], b"\0")

    def test_deletes_actor_without_shifting_indexes(self):
        with tempfile.TemporaryDirectory() as tmp:
            scene_path = os.path.join(tmp, "SCENE.HQR")
            edits_path = os.path.join(tmp, "actor_edits.json")
            data_patch.write_hqr_blobs(
                scene_path,
                [
                    data_patch.stored_hqr_blob(b"max scene"),
                    data_patch.stored_hqr_blob(scene_blob()),
                ],
            )
            with open(edits_path, "w") as f:
                json.dump(
                    {
                        "island": 1,
                        "scenes": [
                            {
                                "scene": 0,
                                "actors": [{"actor": 1, "deleted": True}],
                            }
                        ],
                    },
                    f,
                )

            op = {
                "file": "SCENE.HQR",
                "source": "actor_edits.json",
                "_manifest_dir": tmp,
            }
            self.assertTrue(data_patch.op_apply_actor_edits(op, tmp, True))
            self.assertFalse(data_patch.op_apply_actor_edits(op, tmp, True))

            _ents, _data, _raw, scene = scene_zones.load_scene(scene_path, 0)
            self.assertEqual(scene["objects"], 2)
            actor = scene["actors"][0]
            self.assertEqual(actor["body"], 255)
            self.assertEqual(actor["life"], 0)
            self.assertEqual(actor["move"], 0)
            self.assertTrue(actor["flags"] & data_patch.ACTOR_INVISIBLE)
            self.assertEqual(actor["track"], b"\0")
            self.assertEqual(actor["life_script"], b"\0")

    def test_appends_zone_and_waypoint_idempotently(self):
        with tempfile.TemporaryDirectory() as tmp:
            scene_path = os.path.join(tmp, "SCENE.HQR")
            zone_path = os.path.join(tmp, "zone_edits.json")
            waypoint_path = os.path.join(tmp, "waypoint_edits.json")
            data_patch.write_hqr_blobs(
                scene_path,
                [
                    data_patch.stored_hqr_blob(b"max scene"),
                    data_patch.stored_hqr_blob(scene_blob()),
                ],
            )
            with open(zone_path, "w") as f:
                json.dump(
                    {
                        "scenes": [
                            {
                                "scene": 0,
                                "zones": [
                                    {
                                        "zone": 0,
                                        "add": True,
                                        "type": 2,
                                        "num": 0,
                                        "bounds": [10, 20, 30, 40, 50, 60],
                                        "info": [0] * 8,
                                    }
                                ],
                            }
                        ]
                    },
                    f,
                )
            zone_op = {
                "file": "SCENE.HQR",
                "source": "zone_edits.json",
                "_manifest_dir": tmp,
            }
            self.assertTrue(data_patch.op_apply_zone_edits(zone_op, tmp, True))
            self.assertFalse(data_patch.op_apply_zone_edits(zone_op, tmp, True))

            with open(waypoint_path, "w") as f:
                json.dump(
                    {
                        "scenes": [
                            {
                                "scene": 0,
                                "waypoints": [
                                    {
                                        "waypoint": 0,
                                        "added": True,
                                        "position": [1000, 2000, 3000],
                                    }
                                ],
                            }
                        ]
                    },
                    f,
                )
            waypoint_op = {
                "file": "SCENE.HQR",
                "source": "waypoint_edits.json",
                "_manifest_dir": tmp,
            }
            self.assertTrue(data_patch.op_apply_waypoint_edits(waypoint_op, tmp, True))
            self.assertFalse(data_patch.op_apply_waypoint_edits(waypoint_op, tmp, True))

            _ents, _data, _raw, scene = scene_zones.load_scene(scene_path, 0)
            self.assertEqual(len(scene["zones"]), 1)
            self.assertEqual(scene["zones"][0]["type"], 2)
            self.assertEqual(len(scene["tracks"]), 1)
            self.assertEqual(
                [scene["tracks"][0]["x"], scene["tracks"][0]["y"], scene["tracks"][0]["z"]],
                [1000, 2000, 3000],
            )


if __name__ == "__main__":
    unittest.main()
