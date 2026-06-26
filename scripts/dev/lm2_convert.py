#!/usr/bin/env python3
"""Convert an experimental LBA2-96 body entry to the retail LBA2 LM2 layout.

The 1996 demo uses the older sequential 16-bit body format. Retail LBA2 uses
T_BODY_HEADER: a 96-byte header followed by explicit groups, points, normals,
polygon groups, lines, spheres, and textures sections.

The converter currently supports:

  - animated bodies
  - solid polygons (old material 0)
  - flat polygons with face normals (old material 7)
  - Gouraud and dither polygons (old materials 9 and 10)
  - textured flat-incrust polygons (old material 13)
  - triangles, quads, lines, and spheres
  - texture handles and UV coordinates

Unsupported records are rejected instead of being converted speculatively.
Known unsupported cases include other material types and polygons with more
than four vertices. The conversion is experimental: normals and texture-table
records are reconstructed for the retail layout rather than reproduced from a
known original converter.

Usage:
  lm2_convert.py data/LBA2-96/BODY.HQR --entry 13 --output body_13.lm2
  lm2_convert.py old_body.lm2 --output converted.lm2
"""

import argparse
import math
import struct
import sys

import hqr_inspect


HEADER_SIZE = 96
MASK_OBJECT_ANIMATED = 1 << 8
RETAIL_BODY_VERSION = 16
POLY_GOURAUD = 4
POLY_DITHER = 5
POLY_FLAT = 1
POLY_SOLID = 0
POLY_TEXTURE_FLAT_INC = 13
OLD_GROUP_MATRIX_STRIDE = 36


class ConversionError(Exception):
    pass


def read_u16(data, offset, label):
    if offset + 2 > len(data):
        raise ConversionError("truncated while reading %s" % label)
    return struct.unpack_from("<H", data, offset)[0], offset + 2


def read_source(path, entry):
    if entry is None:
        with open(path, "rb") as source:
            return source.read(), "%s" % path

    entries, archive = hqr_inspect.entries(path)
    if entry < 0 or entry >= len(entries):
        raise ConversionError("entry %d is outside the archive" % entry)

    index, offset, size, compressed_size, method = entries[entry]
    if size is None:
        raise ConversionError("entry %d is empty" % entry)

    data = hqr_inspect.decompress_entry(
        archive, offset, size, compressed_size, method
    )
    return data, "%s entry %d" % (path, index)


