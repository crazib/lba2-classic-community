#!/usr/bin/env python3
"""Inspect and patch LBA2 TEXT.HQR text banks.

TEXT.HQR stores each language/file pair as two HQR entries:
  - order entry: U16 text ids, searched by MESSAGE.CPP FindText()
  - text entry: U16 offsets followed by attributed NUL-terminated strings

The entry formula comes from MESSAGE.CPP InitDial():
  (Language * MAX_TEXT_LANG * 2) + (file * 2) + {0,1}
"""

import argparse
import json
import signal
import struct
import sys

import hqr_inspect


MAX_TEXT_LANG = 15

LANGUAGES = {
    "en": 0,
    "english": 0,
    "fr": 1,
    "french": 1,
    "de": 2,
    "german": 2,
    "sp": 3,
    "spanish": 3,
    "it": 4,
    "italian": 4,
    "po": 5,
    "portuguese": 5,
}

LANGUAGE_NAMES = ["EN", "FR", "DE", "SP", "IT", "PO"]
FILE_NAMES = ["sys", "cre", "gam", "000", "001", "002", "003", "004", "005", "006", "007", "008", "009", "010", "011"]

DIAL_FLAGS = [
    (1 << 0, "DEF"),
    (1 << 1, "BIG"),
    (1 << 2, "FUL"),
    (1 << 3, "SAY"),
    (1 << 4, "INT/HOL"),
    (1 << 5, "EXT/RAD"),
    (1 << 6, "CAV/EXP"),
    (1 << 7, "DEM"),
]


def parse_language(text):
    key = text.lower()
    if key not in LANGUAGES:
        raise ValueError("--language must be one of: en, fr, de, sp, it, po")
    return LANGUAGES[key]


def parse_file(text):
    lowered = text.lower()
    if lowered in FILE_NAMES:
        return FILE_NAMES.index(lowered)
    try:
        value = int(text, 0)
    except ValueError:
        raise ValueError("--file must be sys, cre, gam, 000..011, or an index")
    if value < 0 or value >= MAX_TEXT_LANG:
        raise ValueError("--file index must be in [0,%d)" % MAX_TEXT_LANG)
    return value


def entry_indexes(language, file_index):
    base = language * MAX_TEXT_LANG * 2 + file_index * 2
    return base, base + 1


def rewrite_entry_stored(path, replacements):
    ents, data = hqr_inspect.entries(path)
    new_entries = []

    for index, ent in enumerate(ents):
        _i, off, size, csize, _method = ent
        if size is None:
            new_entries.append(None)
        elif index in replacements:
            payload = replacements[index]
            new_entries.append(struct.pack("<IIh", len(payload), len(payload), 0) + payload)
        else:
            new_entries.append(data[off : off + 10 + csize])

    table_size = len(ents) * 4
    offsets = []
    pos = table_size
    for blob in new_entries:
        if blob is None:
            offsets.append(0)
        else:
            offsets.append(pos)
            pos += len(blob)

    out = bytearray()
    for off in offsets:
        out.extend(struct.pack("<I", off))
    for blob in new_entries:
        if blob is not None:
            out.extend(blob)

    with open(path, "wb") as f:
        f.write(out)

    return len(data), len(out)


