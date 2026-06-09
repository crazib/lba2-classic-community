#!/usr/bin/env python3
"""Apply small JSON mod manifests to retail LBA2 data files.

The mod manifest format is deliberately conservative: operations set absolute
target values and may include expected original values. That makes a patch
safe to re-run and helps catch drift before modifying binary data.
"""

import argparse
import json
import os
import signal
import struct
import sys

import hqr_inspect
import ile_objects
import ile_terrain
import scene_zones
import text_hqr
import vox_hqr


def data_path(data_dir, relpath):
    if os.path.isabs(relpath):
        raise ValueError("mod manifest file paths must be relative to --data-dir")
    return os.path.join(data_dir, relpath)


def manifest_path(manifest_dir, relpath):
    if os.path.isabs(relpath):
        raise ValueError("mod manifest source paths must be relative to the manifest file")
    return os.path.join(manifest_dir, relpath)


def as_triplet(value, name):
    if not isinstance(value, list) or len(value) != 3:
        raise ValueError("%s must be a three-item list" % name)
    return [int(value[0]), int(value[1]), int(value[2])]


def as_bounds(value, name):
    if not isinstance(value, list) or len(value) != 6:
        raise ValueError("%s must be a six-item list" % name)
    return [int(v) for v in value]


def object_state(obj):
    zv = obj["zv"]
    return {
        "position": [obj["x"], obj["y"], obj["z"]],
        "zv": [zv["xmin"], zv["ymin"], zv["zmin"], zv["xmax"], zv["ymax"], zv["zmax"]],
    }


def object_matches(obj, position, zv):
    state = object_state(obj)
    return state["position"] == position and state["zv"] == zv


def check_current_or_expected(label, current, target, expected):
    if current == target:
        return "already"
    if expected is not None and current != expected:
        raise ValueError("%s is %s, expected %s or target %s" % (label, current, expected, target))
    return "change"


def parse_cube(value):
    if not isinstance(value, list) or len(value) != 2:
        raise ValueError("cube must be a two-item list")
    return int(value[0]), int(value[1])


def find_cube_slot(path, cube_x, cube_y):
    cube_map = ile_objects.load_cube_map(path)
    for slot, cells in cube_map.items():
        if (cube_x, cube_y) in cells:
            return slot
    raise ValueError("cube %d,%d is not present in %s" % (cube_x, cube_y, path))


def op_set_ile_object(op, data_dir, write):
    path = data_path(data_dir, op["file"])
    cube_x, cube_y = parse_cube(op["cube"])
    object_index = int(op["object"])
    position = as_triplet(op["position"], "position")
    zv = as_bounds(op["zv"], "zv")

    cube_slot = find_cube_slot(path, cube_x, cube_y)
    entry_index = (
        ile_objects.HQR_START_CUBE
        + ile_objects.HQR_STEP_CUBE * (cube_slot - 1)
        + ile_objects.HQR_CUBE_DOB
    )
    dob_raw = ile_objects.entry_bytes(path, entry_index)
    if dob_raw is None:
        raise ValueError("cube %d,%d slot %d has no DOB entry" % (cube_x, cube_y, cube_slot))
    if len(dob_raw) % ile_objects.T_DECORS_SIZE != 0:
        raise ValueError("DOB entry %d has invalid size %d" % (entry_index, len(dob_raw)))

    count = len(dob_raw) // ile_objects.T_DECORS_SIZE
    if object_index < 0 or object_index >= count:
        raise ValueError("object index %d is out of range [0,%d)" % (object_index, count))

    raw = bytearray(dob_raw)
    offset = object_index * ile_objects.T_DECORS_SIZE
    obj = ile_objects.decode_decor(raw, offset)
    if "expect_body" in op and obj["body_id"] != int(op["expect_body"]):
        raise ValueError(
            "object c%d,%d #%d has body %d, expected %d"
            % (cube_x, cube_y, object_index, obj["body_id"], int(op["expect_body"]))
        )

    current = object_state(obj)
    target = {"position": position, "zv": zv}
    expected = None
    if "expect_position" in op or "expect_zv" in op:
        expected = {
            "position": as_triplet(op.get("expect_position", current["position"]), "expect_position"),
            "zv": as_bounds(op.get("expect_zv", current["zv"]), "expect_zv"),
        }
    state = check_current_or_expected(
        "object c%d,%d #%d" % (cube_x, cube_y, object_index), current, target, expected
    )

    print("set_ile_object %s c%d,%d #%d:" % (op["file"], cube_x, cube_y, object_index))
    print("  current position=%s zv=%s" % (current["position"], current["zv"]))
    print("  target  position=%s zv=%s" % (position, zv))

    if state == "already":
        print("  already applied")
        return False

    obj["x"], obj["y"], obj["z"] = position
    (
        obj["zv"]["xmin"],
        obj["zv"]["ymin"],
        obj["zv"]["zmin"],
        obj["zv"]["xmax"],
        obj["zv"]["ymax"],
        obj["zv"]["zmax"],
    ) = zv
    ile_objects.encode_decor(raw, offset, obj)

    if write:
        ile_objects.rewrite_entry_stored(path, entry_index, bytes(raw))
        print("  wrote DOB entry %d" % entry_index)
    else:
        print("  dry run only")
    return True