def parse_demo_body(data):
    if len(data) < 18:
        raise ConversionError("body is too small")

    info = struct.unpack_from("<H", data, 0)[0]
    bbox = struct.unpack_from("<6h", data, 2)
    dummy = struct.unpack_from("<H", data, 14)[0]
    offset = 16

    point_count, offset = read_u16(data, offset, "point count")
    points = []
    for index in range(point_count):
        if offset + 6 > len(data):
            raise ConversionError("truncated point %d" % index)
        points.append(struct.unpack_from("<3h", data, offset))
        offset += 6

    group_count, offset = read_u16(data, offset, "group count")
    groups = []
    for index in range(group_count):
        if offset + 8 > len(data):
            raise ConversionError("truncated group %d" % index)
        groups.append(struct.unpack_from("<HHhH", data, offset))
        offset += 8

    normal_count, offset = read_u16(data, offset, "normal count")
    normals = []
    for index in range(normal_count):
        if offset + 8 > len(data):
            raise ConversionError("truncated normal %d" % index)
        normals.append(struct.unpack_from("<4h", data, offset))
        offset += 8

    polygon_count, offset = read_u16(data, offset, "polygon count")
    polygons = []
    for index in range(polygon_count):
        if offset + 4 > len(data):
            raise ConversionError("truncated polygon %d" % index)
        material, vertex_count, colour = struct.unpack_from("<BBH", data, offset)
        offset += 4

        if material not in (0, 7, 9, 10, 13):
            raise ConversionError(
                "polygon %d uses unsupported material %d" % (index, material)
            )
        if vertex_count not in (3, 4):
            raise ConversionError(
                "polygon %d has unsupported vertex count %d"
                % (index, vertex_count)
            )

        face_normal = None
        if material == 7:
            face_normal, offset = read_u16(
                data, offset, "polygon %d face normal" % index
            )
            if face_normal >= normal_count:
                raise ConversionError(
                    "polygon %d references face normal %d"
                    % (index, face_normal)
                )

        vertices = []
        for vertex in range(vertex_count):
            vertex_size = 4 if material >= 9 else 2
            if offset + vertex_size > len(data):
                raise ConversionError(
                    "truncated polygon %d vertex %d" % (index, vertex)
                )
            if material >= 9:
                normal_index, point_offset = struct.unpack_from(
                    "<HH", data, offset
                )
            else:
                normal_index = None
                point_offset = struct.unpack_from("<H", data, offset)[0]
            offset += vertex_size
            if point_offset % 6:
                raise ConversionError(
                    "polygon %d has unaligned point offset %d"
                    % (index, point_offset)
                )
            point_index = point_offset // 6
            if point_index >= point_count:
                raise ConversionError(
                    "polygon %d references point %d" % (index, point_index)
                )
            if normal_index is not None and normal_index >= normal_count:
                raise ConversionError(
                    "polygon %d references normal %d" % (index, normal_index)
                )
            vertices.append((point_index, normal_index))

        texture_handle = None
        texture_uv = None
        if material == 13:
            texture_word_count = vertex_count * 2 + 2
            texture_size = texture_word_count * 2
            if offset + texture_size > len(data):
                raise ConversionError(
                    "truncated polygon %d texture payload" % index
                )
            texture_payload = struct.unpack_from(
                "<%dH" % texture_word_count, data, offset
            )
            offset += texture_size
            texture_handle = texture_payload[0]
            texture_uv = texture_payload[1 : 1 + vertex_count * 2]

        polygons.append(
            {
                "material": material,
                "colour": colour,
                "face_normal": face_normal,
                "texture_handle": texture_handle,
                "texture_uv": texture_uv,
                "vertices": vertices,
            }
        )

    line_count, offset = read_u16(data, offset, "line count")
    lines = []
    for index in range(line_count):
        if offset + 8 > len(data):
            raise ConversionError("truncated line %d" % index)
        lines.append(data[offset : offset + 8])
        offset += 8

    sphere_count, offset = read_u16(data, offset, "sphere count")
    spheres = []
    for index in range(sphere_count):
        if offset + 8 > len(data):
            raise ConversionError("truncated sphere %d" % index)
        spheres.append(struct.unpack_from("<4H", data, offset))
        offset += 8

    if offset != len(data):
        raise ConversionError(
            "%d trailing bytes remain after the body" % (len(data) - offset)
        )

    return {
        "info": info,
        "bbox": bbox,
        "dummy": dummy,
        "points": points,
        "groups": groups,
        "normals": normals,
        "polygons": polygons,
        "lines": lines,
        "spheres": spheres,
    }


def convert_groups(source_groups, point_count):
    groups = []
    point_groups = [None] * point_count
    first_group_point = 0

    for index, source in enumerate(source_groups):
        point_span, point_offset, parent_offset, unused_normal_count = source
        if point_offset % 6:
            raise ConversionError(
                "group %d has unaligned point offset %d" % (index, point_offset)
            )

        origin_point = point_offset // 6
        if first_group_point + point_span > point_count:
            raise ConversionError("group %d extends beyond the point table" % index)

        if parent_offset == -1:
            parent = 0xFFFF
        else:
            if parent_offset % OLD_GROUP_MATRIX_STRIDE:
                raise ConversionError(
                    "group %d has unaligned parent offset %d"
                    % (index, parent_offset)
                )
            parent = parent_offset // OLD_GROUP_MATRIX_STRIDE
            if parent >= len(source_groups):
                raise ConversionError(
                    "group %d references parent %d" % (index, parent)
                )

        groups.append((parent, origin_point, point_span, 0))
        for point in range(first_group_point, first_group_point + point_span):
            point_groups[point] = index
        first_group_point += point_span

    for point, group in enumerate(point_groups):
        if group is None:
            raise ConversionError("point %d is not assigned to a group" % point)

    return groups, point_groups


def scale_normal(normal, group):
    x, y, z, unused = normal
    length = math.sqrt(x * x + y * y + z * z)
    if length == 0:
        return 0, 0, 0, group

    scale = 7168.0 / length
    return (
        int(round(x * scale)),
        int(round(y * scale)),
        int(round(z * scale)),
        group,
    )