def load_bank(path, language, file_index):
    order_entry, text_entry = entry_indexes(language, file_index)
    ents, data = hqr_inspect.entries(path)
    if text_entry >= len(ents):
        raise ValueError("TEXT.HQR does not have entry %d" % text_entry)

    order_info = ents[order_entry]
    text_info = ents[text_entry]
    if order_info[2] is None or text_info[2] is None:
        raise ValueError("missing text bank entries %d/%d" % (order_entry, text_entry))

    order_raw = hqr_inspect.decompress_entry(data, order_info[1], order_info[2], order_info[3], order_info[4])
    text_raw = hqr_inspect.decompress_entry(data, text_info[1], text_info[2], text_info[3], text_info[4])

    if len(order_raw) % 2 != 0:
        raise ValueError("order entry %d has odd size %d" % (order_entry, len(order_raw)))

    ids = list(struct.unpack("<%dH" % (len(order_raw) // 2), order_raw))
    if len(text_raw) < (len(ids) + 1) * 2:
        raise ValueError("text entry %d is too small for %d text offsets" % (text_entry, len(ids)))

    offsets = list(struct.unpack_from("<%dH" % (len(ids) + 1), text_raw, 0))
    texts = []
    for index, text_id in enumerate(ids):
        off0 = offsets[index]
        off1 = offsets[index + 1]
        if off0 < 0 or off1 < off0 or off1 > len(text_raw):
            raise ValueError("invalid offsets for text id %d: %d..%d" % (text_id, off0, off1))
        if off0 == off1:
            flag = 0
            raw_text = b""
        else:
            flag = text_raw[off0]
            raw_text = text_raw[off0 + 1 : off1]
            if raw_text.endswith(b"\x00"):
                raw_text = raw_text[:-1]
        texts.append(
            {
                "index": index,
                "id": text_id,
                "flag": flag,
                "text": raw_text.decode("latin-1"),
            }
        )

    return {
        "entries": ents,
        "data": data,
        "language": language,
        "file": file_index,
        "order_entry": order_entry,
        "text_entry": text_entry,
        "ids": ids,
        "offsets": offsets,
        "texts": texts,
    }


def flag_names(flag):
    names = [name for bit, name in DIAL_FLAGS if flag & bit]
    return "|".join(names) if names else "0"


def selected_rows(bank, text_id, contains, limit):
    rows = bank["texts"]
    if text_id is not None:
        rows = [row for row in rows if row["id"] == text_id]
    if contains:
        needle = contains.lower()
        rows = [row for row in rows if needle in row["text"].lower()]
    if limit:
        rows = rows[:limit]
    return rows


def print_rows(bank, rows):
    print(
        "TEXT.HQR %s_%s order entry %d text entry %d count %d"
        % (
            LANGUAGE_NAMES[bank["language"]],
            FILE_NAMES[bank["file"]],
            bank["order_entry"],
            bank["text_entry"],
            len(bank["texts"]),
        )
    )
    for row in rows:
        print(
            "%4d idx=%3d flag=%3d %-15s %s"
            % (row["id"], row["index"], row["flag"], flag_names(row["flag"]), repr(row["text"]))
        )


def rebuild_text_blob(bank, replacement_id, new_text, new_flag):
    ids = bank["ids"]
    old_rows = bank["texts"]
    rebuilt = []

    for row in old_rows:
        text = row["text"]
        flag = row["flag"]
        if row["id"] == replacement_id:
            text = new_text
            if new_flag is not None:
                flag = new_flag
        rebuilt.append((flag, text.encode("latin-1") + b"\x00"))

    offset_table_size = (len(ids) + 1) * 2
    pos = offset_table_size
    offsets = []
    payload = bytearray()

    for flag, encoded in rebuilt:
        if pos > 0xFFFF:
            raise ValueError("rebuilt text bank exceeds 16-bit offset range")
        offsets.append(pos)
        payload.append(flag & 0xFF)
        payload.extend(encoded)
        pos += 1 + len(encoded)

    if pos > 0xFFFF:
        raise ValueError("rebuilt text bank exceeds 16-bit offset range")
    offsets.append(pos)

    out = bytearray()
    out.extend(struct.pack("<%dH" % len(offsets), *offsets))
    out.extend(payload)
    return bytes(out)


def append_text(bank, new_id, new_text, new_flag):
    if new_id in bank["ids"]:
        raise ValueError("text id %d already exists" % new_id)

    ids = list(bank["ids"])
    ids.append(new_id)

    rows = list(bank["texts"])
    rows.append(
        {
            "id": new_id,
            "flag": new_flag,
            "text": new_text,
        }
    )

    offset_table_size = (len(ids) + 1) * 2
    pos = offset_table_size
    offsets = []
    payload = bytearray()

    for row in rows:
        encoded = row["text"].encode("latin-1") + b"\x00"
        if pos > 0xFFFF:
            raise ValueError("rebuilt text bank exceeds 16-bit offset range")
        offsets.append(pos)
        payload.append(row["flag"] & 0xFF)
        payload.extend(encoded)
        pos += 1 + len(encoded)

    if pos > 0xFFFF:
        raise ValueError("rebuilt text bank exceeds 16-bit offset range")
    offsets.append(pos)

    order_raw = struct.pack("<%dH" % len(ids), *ids)
    text_raw = bytearray()
    text_raw.extend(struct.pack("<%dH" % len(offsets), *offsets))
    text_raw.extend(payload)
    return order_raw, bytes(text_raw)


def main():
    signal.signal(signal.SIGPIPE, signal.SIG_DFL)

    ap = argparse.ArgumentParser()
    ap.add_argument("text_hqr", help="Path to TEXT.HQR")
    ap.add_argument("--language", default="en", help="en, fr, de, sp, it, po (default: en)")
    ap.add_argument("--file", default="000", help="sys, cre, gam, 000..011, or index (default: 000)")
    ap.add_argument("--text", type=int, help="Text id to inspect or replace")
    ap.add_argument("--contains", help="Filter listed strings by substring")
    ap.add_argument("--limit", type=int, default=40, help="List limit; 0 = no limit")
    ap.add_argument("--json", action="store_true", help="Emit JSON")
    ap.add_argument("--set", dest="set_text", help="Replacement text for --text")
    ap.add_argument("--flag", type=int, help="Replacement flag byte for --set")
    ap.add_argument("--add", type=int, help="Append a new text id; requires --set")
    ap.add_argument("--write", action="store_true", help="Write replacement text back to TEXT.HQR")
    args = ap.parse_args()

    try:
        language = parse_language(args.language)
        file_index = parse_file(args.file)
        bank = load_bank(args.text_hqr, language, file_index)
    except Exception as exc:
        print(str(exc), file=sys.stderr)
        return 1

    rows = selected_rows(bank, args.text, args.contains, args.limit)

    if args.add is not None:
        if args.set_text is None:
            print("--add requires --set", file=sys.stderr)
            return 2
        flag = args.flag if args.flag is not None else 1
        try:
            order_raw, text_raw = append_text(bank, args.add, args.set_text, flag)
        except Exception as exc:
            print(str(exc), file=sys.stderr)
            return 2

        print("add text %d in %s_%s:" % (args.add, LANGUAGE_NAMES[language], FILE_NAMES[file_index]))
        print("  flag=%d %s" % (flag, repr(args.set_text)))
        print("  count %d -> %d" % (len(bank["texts"]), len(bank["texts"]) + 1))

        if args.write:
            old_size, new_size = rewrite_entry_stored(
                args.text_hqr,
                {
                    bank["order_entry"]: order_raw,
                    bank["text_entry"]: text_raw,
                },
            )
            print(
                "wrote %s order/text entries %d/%d as stored data (%d -> %d bytes)"
                % (args.text_hqr, bank["order_entry"], bank["text_entry"], old_size, new_size)
            )
        else:
            print("dry run only; add --write to update %s" % args.text_hqr)
        return 0

    if args.set_text is not None:
        if args.text is None:
            print("--set requires --text", file=sys.stderr)
            return 2
        if len([row for row in bank["texts"] if row["id"] == args.text]) != 1:
            print("text id %d not found exactly once" % args.text, file=sys.stderr)
            return 2
        try:
            new_blob = rebuild_text_blob(bank, args.text, args.set_text, args.flag)
        except Exception as exc:
            print(str(exc), file=sys.stderr)
            return 2

        old = selected_rows(bank, args.text, None, 0)[0]
        print("replace text %d in %s_%s:" % (args.text, LANGUAGE_NAMES[language], FILE_NAMES[file_index]))
        print("  before flag=%d %s" % (old["flag"], repr(old["text"])))
        print("  after  flag=%d %s" % (args.flag if args.flag is not None else old["flag"], repr(args.set_text)))

        if args.write:
            old_size, new_size = rewrite_entry_stored(
                args.text_hqr, {bank["text_entry"]: new_blob}
            )
            print(
                "wrote %s text entry %d as stored data (%d -> %d bytes)"
                % (args.text_hqr, bank["text_entry"], old_size, new_size)
            )
        else:
            print("dry run only; add --write to update %s" % args.text_hqr)
        return 0

    if args.json:
        out = {
            "language": LANGUAGE_NAMES[language],
            "file": FILE_NAMES[file_index],
            "order_entry": bank["order_entry"],
            "text_entry": bank["text_entry"],
            "count": len(bank["texts"]),
            "texts": rows,
        }
        print(json.dumps(out, indent=2, sort_keys=True))
    else:
        print_rows(bank, rows)

    return 0


if __name__ == "__main__":
    sys.exit(main())
