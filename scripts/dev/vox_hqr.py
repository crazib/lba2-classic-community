#!/usr/bin/env python3
"""Inspect and patch LBA2 VOX voice banks.

VOX files use the same per-entry header shape as HQR entries, but their offset
table has a final sentinel offset. The engine maps dialogue text ids through
TEXT.HQR order rows first, then uses that row index as the VOX slot.
"""

import struct

import hqr_inspect


LANGUAGE_PREFIXES = ["EN_", "FR_", "DE_", "SP_", "IT_", "PO_"]


def default_vox_file(language, file_index, text_hqr):
    return "VOX/%s%s.VOX" % (
        LANGUAGE_PREFIXES[language],
        text_hqr.FILE_NAMES[file_index],
    )


def load_bank(path):
    with open(path, "rb") as f:
        data = f.read()
    if len(data) < 4:
        raise ValueError("%s is too small to be a VOX file" % path)

    (first,) = struct.unpack_from("<I", data, 0)
    if first < 4 or first % 4 != 0 or first > len(data):
        raise ValueError("%s has invalid VOX offset table size %d" % (path, first))

    table_count = first // 4
    offsets = list(struct.unpack_from("<%dI" % table_count, data, 0))
    if table_count < 1:
        raise ValueError("%s has no VOX sentinel" % path)

    slots = []
    for index, offset in enumerate(offsets[:-1]):
        if offset == 0:
            slots.append(None)
            continue
        if offset + 10 > len(data):
            raise ValueError("VOX slot %d has invalid offset %d" % (index, offset))
        size, csize, method = struct.unpack_from("<IIh", data, offset)
        if offset + 10 + csize > len(data):
            raise ValueError("VOX slot %d extends past end of file" % index)
        slots.append(
            {
                "index": index,
                "offset": offset,
                "size": size,
                "compressed_size": csize,
                "method": method,
                "blob": data[offset : offset + 10 + csize],
            }
        )

    return {
        "data": data,
        "offsets": offsets,
        "slots": slots,
    }


def slot_payload(bank, index):
    if index < 0 or index >= len(bank["slots"]):
        return None
    slot = bank["slots"][index]
    if slot is None:
        return None
    return hqr_inspect.decompress_entry(
        bank["data"],
        slot["offset"],
        slot["size"],
        slot["compressed_size"],
        slot["method"],
    )


def stored_blob(payload):
    return struct.pack("<IIh", len(payload), len(payload), 0) + payload


def rewrite_slot_stored(path, bank, index, payload):
    slots = list(bank["slots"])
    while index >= len(slots):
        slots.append(None)
    slots[index] = {
        "index": index,
        "offset": 0,
        "size": len(payload),
        "compressed_size": len(payload),
        "method": 0,
        "blob": stored_blob(payload),
    }

    table_size = (len(slots) + 1) * 4
    offsets = []
    pos = table_size
    blobs = []

    for slot in slots:
        if slot is None:
            offsets.append(0)
            blobs.append(None)
        else:
            blob = slot["blob"]
            offsets.append(pos)
            blobs.append(blob)
            pos += len(blob)
    offsets.append(pos)

    out = bytearray()
    out.extend(struct.pack("<%dI" % len(offsets), *offsets))
    for blob in blobs:
        if blob is not None:
            out.extend(blob)

    with open(path, "wb") as f:
        f.write(out)

    return len(bank["data"]), len(out)


def wav_voice_payload(raw, next_voice):
    if len(raw) < 12:
        raise ValueError("voice WAV source is too small")
    if raw[:4] != b"RIFF" or raw[8:12] != b"WAVE":
        raise ValueError("voice source must be a RIFF WAVE file")
    out = bytearray(raw)
    out[0] = 1 if next_voice else 0
    return bytes(out)
