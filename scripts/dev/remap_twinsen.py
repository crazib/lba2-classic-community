#!/usr/bin/env python3
"""Targeted Twinsen body remap helpers.

This is intentionally narrow: copy Twinsen horn geometry between tunic,
sweater, and mage bodies, copy protopack geometry to other outfits, and write
loose O3D files for mod manifests.

The protopack/jetpack copy mode is a work in progress. The sweater version is
missing faces or should be welded to Twinsen's back. The mage version clips
into Twinsen during animation.
"""

import argparse
import os
import struct
import sys


SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, SCRIPT_DIR)

import hqr_inspect  # noqa: E402


HEADER_FORMAT = "<ihh" + "i" * 22
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)

GROUP_FORMAT = "<HHHH"
POINT_FORMAT = "<hhhh"
LINE_FORMAT = "<HHHH"
SPHERE_FORMAT = "<HHHH"

SWEATER_BODY = 0
MAGE_BODY = 7
TUNIC_PROTOPACK_BODY = 16
TUNIC_TRITON_BODY = 18
MAGE_TRITON_BODY = 19
TUNIC_JETPACK_BODY = 21

HORN_GROUPS = range(19, 24)
HORN_POINT_GROUP = 23
STRING_LINE_COLOR = 18

PROTOPACK_ATTACH_GROUP = 3
PROTOPACK_FACE_INDICES = (
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
    25, 26, 27, 28, 29, 30, 31, 32, 33, 34,
    35, 50, 51, 52, 55, 56, 57, 58, 59, 60,
    61, 62, 63, 64, 65, 66, 67, 70, 71, 188,
)
JETPACK_ATTACH_GROUP = 3
JETPACK_FACE_INDICES = (
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
    25, 26, 27, 28, 29, 30, 31, 46, 47, 52,
    53, 54, 55, 56, 57, 58, 59, 60, 61, 62,
    63, 64, 65, 66, 67, 68, 69, 70, 71, 72,
    73, 76, 77, 78, 195, 304, 305, 306, 307, 308,
    309,
)


class Body(object):
    def __init__(self, data):
        self.data = data
        values = list(struct.unpack_from(HEADER_FORMAT, data, 0))
        self.info = values[0]
        self.size_header = values[1]
        self.dummy = values[2]
        ints = values[3:]

        self.x_min = ints[0]
        self.x_max = ints[1]
        self.y_min = ints[2]
        self.y_max = ints[3]
        self.z_min = ints[4]
        self.z_max = ints[5]
        self.nb_groups = ints[6]
        self.off_groups = ints[7]
        self.nb_points = ints[8]
        self.off_points = ints[9]
        self.nb_normals = ints[10]
        self.off_normals = ints[11]
        self.nb_norm_faces = ints[12]
        self.off_norm_faces = ints[13]
        self.nb_polys = ints[14]
        self.off_polys = ints[15]
        self.nb_lines = ints[16]
        self.off_lines = ints[17]
        self.nb_spheres = ints[18]
        self.off_spheres = ints[19]
        self.nb_textures = ints[20]
        self.off_textures = ints[21]

        self.groups = [
            struct.unpack_from(GROUP_FORMAT, data, self.off_groups + i * 8)
            for i in range(self.nb_groups)
        ]
        self.points = [
            struct.unpack_from(POINT_FORMAT, data, self.off_points + i * 8)
            for i in range(self.nb_points)
        ]
        self.normals = [
            struct.unpack_from(POINT_FORMAT, data, self.off_normals + i * 8)
            for i in range(self.nb_normals)
        ]
        self.norm_faces = [
            struct.unpack_from(POINT_FORMAT, data, self.off_norm_faces + i * 8)
            for i in range(self.nb_norm_faces)
        ]
        self.polys = data[self.off_polys:self.off_lines]
        self.lines = [
            struct.unpack_from(LINE_FORMAT, data, self.off_lines + i * 8)
            for i in range(self.nb_lines)
        ]
        self.spheres = [
            struct.unpack_from(SPHERE_FORMAT, data, self.off_spheres + i * 8)
            for i in range(self.nb_spheres)
        ]
        self.textures = data[self.off_textures:]


