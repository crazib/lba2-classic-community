#!/usr/bin/env python3
"""Compile bulk-decompiled LBA2 scene scripts back into SCENE.HQR."""

import argparse
import os
import re
import struct
import sys

import hqr_inspect
import script_decompile as defs


TRACK_OPCODES = {name: opcode for opcode, name in enumerate(defs.TRACK_NAMES)}
LIFE_OPCODES = {
    name: opcode for opcode, name in enumerate(defs.LIFE_NAMES) if name
}
VAR_OPCODES = {name: opcode for opcode, name in enumerate(defs.VAR_NAMES)}
OPERATOR_OPCODES = {
    name: opcode for opcode, name in enumerate(defs.OPERATORS)
}
BEHAVIOUR_IDS = {
    name: value for value, name in enumerate(defs.BEHAVIOURS)
}
DIRMODE_IDS = {
    name: value for value, name in enumerate(defs.DIRMODES) if name
}

IF_COMMANDS = {"SNIF", "NEVERIF", "IF", "SWIF", "ONEIF"}
CHAIN_COMMANDS = {"OR_IF", "AND_IF"}
CONDITION_COMMANDS = IF_COMMANDS | CHAIN_COMMANDS


class CompileError(ValueError):
    pass


def source_lines(path):
    lines = []
    with open(path, encoding="utf-8") as source:
        for number, raw in enumerate(source, 1):
            match = re.search(r"@target=(\d+)", raw)
            target = int(match.group(1)) if match else None
            text = raw.split("//", 1)[0].rstrip()
            if not text:
                continue
            indent = len(text) - len(text.lstrip(" "))
            lines.append((number, indent, text.strip().split(), target))
    return lines


def parse_int(token, actor=None):
    if isinstance(token, int):
        return token
    if token == "SELF" and actor is not None:
        return actor
    try:
        return int(token, 0)
    except ValueError:
        raise CompileError("expected integer, got %r" % token)


def pack_value(value, size):
    mask = (1 << (size * 8)) - 1
    return int(value & mask).to_bytes(size, "little")


def compile_track_source(path):
    instructions = []
    labels = {}
    offset = 0
    for line, _indent, tokens, _target in source_lines(path):
        name = tokens[0]
        if name not in TRACK_OPCODES:
            raise CompileError("%s:%d: unknown track command %s"
                               % (path, line, name))
        opcode = TRACK_OPCODES[name]
        sizes = defs.TRACK_SIZES[opcode]
        instruction = {
            "line": line,
            "name": name,
            "opcode": opcode,
            "tokens": tokens[1:],
            "offset": offset,
        }
        instructions.append(instruction)
        if sizes is None:
            if len(tokens) != 2:
                raise CompileError("%s:%d: %s expects one text parameter"
                                   % (path, line, name))
            offset += 1 + len(tokens[1].encode("latin-1")) + 1
        else:
            visible = 0 if opcode in defs.TRACK_DUMMY_FIRST else min(1, len(sizes))
            if len(tokens) != 1 + visible:
                raise CompileError("%s:%d: wrong parameter count for %s"
                                   % (path, line, name))
            offset += 1 + sum(sizes)
        if opcode == 9:
            label = parse_int(tokens[1])
            if label not in labels:
                labels[label] = instruction["offset"]

    out = bytearray()
    for instruction in instructions:
        opcode = instruction["opcode"]
        tokens = instruction["tokens"]
        sizes = defs.TRACK_SIZES[opcode]
        out.append(opcode)
        if sizes is None:
            out.extend(tokens[0].encode("latin-1"))
            out.append(0)
            continue
        values = []
        if opcode in defs.TRACK_DUMMY_FIRST:
            values.append(65535 if opcode == 33 else 0)
        elif sizes:
            value = parse_int(tokens[0])
            if opcode == 10 and value != -1:
                if value not in labels:
                    raise CompileError("%s:%d: GOTO references missing LABEL %d"
                                       % (path, instruction["line"], value))
                value = labels[value]
            values.append(value)
        while len(values) < len(sizes):
            values.append(65535 if opcode == 34 else 0)
        for value, size in zip(values, sizes):
            out.extend(pack_value(value, size))
    return bytes(out), labels


