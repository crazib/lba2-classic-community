#!/usr/bin/env python3
"""Inspect and patch scene trigger zones in LBA2 SCENE.HQR.

This decodes the scene blob layout consumed by SOURCES/DISKFUNC.CPP
LoadScene(), then reads the T_ZONE array from SOURCES/COMMON.H. It is meant
for targeted exterior-editing work, not for dumping entire scenes by default.
"""

import argparse
import json
import os
import signal
import struct
import sys

import hqr_inspect


ANIM_3DS = 1 << 18
T_ZONE_SIZE = 60
T_ZONE_STRUCT = "<14ihh"


def read_s8(raw, pos):
    return struct.unpack_from("<b", raw, pos)[0], pos + 1


def read_s16(raw, pos):
    return struct.unpack_from("<h", raw, pos)[0], pos + 2


def read_u32(raw, pos):
    return struct.unpack_from("<I", raw, pos)[0], pos + 4


def parse_int_list(text, optname):
    out = []
    try:
        for part in text.split(","):
            part = part.strip()
            if part:
                out.append(int(part, 0))
    except ValueError:
        raise ValueError("%s must be a comma-separated integer list" % optname)
    return out


def zone_dir(info2):
    return {
        1: "N",
        2: "S",
        4: "E",
        8: "W",
    }.get(info2, "-")


def decode_zone(raw, offset, index):
    values = struct.unpack_from(T_ZONE_STRUCT, raw, offset)
    zone = {
        "index": index,
        "offset": offset,
        "x0": values[0],
        "y0": values[1],
        "z0": values[2],
        "x1": values[3],
        "y1": values[4],
        "z1": values[5],
        "info0": values[6],
        "info1": values[7],
        "info2": values[8],
        "info3": values[9],
        "info4": values[10],
        "info5": values[11],
        "info6": values[12],
        "info7": values[13],
        "type": values[14],
        "num": values[15],
    }
    zone["dir"] = zone_dir(zone["info2"]) if zone["type"] == 5 else ""
    return zone


def encode_zone(raw, zone):
    values = (
        zone["x0"],
        zone["y0"],
        zone["z0"],
        zone["x1"],
        zone["y1"],
        zone["z1"],
        zone["info0"],
        zone["info1"],
        zone["info2"],
        zone["info3"],
        zone["info4"],
        zone["info5"],
        zone["info6"],
        zone["info7"],
        zone["type"],
        zone["num"],
    )
    struct.pack_into(T_ZONE_STRUCT, raw, zone["offset"], *values)


def parse_scene(raw, scene_num, entry_num):
    pos = 0

    island, pos = read_s8(raw, pos)
    cube_x, pos = read_s8(raw, pos)
    cube_y, pos = read_s8(raw, pos)
    shadow_level, pos = read_s8(raw, pos)
    mode_labyrinthe, pos = read_s8(raw, pos)
    cube_mode, pos = read_s8(raw, pos)
    _ambiance_count, pos = read_s8(raw, pos)

    for _i in range(24):
        _v, pos = read_s16(raw, pos)

    cube_jingle, pos = read_s8(raw, pos)

    cube_start_x, pos = read_s16(raw, pos)
    cube_start_y, pos = read_s16(raw, pos)
    cube_start_z, pos = read_s16(raw, pos)

    track_size, pos = read_s16(raw, pos)
    pos += track_size

    life_size, pos = read_s16(raw, pos)
    pos += life_size

    nb_objects, pos = read_s16(raw, pos)
    for _obj in range(1, nb_objects):
        flags, pos = read_u32(raw, pos)
        _index_file_3d, pos = read_s16(raw, pos)

        _gen_body, pos = read_s8(raw, pos)
        _gen_anim, pos = read_s16(raw, pos)
        _sprite, pos = read_s16(raw, pos)

        for _i in range(3):
            _v, pos = read_s16(raw, pos)

        _hit_force, pos = read_s8(raw, pos)
        _option_flags, pos = read_s16(raw, pos)
        _beta, pos = read_s16(raw, pos)
        _srot, pos = read_s16(raw, pos)
        _move, pos = read_s8(raw, pos)

        for _i in range(4):
            _v, pos = read_s16(raw, pos)

        _nb_bonus, pos = read_s16(raw, pos)
        _coul_obj, pos = read_s8(raw, pos)

        if flags & ANIM_3DS:
            _anim_3ds_num, pos = read_u32(raw, pos)
            _nb_fps, pos = read_s16(raw, pos)

        _armure, pos = read_s8(raw, pos)
        _life_point, pos = read_s8(raw, pos)

        track_size, pos = read_s16(raw, pos)
        pos += track_size

        life_size, pos = read_s16(raw, pos)
        pos += life_size

    checksum, pos = read_u32(raw, pos)
    nb_zones, pos = read_s16(raw, pos)
    zone_offset = pos

    if nb_zones < 0:
        raise ValueError("negative zone count in scene %d" % scene_num)
    if zone_offset + nb_zones * T_ZONE_SIZE > len(raw):
        raise ValueError("zone block runs past end of scene %d" % scene_num)

    zones = []
    for index in range(nb_zones):
        zones.append(decode_zone(raw, zone_offset + index * T_ZONE_SIZE, index))

    return {
        "scene": scene_num,
        "entry": entry_num,
        "island": island,
        "cube_x": cube_x,
        "cube_y": cube_y,
        "shadow_level": shadow_level,
        "mode_labyrinthe": mode_labyrinthe,
        "cube_mode": cube_mode,
        "cube_jingle": cube_jingle,
        "cube_start_x": cube_start_x,
        "cube_start_y": cube_start_y,
        "cube_start_z": cube_start_z,
        "objects": nb_objects,
        "checksum": checksum,
        "zone_offset": zone_offset,
        "zones": zones,
    }