def convert_normals(source, point_groups):
    point_to_normal = {}
    for polygon_index, polygon in enumerate(source["polygons"]):
        for point_index, normal_index in polygon["vertices"]:
            if normal_index is None:
                continue
            previous = point_to_normal.get(point_index)
            if previous is not None and previous != normal_index:
                raise ConversionError(
                    "point %d uses normals %d and %d (polygon %d)"
                    % (point_index, previous, normal_index, polygon_index)
                )
            point_to_normal[point_index] = normal_index

    normals = []
    for point_index, group in enumerate(point_groups):
        normal_index = point_to_normal.get(point_index)
        if normal_index is None:
            normals.append((0, 0, 0, 0))
        else:
            normals.append(scale_normal(source["normals"][normal_index], group))
    return normals


def convert_face_normals(source, point_groups):
    by_group = {}
    polygon_keys = {}

    for polygon_index, polygon in enumerate(source["polygons"]):
        normal_index = polygon["face_normal"]
        if normal_index is None:
            continue
        point_group = point_groups[polygon["vertices"][0][0]]
        key = (point_group, normal_index)
        by_group.setdefault(point_group, [])
        if normal_index not in by_group[point_group]:
            by_group[point_group].append(normal_index)
        polygon_keys[polygon_index] = key

    face_normals = []
    normal_map = {}
    group_counts = {}
    for group in range(len(source["groups"])):
        source_indices = by_group.get(group, [])
        group_counts[group] = len(source_indices)
        for normal_index in source_indices:
            normal_map[(group, normal_index)] = len(face_normals)
            face_normals.append(
                scale_normal(source["normals"][normal_index], group)
            )

    polygon_normal_indices = {}
    point_count = len(source["points"])
    for polygon_index, key in polygon_keys.items():
        polygon_normal_indices[polygon_index] = point_count + normal_map[key]

    return face_normals, group_counts, polygon_normal_indices


def build_polygon_section(polygons, polygon_normal_indices, texture_indices):
    records = {}

    for index, polygon in enumerate(polygons):
        points = [vertex[0] for vertex in polygon["vertices"]]
        if polygon["material"] == 9:
            type_poly = POLY_GOURAUD
        elif polygon["material"] == 10:
            type_poly = POLY_DITHER
        elif polygon["material"] == 0:
            type_poly = POLY_SOLID
        elif polygon["material"] == 13:
            type_poly = POLY_TEXTURE_FLAT_INC
        else:
            type_poly = POLY_FLAT
        if len(points) == 4:
            type_poly |= 0x8000
        normal = (
            points[0]
            if polygon["material"] >= 9
            else polygon_normal_indices.get(index, 0)
        )

        if polygon["material"] == 13:
            uv = polygon["texture_uv"]
            handle = texture_indices[polygon["texture_handle"]]
            if len(points) == 3:
                record = struct.pack(
                    "<12H",
                    points[0], points[1], points[2], handle,
                    polygon["colour"], points[0],
                    uv[0], uv[1], uv[2], uv[3], uv[4], uv[5],
                )
            else:
                record = struct.pack(
                    "<16H",
                    points[0], points[1], points[2], points[3],
                    polygon["colour"], points[0],
                    uv[0], uv[1], uv[2], uv[3],
                    uv[4], uv[5], uv[6], uv[7], handle, 0,
                )
        elif len(points) == 3:
            record = struct.pack(
                "<6H",
                points[0],
                points[1],
                points[2],
                0,
                polygon["colour"],
                normal,
            )
        else:
            record = struct.pack(
                "<6H",
                points[0],
                points[1],
                points[2],
                points[3],
                polygon["colour"],
                normal,
            )
        records.setdefault(type_poly, []).append(record)

    blocks = []
    for type_poly in sorted(records):
        payload = b"".join(records[type_poly])
        blocks.append(
            struct.pack("<HHI", type_poly, len(records[type_poly]), 8 + len(payload))
            + payload
        )
    return b"".join(blocks)