def parse_condition(path, line, actor, tokens):
    command = tokens[0]
    if len(tokens) < 4:
        raise CompileError("%s:%d: incomplete %s condition"
                           % (path, line, command))
    var_name = tokens[1]
    if var_name not in VAR_OPCODES:
        raise CompileError("%s:%d: unknown variable %s"
                           % (path, line, var_name))
    var_opcode = VAR_OPCODES[var_name]
    pos = 2
    var_param = None
    if var_opcode in defs.VAR_HAS_PARAM:
        var_param = parse_int(tokens[pos], actor)
        pos += 1
    if pos + 2 != len(tokens):
        raise CompileError("%s:%d: malformed %s condition"
                           % (path, line, command))
    operator = tokens[pos]
    if operator not in OPERATOR_OPCODES:
        raise CompileError("%s:%d: unknown operator %s"
                           % (path, line, operator))
    value_token = tokens[pos + 1]
    if var_opcode == 20 and value_token in BEHAVIOUR_IDS:
        value = BEHAVIOUR_IDS[value_token]
    else:
        value = parse_int(value_token, actor)
    return {
        "kind": "condition",
        "name": command,
        "opcode": LIFE_OPCODES[command],
        "var_opcode": var_opcode,
        "var_param": var_param,
        "operator": OPERATOR_OPCODES[operator],
        "value": value,
    }


def parse_life_source(path, actor):
    instructions = []
    for line, indent, tokens, target in source_lines(path):
        name = tokens[0]
        base = {
            "line": line,
            "indent": indent,
            "name": name,
            "target_override": target,
        }
        if name == "COMPORTMENT":
            if len(tokens) != 2:
                raise CompileError("%s:%d: COMPORTMENT expects a name"
                                   % (path, line))
            base.update(kind="virtual", comp_name=tokens[1])
        elif name == "ENDIF":
            base.update(kind="virtual")
        elif name in CONDITION_COMMANDS:
            base.update(parse_condition(path, line, actor, tokens))
            base["indent"] = indent
            base["line"] = line
        elif name == "SWITCH":
            if len(tokens) < 2:
                raise CompileError("%s:%d: SWITCH expects a variable"
                                   % (path, line))
            var_name = tokens[1]
            if var_name not in VAR_OPCODES:
                raise CompileError("%s:%d: unknown variable %s"
                                   % (path, line, var_name))
            var_opcode = VAR_OPCODES[var_name]
            expected = 3 if var_opcode in defs.VAR_HAS_PARAM else 2
            if len(tokens) != expected:
                raise CompileError("%s:%d: malformed SWITCH" % (path, line))
            base.update(
                kind="switch",
                opcode=LIFE_OPCODES[name],
                var_opcode=var_opcode,
                var_param=(parse_int(tokens[2], actor)
                           if expected == 3 else None),
            )
        elif name in {"CASE", "OR_CASE"}:
            if len(tokens) != 3 or tokens[1] not in OPERATOR_OPCODES:
                raise CompileError("%s:%d: malformed %s"
                                   % (path, line, name))
            base.update(
                kind="case",
                opcode=LIFE_OPCODES[name],
                operator=OPERATOR_OPCODES[tokens[1]],
                value=parse_int(tokens[2], actor),
            )
        else:
            if name not in LIFE_OPCODES:
                raise CompileError("%s:%d: unknown life command %s"
                                   % (path, line, name))
            base.update(
                kind="command",
                opcode=LIFE_OPCODES[name],
                args=tokens[1:],
            )
        instructions.append(base)
    return instructions


def command_size(instruction, switch_sizes):
    kind = instruction["kind"]
    if kind == "virtual":
        return 0
    if kind == "condition":
        var_opcode = instruction["var_opcode"]
        return (1 + 1 + (1 if instruction["var_param"] is not None else 0)
                + 1 + defs.VAR_RETURN_SIZE[var_opcode] + 2)
    if kind == "switch":
        return 1 + 1 + (1 if instruction["var_param"] is not None else 0)
    if kind == "case":
        return 1 + 2 + 1 + instruction["case_size"]
    opcode = instruction["opcode"]
    sizes = defs.LIFE_SIZES[opcode]
    if sizes is None:
        return 1 + len(instruction["args"][0].encode("latin-1")) + 1
    size = 1 + sum(sizes)
    if opcode in {27, 28}:
        mode_token = instruction["args"][0 if opcode == 27 else 1]
        mode = DIRMODE_IDS.get(mode_token)
        if mode in defs.DIRMODE_REQUIRES_ACTOR:
            size += 1
    return size