def zone_bounds(zone):
    return [zone["x0"], zone["y0"], zone["z0"], zone["x1"], zone["y1"], zone["z1"]]


def op_set_scene_zones(op, data_dir, write):
    path = data_path(data_dir, op["file"])
    ents, data, raw, scene = scene_zones.load_scene(path, int(op["scene"]))
    by_index = dict((z["index"], z) for z in scene["zones"])
    dirty = False

    print("set_scene_zones %s scene %d:" % (op["file"], scene["scene"]))
    for zone_spec in op["zones"]:
        zone_id = int(zone_spec["zone"])
        if zone_id not in by_index:
            raise ValueError("zone %d is not present in scene %d" % (zone_id, scene["scene"]))
        zone = dict(by_index[zone_id])
        if "expect_type" in zone_spec and zone["type"] != int(zone_spec["expect_type"]):
            raise ValueError("zone %d has type %d, expected %d" % (zone_id, zone["type"], int(zone_spec["expect_type"])))
        if "expect_num" in zone_spec:
            target_num = int(zone_spec.get("num", zone["num"]))
            expect_num = int(zone_spec["expect_num"])
            if zone["num"] != expect_num and zone["num"] != target_num:
                raise ValueError("zone %d has num %d, expected %d or %d" % (zone_id, zone["num"], expect_num, target_num))

        target_bounds = as_bounds(zone_spec["bounds"], "bounds")
        current_bounds = zone_bounds(zone)
        expected_bounds = None
        if "expect_bounds" in zone_spec:
            expected_bounds = as_bounds(zone_spec["expect_bounds"], "expect_bounds")
        state = check_current_or_expected(
            "scene %d zone %d bounds" % (scene["scene"], zone_id),
            current_bounds,
            target_bounds,
            expected_bounds,
        )
        print("  z%d current=%s target=%s" % (zone_id, current_bounds, target_bounds))

        if state != "already":
            (
                zone["x0"],
                zone["y0"],
                zone["z0"],
                zone["x1"],
                zone["y1"],
                zone["z1"],
            ) = target_bounds
            dirty = True

        if "num" in zone_spec:
            target_num = int(zone_spec["num"])
            if zone["num"] != target_num:
                print("    num %d -> %d" % (zone["num"], target_num))
                zone["num"] = target_num
                dirty = True

        if dirty:
            scene_zones.encode_zone(raw, zone)

    if not dirty:
        print("  already applied")
        return False

    if write:
        scene_zones.rewrite_entry_stored(path, ents, data, scene["entry"], bytes(raw))
        print("  wrote scene entry %d" % scene["entry"])
    else:
        print("  dry run only")
    return True


