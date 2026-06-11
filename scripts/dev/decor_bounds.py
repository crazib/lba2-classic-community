#!/usr/bin/env python3
"""Infer exterior decor body bounds from placed island objects.

Placed exterior decor is stored in island .ILE DOB chunks.
"""

import argparse
import json
import os
import signal
import sys

import hqr_inspect
import ile_objects


EXTS = (".ile",)


def input_files(paths, recursive):
    out = []
    for path in paths:
        if os.path.isdir(path):
            if recursive:
                for root, _dirs, files in os.walk(path):
                    for name in files:
                        if name.lower().endswith(EXTS):
                            out.append(os.path.join(root, name))
            else:
                for name in os.listdir(path):
                    full = os.path.join(path, name)
                    if os.path.isfile(full) and name.lower().endswith(EXTS):
                        out.append(full)
        else:
            out.append(path)
    return sorted(out)


def rel_bounds(obj):
    zv = obj["zv"]
    return [
        zv["xmin"] - obj["x"],
        zv["ymin"] - obj["y"],
        zv["zmin"] - obj["z"],
        zv["xmax"] - obj["x"],
        zv["ymax"] - obj["y"],
        zv["zmax"] - obj["z"],
    ]


def rotate_xz(x, z, quarter_turns):
    quarter_turns %= 4
    if quarter_turns == 0:
        return x, z
    if quarter_turns == 1:
        return z, -x
    if quarter_turns == 2:
        return -x, -z
    return -z, x


