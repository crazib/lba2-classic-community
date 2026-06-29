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


class AddGrfEntryTest(unittest.TestCase):
    def test_inserts_before_bll_span_and_is_idempotent(self):
        with tempfile.TemporaryDirectory() as tmp:
            bkg_path = os.path.join(tmp, "LBA_BKG.HQR")
            grf_path = os.path.join(tmp, "new_fragment.grf")
            header = {
                "Gri_Start": 1,
                "Grm_Start": 6,
                "Bll_Start": 7,
                "Brk_Start": 9,
                "Max_Brk": 1,
                "ForbidenBrick": 42,
                "Max_Size_Gri": 1000,
                "Max_Size_Bll": 2000,
                "Max_Size_Brick_Cube": 3000,
                "Max_Size_Mask_Brick_Cube": 4000,
            }
            data_patch.write_hqr_blobs(
                bkg_path,
                [
                    data_patch.stored_hqr_blob(data_patch.encode_bkg_header(header)),
                    data_patch.stored_hqr_blob(b"gri0"),
                    data_patch.stored_hqr_blob(b"gri1"),
                    data_patch.stored_hqr_blob(b"gri2"),
                    data_patch.stored_hqr_blob(b"grm0"),
                    data_patch.stored_hqr_blob(b"brk0"),
                    data_patch.stored_hqr_blob(b"grf0"),
                    data_patch.stored_hqr_blob(b"bll0"),
                    data_patch.stored_hqr_blob(b"bll1"),
                    data_patch.stored_hqr_blob(b"brk0"),
                ],
            )
            with open(grf_path, "wb") as f:
                f.write(b"new grf payload")

            op = {
                "file": "LBA_BKG.HQR",
                "fragment": 1,
                "source": "new_fragment.grf",
                "_manifest_dir": tmp,
            }

            self.assertTrue(data_patch.op_add_grf_entry(op, tmp, True))
            self.assertFalse(data_patch.op_add_grf_entry(op, tmp, True))

            ents, raw = hqr_inspect.entries(bkg_path)
            decoded = data_patch.decode_bkg_header(
                hqr_inspect.decompress_entry(raw, ents[0][1], ents[0][2], ents[0][3], ents[0][4])
            )
            self.assertEqual(decoded["Gri_Start"], 1)
            self.assertEqual(decoded["Grm_Start"], 6)
            self.assertEqual(decoded["Brk_Start"], 10)
            self.assertEqual(decoded["Max_Brk"], 1)
            self.assertEqual(decoded["Bll_Start"], 8)
            self.assertEqual(decoded["ForbidenBrick"], 42)

            payloads = [
                hqr_inspect.decompress_entry(raw, ent[1], ent[2], ent[3], ent[4])
                for ent in ents
            ]
            self.assertEqual(payloads[6], b"grf0")
            self.assertEqual(payloads[7], b"new grf payload")
            self.assertEqual(payloads[8], b"bll0")
            self.assertEqual(payloads[9], b"bll1")
            self.assertEqual(payloads[10], b"brk0")

    def test_rejects_insert_outside_grf_span(self):
        with tempfile.TemporaryDirectory() as tmp:
            bkg_path = os.path.join(tmp, "LBA_BKG.HQR")
            grf_path = os.path.join(tmp, "new_fragment.grf")
            header = struct.pack("<6H4I", 1, 6, 7, 9, 1, 0, 0, 0, 0, 0)
            data_patch.write_hqr_blobs(
                bkg_path,
                [
                    data_patch.stored_hqr_blob(header),
                    data_patch.stored_hqr_blob(b"gri0"),
                    data_patch.stored_hqr_blob(b"gri1"),
                    data_patch.stored_hqr_blob(b"gri2"),
                    data_patch.stored_hqr_blob(b"grm0"),
                    data_patch.stored_hqr_blob(b"brk0"),
                    data_patch.stored_hqr_blob(b"grf0"),
                    data_patch.stored_hqr_blob(b"bll0"),
                ],
            )
            with open(grf_path, "wb") as f:
                f.write(b"new grf payload")

            with self.assertRaises(ValueError):
                data_patch.op_add_grf_entry(
                    {
                        "file": "LBA_BKG.HQR",
                        "entry": 5,
                        "source": "new_fragment.grf",
                        "_manifest_dir": tmp,
                    },
                    tmp,
                    True,
                )

    def test_rejects_fragment_after_current_grf_span(self):
        with tempfile.TemporaryDirectory() as tmp:
            bkg_path = os.path.join(tmp, "LBA_BKG.HQR")
            grf_path = os.path.join(tmp, "new_fragment.grf")
            header = struct.pack("<6H4I", 1, 6, 7, 9, 1, 0, 0, 0, 0, 0)
            data_patch.write_hqr_blobs(
                bkg_path,
                [
                    data_patch.stored_hqr_blob(header),
                    data_patch.stored_hqr_blob(b"gri0"),
                    data_patch.stored_hqr_blob(b"gri1"),
                    data_patch.stored_hqr_blob(b"gri2"),
                    data_patch.stored_hqr_blob(b"grm0"),
                    data_patch.stored_hqr_blob(b"brk0"),
                    data_patch.stored_hqr_blob(b"grf0"),
                    data_patch.stored_hqr_blob(b"bll0"),
                ],
            )
            with open(grf_path, "wb") as f:
                f.write(b"new grf payload")

            with self.assertRaises(ValueError):
                data_patch.op_add_grf_entry(
                    {
                        "file": "LBA_BKG.HQR",
                        "fragment": 2,
                        "source": "new_fragment.grf",
                        "_manifest_dir": tmp,
                    },
                    tmp,
                    True,
                )

    def test_replaces_existing_fragment_when_requested(self):
        with tempfile.TemporaryDirectory() as tmp:
            bkg_path = os.path.join(tmp, "LBA_BKG.HQR")
            grf_path = os.path.join(tmp, "new_fragment.grf")
            header = {
                "Gri_Start": 1,
                "Grm_Start": 6,
                "Bll_Start": 8,
                "Brk_Start": 10,
                "Max_Brk": 1,
                "ForbidenBrick": 42,
                "Max_Size_Gri": 1000,
                "Max_Size_Bll": 2000,
                "Max_Size_Brick_Cube": 3000,
                "Max_Size_Mask_Brick_Cube": 4000,
            }
            data_patch.write_hqr_blobs(
                bkg_path,
                [
                    data_patch.stored_hqr_blob(data_patch.encode_bkg_header(header)),
                    data_patch.stored_hqr_blob(b"gri0"),
                    data_patch.stored_hqr_blob(b"gri1"),
                    data_patch.stored_hqr_blob(b"gri2"),
                    data_patch.stored_hqr_blob(b"grm0"),
                    data_patch.stored_hqr_blob(b"brk0"),
                    data_patch.stored_hqr_blob(b"grf0"),
                    data_patch.stored_hqr_blob(b"old grf payload"),
                    data_patch.stored_hqr_blob(b"bll0"),
                    data_patch.stored_hqr_blob(b"bll1"),
                    data_patch.stored_hqr_blob(b"brk0"),
                ],
            )
            with open(grf_path, "wb") as f:
                f.write(b"new grf payload")

            self.assertTrue(
                data_patch.op_add_grf_entry(
                    {
                        "file": "LBA_BKG.HQR",
                        "fragment": 1,
                        "source": "new_fragment.grf",
                        "replace": True,
                        "_manifest_dir": tmp,
                    },
                    tmp,
                    True,
                )
            )
            self.assertFalse(
                data_patch.op_add_grf_entry(
                    {
                        "file": "LBA_BKG.HQR",
                        "fragment": 1,
                        "source": "new_fragment.grf",
                        "replace": True,
                        "_manifest_dir": tmp,
                    },
                    tmp,
                    True,
                )
            )

            ents, raw = hqr_inspect.entries(bkg_path)
            decoded = data_patch.decode_bkg_header(
                hqr_inspect.decompress_entry(raw, ents[0][1], ents[0][2], ents[0][3], ents[0][4])
            )
            self.assertEqual(decoded["Bll_Start"], 8)
            self.assertEqual(decoded["Brk_Start"], 10)
            self.assertEqual(len(ents), 11)

            payloads = [
                hqr_inspect.decompress_entry(raw, ent[1], ent[2], ent[3], ent[4])
                for ent in ents
            ]
            self.assertEqual(payloads[6], b"grf0")
            self.assertEqual(payloads[7], b"new grf payload")
            self.assertEqual(payloads[8], b"bll0")


if __name__ == "__main__":
    unittest.main()