def load_hqr_entry(path, entry):
    entries, raw = hqr_inspect.entries(path)
    info = entries[entry]
    return hqr_inspect.decompress_entry(raw, *info[1:])


def load_body_source(path, entry=None):
    if entry is not None:
        return load_hqr_entry(path, entry)
    with open(path, "rb") as f:
        return f.read()


def group_ranges(groups):
    ranges = []
    start = 0
    for group in groups:
        count = group[2]
        ranges.append((start, start + count))
        start += count
    return ranges


def nearest_point_with_group(points, source_point):
    sx, sy, sz, sg = source_point
    best_index = None
    best_dist = None
    for i, point in enumerate(points):
        x, y, z, group = point
        if group != sg:
            continue
        dist = (x - sx) * (x - sx) + (y - sy) * (y - sy) + (z - sz) * (z - sz)
        if best_dist is None or dist < best_dist:
            best_index = i
            best_dist = dist
    if best_index is None:
        raise ValueError("no target point with group %d" % sg)
    return best_index


def poly_record_size(type_poly, block_size, count):
    if count <= 0:
        return 0
    return (block_size - 8) // count


def poly_points(type_poly, record):
    is_quad = (type_poly & 0x8000) != 0
    if is_quad:
        return list(struct.unpack_from("<HHHH", record, 0))
    return list(struct.unpack_from("<HHH", record, 0))


def patch_poly_record(type_poly, record, point_map, normal_map):
    out = bytearray(record)
    is_quad = (type_poly & 0x8000) != 0
    point_offsets = [0, 2, 4, 6] if is_quad else [0, 2, 4]
    for off in point_offsets:
        old = struct.unpack_from("<H", out, off)[0]
        struct.pack_into("<H", out, off, point_map[old])
    old_normal = struct.unpack_from("<H", out, 10)[0]
    if old_normal in normal_map:
        struct.pack_into("<H", out, 10, normal_map[old_normal])
    return bytes(out)


def shift_poly_record(type_poly, record, point_insert, point_count,
                      old_vertex_normal_count, face_normal_insert, face_normal_count):
    out = bytearray(record)
    is_quad = (type_poly & 0x8000) != 0
    point_offsets = [0, 2, 4, 6] if is_quad else [0, 2, 4]
    for off in point_offsets:
        point = struct.unpack_from("<H", out, off)[0]
        if point >= point_insert:
            struct.pack_into("<H", out, off, point + point_count)
    normal = struct.unpack_from("<H", out, 10)[0]
    if normal < old_vertex_normal_count:
        if normal >= point_insert:
            struct.pack_into("<H", out, 10, normal + point_count)
    else:
        face_normal = normal - old_vertex_normal_count
        normal = old_vertex_normal_count + point_count + face_normal
        if face_normal >= face_normal_insert:
            normal += face_normal_count
        struct.pack_into("<H", out, 10, normal)
    return bytes(out)


def patch_point_group(point, group_index):
    return (point[0], point[1], point[2], group_index)


def parse_poly_blocks(blob):
    blocks = []
    pos = 0
    while pos < len(blob):
        type_poly, count, block_size = struct.unpack_from("<HHI", blob, pos)
        if block_size <= 0 or pos + block_size > len(blob):
            raise ValueError("bad poly block at offset %d" % pos)
        rec_size = poly_record_size(type_poly, block_size, count)
        records = []
        rec_pos = pos + 8
        for _ in range(count):
            records.append(blob[rec_pos:rec_pos + rec_size])
            rec_pos += rec_size
        blocks.append([type_poly, records, rec_size])
        pos += block_size
    return blocks


def iter_poly_records(blocks):
    index = 0
    for type_poly, records, rec_size in blocks:
        for record in records:
            yield index, type_poly, record
            index += 1


def build_poly_blocks(blocks):
    out = bytearray()
    total = 0
    for type_poly, records, rec_size in blocks:
        if not records:
            continue
        block_size = 8 + len(records) * rec_size
        out += struct.pack("<HHI", type_poly, len(records), block_size)
        for record in records:
            out += record
        total += len(records)
    return bytes(out), total