def assign_switches(instructions):
    stack = []
    switch_sizes = {}
    switch_id = 0
    case_size = 0
    for instruction in instructions:
        name = instruction["name"]
        while (stack and name != "END_SWITCH"
               and instruction["indent"] <= stack[-1][0] + 2
               and name not in ({"CASE", "OR_CASE", "DEFAULT", "BREAK",
                                 "ENDIF"} | IF_COMMANDS | CHAIN_COMMANDS)):
            stack.pop()
        if name in {"END_COMPORTMENT", "COMPORTMENT", "END"}:
            stack = []
        if name == "SWITCH":
            instruction["switch_id"] = switch_id
            case_size = defs.VAR_RETURN_SIZE[instruction["var_opcode"]]
            switch_sizes[switch_id] = case_size
            stack.append((instruction["indent"], switch_id))
            switch_id += 1
        elif name in {"CASE", "OR_CASE"}:
            if not stack:
                raise CompileError("CASE outside SWITCH")
            instruction["switch_id"] = stack[-1][1]
            instruction["case_size"] = case_size
        elif name == "END_SWITCH":
            if stack:
                instruction["switch_id"] = stack[-1][1]
                stack.pop()
        elif name == "BREAK":
            if not stack:
                raise CompileError("BREAK outside SWITCH")
            instruction["switch_id"] = stack[-1][1]
    return switch_sizes


def assign_offsets(instructions, switch_sizes):
    offset = 0
    comp_offsets = {}
    for instruction in instructions:
        instruction["offset"] = offset
        if instruction["name"] == "COMPORTMENT":
            name = instruction["comp_name"]
            if name in comp_offsets:
                raise CompileError("duplicate COMPORTMENT %s" % name)
            comp_offsets[name] = offset
        offset += command_size(instruction, switch_sizes)
        instruction["end_offset"] = offset
    return comp_offsets


def find_block_targets(instructions):
    endifs = {}
    elses = {}
    stack = []
    for index, instruction in enumerate(instructions):
        name = instruction["name"]
        if name in {"COMPORTMENT", "END"}:
            while stack:
                endifs[stack.pop()] = index
        if name in IF_COMMANDS:
            stack.append(index)
        elif name == "ELSE":
            if not stack:
                raise CompileError("ELSE without IF")
            elses[stack[-1]] = index
        elif name == "ENDIF":
            if stack:
                endifs[stack.pop()] = index
    while stack:
        endifs[stack.pop()] = len(instructions) - 1
    return endifs, elses


def next_main_condition(instructions, start):
    for index in range(start + 1, len(instructions)):
        if instructions[index]["name"] in IF_COMMANDS:
            return index
    raise CompileError("condition chain has no terminating IF")


def branch_targets(instructions):
    endifs, elses = find_block_targets(instructions)
    targets = {}
    for index, instruction in enumerate(instructions):
        name = instruction["name"]
        if name in IF_COMMANDS:
            end_index = endifs[index]
            if index in elses:
                targets[index] = instructions[elses[index]]["end_offset"]
            else:
                targets[index] = instructions[end_index]["offset"]
            if index in elses:
                targets[elses[index]] = instructions[end_index]["offset"]
        elif name == "OR_IF":
            main = next_main_condition(instructions, index)
            targets[index] = instructions[main]["end_offset"]
        elif name == "AND_IF":
            main = next_main_condition(instructions, index)
            end_index = endifs[main]
            targets[index] = (
                instructions[elses[main]]["end_offset"]
                if main in elses
                else instructions[end_index]["offset"]
            )
    return targets