def load_scene(path, scene_num):
    entry_num = scene_num + 1
    ents, data = hqr_inspect.entries(path)
    if entry_num < 0 or entry_num >= len(ents):
        raise ValueError("scene %d maps to missing HQR entry %d" % (scene_num, entry_num))

    _i, off, size, csize, method = ents[entry_num]
    if size is None:
        raise ValueError("scene %d maps to empty HQR entry %d" % (scene_num, entry_num))

    raw = hqr_inspect.decompress_entry(data, off, size, csize, method)
    scene = parse_scene(raw, scene_num, entry_num)
    return ents, data, bytearray(raw), scene


def rewrite_entry_stored(path, ents, data, entry_num, payload):
    new_entries = []
    for index, ent in enumerate(ents):
        _i, off, size, csize, _method = ent
        if size is None:
            new_entries.append(None)
        elif index == entry_num:
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


def selected_zones(scene, zone_filter, type_filter, num_filter):
    zones = scene["zones"]
    if zone_filter is not None:
        wanted = set(zone_filter)
        zones = [z for z in zones if z["index"] in wanted]
    if type_filter is not None:
        zones = [z for z in zones if z["type"] == type_filter]
    if num_filter is not None:
        zones = [z for z in zones if z["num"] == num_filter]
    return zones


def zone_row(zone):
    suffix = ""
    if zone["type"] == 5:
        suffix = " d%s" % zone["dir"]
    return (
        "z%(index)d t%(type)d n%(num)d%(suffix)s  "
        "[%(x0)d,%(y0)d,%(z0)d]-[%(x1)d,%(y1)d,%(z1)d]  "
        "info %(info0)d,%(info1)d,%(info2)d,%(info3)d,%(info4)d,%(info5)d,%(info6)d,%(info7)d"
        % dict(zone, suffix=suffix)
    )


def format_scene(scene, zones):
    print(
        "scene %(scene)d entry %(entry)d island %(island)d cube %(cube_x)d,%(cube_y)d "
        "mode %(cube_mode)d objects %(objects)d zones %(count)d zone_off %(zone_offset)d"
        % dict(scene, count=len(scene["zones"]))
    )
    for zone in zones:
        print(zone_row(zone))


def find_zones(path, type_filter, num_filter, zone_filter, limit):
    ents, data = hqr_inspect.entries(path)
    found = 0

    for entry_num in range(1, len(ents)):
        _i, off, size, csize, method = ents[entry_num]
        if size is None:
            continue

        try:
            raw = hqr_inspect.decompress_entry(data, off, size, csize, method)
            scene = parse_scene(raw, entry_num - 1, entry_num)
        except Exception:
            continue

        zones = selected_zones(scene, zone_filter, type_filter, num_filter)
        if not zones:
            continue

        print(
            "scene %(scene)d entry %(entry)d island %(island)d cube %(cube_x)d,%(cube_y)d "
            "mode %(cube_mode)d objects %(objects)d zones %(count)d"
            % dict(scene, count=len(scene["zones"]))
        )
        for zone in zones:
            print("  " + zone_row(zone))
            found += 1
            if limit and found >= limit:
                return


def move_zones(raw, scene, zone_ids, dx, dy, dz):
    changed = []
    by_index = dict((z["index"], z) for z in scene["zones"])
    for zone_id in zone_ids:
        if zone_id not in by_index:
            raise ValueError("zone %d is not present in scene %d" % (zone_id, scene["scene"]))

        zone = dict(by_index[zone_id])
        before = dict(zone)
        zone["x0"] += dx
        zone["x1"] += dx
        zone["y0"] += dy
        zone["y1"] += dy
        zone["z0"] += dz
        zone["z1"] += dz
        encode_zone(raw, zone)
        changed.append((before, zone))

    return changed


