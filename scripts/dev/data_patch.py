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
import shutil
import struct
import sys

import hqr_inspect
import ile_objects
import ile_terrain
import scene_zones
import text_hqr
import vox_hqr


def hqr_entry_blob(ents, data, index):
    if index < 0 or index >= len(ents):
        raise ValueError("entry %d is out of range [0,%d)" % (index, len(ents)))
    ent = ents[index]
    if ent[2] is None:
        return None
    _i, off, _size, csize, _method = ent
    return data[off : off + 10 + csize]


def rewrite_hqr_blobs(path, ents, data, replacements, min_slots=None):
    slot_count = len(ents)
    if min_slots is not None and min_slots > slot_count:
        slot_count = min_slots

    new_entries = []
    for index in range(slot_count):
        if index in replacements:
            new_entries.append(replacements[index])
            continue
        if index >= len(ents):
            new_entries.append(None)
            continue
        blob = hqr_entry_blob(ents, data, index)
        new_entries.append(blob)

    table_size = len(new_entries) * 4
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

    return len(data), len(out)


def stored_hqr_blob(payload):
    return struct.pack("<IIh", len(payload), len(payload), 0) + payload


def encode_hqr_blobs(blobs):
    table_size = len(blobs) * 4
    offsets = []
    pos = table_size

    for blob in blobs:
        if blob is None:
            offsets.append(0)
        else:
            offsets.append(pos)
            pos += len(blob)

    out = bytearray()
    for offset in offsets:
        out.extend(struct.pack("<I", offset))
    for blob in blobs:
        if blob is not None:
            out.extend(blob)

    return bytes(out)


def write_hqr_blobs(path, blobs):
    with open(path, "wb") as f:
        f.write(encode_hqr_blobs(blobs))


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


def build_island_texdef_catalog(path):
    pair_size = 24
    catalog = bytearray()
    seen = set()

    cube_map = ile_objects.entry_bytes(path, ile_objects.HQR_MAP_IDM)
    if cube_map is None or len(cube_map) != ile_objects.SIZE_MAIN_MAP * ile_objects.SIZE_MAIN_MAP:
        raise ValueError("source island has an invalid IDM map")
    for map_value in cube_map:
        slot = map_value & 0x7F
        if not slot:
            continue
        entry_index = ile_objects.HQR_START_CUBE + ile_objects.HQR_STEP_CUBE * (slot - 1) + 3
        raw = ile_objects.entry_bytes(path, entry_index)
        if raw is None:
            raise ValueError("source island cube slot %d is missing TXD data" % slot)
        for offset in range(0, len(raw) - pair_size + 1, pair_size):
            pair = raw[offset : offset + pair_size]
            if pair in seen:
                continue
            seen.add(pair)
            catalog.extend(pair)

    if not catalog:
        raise ValueError("source island has no paired TXD data")
    return bytes(catalog)


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