def convert_body(source):
    groups, point_groups = convert_groups(source["groups"], len(source["points"]))
    normals = convert_normals(source, point_groups)
    face_normals, group_normal_counts, polygon_normal_indices = (
        convert_face_normals(source, point_groups)
    )
    texture_handles = sorted(
        set(
            polygon["texture_handle"]
            for polygon in source["polygons"]
            if polygon["texture_handle"] is not None
        )
    )
    texture_indices = dict(
        (handle, index) for index, handle in enumerate(texture_handles)
    )
    groups = [
        (group[0], group[1], group[2], group_normal_counts[index])
        for index, group in enumerate(groups)
    ]
    polygon_data = build_polygon_section(
        source["polygons"], polygon_normal_indices, texture_indices
    )

    group_data = b"".join(struct.pack("<4H", *group) for group in groups)
    point_data = b"".join(
        struct.pack("<3hH", point[0], point[1], point[2], point_groups[index])
        for index, point in enumerate(source["points"])
    )
    normal_data = b"".join(struct.pack("<4h", *normal) for normal in normals)
    face_normal_data = b"".join(
        struct.pack("<4h", *normal) for normal in face_normals
    )
    line_data = b"".join(source["lines"])
    sphere_data = b"".join(
        struct.pack("<4H", *sphere) for sphere in source["spheres"]
    )
    texture_data = b"".join(
        struct.pack("<HH", handle, 0) for handle in texture_handles
    )

    offset = HEADER_SIZE
    groups_offset = offset
    offset += len(group_data)
    points_offset = offset
    offset += len(point_data)
    normals_offset = offset
    offset += len(normal_data)
    face_normals_offset = offset
    offset += len(face_normal_data)
    polygons_offset = offset
    offset += len(polygon_data)
    lines_offset = offset
    offset += len(line_data)
    spheres_offset = offset
    offset += len(sphere_data)
    textures_offset = offset
    offset += len(texture_data)

    info = RETAIL_BODY_VERSION
    if groups:
        info |= MASK_OBJECT_ANIMATED

    header = struct.pack(
        "<ihh6i16i",
        info,
        HEADER_SIZE,
        0,
        *source["bbox"],
        len(groups),
        groups_offset,
        len(source["points"]),
        points_offset,
        len(normals),
        normals_offset,
        len(face_normals),
        face_normals_offset,
        len(source["polygons"]),
        polygons_offset,
        len(source["lines"]),
        lines_offset,
        len(source["spheres"]),
        spheres_offset,
        len(texture_handles),
        textures_offset
    )
    if len(header) != HEADER_SIZE:
        raise AssertionError("unexpected retail body header size")

    return (
        header
        + group_data
        + point_data
        + normal_data
        + face_normal_data
        + polygon_data
        + line_data
        + sphere_data
        + texture_data
    )


def describe(source, label):
    materials = {}
    vertex_counts = {}
    for polygon in source["polygons"]:
        material = polygon["material"]
        count = len(polygon["vertices"])
        materials[material] = materials.get(material, 0) + 1
        vertex_counts[count] = vertex_counts.get(count, 0) + 1

    print("%s:" % label)
    print("  old info: 0x%04x, dummy: %d" % (source["info"], source["dummy"]))
    print("  bbox: %s" % (source["bbox"],))
    print(
        "  %d points, %d groups, %d normals, %d polygons, %d lines, %d spheres"
        % (
            len(source["points"]),
            len(source["groups"]),
            len(source["normals"]),
            len(source["polygons"]),
            len(source["lines"]),
            len(source["spheres"]),
        )
    )
    print("  polygon materials: %s" % materials)
    print("  polygon vertex counts: %s" % vertex_counts)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("path", help="LBA2-96 BODY.HQR or a decompressed old LM2")
    parser.add_argument("--entry", type=int, help="HQR entry to decompress and convert")
    parser.add_argument("--output", help="output retail-format LM2 path")
    parser.add_argument(
        "--inspect-only", action="store_true", help="parse and report without converting"
    )
    args = parser.parse_args()

    if args.entry is not None and not args.path.lower().endswith(".hqr"):
        parser.error("--entry requires an HQR input")
    if not args.inspect_only and not args.output:
        parser.error("--output is required unless --inspect-only is used")

    try:
        data, label = read_source(args.path, args.entry)
        source = parse_demo_body(data)
        describe(source, label)

        if args.inspect_only:
            return 0

        converted = convert_body(source)
        with open(args.output, "wb") as output:
            output.write(converted)
        print("  wrote %s (%d bytes)" % (args.output, len(converted)))
        return 0
    except (ConversionError, OSError, struct.error) as error:
        print("error: %s" % error, file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
