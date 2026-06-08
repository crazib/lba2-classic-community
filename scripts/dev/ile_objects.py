#!/usr/bin/env python3
"""Inspect exterior island placed objects from an LBA2 .ILE file.

This is intentionally not a full island dump. It decodes only the DOB chunks
loaded as T_DECORS in SOURCES/3DEXT/LOADISLE.CPP, which are the fixed exterior
object placements rendered by SOURCES/3DEXT/DECORS.CPP.
"""

import argparse
import json
import os
import signal
import struct
import sys

import hqr_inspect


HQR_MAP_IDM = 0
HQR_START_CUBE = 3
HQR_CUBE_INF = 0
HQR_CUBE_DOB = 1
HQR_STEP_CUBE = 6

SIZE_MAIN_MAP = 16
WORLD_SIZE = 32768
T_DECORS_SIZE = 48


def entry_bytes(path, index):
    ents, data = hqr_inspect.entries(path)
    if index < 0 or index >= len(ents):
        return None

    _i, off, size, csize, method = ents[index]
    if size is None:
        return None

    return hqr_inspect.decompress_entry(data, off, size, csize, method)


def rewrite_entry_stored(path, entry_index, payload):
    ents, data = hqr_inspect.entries(path)
    if entry_index < 0 or entry_index >= len(ents):
        raise ValueError("entry %d is out of range" % entry_index)

    new_entries = []
    for index, ent in enumerate(ents):
        _i, off, size, csize, _method = ent
        if size is None:
            new_entries.append(None)
        elif index == entry_index:
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