def op_apply_decor_edits(op, data_dir, write):
    path = data_path(data_dir, op["file"])
    source_path = manifest_path(op.get("_manifest_dir", "."), op["source"])
    with open(source_path, "r") as f:
        edits = json.load(f)

    if not isinstance(edits, dict):
        raise ValueError("decor edit source must be an object")

    print("apply_decor_edits %s <- %s:" % (op["file"], op["source"]))
    changed = False
    for cube_edit in edits.get("cubes", []):
        cube_x, cube_y = parse_cube(cube_edit["cube"])
        objects = cube_edit.get("objects", [])
        if not objects:
            continue

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

        raw = bytearray(dob_raw)
        count = len(raw) // ile_objects.T_DECORS_SIZE
        dirty = False

        for obj_edit in objects:
            object_index = int(obj_edit["object"])
            if object_index < 0 or object_index >= count:
                raise ValueError("object index %d is out of range [0,%d)" % (object_index, count))

            offset = object_index * ile_objects.T_DECORS_SIZE
            obj = ile_objects.decode_decor(raw, offset)
            target = dict(obj)

            if "expect_body" in obj_edit:
                expect_body = int(obj_edit["expect_body"])
                target_body = int(obj_edit.get("body", obj["body_id"]))
                if obj["body_id"] != expect_body and obj["body_id"] != target_body:
                    raise ValueError(
                        "object c%d,%d #%d has body %d, expected %d or target %d"
                        % (cube_x, cube_y, object_index, obj["body_id"], expect_body, target_body)
                    )

            if obj_edit.get("deleted", False):
                target["body_raw"] = 0
                target["x"] = 0
                target["y"] = -1000000
                target["z"] = 0
                target["code_jeu"] = 0
                target["beta_raw"] = 0
                target["zv"] = {"xmin": 0, "ymin": 0, "zmin": 0, "xmax": 0, "ymax": 0, "zmax": 0}
            else:
                if "body_raw" in obj_edit:
                    target["body_raw"] = int(obj_edit["body_raw"])
                elif "body" in obj_edit:
                    target["body_raw"] = (target["body_raw"] & ~0xFFFF) | (int(obj_edit["body"]) & 0xFFFF)
                if "code_jeu" in obj_edit:
                    target["code_jeu"] = int(obj_edit["code_jeu"])
                if "beta_raw" in obj_edit:
                    target["beta_raw"] = int(obj_edit["beta_raw"])
                if "position" in obj_edit:
                    target["x"], target["y"], target["z"] = as_triplet(obj_edit["position"], "position")
                if "zv" in obj_edit:
                    zv = as_bounds(obj_edit["zv"], "zv")
                    (
                        target["zv"]["xmin"],
                        target["zv"]["ymin"],
                        target["zv"]["zmin"],
                        target["zv"]["xmax"],
                        target["zv"]["ymax"],
                        target["zv"]["zmax"],
                    ) = zv

            current_state = {
                "body_raw": obj["body_raw"],
                "position": [obj["x"], obj["y"], obj["z"]],
                "code_jeu": obj["code_jeu"],
                "beta_raw": obj["beta_raw"],
                "zv": [
                    obj["zv"]["xmin"],
                    obj["zv"]["ymin"],
                    obj["zv"]["zmin"],
                    obj["zv"]["xmax"],
                    obj["zv"]["ymax"],
                    obj["zv"]["zmax"],
                ],
            }
            target_state = {
                "body_raw": target["body_raw"],
                "position": [target["x"], target["y"], target["z"]],
                "code_jeu": target["code_jeu"],
                "beta_raw": target["beta_raw"],
                "zv": [
                    target["zv"]["xmin"],
                    target["zv"]["ymin"],
                    target["zv"]["zmin"],
                    target["zv"]["xmax"],
                    target["zv"]["ymax"],
                    target["zv"]["zmax"],
                ],
            }

            print("  c%d,%d #%d body %d -> %d%s" % (
                cube_x,
                cube_y,
                object_index,
                obj["body_id"],
                target["body_raw"] & 0xFFFF,
                " deleted" if obj_edit.get("deleted", False) else "",
            ))
            if current_state == target_state:
                continue

            ile_objects.encode_decor(raw, offset, target)
            dirty = True

        if dirty:
            changed = True
            if write:
                ile_objects.rewrite_entry_stored(path, entry_index, bytes(raw))
                print("  wrote DOB entry %d" % entry_index)
            else:
                print("  dry run only")

    if not changed:
        print("  already applied")
    return changed


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


def patch_scene_header(raw, patch):
    fields = {
        "island": 0,
        "cube_x": 1,
        "cube_y": 2,
        "shadow_level": 3,
        "mode_labyrinthe": 4,
        "cube_mode": 5,
    }
    changed = []
    for name, offset in fields.items():
        if name not in patch:
            continue
        target = int(patch[name])
        if target < -128 or target > 127:
            raise ValueError("scene header %s=%d is outside signed-byte range" % (name, target))
        current = struct.unpack_from("<b", raw, offset)[0]
        if current != target:
            struct.pack_into("<b", raw, offset, target)
            changed.append((name, current, target))
    return changed


def op_set_scene_header(op, data_dir, write):
    path = data_path(data_dir, op["file"])
    scene_num = int(op["scene"])
    ents, data, raw, scene = scene_zones.load_scene(path, scene_num)
    patch = op.get("patch", {})
    if not isinstance(patch, dict):
        raise ValueError("set_scene_header patch must be an object")

    print("set_scene_header %s scene %d:" % (op["file"], scene_num))
    changed = patch_scene_header(raw, patch)
    if not changed:
        print("  already applied")
        return False
    for name, current, target in changed:
        print("  %s %d -> %d" % (name, current, target))

    if write:
        scene_zones.rewrite_entry_stored(path, ents, data, scene["entry"], bytes(raw))
        print("  wrote scene entry %d" % scene["entry"])
    else:
        print("  dry run only")
    return True