def normalize_bounds_90(bounds, angle, tolerance):
    remainder = angle % 1024
    nearest = int((angle + 512) // 1024) % 4
    distance = min(remainder, 1024 - remainder)

    if distance > tolerance:
        return None, None

    corners = [
        (bounds[0], bounds[2]),
        (bounds[0], bounds[5]),
        (bounds[3], bounds[2]),
        (bounds[3], bounds[5]),
    ]
    rotated = [rotate_xz(x, z, -nearest) for x, z in corners]
    xs = [x for x, _z in rotated]
    zs = [z for _x, z in rotated]
    return [min(xs), bounds[1], min(zs), max(xs), bounds[4], max(zs)], nearest


def example(obj):
    return {
        "file": obj["island_file"],
        "cube": [obj["cube_x"], obj["cube_y"]],
        "object": obj["object_index"],
        "angle": obj["angle"],
    }


def paired_obl_path(ile_path):
    root, _ext = os.path.splitext(ile_path)
    for ext in (".OBL", ".obl"):
        path = root + ext
        if os.path.isfile(path):
            return path
    return None


def present_hqr_entries(path):
    ents, _data = hqr_inspect.entries(path)
    return [index for index, _off, size, _csize, _method in ents if size is not None]


def analyze(paths, recursive, strict, angle_tolerance):
    files = input_files(paths, recursive)
    bodies = {}
    placed_by_file = {}
    scanned = []
    skipped = []
    unplaced = {}

    for path in files:
        try:
            objects = ile_objects.iter_objects(path)
        except Exception as exc:
            if strict:
                raise
            skipped.append({"file": path, "error": str(exc)})
            continue

        scanned.append(path)
        placed_by_file.setdefault(os.path.basename(path), set())
        for obj in objects:
            placed_by_file[os.path.basename(path)].add(obj["body_id"])
            body = "%s:%d" % (obj["island_file"], obj["body_id"])
            bounds = tuple(rel_bounds(obj))
            body_info = bodies.setdefault(
                body,
                {
                    "count": 0,
                    "body_id": obj["body_id"],
                    "island_file": obj["island_file"],
                    "variants": {},
                    "orientation_variants": {},
                    "non_orthogonal_count": 0,
                },
            )
            body_info["count"] += 1
            variant = body_info["variants"].setdefault(bounds, {"count": 0, "examples": []})
            variant["count"] += 1
            if len(variant["examples"]) < 5:
                variant["examples"].append(example(obj))

            normalized, quarter_turn = normalize_bounds_90(bounds, obj["angle"], angle_tolerance)
            if normalized is None:
                body_info["non_orthogonal_count"] += 1
            else:
                normalized = tuple(normalized)
                orient = body_info["orientation_variants"].setdefault(
                    normalized, {"count": 0, "quarter_turns": {}, "examples": []}
                )
                orient["count"] += 1
                key = str(quarter_turn)
                orient["quarter_turns"][key] = orient["quarter_turns"].get(key, 0) + 1
                if len(orient["examples"]) < 5:
                    orient["examples"].append(example(obj))

        obl = paired_obl_path(path)
        if obl:
            try:
                present = set(present_hqr_entries(obl))
                missing = sorted(present - placed_by_file[os.path.basename(path)])
                unplaced[os.path.basename(path)] = {
                    "obl": os.path.basename(obl),
                    "present_count": len(present),
                    "placed_body_count": len(placed_by_file[os.path.basename(path)]),
                    "unplaced_count": len(missing),
                    "unplaced": missing,
                }
            except Exception as exc:
                if strict:
                    raise
                skipped.append({"file": obl, "error": str(exc)})

    result = {
        "description": "Relative ZV bounds inferred from placed exterior decor objects.",
        "inputs": scanned,
        "skipped": skipped,
        "body_count": len(bodies),
        "unplaced_bodies": unplaced,
        "bodies": {},
    }

    def sort_key(value):
        if ":" in value:
            name, body_id = value.rsplit(":", 1)
            return (name, int(body_id))
        return ("", int(value))

    for body in sorted(bodies, key=sort_key):
        info = bodies[body]
        variants = []
        for bounds, variant in sorted(
            info["variants"].items(), key=lambda item: (-item[1]["count"], item[0])
        ):
            variants.append(
                {
                    "bounds": list(bounds),
                    "count": variant["count"],
                    "examples": variant["examples"],
                }
            )

        orientation_variants = []
        for bounds, variant in sorted(
            info["orientation_variants"].items(), key=lambda item: (-item[1]["count"], item[0])
        ):
            orientation_variants.append(
                {
                    "bounds": list(bounds),
                    "count": variant["count"],
                    "quarter_turns": variant["quarter_turns"],
                    "examples": variant["examples"],
                }
            )

        stable = len(variants) == 1
        majority = variants[0]
        orientation_majority = orientation_variants[0] if orientation_variants else {"bounds": None, "count": 0}
        orientation_stable = len(orientation_variants) == 1 and info["non_orthogonal_count"] == 0
        one_off_variants = sum(1 for variant in variants if variant["count"] == 1)
        one_off_count = sum(variant["count"] for variant in variants if variant["count"] == 1)
        result["bodies"][body] = {
            "count": info["count"],
            "body_id": info["body_id"],
            "island_file": info["island_file"],
            "stable": stable,
            "bounds": variants[0]["bounds"] if stable else None,
            "variant_count": len(variants),
            "majority_bounds": majority["bounds"],
            "majority_count": majority["count"],
            "majority_fraction": float(majority["count"]) / float(info["count"]) if info["count"] else 0.0,
            "non_majority_count": info["count"] - majority["count"],
            "one_off_variant_count": one_off_variants,
            "one_off_count": one_off_count,
            "orientation_stable": orientation_stable,
            "orientation_variant_count": len(orientation_variants),
            "orientation_majority_bounds": orientation_majority["bounds"],
            "orientation_majority_count": orientation_majority["count"],
            "orientation_majority_fraction": float(orientation_majority["count"]) / float(info["count"]) if info["count"] else 0.0,
            "non_orthogonal_count": info["non_orthogonal_count"],
            "orientation_variants": orientation_variants,
            "variants": variants,
        }

    return result


def print_summary(result):
    stable = sum(1 for body in result["bodies"].values() if body["stable"])
    orientation_stable = sum(1 for body in result["bodies"].values() if body["orientation_stable"])
    variants = result["body_count"] - stable
    placed = sum(body["count"] for body in result["bodies"].values())
    print(
        "scanned %d file(s), skipped %d, placed %d, bodies %d: stable %d, orientation-stable %d, variants %d"
        % (len(result["inputs"]), len(result["skipped"]), placed, result["body_count"], stable, orientation_stable, variants)
    )
    if result["unplaced_bodies"]:
        total_present = sum(v["present_count"] for v in result["unplaced_bodies"].values())
        total_unplaced = sum(v["unplaced_count"] for v in result["unplaced_bodies"].values())
        print("paired OBL bodies: present %d, unplaced %d" % (total_present, total_unplaced))
    for body_id, body in result["bodies"].items():
        if body["stable"]:
            print("body %s count %d bounds %s" % (body_id, body["count"], body["bounds"]))
        else:
            counts = ["%s x%d" % (v["bounds"], v["count"]) for v in body["variants"][:4]]
            print(
                "body %s count %d variants %d majority %d/%d %.1f%% one-offs %d variants %d objs: %s"
                % (
                    body_id,
                    body["count"],
                    body["variant_count"],
                    body["majority_count"],
                    body["count"],
                    body["majority_fraction"] * 100.0,
                    body["one_off_variant_count"],
                    body["one_off_count"],
                    "; ".join(counts),
                )
            )
            if body["orientation_variant_count"] != body["variant_count"]:
                print(
                    "  90deg-normalized variants %d majority %d/%d %.1f%% non-orthogonal %d"
                    % (
                        body["orientation_variant_count"],
                        body["orientation_majority_count"],
                        body["count"],
                        body["orientation_majority_fraction"] * 100.0,
                        body["non_orthogonal_count"],
                    )
                )


def main():
    signal.signal(signal.SIGPIPE, signal.SIG_DFL)

    ap = argparse.ArgumentParser()
    ap.add_argument("paths", nargs="+", help="Island .ILE file(s), or directories containing them")
    ap.add_argument("-o", "--output", help="Write JSON report, e.g. decor_bounds.json")
    ap.add_argument("--json", action="store_true", help="Print JSON report to stdout")
    ap.add_argument("--recursive", action="store_true", help="Recurse through input directories")
    ap.add_argument("--strict", action="store_true", help="Fail on the first unreadable/non-island file")
    ap.add_argument("--angle-tolerance", type=int, default=0, help="Allowed angle units away from a 90-degree increment")
    args = ap.parse_args()

    result = analyze(args.paths, args.recursive, args.strict, args.angle_tolerance)

    if args.output:
        with open(args.output, "w") as f:
            json.dump(result, f, indent=2, sort_keys=True)
            f.write("\n")
        print("wrote %s" % args.output)

    if args.json:
        print(json.dumps(result, indent=2, sort_keys=True))
    elif not args.output:
        print_summary(result)

    return 0


if __name__ == "__main__":
    sys.exit(main())
