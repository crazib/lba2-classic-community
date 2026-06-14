#!/usr/bin/env python3

import json
import os
import struct
import sys
import tempfile
import unittest


SOURCE_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
sys.path.insert(0, os.path.join(SOURCE_ROOT, "scripts", "dev"))

import data_patch
import ile_objects
import ile_terrain


class BlankIslandTest(unittest.TestCase):
    def test_builds_blank_water_cube_and_is_idempotent(self):
        with tempfile.TemporaryDirectory() as tmp:
            source_path = os.path.join(tmp, "SOURCE.ILE")
            target_path = os.path.join(tmp, "TARGET.ILE")
            layout_path = os.path.join(tmp, "layout.json")
            cube_map = bytearray(ile_objects.SIZE_MAIN_MAP * ile_objects.SIZE_MAIN_MAP)
            cube_map[8 * ile_objects.SIZE_MAIN_MAP + 8] = 2
            cube_map[8 * ile_objects.SIZE_MAIN_MAP + 9] = 1
            info = struct.pack("<I9i", 0x12340135, 0, 7, 0, 8000, 48000, 0, 0, -1, -1)
            texdef_pair_a = bytes(range(24))
            texdef_pair_b = bytes(range(24, 48))

            data_patch.write_hqr_blobs(
                source_path,
                [
                    data_patch.stored_hqr_blob(bytes(cube_map)),
                    data_patch.stored_hqr_blob(b"ground texture"),
                    data_patch.stored_hqr_blob(b"object texture"),
                    data_patch.stored_hqr_blob(info),
                    data_patch.stored_hqr_blob(b"source decor"),
                    data_patch.stored_hqr_blob(b"source terrain"),
                    data_patch.stored_hqr_blob(texdef_pair_a),
                    data_patch.stored_hqr_blob(b"source heights"),
                    data_patch.stored_hqr_blob(bytes([127]) * ile_terrain.HEIGHT_COUNT),
                    data_patch.stored_hqr_blob(info),
                    None,
                    data_patch.stored_hqr_blob(b"other source terrain"),
                    data_patch.stored_hqr_blob(texdef_pair_b + texdef_pair_a),
                    data_patch.stored_hqr_blob(b"other source heights"),
                    data_patch.stored_hqr_blob(bytes([63]) * ile_terrain.HEIGHT_COUNT),
                ],
            )
            with open(layout_path, "w") as f:
                json.dump({"island": 12, "cubes": [[8, 8], [9, 8]]}, f)

            op = {
                "from": "SOURCE.ILE",
                "to": "TARGET.ILE",
                "layout": "layout.json",
                "template_cube": [9, 8],
                "_manifest_dir": tmp,
            }
            self.assertTrue(data_patch.op_create_blank_island(op, tmp, True))
            self.assertFalse(data_patch.op_create_blank_island(op, tmp, True))

            self.assertEqual(ile_objects.load_cube_map(target_path), {1: [(8, 8)], 2: [(9, 8)]})
            self.assertEqual(ile_objects.iter_objects(target_path), [])
            self.assertEqual(set(ile_terrain.load_heights(target_path, 8, 8)[3]), {0})
            self.assertEqual(set(ile_terrain.load_polys(target_path, 8, 8)[3]), {1 << 12})
            self.assertEqual(set(ile_terrain.load_heights(target_path, 9, 8)[3]), {0})
            self.assertEqual(set(ile_terrain.load_polys(target_path, 9, 8)[3]), {1 << 12})
            for slot in (1, 2):
                cube_info = ile_objects.entry_bytes(
                    target_path,
                    ile_objects.HQR_START_CUBE + ile_objects.HQR_STEP_CUBE * (slot - 1),
                )
                alpha_light = struct.unpack_from("<I", cube_info, 0)[0]
                self.assertEqual(alpha_light >> 16, 0xFFFF)
                self.assertEqual(alpha_light & 0xFFFF, 0x0135)
            self.assertEqual(
                ile_objects.entry_bytes(target_path, ile_objects.HQR_START_CUBE + 3),
                texdef_pair_b + texdef_pair_a,
            )
            self.assertEqual(
                ile_objects.entry_bytes(target_path, ile_objects.HQR_START_CUBE + ile_objects.HQR_STEP_CUBE + 3),
                texdef_pair_b + texdef_pair_a,
            )


if __name__ == "__main__":
    unittest.main()
