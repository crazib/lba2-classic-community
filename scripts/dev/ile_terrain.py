#!/usr/bin/env python3
"""Inspect and patch exterior terrain heights in an LBA2 .ILE file."""

import argparse
import signal
import struct
import sys

import hqr_inspect
import ile_objects


HQR_CUBE_Y = 4
HQR_STEP_CUBE = 6
HQR_START_CUBE = 3
HEIGHT_SIDE = 65
HEIGHT_COUNT = HEIGHT_SIDE * HEIGHT_SIDE


def parse_cube(text):
    try:
        x, y = [int(v) for v in text.split(",", 1)]
    except ValueError:
        raise ValueError("--cube must be X,Y")
    return x, y


def parse_vertex(text):
    try:
        x, z = [int(v) for v in text.split(",", 1)]
    except ValueError:
        raise ValueError("vertex must be X,Z")
    if x < 0 or x >= HEIGHT_SIDE or z < 0 or z >= HEIGHT_SIDE:
        raise ValueError("vertex must be in [0,64],[0,64]")
    return x, z


def parse_set_height(text):
    try:
        x, z, height = [int(v) for v in text.split(",", 2)]
    except ValueError:
        raise ValueError("--set-height must be X,Z,H")
    if x < 0 or x >= HEIGHT_SIDE or z < 0 or z >= HEIGHT_SIDE:
        raise ValueError("vertex must be in [0,64],[0,64]")
    return x, z, height


def rewrite_entry_stored(path, entry_index, payload):
    ents, data = hqr_inspect.entries(path)
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


def cube_slot(path, cube_x, cube_y):
    cube_map = ile_objects.load_cube_map(path)
    for slot, cells in cube_map.items():
        if (cube_x, cube_y) in cells:
            return slot
    raise ValueError("cube %d,%d is not present in %s" % (cube_x, cube_y, path))


def load_heights(path, cube_x, cube_y):
    slot = cube_slot(path, cube_x, cube_y)
    entry_index = HQR_START_CUBE + HQR_STEP_CUBE * (slot - 1) + HQR_CUBE_Y
    raw = ile_objects.entry_bytes(path, entry_index)
    if raw is None:
        raise ValueError("cube %d,%d slot %d has no Y entry" % (cube_x, cube_y, slot))
    if len(raw) != HEIGHT_COUNT * 2:
        raise ValueError("Y entry %d has unexpected size %d" % (entry_index, len(raw)))
    heights = list(struct.unpack("<%dh" % HEIGHT_COUNT, raw))
    return slot, entry_index, bytearray(raw), heights


def print_patch(heights, center_x, center_z, radius):
    x0 = max(0, center_x - radius)
    x1 = min(HEIGHT_SIDE - 1, center_x + radius)
    z0 = max(0, center_z - radius)
    z1 = min(HEIGHT_SIDE - 1, center_z + radius)

    print("      " + " ".join("%5d" % x for x in range(x0, x1 + 1)))
    for z in range(z0, z1 + 1):
        row = [heights[z * HEIGHT_SIDE + x] for x in range(x0, x1 + 1)]
        print("z%02d: " % z + " ".join("%5d" % v for v in row))


def main():
    signal.signal(signal.SIGPIPE, signal.SIG_DFL)

    ap = argparse.ArgumentParser()
    ap.add_argument("ile", help="Path to exterior .ILE file")
    ap.add_argument("--cube", required=True, help="Island map cube X,Y")
    ap.add_argument("--height", help="Print one height vertex X,Z")
    ap.add_argument("--patch", help="Print a square patch centered at X,Z")
    ap.add_argument("--radius", type=int, default=2, help="Patch print radius")
    ap.add_argument("--set-height", action="append", default=[], help="Set one vertex: X,Z,H")
    ap.add_argument("--plus", help="Set a plus shape: X,Z,H,RADIUS")
    ap.add_argument("--write", action="store_true", help="Write changes back to the .ILE")
    args = ap.parse_args()

    try:
        cube_x, cube_y = parse_cube(args.cube)
        slot, entry_index, raw, heights = load_heights(args.ile, cube_x, cube_y)
    except ValueError as exc:
        print(str(exc), file=sys.stderr)
        return 2

    print("cube %d,%d slot %d Y entry %d" % (cube_x, cube_y, slot, entry_index))

    if args.height:
        try:
            x, z = parse_vertex(args.height)
        except ValueError as exc:
            print(str(exc), file=sys.stderr)
            return 2
        print("height %d,%d = %d" % (x, z, heights[z * HEIGHT_SIDE + x]))

    if args.patch:
        try:
            x, z = parse_vertex(args.patch)
        except ValueError as exc:
            print(str(exc), file=sys.stderr)
            return 2
        print_patch(heights, x, z, args.radius)

    changes = []
    try:
        for spec in args.set_height:
            changes.append(parse_set_height(spec))
        if args.plus:
            parts = [int(v) for v in args.plus.split(",")]
            if len(parts) != 4:
                raise ValueError("--plus must be X,Z,H,RADIUS")
            cx, cz, height, radius = parts
            if radius < 0:
                raise ValueError("--plus radius must be >= 0")
            for delta in range(-radius, radius + 1):
                changes.append((cx + delta, cz, height))
                changes.append((cx, cz + delta, height))
    except ValueError as exc:
        print(str(exc), file=sys.stderr)
        return 2

    if changes:
        seen = set()
        for x, z, height in changes:
            if x < 0 or x >= HEIGHT_SIDE or z < 0 or z >= HEIGHT_SIDE:
                print("vertex %d,%d is outside [0,64],[0,64]" % (x, z), file=sys.stderr)
                return 2
            key = (x, z)
            if key in seen:
                continue
            seen.add(key)
            idx = z * HEIGHT_SIDE + x
            old = heights[idx]
            heights[idx] = height
            struct.pack_into("<h", raw, idx * 2, height)
            print("set %d,%d: %d -> %d" % (x, z, old, height))

        if args.write:
            old_size, new_size = rewrite_entry_stored(args.ile, entry_index, bytes(raw))
            print(
                "wrote %s Y entry %d as stored data (%d -> %d bytes)"
                % (args.ile, entry_index, old_size, new_size)
            )
        else:
            print("dry run only; add --write to update %s" % args.ile)

    return 0


if __name__ == "__main__":
    sys.exit(main())