def set_zone_nums(raw, scene, zone_ids, new_num):
    changed = []
    by_index = dict((z["index"], z) for z in scene["zones"])
    for zone_id in zone_ids:
        if zone_id not in by_index:
            raise ValueError("zone %d is not present in scene %d" % (zone_id, scene["scene"]))

        zone = dict(by_index[zone_id])
        before = dict(zone)
        zone["num"] = new_num
        encode_zone(raw, zone)
        changed.append((before, zone))

    return changed


def main():
    signal.signal(signal.SIGPIPE, signal.SIG_DFL)

    ap = argparse.ArgumentParser()
    ap.add_argument("scene_hqr", help="Path to SCENE.HQR")
    ap.add_argument("--scene", type=int, help="Scene/cube number. HQR entry is scene + 1.")
    ap.add_argument("--zone", help="Filter zones by index list, for example 2,3,4")
    ap.add_argument("--type", type=int, help="Filter by zone Type")
    ap.add_argument("--num", type=int, help="Filter by zone Num")
    ap.add_argument("--find", action="store_true", help="Scan all scenes with the filters")
    ap.add_argument("--json", action="store_true", help="Emit JSON for a single-scene listing")
    ap.add_argument("--limit", type=int, default=0, help="Limit --find zone rows; 0 = no limit")
    ap.add_argument("--move-zones", help="Comma-separated zone indexes to move")
    ap.add_argument("--dx", type=int, default=0, help="Move X0/X1 by this amount")
    ap.add_argument("--dy", type=int, default=0, help="Move Y0/Y1 by this amount")
    ap.add_argument("--dz", type=int, default=0, help="Move Z0/Z1 by this amount")
    ap.add_argument("--set-num-zones", help="Comma-separated zone indexes whose Num field should be replaced")
    ap.add_argument("--set-num", type=int, help="Replacement Num value for --set-num-zones")
    ap.add_argument("--write", action="store_true", help="Write the moved scene back to SCENE.HQR")
    args = ap.parse_args()

    try:
        zone_filter = parse_int_list(args.zone, "--zone") if args.zone else None
        move_filter = parse_int_list(args.move_zones, "--move-zones") if args.move_zones else None
        set_num_filter = parse_int_list(args.set_num_zones, "--set-num-zones") if args.set_num_zones else None
    except ValueError as exc:
        print(str(exc), file=sys.stderr)
        return 2

    if args.find:
        find_zones(args.scene_hqr, args.type, args.num, zone_filter, args.limit)
        return 0

    if args.scene is None:
        print("--scene is required unless --find is used", file=sys.stderr)
        return 2

    try:
        ents, data, raw, scene = load_scene(args.scene_hqr, args.scene)
    except Exception as exc:
        print("failed to parse scene: %s" % exc, file=sys.stderr)
        return 1

    if move_filter is not None:
        try:
            changed = move_zones(raw, scene, move_filter, args.dx, args.dy, args.dz)
        except ValueError as exc:
            print(str(exc), file=sys.stderr)
            return 2

        for before, after in changed:
            print("move z%d:" % before["index"])
            print("  before " + zone_row(before))
            print("  after  " + zone_row(after))

        if args.write:
            old_size, new_size = rewrite_entry_stored(
                args.scene_hqr, ents, data, scene["entry"], bytes(raw)
            )
            print(
                "wrote %s scene %d entry %d as stored data (%d -> %d bytes)"
                % (args.scene_hqr, scene["scene"], scene["entry"], old_size, new_size)
            )
        else:
            print("dry run only; add --write to update %s" % args.scene_hqr)
        return 0

    if set_num_filter is not None:
        if args.set_num is None:
            print("--set-num-zones requires --set-num", file=sys.stderr)
            return 2
        try:
            changed = set_zone_nums(raw, scene, set_num_filter, args.set_num)
        except ValueError as exc:
            print(str(exc), file=sys.stderr)
            return 2

        for before, after in changed:
            print("set num z%d:" % before["index"])
            print("  before " + zone_row(before))
            print("  after  " + zone_row(after))

        if args.write:
            old_size, new_size = rewrite_entry_stored(
                args.scene_hqr, ents, data, scene["entry"], bytes(raw)
            )
            print(
                "wrote %s scene %d entry %d as stored data (%d -> %d bytes)"
                % (args.scene_hqr, scene["scene"], scene["entry"], old_size, new_size)
            )
        else:
            print("dry run only; add --write to update %s" % args.scene_hqr)
        return 0

    zones = selected_zones(scene, zone_filter, args.type, args.num)
    if args.json:
        out = dict(scene)
        out["zones"] = zones
        print(json.dumps(out, indent=2, sort_keys=True))
    else:
        format_scene(scene, zones)

    return 0


if __name__ == "__main__":
    sys.exit(main())