def add_poly_record(blocks, order, type_poly, record):
    for block in blocks:
        if block[0] == type_poly and block[2] == len(record):
            block[1].append(record)
            return
    blocks.append([type_poly, [record], len(record)])
    order.append(type_poly)


def rebuild_body(template, bounds_source, groups, points, normals, norm_faces, poly_blob, poly_count, lines):
    header = bytearray(HEADER_SIZE)
    chunks = []
    offset = HEADER_SIZE

    off_groups = offset
    group_blob = b"".join(struct.pack(GROUP_FORMAT, *group) for group in groups)
    chunks.append(group_blob)
    offset += len(group_blob)

    off_points = offset
    point_blob = b"".join(struct.pack(POINT_FORMAT, *point) for point in points)
    chunks.append(point_blob)
    offset += len(point_blob)

    off_normals = offset
    normal_blob = b"".join(struct.pack(POINT_FORMAT, *normal) for normal in normals)
    chunks.append(normal_blob)
    offset += len(normal_blob)

    off_norm_faces = offset
    norm_face_blob = b"".join(struct.pack(POINT_FORMAT, *normal) for normal in norm_faces)
    chunks.append(norm_face_blob)
    offset += len(norm_face_blob)

    off_polys = offset
    chunks.append(poly_blob)
    offset += len(poly_blob)

    off_lines = offset
    line_blob = b"".join(struct.pack(LINE_FORMAT, *line) for line in lines)
    chunks.append(line_blob)
    offset += len(line_blob)

    off_spheres = offset
    sphere_blob = b"".join(struct.pack(SPHERE_FORMAT, *sphere) for sphere in template.spheres)
    chunks.append(sphere_blob)
    offset += len(sphere_blob)

    off_textures = offset
    chunks.append(template.textures)
    offset += len(template.textures)

    values = [
        template.info,
        HEADER_SIZE,
        template.dummy,
        min(template.x_min, bounds_source.x_min),
        max(template.x_max, bounds_source.x_max),
        min(template.y_min, bounds_source.y_min),
        max(template.y_max, bounds_source.y_max),
        min(template.z_min, bounds_source.z_min),
        max(template.z_max, bounds_source.z_max),
        len(groups),
        off_groups,
        len(points),
        off_points,
        len(normals),
        off_normals,
        len(norm_faces),
        off_norm_faces,
        poly_count,
        off_polys,
        len(lines),
        off_lines,
        template.nb_spheres,
        off_spheres,
        template.nb_textures,
        off_textures,
    ]
    struct.pack_into(HEADER_FORMAT, header, 0, *values)
    return bytes(header) + b"".join(chunks)


