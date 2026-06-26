#!/usr/bin/env python3

import os
import struct
import sys
import tempfile
import unittest


SOURCE_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
sys.path.insert(0, os.path.join(SOURCE_ROOT, "scripts", "dev"))

import data_patch
import hqr_inspect


class HolomapArrowTest(unittest.TestCase):
    def test_replaces_arrow_record_and_is_idempotent(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "HOLOMAP.HQR")
            empty_arrow = struct.pack(
                data_patch.HOLOMAP_ARROW_FORMAT,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0xFF,
                0,
                0xFF,
                0xFF,
            )
            arrows = empty_arrow * 15
            data_patch.write_hqr_blobs(
                path,
                [data_patch.stored_hqr_blob(b"untouched")]
                + [None] * 11
                + [data_patch.stored_hqr_blob(arrows)],
            )
            op = {
                "file": "HOLOMAP.HQR",
                "id": 12,
                "index": 12,
                "alpha": 741,
                "beta": 3782,
                "altitude": 1047,
                "message": 620,
                "planet": 0,
                "island": 12,
            }

            self.assertTrue(data_patch.op_replace_holomap_arrow(op, tmp, True))
            self.assertFalse(data_patch.op_replace_holomap_arrow(op, tmp, True))

            ents, hqr_data = hqr_inspect.entries(path)
            raw = hqr_inspect.decompress_entry(hqr_data, *ents[12][1:])
            self.assertEqual(raw[: 12 * data_patch.HOLOMAP_ARROW_SIZE], arrows[: 12 * data_patch.HOLOMAP_ARROW_SIZE])
            self.assertEqual(
                struct.unpack_from(
                    data_patch.HOLOMAP_ARROW_FORMAT,
                    raw,
                    12 * data_patch.HOLOMAP_ARROW_SIZE,
                ),
                (0, 0, 0, 741, 3782, 1047, 620, 0xFF, 0, 0, 12),
            )
            self.assertEqual(raw[13 * data_patch.HOLOMAP_ARROW_SIZE :], arrows[13 * data_patch.HOLOMAP_ARROW_SIZE :])
            self.assertEqual(
                hqr_inspect.decompress_entry(hqr_data, *ents[0][1:]),
                b"untouched",
            )


if __name__ == "__main__":
    unittest.main()