def op_add_text(op, data_dir, write):
    path = data_path(data_dir, op["file"])
    language = text_hqr.parse_language(op.get("language", "en"))
    file_index = text_hqr.parse_file(op.get("text_file", op.get("textFile", "000")))
    text_id = int(op["id"])
    new_text = op["text"]
    new_flag = int(op.get("flag", 1))
    bank = text_hqr.load_bank(path, language, file_index)
    existing = [row for row in bank["texts"] if row["id"] == text_id]

    print(
        "add_text %s %s_%s id %d:"
        % (
            op["file"],
            text_hqr.LANGUAGE_NAMES[language],
            text_hqr.FILE_NAMES[file_index],
            text_id,
        )
    )

    if existing:
        if len(existing) != 1:
            raise ValueError("text id %d appears %d times" % (text_id, len(existing)))
        row = existing[0]
        if row["text"] != new_text or row["flag"] != new_flag:
            raise ValueError(
                "text id %d already exists as flag=%d %r, target is flag=%d %r"
                % (text_id, row["flag"], row["text"], new_flag, new_text)
            )
        print("  already applied")
        return False

    print("  append flag=%d %r" % (new_flag, new_text))
    order_raw, text_raw = text_hqr.append_text(bank, text_id, new_text, new_flag)
    if write:
        text_hqr.rewrite_entry_stored(
            path,
            {
                bank["order_entry"]: order_raw,
                bank["text_entry"]: text_raw,
            },
        )
        print("  wrote order/text entries %d/%d" % (bank["order_entry"], bank["text_entry"]))
    else:
        print("  dry run only")
    return True


def planned_text_index(op, bank):
    op_index = op.get("_manifest_index")
    if op_index is None:
        return None

    language = text_hqr.parse_language(op.get("language", "en"))
    file_index = text_hqr.parse_file(op.get("text_file", op.get("textFile", "000")))
    text_path = op.get("text_hqr", op.get("file", "TEXT.HQR"))
    text_id = int(op["id"])
    index = len(bank["texts"])

    for prior in op.get("_manifest_ops", [])[:op_index]:
        if prior.get("type") != "add_text":
            continue
        prior_path = prior.get("file", "TEXT.HQR")
        prior_language = text_hqr.parse_language(prior.get("language", "en"))
        prior_file = text_hqr.parse_file(prior.get("text_file", prior.get("textFile", "000")))
        prior_id = int(prior["id"])
        if prior_path != text_path or prior_language != language or prior_file != file_index:
            continue
        if prior_id == text_id:
            return index
        if prior_id not in bank["ids"]:
            index += 1

    return None


def op_add_voice(op, data_dir, write):
    manifest_dir = op.get("_manifest_dir", ".")
    text_path = data_path(data_dir, op.get("text_hqr", "TEXT.HQR"))
    language = text_hqr.parse_language(op.get("language", "en"))
    file_index = text_hqr.parse_file(op.get("text_file", op.get("textFile", "000")))
    text_id = int(op["id"])
    bank = text_hqr.load_bank(text_path, language, file_index)
    rows = [row for row in bank["texts"] if row["id"] == text_id]

    if rows:
        if len(rows) != 1:
            raise ValueError("text id %d appears %d times" % (text_id, len(rows)))
        voice_index = rows[0]["index"]
    else:
        voice_index = planned_text_index(op, bank)
        if voice_index is None:
            raise ValueError("text id %d is not present in %s" % (text_id, op.get("text_hqr", "TEXT.HQR")))

    vox_file = op.get("file", op.get("vox_file"))
    if vox_file is None:
        vox_file = vox_hqr.default_vox_file(language, file_index, text_hqr)
    vox_path = data_path(data_dir, vox_file)
    source_path = manifest_path(manifest_dir, op["source"])

    with open(source_path, "rb") as f:
        source_raw = f.read()
    payload = vox_hqr.wav_voice_payload(source_raw, bool(op.get("next", False)))
    voice_bank = vox_hqr.load_bank(vox_path)
    existing = vox_hqr.slot_payload(voice_bank, voice_index)

    print(
        "add_voice %s %s_%s id %d slot %d <- %s:"
        % (
            vox_file,
            text_hqr.LANGUAGE_NAMES[language],
            text_hqr.FILE_NAMES[file_index],
            text_id,
            voice_index,
            op["source"],
        )
    )
    print("  source size=%d next=%s" % (len(source_raw), "true" if op.get("next", False) else "false"))

    if existing is not None:
        if existing == payload:
            print("  already applied")
            return False
        raise ValueError("voice slot %d in %s is not empty" % (voice_index, vox_file))

    if voice_index >= len(voice_bank["slots"]):
        print("  appending VOX slot %d" % voice_index)
    else:
        print("  target slot is empty")

    if write:
        old_size, new_size = vox_hqr.rewrite_slot_stored(vox_path, voice_bank, voice_index, payload)
        print("  wrote VOX slot %d (%d -> %d bytes)" % (voice_index, old_size, new_size))
    else:
        print("  dry run only")
    return True