def copy_horn(source_raw, target_raw, output):
    source = Body(source_raw)
    target = Body(target_raw)
    target_ranges = group_ranges(target.groups)
    replace_target_horn = target.nb_groups >= max(HORN_GROUPS) + 1
    base_group_count = min(HORN_GROUPS) if replace_target_horn else target.nb_groups
    base_point_end = target_ranges[base_group_count - 1][1] if base_group_count > 0 else 0

    target_points = list(target.points[:base_point_end])
    target_normals = list(target.normals[:base_point_end])
    point_map = {}
    normal_map = {}

    source_ranges = group_ranges(source.groups)

    for index in range(base_point_end):
        point_map[("target", index)] = index
        normal_map[("target", index)] = index

    def map_target_point(index):
        key = ("target", index)
        if key in point_map:
            return point_map[key]
        point = target.points[index]
        point_map[key] = len(target_points)
        target_points.append(point)
        return point_map[key]

    def map_target_normal(index):
        key = ("target", index)
        if key in normal_map:
            return normal_map[key]
        normal = target.normals[index]
        normal_map[key] = len(target_normals)
        target_normals.append(normal)
        return normal_map[key]

    def map_source_point(index):
        if index in point_map:
            return point_map[index]
        point = source.points[index]
        if point[3] in HORN_GROUPS:
            point_map[index] = len(target_points)
            target_points.append(point)
            return point_map[index]
        point_map[index] = nearest_point_with_group(target.points, point)
        return point_map[index]

    def map_source_normal(index, force_append=False):
        if index in normal_map:
            return normal_map[index]
        normal = source.normals[index]
        if force_append or normal[3] in HORN_GROUPS:
            normal_map[index] = len(target_normals)
            target_normals.append(normal)
            return normal_map[index]
        normal_map[index] = nearest_point_with_group(target.normals, normal)
        return normal_map[index]

    # Append the exact contiguous point/normal runs that body 18 uses for
    # groups 19..23. ObjectDisplay walks groups by these counts, not by the
    # point Group field.
    for group_index in HORN_GROUPS:
        start, end = source_ranges[group_index]
        for source_index in range(start, end):
            map_source_point(source_index)
            map_source_normal(source_index, True)

    target_groups = list(target.groups[:base_group_count])
    for group_index in HORN_GROUPS:
        parent, org_point, nb_pts, nb_norm = source.groups[group_index]
        target_groups.append((parent, map_source_point(org_point), nb_pts, nb_norm))

    if replace_target_horn:
        target_blocks = []
        target_source_blocks = parse_poly_blocks(target.polys)
        order = []
        kept_target_faces = 0

        for type_poly, records, rec_size in target_source_blocks:
            for record in records:
                points = poly_points(type_poly, record)
                if any(target.points[p][3] in HORN_GROUPS for p in points):
                    continue
                normal = struct.unpack_from("<H", record, 10)[0]
                record_point_map = {}
                for point in points:
                    record_point_map[point] = map_target_point(point)
                record_normal_map = {}
                if normal < target.nb_normals:
                    record_normal_map[normal] = map_target_normal(normal)
                add_poly_record(target_blocks, order, type_poly,
                                patch_poly_record(type_poly, record, record_point_map, record_normal_map))
                kept_target_faces += 1
    else:
        target_blocks = parse_poly_blocks(target.polys)
        order = [block[0] for block in target_blocks]
        kept_target_faces = target.nb_polys

    source_blocks = parse_poly_blocks(source.polys)
    copied_faces = 0

    for type_poly, records, rec_size in source_blocks:
        for record in records:
            points = poly_points(type_poly, record)
            if points and all(source.points[p][3] == HORN_POINT_GROUP for p in points):
                normal = struct.unpack_from("<H", record, 10)[0]
                for point in points:
                    map_source_point(point)
                map_source_normal(normal)
                add_poly_record(target_blocks, order, type_poly,
                                patch_poly_record(type_poly, record, point_map, normal_map))
                copied_faces += 1

    if replace_target_horn:
        target_lines = []
        for line in target.lines:
            type_line, color, p1, p2 = line
            if color == STRING_LINE_COLOR:
                continue
            if target.points[p1][3] in HORN_GROUPS or target.points[p2][3] in HORN_GROUPS:
                continue
            target_lines.append((type_line, color, map_target_point(p1), map_target_point(p2)))
    else:
        target_lines = list(target.lines)

    copied_lines = 0
    for line in source.lines:
        type_line, color, p1, p2 = line
        if color == STRING_LINE_COLOR:
            target_lines.append((type_line, color, map_source_point(p1), map_source_point(p2)))
            copied_lines += 1

    poly_blob, poly_count = build_poly_blocks(target_blocks)
    result = rebuild_body(target, source, target_groups, target_points, target_normals, target.norm_faces,
                          poly_blob, poly_count, target_lines)

    out_dir = os.path.dirname(output)
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)
    with open(output, "wb") as f:
        f.write(result)

    print("wrote %s" % output)
    print("  groups: %d -> %d" % (target.nb_groups, len(target_groups)))
    print("  points: %d -> %d" % (target.nb_points, len(target_points)))
    print("  normals: %d -> %d" % (target.nb_normals, len(target_normals)))
    print("  target horn mode: %s" % ("replace" if replace_target_horn else "append"))
    print("  target faces kept: %d" % kept_target_faces)
    print("  faces copied: %d" % copied_faces)
    print("  color-%d lines copied: %d" % (STRING_LINE_COLOR, copied_lines))