def scene_num_map(value):
    if not isinstance(value, dict):
        raise ValueError("scene map must be an object")
    out = {}
    for src, dst in value.items():
        out[int(src)] = int(dst)
    return out


def op_remap_scene_zone_nums(op, data_dir, write):
    path = data_path(data_dir, op["file"])
    scene_num = int(op["scene"])
    mapping = scene_num_map(op["map"])
    ents, data, raw, scene = scene_zones.load_scene(path, scene_num)
    dirty = False

    print("remap_scene_zone_nums %s scene %d:" % (op["file"], scene_num))
    for zone in scene["zones"]:
        if zone["type"] != 0:
            continue
        if zone["num"] not in mapping:
            continue
        old_num = zone["num"]
        zone["num"] = mapping[old_num]
        scene_zones.encode_zone(raw, zone)
        dirty = True
        print("  z%d num %d -> %d" % (zone["index"], old_num, zone["num"]))

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


def op_replace_text(op, data_dir, write):
    path = data_path(data_dir, op["file"])
    language = text_hqr.parse_language(op.get("language", "en"))
    file_index = text_hqr.parse_file(op.get("text_file", op.get("textFile", "000")))
    text_id = int(op["id"])
    new_text = op["text"]
    new_flag = int(op.get("flag", 1))

    bank = text_hqr.load_bank(path, language, file_index)

    rows = [row for row in bank["texts"] if row["id"] == text_id]

    print(
        "replace_text %s %s_%s id %d:"
        % (
            op["file"],
            text_hqr.LANGUAGE_NAMES[language],
            text_hqr.FILE_NAMES[file_index],
            text_id,
        )
    )

    if not rows:
        raise ValueError("text id %d not found for replace" % text_id)

    if len(rows) != 1:
        raise ValueError("text id %d appears %d times" % (text_id, len(rows)))

    row = rows[0]
    print("  current flag=%d text=%r" % (row["flag"], row["text"]))
    print("  target  flag=%d text=%r" % (new_flag, new_text))

    # remove old entry so append_text can safely reinsert updated version
    bank["texts"] = [r for r in bank["texts"] if r["id"] != text_id]
    if "ids" in bank and text_id in bank["ids"]:
        bank["ids"].remove(text_id)

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


def op_replace_voice(op, data_dir, write):
    manifest_dir = op.get("_manifest_dir", ".")
    text_path = data_path(data_dir, op.get("text_hqr", "TEXT.HQR"))
    language = text_hqr.parse_language(op.get("language", "en"))
    file_index = text_hqr.parse_file(op.get("text_file", op.get("textFile", "000")))
    text_id = int(op["id"])

    bank = text_hqr.load_bank(text_path, language, file_index)
    rows = [row for row in bank["texts"] if row["id"] == text_id]

    if rows:
        voice_index = rows[0]["index"]
    else:
        voice_index = planned_text_index(op, bank)
        if voice_index is None:
            raise ValueError("text id %d not found for voice replace" % text_id)

    vox_file = op.get("file", op.get("vox_file"))
    if vox_file is None:
        vox_file = vox_hqr.default_vox_file(language, file_index, text_hqr)

    vox_path = data_path(data_dir, vox_file)
    source_path = manifest_path(manifest_dir, op["source"])

    with open(source_path, "rb") as f:
        source_raw = f.read()

    payload = vox_hqr.wav_voice_payload(source_raw, bool(op.get("next", False)))
    voice_bank = vox_hqr.load_bank(vox_path)

    print(
        "replace_voice %s %s_%s id %d slot %d <- %s:"
        % (
            vox_file,
            text_hqr.LANGUAGE_NAMES[language],
            text_hqr.FILE_NAMES[file_index],
            text_id,
            voice_index,
            op["source"],
        )
    )

    existing = vox_hqr.slot_payload(voice_bank, voice_index)

    if existing == payload:
        print("  already applied")
        return False

    if voice_index >= len(voice_bank["slots"]):
        print("  appending VOX slot %d" % voice_index)
    else:
        print("  overwriting VOX slot %d" % voice_index)

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
    if target_index < 0:
        raise ValueError("target entry %d is out of range" % target_index)

    source = ents[source_index]
    target = ents[target_index] if target_index < len(ents) else (target_index, 0, None, None, None)

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
        rewrite_hqr_blobs(path, ents, data, {target_index: source_blob}, target_index + 1)
        print("  wrote copied entry")
    else:
        print("  dry run only")

    return True