# Copy an HQR entry from one index to another in the same file.
def op_copy_hqr_entry(op, data_dir, write):
    path = data_path(data_dir, op["file"])
    source_index = int(op["from"])
    target_index = int(op["to"])
    ents, data = hqr_inspect.entries(path)

    if source_index < 0 or source_index >= len(ents):
        raise ValueError("source entry %d is out of range [0,%d)" % (source_index, len(ents)))
    if target_index < 0 or target_index >= len(ents):
        raise ValueError("target entry %d is out of range [0,%d)" % (target_index, len(ents)))

    source = ents[source_index]
    target = ents[target_index]

    print("copy_hqr_entry %s %d -> %d:" % (op["file"], source_index, target_index))

    if source[2] is None:
        raise ValueError("source entry %d is empty" % source_index)

    source_offset = source[1]
    source_size = source[2]
    source_compressed_size = source[3]
    source_method = source[4]
    source_blob = data[source_offset : source_offset + 10 + source_compressed_size]

    print(
        "  source size=%d compressed=%d method=%d"
        % (source_size, source_compressed_size, source_method)
    )

    if target[2] is not None:
        target_offset = target[1]
        target_size = target[2]
        target_compressed_size = target[3]
        target_method = target[4]
        target_blob = data[target_offset : target_offset + 10 + target_compressed_size]
        if target_blob == source_blob:
            print("  already applied")
            return False
        raise ValueError(
            "target entry %d is not empty: size=%d compressed=%d method=%d"
            % (target_index, target_size, target_compressed_size, target_method)
        )

    print("  target is empty")

    if write:
        new_entries = []
        for index, ent in enumerate(ents):
            if index == target_index:
                new_entries.append(source_blob)
            elif ent[2] is None:
                new_entries.append(None)
            else:
                offset = ent[1]
                compressed_size = ent[3]
                new_entries.append(data[offset : offset + 10 + compressed_size])

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
        for offset in offsets:
            out.extend(struct.pack("<I", offset))
        for blob in new_entries:
            if blob is not None:
                out.extend(blob)

        with open(path, "wb") as f:
            f.write(out)
        print("  wrote copied entry")
    else:
        print("  dry run only")

    return True


def op_replace_hqr_entry(op, data_dir, write):
    path = data_path(data_dir, op["file"])
    entry_index = int(op["entry"])
    manifest_dir = op.get("_manifest_dir", ".")
    source_path = manifest_path(manifest_dir, op["source"])

    ents, data = hqr_inspect.entries(path)

    if entry_index < 0 or entry_index >= len(ents):
        raise ValueError("entry %d is out of range [0,%d)" % (entry_index, len(ents)))

    with open(source_path, "rb") as f:
        payload = f.read()

    if len(payload) == 0:
        raise ValueError("source file %s is empty" % source_path)

    source_blob = struct.pack("<IIH", len(payload), len(payload), 0) + payload
    target = ents[entry_index]

    print("replace_hqr_entry %s entry %d <- %s:" % (op["file"], entry_index, op["source"]))
    print("  source size=%d stored=%d" % (len(payload), len(source_blob)))

    if target[2] is not None:
        target_offset = target[1]
        target_compressed_size = target[3]
        target_blob = data[target_offset : target_offset + 10 + target_compressed_size]
        if target_blob == source_blob:
            print("  already applied")
            return False
        print(
            "  replacing existing size=%d compressed=%d method=%d"
            % (target[2], target[3], target[4])
        )
    else:
        print("  replacing empty entry")

    if write:
        new_entries = []
        for index, ent in enumerate(ents):
            if index == entry_index:
                new_entries.append(source_blob)
            elif ent[2] is None:
                new_entries.append(None)
            else:
                offset = ent[1]
                compressed_size = ent[3]
                new_entries.append(data[offset : offset + 10 + compressed_size])

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

        for offset in offsets:
            out.extend(struct.pack("<I", offset))

        for blob in new_entries:
            if blob is not None:
                out.extend(blob)

        with open(path, "wb") as f:
            f.write(out)

        print("  wrote replaced entry")
    else:
        print("  dry run only")

    return True