def switch_targets(instructions):
    targets = {}
    switch_stack = []
    endifs, _elses = find_block_targets(instructions)

    def enclosing_if(index, switch_index):
        candidates = [
            (start, end)
            for start, end in endifs.items()
            if start > switch_index and start < index < end
        ]
        return max(candidates, default=None)

    def close_switch(frame, end_offset):
        for break_index in frame["breaks"]:
            block = enclosing_if(break_index, frame["index"])
            targets[break_index] = (
                instructions[block[1]]["offset"] if block else end_offset
            )
        clauses = frame["clauses"]
        cases = [item for item in clauses
                 if instructions[item]["name"] in {"CASE", "OR_CASE"}]
        for case_index in cases:
            case = instructions[case_index]
            if case["name"] == "OR_CASE":
                following = None
                block = enclosing_if(case_index, frame["index"])
                limit = block[1] if block else None
                for later in clauses[clauses.index(case_index) + 1:]:
                    if limit is not None and later >= limit:
                        break
                    if instructions[later]["name"] == "CASE":
                        following = later
                        break
                targets[case_index] = (
                    instructions[following]["end_offset"]
                    if following is not None
                    else (instructions[limit]["offset"]
                          if limit is not None else end_offset)
                )
            else:
                pos = clauses.index(case_index)
                block = enclosing_if(case_index, frame["index"])
                limit = block[1] if block else None
                following = (
                    clauses[pos + 1]
                    if pos + 1 < len(clauses)
                    and (limit is None or clauses[pos + 1] < limit)
                    else None
                )
                targets[case_index] = (
                    instructions[following]["offset"]
                    if following is not None
                    else (instructions[limit]["offset"]
                          if limit is not None else end_offset)
                )

    for index, instruction in enumerate(instructions):
        name = instruction["name"]
        while (switch_stack and name != "END_SWITCH"
               and instruction["indent"] <= switch_stack[-1]["indent"] + 2
               and name not in ({"CASE", "OR_CASE", "DEFAULT", "BREAK",
                                 "ENDIF"} | IF_COMMANDS | CHAIN_COMMANDS)):
            close_switch(switch_stack.pop(), instruction["offset"])
        if name in {"END_COMPORTMENT", "COMPORTMENT", "END"}:
            while switch_stack:
                close_switch(switch_stack.pop(), instruction["offset"])
        if name == "SWITCH":
            switch_stack.append({"id": instruction["switch_id"],
                                 "index": index,
                                 "indent": instruction["indent"],
                                 "clauses": [], "breaks": []})
        elif name in {"CASE", "OR_CASE"}:
            switch_stack[-1]["clauses"].append(index)
        elif name == "DEFAULT":
            switch_stack[-1]["clauses"].append(index)
        elif name == "BREAK":
            switch_stack[-1]["breaks"].append(index)
        elif name == "END_SWITCH":
            if switch_stack:
                frame = switch_stack.pop()
                close_switch(frame, instruction["offset"])
    return targets


def numeric_args(instruction, actor):
    opcode = instruction["opcode"]
    args = instruction["args"]
    if opcode == 30:
        if len(args) != 1 or args[0] not in BEHAVIOUR_IDS:
            raise CompileError("invalid SET_BEHAVIOUR")
        return [BEHAVIOUR_IDS[args[0]]]
    if opcode in {27, 28}:
        actor_args = 0 if opcode == 27 else 1
        if len(args) < actor_args + 1:
            raise CompileError("invalid direction mode command")
        values = []
        if opcode == 28:
            values.append(parse_int(args[0], actor))
        mode_token = args[actor_args]
        if mode_token not in DIRMODE_IDS:
            raise CompileError("unknown direction mode %s" % mode_token)
        mode = DIRMODE_IDS[mode_token]
        values.append(mode)
        if mode in defs.DIRMODE_REQUIRES_ACTOR:
            if len(args) != actor_args + 2:
                raise CompileError("direction mode requires actor")
            values.append(parse_int(args[-1], actor))
        elif len(args) != actor_args + 1:
            raise CompileError("unexpected direction mode parameter")
        return values
    sizes = defs.LIFE_SIZES[opcode]
    if sizes is None:
        if len(args) != 1:
            raise CompileError("%s expects text parameter" % instruction["name"])
        return args
    if opcode in {15, 117}:
        if args:
            raise CompileError("%s has no visible parameters"
                               % instruction["name"])
        return []
    if opcode in {33, 34}:
        return args
    if len(args) != len(sizes):
        raise CompileError("%s expects %d parameters"
                           % (instruction["name"], len(sizes)))
    return [parse_int(token, actor) for token in args]