def decode_s32_list(raw):
    if len(raw) % 4 != 0:
        raise ValueError("S32 list has non-multiple-of-4 size")
    return list(struct.unpack("<%di" % (len(raw) // 4), raw))


def load_cube_map(path):
    raw = entry_bytes(path, HQR_MAP_IDM)
    if raw is None:
        raise ValueError("missing IDM map entry 0")
    if len(raw) != SIZE_MAIN_MAP * SIZE_MAIN_MAP:
        raise ValueError("unexpected IDM map size %d" % len(raw))

    cubes = {}
    for y in range(SIZE_MAIN_MAP):
        for x in range(SIZE_MAIN_MAP):
            slot = raw[y * SIZE_MAIN_MAP + x] & 0x7F
            if slot:
                cubes.setdefault(slot, []).append((x, y))
    return cubes


def decode_decor(raw, offset):
    fields = struct.unpack_from("<12i", raw, offset)
    body, x, y, z, code_jeu, beta, xmin, ymin, zmin, xmax, ymax, zmax = fields
    body_id = body & 0xFFFF
    body_flags = (body >> 16) & 0xFFFF
    angle = beta & 0xFFFF
    visibility_var = beta >> 16

    return {
        "body_raw": body,
        "body_id": body_id,
        "body_flags": body_flags,
        "x": x,
        "y": y,
        "z": z,
        "code_jeu": code_jeu,
        "beta_raw": beta,
        "angle": angle,
        "angle_deg": angle * 360.0 / 4096.0,
        "visibility_var": visibility_var,
        "zv": {
            "xmin": xmin,
            "ymin": ymin,
            "zmin": zmin,
            "xmax": xmax,
            "ymax": ymax,
            "zmax": zmax,
        },
    }


def encode_decor(raw, offset, obj):
    struct.pack_into(
        "<12i",
        raw,
        offset,
        obj["body_raw"],
        obj["x"],
        obj["y"],
        obj["z"],
        obj["code_jeu"],
        obj["beta_raw"],
        obj["zv"]["xmin"],
        obj["zv"]["ymin"],
        obj["zv"]["zmin"],
        obj["zv"]["xmax"],
        obj["zv"]["ymax"],
        obj["zv"]["zmax"],
    )


def iter_objects(path):
    cube_map = load_cube_map(path)
    objects = []

    for cube_slot in sorted(cube_map):
        base = HQR_START_CUBE + HQR_STEP_CUBE * (cube_slot - 1)
        info_raw = entry_bytes(path, base + HQR_CUBE_INF)
        dob_raw = entry_bytes(path, base + HQR_CUBE_DOB)

        if info_raw is None:
            continue

        info = decode_s32_list(info_raw)
        nb_decors = info[2] if len(info) > 2 else 0
        if dob_raw is None:
            actual_decors = 0
        else:
            if len(dob_raw) % T_DECORS_SIZE != 0:
                raise ValueError(
                    "cube slot %d DOB size %d is not a multiple of %d"
                    % (cube_slot, len(dob_raw), T_DECORS_SIZE)
                )
            actual_decors = len(dob_raw) // T_DECORS_SIZE

        for map_x, map_y in cube_map[cube_slot]:
            for i in range(actual_decors):
                obj = decode_decor(dob_raw, i * T_DECORS_SIZE)
                obj["island_file"] = os.path.basename(path)
                obj["cube_slot"] = cube_slot
                obj["cube_x"] = map_x
                obj["cube_y"] = map_y
                obj["object_index"] = i
                obj["declared_decors"] = nb_decors
                obj["global_x"] = map_x * WORLD_SIZE + obj["x"]
                obj["global_z"] = map_y * WORLD_SIZE + obj["z"]
                objects.append(obj)

    return objects


def format_table(objects, limit):
    rows = objects if limit == 0 else objects[:limit]
    print(
        "idx cube slot body   local_x local_y local_z  global_x global_z  angle  vis  zv"
    )
    for obj in rows:
        zv = obj["zv"]
        print(
            "%3d %2d,%2d %4d %4d %8d %7d %7d %9d %8d %6.1f %4d  [%d,%d,%d]-[%d,%d,%d]"
            % (
                obj["object_index"],
                obj["cube_x"],
                obj["cube_y"],
                obj["cube_slot"],
                obj["body_id"],
                obj["x"],
                obj["y"],
                obj["z"],
                obj["global_x"],
                obj["global_z"],
                obj["angle_deg"],
                obj["visibility_var"],
                zv["xmin"],
                zv["ymin"],
                zv["zmin"],
                zv["xmax"],
                zv["ymax"],
                zv["zmax"],
            )
        )

    if limit and len(objects) > limit:
        print("... %d more" % (len(objects) - limit))


def format_cubes(path):
    cube_map = load_cube_map(path)
    print("%s: %d used exterior map cells" % (path, sum(len(v) for v in cube_map.values())))
    print("cube   slot")
    for slot in sorted(cube_map):
        for x, y in cube_map[slot]:
            print("%2d,%2d  %4d" % (x, y, slot))


def object_summary(obj):
    zv = obj["zv"]
    return (
        "idx %(object_index)d c%(cube_x)d,%(cube_y)d slot %(cube_slot)d body %(body_id)d "
        "local [%(x)d,%(y)d,%(z)d] "
        "zv [%(xmin)d,%(ymin)d,%(zmin)d]-[%(xmax)d,%(ymax)d,%(zmax)d]"
        % dict(
            obj,
            xmin=zv["xmin"],
            ymin=zv["ymin"],
            zmin=zv["zmin"],
            xmax=zv["xmax"],
            ymax=zv["ymax"],
            zmax=zv["zmax"],
        )
    )


def move_object(path, cube_text, object_index, dx, dy, dz, write):
    if cube_text is None:
        raise ValueError("--cube X,Y is required with --move-object")
    if object_index is None:
        raise ValueError("--move-object requires --object <index>")

    try:
        cube_x, cube_y = [int(v) for v in cube_text.split(",", 1)]
    except ValueError:
        raise ValueError("--cube must be X,Y")

    cube_map = load_cube_map(path)
    cube_slot = None
    for slot, cells in cube_map.items():
        if (cube_x, cube_y) in cells:
            cube_slot = slot
            break
    if cube_slot is None:
        raise ValueError("cube %d,%d is not present in %s" % (cube_x, cube_y, path))

    entry_index = HQR_START_CUBE + HQR_STEP_CUBE * (cube_slot - 1) + HQR_CUBE_DOB
    dob_raw = entry_bytes(path, entry_index)
    if dob_raw is None:
        raise ValueError("cube %d,%d slot %d has no DOB entry" % (cube_x, cube_y, cube_slot))
    if len(dob_raw) % T_DECORS_SIZE != 0:
        raise ValueError("DOB entry %d has invalid size %d" % (entry_index, len(dob_raw)))

    count = len(dob_raw) // T_DECORS_SIZE
    if object_index < 0 or object_index >= count:
        raise ValueError("object index %d is out of range [0,%d)" % (object_index, count))

    raw = bytearray(dob_raw)
    offset = object_index * T_DECORS_SIZE
    before = decode_decor(raw, offset)
    before["cube_slot"] = cube_slot
    before["cube_x"] = cube_x
    before["cube_y"] = cube_y
    before["object_index"] = object_index

    after = json.loads(json.dumps(before))
    after["x"] += dx
    after["y"] += dy
    after["z"] += dz
    after["zv"]["xmin"] += dx
    after["zv"]["xmax"] += dx
    after["zv"]["ymin"] += dy
    after["zv"]["ymax"] += dy
    after["zv"]["zmin"] += dz
    after["zv"]["zmax"] += dz
    encode_decor(raw, offset, after)

    print("move object:")
    print("  before " + object_summary(before))
    print("  after  " + object_summary(after))

    if write:
        old_size, new_size = rewrite_entry_stored(path, entry_index, bytes(raw))
        print(
            "wrote %s DOB entry %d as stored data (%d -> %d bytes)"
            % (path, entry_index, old_size, new_size)
        )
    else:
        print("dry run only; add --write to update %s" % path)


def main():
    signal.signal(signal.SIGPIPE, signal.SIG_DFL)

    ap = argparse.ArgumentParser()
    ap.add_argument("ile", help="Path to an exterior island .ILE file")
    ap.add_argument("--json", action="store_true", help="Emit JSON instead of a table")
    ap.add_argument("--limit", type=int, default=40, help="Table row limit; 0 = no limit")
    ap.add_argument("--cube", help="Filter to map cube X,Y, for example 14,22")
    ap.add_argument("--body", type=int, help="Filter to a placed object body id")
    ap.add_argument("--list-cubes", action="store_true", help="List used 16x16 island map cells")
    ap.add_argument("--move-object", action="store_true", help="Move one placed object; requires --cube and --object")
    ap.add_argument("--object", type=int, help="Placed object index within the cube DOB")
    ap.add_argument("--dx", type=int, default=0, help="Move X and X ZV bounds by this amount")
    ap.add_argument("--dy", type=int, default=0, help="Move Y and Y ZV bounds by this amount")
    ap.add_argument("--dz", type=int, default=0, help="Move Z and Z ZV bounds by this amount")
    ap.add_argument("--write", action="store_true", help="Write the moved object back to the .ILE")
    args = ap.parse_args()

    if args.list_cubes:
        format_cubes(args.ile)
        return 0

    if args.move_object:
        try:
            move_object(args.ile, args.cube, args.object, args.dx, args.dy, args.dz, args.write)
        except ValueError as exc:
            print(str(exc), file=sys.stderr)
            return 2
        return 0

    objects = iter_objects(args.ile)

    if args.cube:
        try:
            cube_x, cube_y = [int(v) for v in args.cube.split(",", 1)]
        except ValueError:
            print("--cube must be X,Y", file=sys.stderr)
            return 2
        objects = [o for o in objects if o["cube_x"] == cube_x and o["cube_y"] == cube_y]

    if args.body is not None:
        objects = [o for o in objects if o["body_id"] == args.body]

    if args.json:
        rows = objects if args.limit == 0 else objects[: args.limit]
        print(json.dumps(rows, indent=2, sort_keys=True))
    else:
        print("%s: %d placed exterior objects" % (args.ile, len(objects)))
        format_table(objects, args.limit)

    return 0


if __name__ == "__main__":
    sys.exit(main())