def op_set_terrain_heights(op, data_dir, write):
    path = data_path(data_dir, op["file"])
    cube_x, cube_y = parse_cube(op["cube"])
    slot, entry_index, raw, heights = ile_terrain.load_heights(path, cube_x, cube_y)
    dirty = False

    print("set_terrain_heights %s c%d,%d slot %d:" % (op["file"], cube_x, cube_y, slot))
    for point in op["heights"]:
        x = int(point["x"])
        z = int(point["z"])
        target = int(point["height"])
        if x < 0 or x >= ile_terrain.HEIGHT_SIDE or z < 0 or z >= ile_terrain.HEIGHT_SIDE:
            raise ValueError("terrain vertex %d,%d is outside [0,64],[0,64]" % (x, z))
        index = z * ile_terrain.HEIGHT_SIDE + x
        current = heights[index]
        expected = int(point["expect"]) if "expect" in point else None
        state = check_current_or_expected("terrain %d,%d" % (x, z), current, target, expected)
        print("  %d,%d current=%d target=%d" % (x, z, current, target))
        if state == "already":
            continue
        heights[index] = target
        struct.pack_into("<h", raw, index * 2, target)
        dirty = True

    if not dirty:
        print("  already applied")
        return False

    if write:
        ile_terrain.rewrite_entry_stored(path, entry_index, bytes(raw))
        print("  wrote Y entry %d" % entry_index)
    else:
        print("  dry run only")
    return True


OPERATIONS = {
    "set_ile_object": op_set_ile_object,
    "set_scene_zones": op_set_scene_zones,
    "add_text": op_add_text,
    "add_voice": op_add_voice,
    "copy_hqr_entry": op_copy_hqr_entry,
    "replace_hqr_entry": op_replace_hqr_entry,
    "set_terrain_heights": op_set_terrain_heights,
}


def load_manifest(path):
    with open(path, "r") as f:
        manifest = json.load(f)
    if int(manifest.get("version", 0)) != 1:
        raise ValueError("mod manifest version must be 1")
    if "operations" not in manifest or not isinstance(manifest["operations"], list):
        raise ValueError("mod manifest must contain an operations list")
    return manifest


def main():
    signal.signal(signal.SIGPIPE, signal.SIG_DFL)

    ap = argparse.ArgumentParser()
    ap.add_argument("manifest", help="JSON mod manifest to apply")
    ap.add_argument("--data-dir", default="data", help="Retail data directory (default: data)")
    ap.add_argument("--write", action="store_true", help="Write changes; default is dry-run")
    args = ap.parse_args()

    try:
        manifest = load_manifest(args.manifest)
        manifest_dir = os.path.dirname(os.path.abspath(args.manifest))
        changed = 0
        for index, op in enumerate(manifest["operations"]):
            op["_manifest_dir"] = manifest_dir
            op["_manifest_index"] = index
            op["_manifest_ops"] = manifest["operations"]
            op_type = op.get("type")
            if op_type not in OPERATIONS:
                raise ValueError("operation %d has unknown type %r" % (index, op_type))
            if OPERATIONS[op_type](op, args.data_dir, args.write):
                changed += 1
        if args.write:
            print("done; %d operation(s) wrote changes" % changed)
        else:
            print("dry run done; %d operation(s) would write changes" % changed)
    except Exception as exc:
        print(str(exc), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