def compile_life_sources(paths, track_labels):
    actors = [parse_life_source(path, actor)
              for actor, path in enumerate(paths)]
    switch_sizes = []
    comp_offsets = []
    for instructions in actors:
        sizes = assign_switches(instructions)
        switch_sizes.append(sizes)
        comp_offsets.append(assign_offsets(instructions, sizes))

    binaries = []
    for actor, instructions in enumerate(actors):
        try:
            branches = branch_targets(instructions)
            switches = switch_targets(instructions)
        except CompileError as exc:
            raise CompileError("actor %d: %s" % (actor, exc))
        out = bytearray()
        for index, instruction in enumerate(instructions):
            kind = instruction["kind"]
            name = instruction["name"]
            if kind == "virtual":
                continue
            opcode = instruction["opcode"]
            out.append(opcode)
            if kind == "condition":
                var_opcode = instruction["var_opcode"]
                out.append(var_opcode)
                if instruction["var_param"] is not None:
                    out.append(instruction["var_param"] & 0xFF)
                out.append(instruction["operator"])
                out.extend(pack_value(
                    instruction["value"], defs.VAR_RETURN_SIZE[var_opcode]))
                target = instruction["target_override"]
                out.extend(pack_value(
                    branches[index] if target is None else target, 2))
            elif kind == "switch":
                out.append(instruction["var_opcode"])
                if instruction["var_param"] is not None:
                    out.append(instruction["var_param"] & 0xFF)
            elif kind == "case":
                target = instruction["target_override"]
                out.extend(pack_value(
                    switches[index] if target is None else target, 2))
                out.append(instruction["operator"])
                out.extend(pack_value(
                    instruction["value"],
                    instruction["case_size"]))
            else:
                values = numeric_args(instruction, actor)
                sizes = defs.LIFE_SIZES[opcode]
                if sizes is None:
                    out.extend(values[0].encode("latin-1"))
                    out.append(0)
                elif opcode == 15:
                    target = instruction["target_override"]
                    out.extend(pack_value(
                        (branches.get(index, instruction["end_offset"])
                         if target is None else target), 2))
                elif opcode == 117:
                    target = instruction["target_override"]
                    out.extend(pack_value(
                        switches[index] if target is None else target, 2))
                elif opcode == 23:
                    label = parse_int(values[0])
                    target = -1 if label == -1 else track_labels[actor][label]
                    out.extend(pack_value(target, 2))
                elif opcode == 24:
                    target_actor = parse_int(values[0], actor)
                    label = parse_int(values[1])
                    target = (-1 if label == -1
                              else track_labels[target_actor][label])
                    out.append(target_actor & 0xFF)
                    out.extend(pack_value(target, 2))
                elif opcode == 33:
                    target = values[0]
                    target = (65535 if target == "break"
                              else comp_offsets[actor][target])
                    out.extend(pack_value(target, 2))
                elif opcode == 34:
                    target_actor = parse_int(values[0], actor)
                    target = values[1]
                    target = (65535 if target == "break"
                              else comp_offsets[target_actor][target])
                    out.append(target_actor & 0xFF)
                    out.extend(pack_value(target, 2))
                elif opcode in {27, 28}:
                    base_count = len(sizes)
                    for value, size in zip(values[:base_count], sizes):
                        out.extend(pack_value(value, size))
                    if len(values) > base_count:
                        out.append(values[-1] & 0xFF)
                else:
                    for value, size in zip(values, sizes):
                        out.extend(pack_value(value, size))
        binaries.append(bytes(out))
    return binaries


def original_targets(binary):
    commands, _comp_offsets = defs.parse_life(binary)
    targets = {}
    for index, command in enumerate(commands):
        if command["type"] == "operator" and index >= 2:
            targets[commands[index - 2]["offset"]] = command["params"][1]
        elif command["type"] == "command" and command["opcode"] in {
                15, 114, 115, 117}:
            targets[command["offset"]] = command["params"][0]
    return targets


def add_target_metadata(path, line_targets):
    lines = open(path, encoding="utf-8").read().splitlines(True)
    for line_number, target in line_targets.items():
        index = line_number - 1
        ending = "\n" if lines[index].endswith("\n") else ""
        line = lines[index].rstrip("\n")
        separator = " | " if "//" in line else " // "
        lines[index] = line + separator + "@target=%d" % target + ending
    with open(path, "w", encoding="utf-8", newline="\n") as target:
        target.writelines(lines)


def annotate_life_targets(paths, binaries):
    for actor, (path, binary) in enumerate(zip(paths, binaries)):
        instructions = parse_life_source(path, actor)
        sizes = assign_switches(instructions)
        assign_offsets(instructions, sizes)
        branches = branch_targets(instructions)
        switches = switch_targets(instructions)
        actual = original_targets(binary)
        metadata = {}
        for index, instruction in enumerate(instructions):
            if instruction["offset"] not in actual:
                continue
            if instruction["kind"] == "condition" or instruction["name"] == "ELSE":
                predicted = branches.get(index, instruction["end_offset"])
            elif instruction["kind"] == "case" or instruction["name"] == "BREAK":
                predicted = switches.get(index, instruction["end_offset"])
            else:
                continue
            target = actual[instruction["offset"]]
            if predicted != target:
                metadata[instruction["line"]] = target
        if metadata:
            add_target_metadata(path, metadata)
    return sum(
        1 for path in paths
        if "@target=" in open(path, encoding="utf-8").read()
    )