def group_norm_face_ranges(groups):
    ranges = []
    start = 0
    for group in groups:
        count = group[3]
        ranges.append((start, start + count))
        start += count
    return ranges


def copy_selected_faces_to_group(source_raw, target_raw, face_indices, attach_group, output, label):
    source = Body(source_raw)
    target = Body(target_raw)
    source_blocks = parse_poly_blocks(source.polys)
    selected = set(face_indices)
    selected_records = []
    source_points = set()
    source_vertex_normals = set()
    source_face_normals = set()

    for face_index, type_poly, record in iter_poly_records(source_blocks):
        if face_index not in selected:
            continue
        points = poly_points(type_poly, record)
        normal = struct.unpack_from("<H", record, 10)[0]
        selected_records.append((type_poly, record))
        source_points.update(points)
        if normal < source.nb_normals:
            source_vertex_normals.add(normal)
        else:
            source_face_normals.add(normal - source.nb_normals)

    missing = selected.difference(
        face_index for face_index, _, _ in iter_poly_records(source_blocks)
    )
    if missing:
        raise ValueError("missing %s face indices: %s" % (label, sorted(missing)))
    if len(selected_records) != len(face_indices):
        raise ValueError("expected %d %s faces, found %d" %
                         (len(face_indices), label, len(selected_records)))
    if attach_group >= target.nb_groups:
        raise ValueError("target has no attach group %d" % attach_group)

    target_point_ranges = group_ranges(target.groups)
    target_norm_face_ranges = group_norm_face_ranges(target.groups)
    point_insert = target_point_ranges[attach_group][1]
    face_normal_insert = target_norm_face_ranges[attach_group][1]
    point_map = {}
    normal_map = {}
    append_source_points = sorted(source_points.union(source_vertex_normals))
    append_source_face_normals = sorted(source_face_normals)
    point_count = len(append_source_points)
    face_normal_count = len(append_source_face_normals)
    final_vertex_normal_count = target.nb_normals + point_count

    target_points = (
        list(target.points[:point_insert]) +
        [patch_point_group(source.points[source_index], attach_group)
         for source_index in append_source_points] +
        list(target.points[point_insert:])
    )
    target_normals = (
        list(target.normals[:point_insert]) +
        [patch_point_group(source.normals[source_index], attach_group)
         for source_index in append_source_points] +
        list(target.normals[point_insert:])
    )
    target_norm_faces = (
        list(target.norm_faces[:face_normal_insert]) +
        [patch_point_group(source.norm_faces[source_index], attach_group)
         for source_index in append_source_face_normals] +
        list(target.norm_faces[face_normal_insert:])
    )

    for offset, source_index in enumerate(append_source_points):
        point_map[source_index] = point_insert + offset
        normal_map[source_index] = point_insert + offset

    for offset, source_face_normal in enumerate(append_source_face_normals):
        normal_map[source.nb_normals + source_face_normal] = (
            final_vertex_normal_count + face_normal_insert + offset
        )

    target_groups = []
    for index, group in enumerate(target.groups):
        parent, org_point, nb_pts, nb_norm = group
        if org_point >= point_insert:
            org_point += point_count
        if index == attach_group:
            nb_pts += point_count
            nb_norm += face_normal_count
        target_groups.append((parent, org_point, nb_pts, nb_norm))

    target_blocks = []
    order = []
    for type_poly, records, rec_size in parse_poly_blocks(target.polys):
        for record in records:
            add_poly_record(target_blocks, order, type_poly,
                            shift_poly_record(type_poly, record,
                                              point_insert, point_count,
                                              target.nb_normals,
                                              face_normal_insert,
                                              face_normal_count))

    target_lines = []
    for line in target.lines:
        type_line, color, p1, p2 = line
        if p1 >= point_insert:
            p1 += point_count
        if p2 >= point_insert:
            p2 += point_count
        target_lines.append((type_line, color, p1, p2))

    target_spheres = []
    for sphere in target.spheres:
        sphere_type, color, point, radius = sphere
        if point >= point_insert:
            point += point_count
        target_spheres.append((sphere_type, color, point, radius))
    target.spheres = target_spheres

    copied_faces = 0
    for type_poly, record in selected_records:
        add_poly_record(target_blocks, order, type_poly,
                        patch_poly_record(type_poly, record, point_map, normal_map))
        copied_faces += 1

    poly_blob, poly_count = build_poly_blocks(target_blocks)
    result = rebuild_body(target, source, target_groups, target_points, target_normals,
                          target_norm_faces, poly_blob, poly_count, target_lines)

    out_dir = os.path.dirname(output)
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)
    with open(output, "wb") as f:
        f.write(result)

    print("wrote %s" % output)
    print("  %s faces copied: %d" % (label, copied_faces))
    print("  attach group: %d" % attach_group)
    print("  groups: %d -> %d" % (target.nb_groups, len(target_groups)))
    print("  points: %d -> %d" % (target.nb_points, len(target_points)))
    print("  normals: %d -> %d" % (target.nb_normals, len(target_normals)))
    print("  face normals: %d -> %d" % (target.nb_norm_faces, len(target_norm_faces)))