def op_copy_file(op, data_dir, write):
    source_path = data_path(data_dir, op["from"])
    target_path = data_path(data_dir, op["to"])

    print("copy_file %s -> %s:" % (op["from"], op["to"]))
    if not os.path.exists(source_path):
        raise ValueError("source file %s is missing" % source_path)

    with open(source_path, "rb") as f:
        source_raw = f.read()

    if os.path.exists(target_path):
        with open(target_path, "rb") as f:
            target_raw = f.read()
        if target_raw == source_raw:
            print("  already applied")
            return False
        raise ValueError("target file %s already exists and differs" % target_path)

    print("  target is missing")
    if write:
        target_dir = os.path.dirname(target_path)
        if target_dir and not os.path.isdir(target_dir):
            os.makedirs(target_dir)
        shutil.copyfile(source_path, target_path)
        print("  wrote copied file")
    else:
        print("  dry run only")
    return True


def op_create_blank_island(op, data_dir, write):
    source_path = data_path(data_dir, op["from"])
    target_path = data_path(data_dir, op["to"])
    layout_path = manifest_path(op.get("_manifest_dir", "."), op["layout"])
    template_x, template_y = parse_cube(op.get("template_cube", [8, 8]))

    print(
        "create_blank_island %s -> %s from layout %s:"
        % (op["from"], op["to"], op["layout"])
    )
    if not os.path.exists(source_path):
        raise ValueError("source file %s is missing" % source_path)

    with open(layout_path, "r") as f:
        layout = json.load(f)
    if not isinstance(layout, dict) or not isinstance(layout.get("cubes"), list):
        raise ValueError("blank island layout must be an object with a cubes list")

    cubes = []
    seen = set()
    for value in layout["cubes"]:
        cube_x, cube_y = parse_cube(value)
        if cube_x < 0 or cube_x >= ile_objects.SIZE_MAIN_MAP or cube_y < 0 or cube_y >= ile_objects.SIZE_MAIN_MAP:
            raise ValueError("layout cube %d,%d is outside [0,15],[0,15]" % (cube_x, cube_y))
        if (cube_x, cube_y) in seen:
            raise ValueError("layout cube %d,%d is duplicated" % (cube_x, cube_y))
        seen.add((cube_x, cube_y))
        cubes.append((cube_x, cube_y))
    if not cubes:
        raise ValueError("blank island layout must allocate at least one cube")
    if len(cubes) > 20:
        raise ValueError("blank island layout has %d cubes; engine limit is 20" % len(cubes))

    source_ents, source_data = hqr_inspect.entries(source_path)
    template_slot = find_cube_slot(source_path, template_x, template_y)
    template_base = ile_objects.HQR_START_CUBE + ile_objects.HQR_STEP_CUBE * (template_slot - 1)
    info_raw = ile_objects.entry_bytes(source_path, template_base + ile_objects.HQR_CUBE_INF)
    texdefs = build_island_texdef_catalog(source_path)
    texdefs_blob = stored_hqr_blob(texdefs)
    intensity_blob = hqr_entry_blob(source_ents, source_data, template_base + 5)
    if info_raw is None or intensity_blob is None:
        raise ValueError("template cube %d,%d is missing INF or LUM data" % (template_x, template_y))
    if len(info_raw) != 40:
        raise ValueError("template cube INF has unexpected size %d" % len(info_raw))

    info = bytearray(info_raw)
    alpha_light = struct.unpack_from("<I", info, 0)[0]
    # CubeBitField is the upper half and selects the 4x4 coarse sea tiles.
    struct.pack_into("<I", info, 0, alpha_light | 0xFFFF0000)
    struct.pack_into("<i", info, 2 * 4, 0)
    cube_map = bytearray(ile_objects.SIZE_MAIN_MAP * ile_objects.SIZE_MAIN_MAP)
    for slot, (cube_x, cube_y) in enumerate(cubes, 1):
        cube_map[cube_y * ile_objects.SIZE_MAIN_MAP + cube_x] = slot

    water_poly = struct.pack("<I", 1 << 12)
    blank_ground = water_poly * ile_terrain.POLY_COUNT
    blank_heights = b"\0" * (ile_terrain.HEIGHT_COUNT * 2)
    blobs = [
        stored_hqr_blob(bytes(cube_map)),
        hqr_entry_blob(source_ents, source_data, 1),
        hqr_entry_blob(source_ents, source_data, 2),
    ]
    for _cube in cubes:
        blobs.extend(
            [
                stored_hqr_blob(bytes(info)),
                None,
                stored_hqr_blob(blank_ground),
                texdefs_blob,
                stored_hqr_blob(blank_heights),
                intensity_blob,
            ]
        )

    if blobs[1] is None or blobs[2] is None:
        raise ValueError("source island is missing ground or object texture entries")

    target_raw = encode_hqr_blobs(blobs)
    if os.path.exists(target_path):
        with open(target_path, "rb") as f:
            current_raw = f.read()
        if current_raw == target_raw:
            print("  already applied")
            return False
        raise ValueError("target file %s already exists and differs; rebuild from clean retail data" % target_path)

    print("  allocated cubes: %s" % ", ".join("%d,%d" % cube for cube in cubes))
    print("  blank terrain: zero-height water, no decor placements")
    print("  terrain texture catalog: %d paired definitions" % (len(texdefs) // 24))
    if write:
        target_dir = os.path.dirname(target_path)
        if target_dir and not os.path.isdir(target_dir):
            os.makedirs(target_dir)
        with open(target_path, "wb") as f:
            f.write(target_raw)
        print("  wrote blank island")
    else:
        print("  dry run only")
    return True


def op_clone_scene(op, data_dir, write):
    path = data_path(data_dir, op["file"])
    source_scene = int(op["from"])
    target_scene = int(op["to"])
    source_entry = source_scene + 1
    target_entry = target_scene + 1
    ents, data = hqr_inspect.entries(path)
    source = ents[source_entry] if 0 <= source_entry < len(ents) else None
    target = ents[target_entry] if 0 <= target_entry < len(ents) else (target_entry, 0, None, None, None)

    print("clone_scene %s scene %d -> %d:" % (op["file"], source_scene, target_scene))

    if source is None or source[2] is None:
        raise ValueError("source scene %d maps to missing/empty entry %d" % (source_scene, source_entry))

    raw = bytearray(hqr_inspect.decompress_entry(data, source[1], source[2], source[3], source[4]))
    patch = op.get("patch", {})
    if not isinstance(patch, dict):
        raise ValueError("clone_scene patch must be an object")
    changed = patch_scene_header(raw, patch)

    source_blob = struct.pack("<IIh", len(raw), len(raw), 0) + raw

    if target[2] is not None:
        target_blob = hqr_entry_blob(ents, data, target_entry)
        if target_blob == source_blob:
            print("  already applied")
            return False
        target_raw = bytearray(hqr_inspect.decompress_entry(data, target[1], target[2], target[3], target[4]))
        target_scene_data = scene_zones.parse_scene(target_raw, target_scene, target_entry)
        header_matches = True
        for name, value in patch.items():
            if name in target_scene_data and int(target_scene_data[name]) != int(value):
                header_matches = False
        if patch and header_matches:
            print("  target scene already exists with requested header")
            return False
        raise ValueError("target scene %d maps to non-empty entry %d" % (target_scene, target_entry))

    for name, current, target_value in changed:
        print("  %s %d -> %d" % (name, current, target_value))
    print("  target entry %d is empty" % target_entry)

    if write:
        rewrite_hqr_blobs(path, ents, data, {target_entry: source_blob}, target_entry + 1)
        print("  wrote scene entry %d" % target_entry)
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


def op_add_hqr_entry(op, data_dir, write):
    path = data_path(data_dir, op["file"])
    entry_index = int(op["entry"])
    manifest_dir = op.get("_manifest_dir", ".")
    source_path = manifest_path(manifest_dir, op["source"])

    ents, data = hqr_inspect.entries(path)

    with open(source_path, "rb") as f:
        payload = f.read()

    if len(payload) == 0:
        raise ValueError("source file %s is empty" % source_path)

    source_blob = struct.pack("<IIH", len(payload), len(payload), 0) + payload

    print("add_hqr_entry %s entry %d <- %s:" % (op["file"], entry_index, op["source"]))
    print("  source size=%d stored=%d" % (len(payload), len(source_blob)))

    # extend ents if needed (this is the important bit)
    if entry_index > len(ents):
        print("  extending table from %d to %d entries" % (len(ents), entry_index))
        while len(ents) < entry_index:
            ents.append((0, 0, None, 0, 0))  # empty slot placeholder

    # if exactly at end, just append
    if entry_index == len(ents):
        ents.append((0, 0, None, 0, 0))

    if write:
        new_entries = []

        for i, ent in enumerate(ents):
            if i == entry_index:
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

        print("  wrote added entry")
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


def terrain_poly_unpack(raw):
    return {
        "bank": raw & 0xF,
        "tex_flag": (raw >> 4) & 0x3,
        "poly_flag": (raw >> 6) & 0x3,
        "sample_step": (raw >> 8) & 0xF,
        "code_jeu": (raw >> 12) & 0xF,
        "sens": (raw >> 16) & 0x1,
        "col": (raw >> 17) & 0x1,
        "dummy": (raw >> 18) & 0x1,
        "index_tex": (raw >> 19) & 0x1FFF,
    }


def terrain_poly_pack(poly):
    return (
        (int(poly.get("bank", 0)) & 0xF)
        | ((int(poly.get("tex_flag", 0)) & 0x3) << 4)
        | ((int(poly.get("poly_flag", 0)) & 0x3) << 6)
        | ((int(poly.get("sample_step", 0)) & 0xF) << 8)
        | ((int(poly.get("code_jeu", 0)) & 0xF) << 12)
        | ((int(poly.get("sens", 0)) & 0x1) << 16)
        | ((int(poly.get("col", 0)) & 0x1) << 17)
        | ((int(poly.get("dummy", 0)) & 0x1) << 18)
        | ((int(poly.get("index_tex", 0)) & 0x1FFF) << 19)
    )


def terrain_poly_from_spec(spec, prefix=""):
    fields = {}
    for key in (
        "bank",
        "tex_flag",
        "poly_flag",
        "sample_step",
        "code_jeu",
        "sens",
        "col",
        "dummy",
        "index_tex",
    ):
        json_key = prefix + key
        if json_key in spec:
            fields[key] = int(spec[json_key])
    return fields


def op_set_terrain_triangles(op, data_dir, write):
    path = data_path(data_dir, op["file"])
    cube_x, cube_y = parse_cube(op["cube"])
    slot, entry_index, raw, polys = ile_terrain.load_polys(path, cube_x, cube_y)
    dirty = False

    print("set_terrain_triangles %s c%d,%d slot %d:" % (op["file"], cube_x, cube_y, slot))
    for tri in op["triangles"]:
        index = int(tri["triangle"])
        if index < 0 or index >= ile_terrain.POLY_COUNT:
            raise ValueError("terrain triangle %d is outside [0,%d)" % (index, ile_terrain.POLY_COUNT))
        current_raw = polys[index]
        current = terrain_poly_unpack(current_raw)
        target = current.copy()
        target.update(terrain_poly_from_spec(tri))
        target_raw = terrain_poly_pack(target)
        expect = terrain_poly_from_spec(tri, "expect_")

        if expect:
            for key, value in expect.items():
                if current[key] != value and target[key] != current[key]:
                    raise ValueError(
                        "terrain triangle %d field %s has %d, expected %d"
                        % (index, key, current[key], value)
                    )

        print("  t%d current=0x%08x target=0x%08x" % (index, current_raw, target_raw))
        if current_raw == target_raw:
            continue
        polys[index] = target_raw
        struct.pack_into("<I", raw, index * 4, target_raw)
        dirty = True

    if not dirty:
        print("  already applied")
        return False

    if write:
        ile_terrain.rewrite_entry_stored(path, entry_index, bytes(raw))
        print("  wrote GRD entry %d" % entry_index)
    else:
        print("  dry run only")
    return True


def op_set_terrain_intensities(op, data_dir, write):
    path = data_path(data_dir, op["file"])
    cube_x, cube_y = parse_cube(op["cube"])
    slot, entry_index, raw, intensities = ile_terrain.load_intensities(path, cube_x, cube_y)
    dirty = False

    print("set_terrain_intensities %s c%d,%d slot %d:" % (op["file"], cube_x, cube_y, slot))
    for point in op["intensities"]:
        x = int(point["x"])
        z = int(point["z"])
        target = int(point["intensity"])
        if x < 0 or x >= ile_terrain.HEIGHT_SIDE or z < 0 or z >= ile_terrain.HEIGHT_SIDE:
            raise ValueError("terrain intensity vertex %d,%d is outside [0,64],[0,64]" % (x, z))
        if target < 0 or target > 255:
            raise ValueError("terrain intensity %d is outside [0,255]" % target)
        index = z * ile_terrain.HEIGHT_SIDE + x
        current = intensities[index]
        expected = int(point["expect"]) if "expect" in point else None
        state = check_current_or_expected("terrain intensity %d,%d" % (x, z), current, target, expected)
        print("  %d,%d current=%d target=%d" % (x, z, current, target))
        if state == "already":
            continue
        intensities[index] = target
        raw[index] = target
        dirty = True

    if not dirty:
        print("  already applied")
        return False

    if write:
        ile_terrain.rewrite_entry_stored(path, entry_index, bytes(raw))
        print("  wrote LUM entry %d" % entry_index)
    else:
        print("  dry run only")
    return True


def op_apply_terrain_edits(op, data_dir, write):
    source_path = manifest_path(op.get("_manifest_dir", "."), op["source"])
    with open(source_path, "r") as f:
        edits = json.load(f)

    file_name = op["file"]
    operations = []
    if isinstance(edits, dict) and "cubes" in edits:
        for cube in edits["cubes"]:
            operations.append(cube)
    elif isinstance(edits, dict):
        operations.append(edits)
    else:
        raise ValueError("terrain edit source must be an object")

    changed = False
    print("apply_terrain_edits %s <- %s:" % (file_name, op["source"]))
    for cube_edit in operations:
        if "terrain" in cube_edit:
            terrain = cube_edit["terrain"]
        else:
            terrain = cube_edit
        heights = terrain.get("heights", [])
        if heights:
            subop = {
                "type": "set_terrain_heights",
                "file": file_name,
                "cube": cube_edit["cube"],
                "heights": heights,
            }
            if op_set_terrain_heights(subop, data_dir, write):
                changed = True
        triangles = terrain.get("triangles", [])
        if triangles:
            subop = {
                "type": "set_terrain_triangles",
                "file": file_name,
                "cube": cube_edit["cube"],
                "triangles": triangles,
            }
            if op_set_terrain_triangles(subop, data_dir, write):
                changed = True
        intensities = terrain.get("intensities", [])
        if intensities:
            subop = {
                "type": "set_terrain_intensities",
                "file": file_name,
                "cube": cube_edit["cube"],
                "intensities": intensities,
            }
            if op_set_terrain_intensities(subop, data_dir, write):
                changed = True
    if not changed:
        print("  already applied")
    return changed


def op_include_manifest(op, data_dir, write):
    manifest_dir = op.get("_manifest_dir", ".")
    include_path = manifest_path(manifest_dir, op["source"])
    manifest = load_manifest(include_path)
    include_dir = os.path.dirname(os.path.abspath(include_path))
    changed = 0

    print("include_manifest %s:" % op["source"])
    for index, subop in enumerate(manifest["operations"]):
        subop["_manifest_dir"] = include_dir
        subop["_manifest_index"] = index
        subop["_manifest_ops"] = manifest["operations"]
        op_type = subop.get("type")
        if op_type not in OPERATIONS:
            raise ValueError("%s operation %d has unknown type %r" % (op["source"], index, op_type))
        if OPERATIONS[op_type](subop, data_dir, write):
            changed += 1
    print("  %d included operation(s) changed data" % changed)
    return changed != 0


OPERATIONS = {
    "apply_decor_edits": op_apply_decor_edits,
    "apply_terrain_edits": op_apply_terrain_edits,
    "clone_scene": op_clone_scene,
    "copy_file": op_copy_file,
    "create_blank_island": op_create_blank_island,
    "include_manifest": op_include_manifest,
    "set_ile_object": op_set_ile_object,
    "set_scene_zones": op_set_scene_zones,
    "set_scene_header": op_set_scene_header,
    "remap_scene_zone_nums": op_remap_scene_zone_nums,
    "add_text": op_add_text,
    "replace_text": op_replace_text,
    "add_voice": op_add_voice,
    "replace_voice": op_replace_voice,
    "copy_hqr_entry": op_copy_hqr_entry,
    "replace_hqr_entry": op_replace_hqr_entry,
    "add_hqr_entry": op_add_hqr_entry,
    "set_terrain_heights": op_set_terrain_heights,
    "set_terrain_intensities": op_set_terrain_intensities,
    "set_terrain_triangles": op_set_terrain_triangles,
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