def scene_script_layout(raw, scene):
    pos = 62
    actors = []
    track_size = struct.unpack_from("<H", raw, pos)[0]
    track_size_pos = pos
    pos += 2
    track_start = pos
    pos += track_size
    life_size_pos = pos
    life_size = struct.unpack_from("<H", raw, pos)[0]
    pos += 2
    life_start = pos
    pos += life_size
    actors.append((track_size_pos, track_start, track_size,
                   life_size_pos, life_start, life_size))
    count = struct.unpack_from("<H", raw, pos)[0]
    pos += 2
    for actor in range(1, count):
        flags = struct.unpack_from("<I", raw, pos)[0]
        pos += 38
        if flags & (1 << 18):
            pos += 6
        track_size_pos = pos
        track_size = struct.unpack_from("<H", raw, pos)[0]
        pos += 2
        track_start = pos
        pos += track_size
        life_size_pos = pos
        life_size = struct.unpack_from("<H", raw, pos)[0]
        pos += 2
        life_start = pos
        pos += life_size
        actors.append((track_size_pos, track_start, track_size,
                       life_size_pos, life_start, life_size))
    return actors


def replace_scene_scripts(raw, layout, tracks, lifes):
    out = bytearray()
    pos = 0
    for actor, item in enumerate(layout):
        track_size_pos, track_start, track_size, life_size_pos, life_start, life_size = item
        out.extend(raw[pos:track_size_pos])
        out.extend(struct.pack("<H", len(tracks[actor])))
        out.extend(tracks[actor])
        out.extend(struct.pack("<H", len(lifes[actor])))
        out.extend(lifes[actor])
        pos = life_start + life_size
    out.extend(raw[pos:])
    return bytes(out)


def script_paths(directory, scene, actor):
    base = os.path.join(directory, "%d_%d" % (scene, actor))
    return base + "_track.ls2s", base + "_life.ls2s"


def rebuild_hqr(path, entries, archive, replacements):
    blobs = []
    for entry, offset, size, compressed_size, method in entries:
        if size is None:
            blobs.append(None)
        elif entry not in replacements:
            blobs.append(archive[offset:offset + 10 + compressed_size])
        else:
            payload = replacements[entry]
            original = hqr_inspect.decompress_entry(
                archive, offset, size, compressed_size, method)
            if payload == original:
                blobs.append(archive[offset:offset + 10 + compressed_size])
            else:
                blobs.append(struct.pack("<IIh", len(payload), len(payload), 0)
                             + payload)

    table_size = len(entries) * 4
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
    with open(path, "wb") as target:
        target.write(out)


def main():
    parser = argparse.ArgumentParser(
        description="Compile LBA2 .ls2s files into SCENE.HQR.")
    parser.add_argument("script_dir", help="Directory containing bulk .ls2s files")
    parser.add_argument("scene_hqr", help="Target SCENE.HQR to update")
    args = parser.parse_args()

    entries, archive = hqr_inspect.entries(args.scene_hqr)
    replacements = {}
    script_count = 0
    for entry, offset, size, compressed_size, method in entries[1:]:
        if size is None:
            continue
        scene = entry - 1
        raw = hqr_inspect.decompress_entry(
            archive, offset, size, compressed_size, method)
        layout = scene_script_layout(raw, scene)
        tracks = []
        track_labels = []
        life_paths = []
        for actor in range(len(layout)):
            track_path, life_path = script_paths(args.script_dir, scene, actor)
            if not os.path.isfile(track_path) or not os.path.isfile(life_path):
                raise CompileError(
                    "missing scripts for scene %d actor %d" % (scene, actor))
            track, labels = compile_track_source(track_path)
            tracks.append(track)
            track_labels.append(labels)
            life_paths.append(life_path)
            script_count += 2
        try:
            lifes = compile_life_sources(life_paths, track_labels)
        except (CompileError, KeyError) as exc:
            raise CompileError("scene %d: %s" % (scene, exc))
        replacements[entry] = replace_scene_scripts(
            raw, layout, tracks, lifes)

    rebuild_hqr(args.scene_hqr, entries, archive, replacements)
    print("compiled %d scripts into %s" % (script_count, args.scene_hqr))


if __name__ == "__main__":
    try:
        main()
    except (CompileError, KeyError, OSError, struct.error) as exc:
        print("script_compile.py: error: %s" % exc, file=sys.stderr)
        sys.exit(1)