def copy_protopack(source_raw, target_raw, output):
    copy_selected_faces_to_group(source_raw, target_raw, PROTOPACK_FACE_INDICES,
                                 PROTOPACK_ATTACH_GROUP, output, "protopack")


def copy_jetpack(source_raw, target_raw, output):
    copy_selected_faces_to_group(source_raw, target_raw, JETPACK_FACE_INDICES,
                                 JETPACK_ATTACH_GROUP, output, "jetpack")


def main():
    parser = argparse.ArgumentParser(description="Build loose Twinsen body remap O3D files.")
    parser.add_argument("--body-hqr", default="data/BODY.HQR")
    parser.add_argument("--mod-dir", default="mods")
    parser.add_argument("--gawley-tunic", default=None)
    args = parser.parse_args()

    gawley_tunic = args.gawley_tunic
    if gawley_tunic is None:
        candidate = os.path.join(args.mod_dir, "twinsen_tunic_gawleys_horn.o3d")
        if os.path.exists(candidate):
            gawley_tunic = candidate
        else:
            gawley_tunic = os.path.join(args.mod_dir, "MOD_BODY_18.O3D")

    triton_tunic = load_body_source(args.body_hqr, TUNIC_TRITON_BODY)
    protopack = load_body_source(args.body_hqr, TUNIC_PROTOPACK_BODY)
    jetpack = load_body_source(args.body_hqr, TUNIC_JETPACK_BODY)
    gawley_tunic_raw = load_body_source(gawley_tunic)
    sweater = load_body_source(args.body_hqr, SWEATER_BODY)
    mage = load_body_source(args.body_hqr, MAGE_BODY)
    triton_mage = load_body_source(args.body_hqr, MAGE_TRITON_BODY)

    copy_horn(triton_tunic, sweater, os.path.join(args.mod_dir, "twinsen_sweater_tritons_horn.o3d"))
    copy_horn(gawley_tunic_raw, sweater, os.path.join(args.mod_dir, "twinsen_sweater_gawleys_horn.o3d"))
    copy_horn(gawley_tunic_raw, triton_mage, os.path.join(args.mod_dir, "twinsen_mage_gawleys_horn.o3d"))
    copy_protopack(protopack, sweater, os.path.join(args.mod_dir, "twinsen_sweater_protopack.o3d"))
    copy_protopack(protopack, mage, os.path.join(args.mod_dir, "twinsen_mage_protopack.o3d"))
    copy_jetpack(jetpack, sweater, os.path.join(args.mod_dir, "twinsen_sweater_jetpack.o3d"))
    copy_jetpack(jetpack, mage, os.path.join(args.mod_dir, "twinsen_mage_jetpack.o3d"))

    tunic_out = os.path.join(args.mod_dir, "twinsen_tunic_gawleys_horn.o3d")
    if os.path.abspath(gawley_tunic) != os.path.abspath(tunic_out):
        with open(tunic_out, "wb") as f:
            f.write(gawley_tunic_raw)
        print("wrote %s" % tunic_out)


if __name__ == "__main__":
    main()
