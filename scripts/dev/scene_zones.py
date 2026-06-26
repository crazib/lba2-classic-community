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


def read_bytes(raw, pos, size):
    if size < 0 or pos + size > len(raw):
        raise ValueError("scene actor script runs past the end of the scene")
    return bytes(raw[pos : pos + size]), pos + size


def signed_byte(value):
    value = int(value)
    if value < -128 or value > 255:
        raise ValueError("value %d does not fit in a scene byte" % value)
    return value - 256 if value > 127 else value


def signed_word(value):
    value = int(value)
    if value < -32768 or value > 65535:
        raise ValueError("value %d does not fit in a scene word" % value)
    return value - 65536 if value > 32767 else value


def decode_actor(raw, pos, index):
    start = pos
    flags, pos = read_u32(raw, pos)
    model, pos = read_s16(raw, pos)
    body, pos = read_s8(raw, pos)
    animation, pos = read_s16(raw, pos)
    sprite, pos = read_s16(raw, pos)
    x, pos = read_s16(raw, pos)
    y, pos = read_s16(raw, pos)
    z, pos = read_s16(raw, pos)
    hit_force, pos = read_s8(raw, pos)
    option_flags, pos = read_s16(raw, pos)
    beta, pos = read_s16(raw, pos)
    srot, pos = read_s16(raw, pos)
    move, pos = read_s8(raw, pos)
    info = []
    for _i in range(4):
        value, pos = read_s16(raw, pos)
        info.append(value)
    bonus, pos = read_s16(raw, pos)
    color, pos = read_s8(raw, pos)
    anim_3ds_num = None
    anim_3ds_fps = None
    if flags & ANIM_3DS:
        anim_3ds_num, pos = read_u32(raw, pos)
        anim_3ds_fps, pos = read_s16(raw, pos)
    armour, pos = read_s8(raw, pos)
    life, pos = read_s8(raw, pos)
    track_size, pos = read_s16(raw, pos)
    track, pos = read_bytes(raw, pos, track_size)
    life_size, pos = read_s16(raw, pos)
    life_script, pos = read_bytes(raw, pos, life_size)
    return {
        "index": index,
        "offset": start,
        "end": pos,
        "flags": flags,
        "model": model,
        "body": body & 0xFF,
        "animation": animation & 0xFFFF,
        "sprite": sprite,
        "x": x,
        "y": y,
        "z": z,
        "hit_force": hit_force & 0xFF,
        "option_flags": option_flags,
        "beta": beta,
        "srot": srot,
        "move": move & 0xFF,
        "info": info,
        "bonus": bonus,
        "color": color & 0xFF,
        "anim_3ds_num": anim_3ds_num,
        "anim_3ds_fps": anim_3ds_fps,
        "armour": armour & 0xFF,
        "life": life & 0xFF,
        "track": track,
        "life_script": life_script,
    }, pos


def encode_actor(actor):
    out = bytearray()
    out.extend(struct.pack("<Ihbhhhhhbhhhb", actor["flags"], actor["model"],
                           signed_byte(actor["body"]), signed_word(actor["animation"]), actor["sprite"],
                           actor["x"], actor["y"], actor["z"],
                           signed_byte(actor["hit_force"]), actor["option_flags"],
                           actor["beta"], actor["srot"], signed_byte(actor["move"])))
    out.extend(struct.pack("<4h", *actor["info"]))
    out.extend(struct.pack("<hb", actor["bonus"], signed_byte(actor["color"])))
    if actor["flags"] & ANIM_3DS:
        out.extend(struct.pack("<Ih", actor["anim_3ds_num"], actor["anim_3ds_fps"]))
    out.extend(struct.pack("<bb", signed_byte(actor["armour"]), signed_byte(actor["life"])))
    out.extend(struct.pack("<h", len(actor["track"])))
    out.extend(actor["track"])
    out.extend(struct.pack("<h", len(actor["life_script"])))
    out.extend(actor["life_script"])
    return bytes(out)


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

    objects_count_offset = pos
    nb_objects, pos = read_s16(raw, pos)
    actors_offset = pos
    actors = []
    for actor_index in range(1, nb_objects):
        actor, pos = decode_actor(raw, pos, actor_index)
        actors.append(actor)

    checksum_offset = pos
    checksum, pos = read_u32(raw, pos)
    zone_count_offset = pos
    nb_zones, pos = read_s16(raw, pos)
    zone_offset = pos

    if nb_zones < 0:
        raise ValueError("negative zone count in scene %d" % scene_num)
    if zone_offset + nb_zones * T_ZONE_SIZE > len(raw):
        raise ValueError("zone block runs past end of scene %d" % scene_num)

    zones = []
    for index in range(nb_zones):
        zones.append(decode_zone(raw, zone_offset + index * T_ZONE_SIZE, index))
    zone_end = zone_offset + nb_zones * T_ZONE_SIZE
    pos = zone_end

    track_count_offset = pos
    track_count, pos = read_s16(raw, pos)
    if track_count < 0:
        raise ValueError("negative waypoint count in scene %d" % scene_num)
    tracks_offset = pos
    tracks = []
    for index in range(track_count):
        x, y, z = struct.unpack_from("<iii", raw, pos)
        tracks.append({"index": index, "x": x, "y": y, "z": z})
        pos += 12
    tracks_end = pos

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
        "objects_count_offset": objects_count_offset,
        "actors_offset": actors_offset,
        "actors_end": checksum_offset,
        "actors": actors,
        "checksum": checksum,
        "zone_count_offset": zone_count_offset,
        "zone_offset": zone_offset,
        "zone_end": zone_end,
        "zones": zones,
        "track_count_offset": track_count_offset,
        "tracks_offset": tracks_offset,
        "tracks_end": tracks_end,
        "tracks": tracks,
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
