#!/usr/bin/env python3
"""Bulk-decompile LBA2 life and track scripts from SCENE.HQR."""

import argparse
import json
import os
import struct
import sys

import hqr_inspect
import text_hqr


MAPPINGS_DIR = os.path.join(os.path.dirname(__file__), "mappings")


def load_json_mapping(filename):
    path = os.path.join(MAPPINGS_DIR, filename)
    with open(path, encoding="utf-8") as source:
        return json.load(source)


SCENE_DESCRIPTIONS = [
    scene["name"] for scene in load_json_mapping("lba2_scenes.json")["scenes"]
]
GAME_VARS = {
    flag["id"]: flag
    for flag in load_json_mapping("lba2_game_vars.json")["flags"]
    if flag.get("name")
}


TRACK_NAMES = [
    "END", "NOP", "BODY", "ANIM", "GOTO_POINT", "WAIT_ANIM", "LOOP",
    "ANGLE", "POS_POINT", "LABEL", "GOTO", "STOP", "GOTO_SYM_POINT",
    "WAIT_NUM_ANIM", "SAMPLE", "GOTO_POINT_3D", "SPEED", "BACKGROUND",
    "WAIT_NUM_SECOND", "NO_BODY", "BETA", "OPEN_LEFT", "OPEN_RIGHT",
    "OPEN_UP", "OPEN_DOWN", "CLOSE", "WAIT_DOOR", "SAMPLE_RND",
    "SAMPLE_ALWAYS", "SAMPLE_STOP", "PLAY_SMK", "REPEAT_SAMPLE",
    "SIMPLE_SAMPLE", "FACE_TWINSEN", "ANGLE_RND", "REPLACE",
    "WAIT_NUM_DSEC", "DO", "SPRITE", "WAIT_NUM_SEC_RND", "AFF_TIMER",
    "SET_FRAME", "SET_FRAME_3DS", "SET_START_3DS", "SET_END_3DS",
    "START_ANIM_3DS", "STOP_ANIM_3DS", "WAIT_ANIM_3DS",
    "WAIT_FRAME_3DS", "WAIT_NUM_DSEC_RND", "INTERVAL", "FREQUENCY",
    "VOLUME",
]

# Parameter sizes from Builder/SceneLib/SceneLib2Tab.pas Track2Props.
TRACK_SIZES = [
    (), (), (1,), (2,), (1,), (), (), (2,), (1,), (1,), (2,), (), (1,),
    (1, 1), (2,), (1,), (2,), (1,), (1, 4), (), (2,), (2,), (2,),
    (2,), (2,), (), (), (2,), (2,), (2,), None, (2,), (2,), (2,),
    (2, 2), (), (1, 4), (), (2,), (1, 4), (), (1,), (1,), (1,), (1,),
    (1,), (), (), (), (1, 4), (2,), (2,), (1,),
]
TRACK_DUMMY_FIRST = {33}
TRACK_DUMMY_AFTER_FIRST = {13, 18, 34, 36, 39, 49}
TRACK_SIGNED_8 = {2}
TRACK_SIGNED_16 = {7, 20, 21, 22, 23, 24, 50, 51}

LIFE_NAMES = [
    "END", "NOP", "SNIF", "OFFSET", "NEVERIF", "", "", "", "", "",
    "PALETTE", "RETURN", "IF", "SWIF", "ONEIF", "ELSE", "ENDIF", "BODY",
    "BODY_OBJ", "ANIM", "ANIM_OBJ", "SET_CAMERA", "CAMERA_CENTER",
    "SET_TRACK", "SET_TRACK_OBJ", "MESSAGE", "CAN_FALL", "SET_DIRMODE",
    "SET_DIRMODE_OBJ", "CAM_FOLLOW", "SET_BEHAVIOUR", "SET_VAR_CUBE",
    "COMPORTMENT", "SET_COMPORTMENT", "SET_COMPORTMENT_OBJ",
    "END_COMPORTMENT", "SET_VAR_GAME", "KILL_OBJ", "SUICIDE",
    "USE_ONE_LITTLE_KEY", "GIVE_GOLD_PIECES", "END_LIFE", "STOP_TRACK",
    "RESTORE_TRACK", "MESSAGE_OBJ", "INC_CHAPTER", "FOUND_OBJECT",
    "SET_DOOR_LEFT", "SET_DOOR_RIGHT", "SET_DOOR_UP", "SET_DOOR_DOWN",
    "GIVE_BONUS", "CHANGE_CUBE", "OBJ_COL", "BRICK_COL", "OR_IF",
    "INVISIBLE", "SHADOW_OBJ", "POS_POINT", "SET_MAGIC_LEVEL",
    "SUB_MAGIC_POINT", "SET_LIFE_POINT_OBJ", "SUB_LIFE_POINT_OBJ",
    "HIT_OBJ", "PLAY_SMK", "LIGHTNING", "INC_CLOVER_BOX",
    "SET_USED_INVENTORY", "ADD_CHOICE", "ASK_CHOICE", "INIT_BUGGY",
    "MEMO_SLATE", "SET_HOLO_POS", "CLR_HOLO_POS", "ADD_FUEL", "SUB_FUEL",
    "SET_GRM", "SET_CHANGE_CUBE", "MESSAGE_ZOE", "FULL_POINT", "BETA",
    "FADE_TO_PAL", "ACTION", "SET_FRAME", "SET_SPRITE", "SET_FRAME_3DS",
    "IMPACT_OBJ", "IMPACT_POINT", "ADD_MESSAGE", "BALLOON", "NO_SHOCK",
    "ASK_CHOICE_OBJ", "CINEMA_MODE", "SAVE_HERO", "RESTORE_HERO",
    "ANIM_SET", "RAIN", "GAME_OVER", "THE_END", "CONVEYOR", "PLAY_MUSIC",
    "TRACK_TO_VAR_GAME", "VAR_GAME_TO_TRACK", "ANIM_TEXTURE",
    "ADD_MESSAGE_OBJ", "BRUTAL_EXIT", "REPLACE", "LADDER", "SET_ARMOUR",
    "SET_ARMOUR_OBJ", "ADD_LIFE_POINT_OBJ", "STATE_INVENTORY", "AND_IF",
    "SWITCH", "OR_CASE", "CASE", "DEFAULT", "BREAK", "END_SWITCH",
    "SET_HIT_ZONE", "SAVE_COMP", "RESTORE_COMP", "SAMPLE", "SAMPLE_RND",
    "SAMPLE_ALWAYS", "SAMPLE_STOP", "REPEAT_SAMPLE", "BACKGROUND",
    "ADD_VAR_GAME", "SUB_VAR_GAME", "ADD_VAR_CUBE", "SUB_VAR_CUBE", "",
    "SET_RAIL", "INVERSE_BETA", "NO_BODY", "ADD_GOLD_PIECES",
    "STOP_TRACK_OBJ", "RESTORE_TRACK_OBJ", "SAVE_COMP_OBJ",
    "RESTORE_COMP_OBJ", "SPY", "DEBUG", "DEBUG_OBJ", "POPCORN",
    "FLOW_POINT", "FLOW_OBJ", "SET_ANIM_DIAL", "PCX", "END_MESSAGE",
    "END_MESSAGE_OBJ", "PARM_SAMPLE", "NEW_SAMPLE", "POS_OBJ_AROUND",
    "PCX_MESS_OBJ",
]

# None means a zero-terminated string. Sizes are from Life2Props.
LIFE_SIZES_BY_OPCODE = {
    3: (2,), 10: (1,), 15: (2,), 17: (1,), 18: (1, 1), 19: (2,),
    20: (1, 2), 21: (2,), 22: (1,), 23: (2,), 24: (1, 2), 25: (2,),
    26: (1,), 27: (1,), 28: (1, 1), 29: (1,), 30: (1,), 31: (1, 1),
    32: (1,), 33: (2,), 34: (1, 2), 36: (1, 2), 37: (1,), 40: (2,),
    44: (1, 2), 46: (1,), 47: (2,), 48: (2,), 49: (2,), 50: (2,),
    51: (1,), 52: (1,), 53: (1,), 54: (1,), 56: (1,), 57: (1, 1),
    58: (1,), 59: (1,), 60: (1,), 61: (1, 1), 62: (1, 1), 63: (1, 1),
    64: None, 65: (1,), 67: (1,), 68: (2,), 69: (2,), 70: (1,),
    71: (1,), 72: (1,), 73: (1,), 74: (1,), 75: (1,), 76: (1, 1),
    77: (2,), 78: (2,), 80: (2,), 81: (1,), 83: (1,), 84: (2,),
    85: (1,), 86: (1, 2, 2), 87: (1, 2), 88: (2,), 89: (1,),
    90: (1,), 91: (1, 2), 92: (1,), 95: (2,), 96: (1,), 99: (2,),
    100: (1,), 101: (1,), 102: (1,), 103: (1,), 104: (1, 2),
    107: (1, 1), 108: (1,), 109: (1, 1), 110: (1, 1), 111: (1, 1),
    114: (2, 1), 115: (2, 1), 117: (2,), 119: (1, 1), 122: (2,),
    123: (2,), 124: (2,), 125: (2,), 126: (2, 1), 127: (1,),
    128: (1, 2), 129: (1, 2), 130: (1, 1), 131: (1, 1), 133: (1, 1),
    136: (2,), 137: (1,), 138: (1,), 139: (1,), 140: (1,), 141: (1,),
    143: (1,), 145: (1, 1), 146: (1, 1), 147: (2,), 148: (2,),
    150: (1,), 151: (2, 1, 2), 152: (2, 2, 1, 2), 153: (1, 1),
    154: (1, 2, 2),
}
LIFE_SIZES = [LIFE_SIZES_BY_OPCODE.get(opcode, ())
              for opcode in range(len(LIFE_NAMES))]

LIFE_TYPES_IF = {2, 4, 12, 13, 14, 55, 112, 113}
LIFE_ACTOR_FIRST = {
    18, 20, 24, 28, 29, 34, 37, 44, 61, 62, 63, 86, 91, 104, 109,
    110, 137, 138, 139, 140, 143, 146, 150, 153, 154,
}
LIFE_SIGNED_FIELDS = {
    3: {0}, 17: {0}, 18: {1}, 21: {0}, 40: {0}, 47: {0}, 48: {0},
    49: {0}, 50: {0}, 80: {0},
}
DIRMODE_REQUIRES_ACTOR = {2, 4, 6, 9, 10, 11}
DIRMODES = [
    "NO_MOVE", "MANUAL", "FOLLOW", "", "", "", "SAME_XZ", "", "RAIL",
    "DIRMODE9", "DIRMODE10", "DIRMODE11", "DIRMODE12", "DIRMODE13",
]
BEHAVIOURS = [
    "NORMAL", "SPORTY", "AGGRESSIVE", "DISCREET", "JETPACK", "BEHAV5",
    "BEHAV6", "BEHAV7", "BEHAV8", "BEHAV9", "BEHAV10", "BEHAV11",
    "BEHAV12", "BEHAV13",
]
VAR_NAMES = [
    "COL", "COL_OBJ", "DISTANCE", "ZONE", "ZONE_OBJ", "BODY", "BODY_OBJ",
    "ANIM", "ANIM_OBJ", "CURRENT_TRACK", "CURRENT_TRACK_OBJ", "VAR_CUBE",
    "CONE_VIEW", "HIT_BY", "ACTION", "VAR_GAME", "LIFE_POINT",
    "LIFE_POINT_OBJ", "NUM_LITTLE_KEYS", "NUM_GOLD_PIECES", "BEHAVIOUR",
    "CHAPTER", "DISTANCE_3D", "MAGIC_LEVEL", "MAGIC_POINT", "USE_INVENTORY",
    "CHOICE", "FUEL", "CARRIED_BY", "CDROM", "LADDER", "RND", "RAIL",
    "BETA", "BETA_OBJ", "CARRIED_OBJ_BY", "ANGLE", "DISTANCE_MESSAGE",
    "HIT_OBJ_BY", "REAL_ANGLE", "DEMO", "COL_DECORS", "COL_DECORS_OBJ",
    "PROCESSOR", "OBJECT_DISPLAYED", "ANGLE_OBJ",
]
VAR_HAS_PARAM = {
    1, 2, 4, 6, 8, 10, 11, 12, 15, 17, 22, 25, 31, 32, 34, 35, 36,
    37, 38, 39, 42, 44, 45,
}
VAR_PARAM_ACTOR = {1, 2, 4, 6, 8, 10, 12, 17, 22, 34, 35, 36, 37, 38, 39, 42, 44, 45}
VAR_COMPARE_ACTOR = {0, 1, 13, 28, 35, 38}
VAR_RETURN_SIZE = [
    1, 1, 2, 1, 1, 1, 1, 2, 2, 1, 1, 1, 2, 1, 1, 2, 2, 2, 1, 2, 1,
    1, 2, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2, 1, 2, 2, 1, 2, 1, 1,
    1, 1, 1, 2,
]
VAR_SIGNED_RESULT = {5, 6, 31, 39}
OPERATORS = ["==", ">", "<", ">=", "<=", "!="]
LIFE_TEXT_PARAM = {
    25: 0,   # MESSAGE
    44: 1,   # MESSAGE_OBJ
    68: 0,   # ADD_CHOICE
    69: 0,   # ASK_CHOICE
    78: 0,   # MESSAGE_ZOE
    88: 0,   # ADD_MESSAGE
    91: 1,   # ASK_CHOICE_OBJ
    104: 1,  # ADD_MESSAGE_OBJ
    154: 2,  # PCX_MESS_OBJ
}


class DecodeError(ValueError):
    pass


def read_int(raw, pos, size, signed=False):
    if pos + size > len(raw):
        raise DecodeError("unexpected end of data at offset %d" % pos)
    return int.from_bytes(raw[pos : pos + size], "little", signed=signed), pos + size


def read_cstring(raw, pos):
    end = raw.find(b"\0", pos)
    if end < 0:
        raise DecodeError("unterminated string at offset %d" % pos)
    return raw[pos:end].decode("latin-1"), end + 1


def actor_name(value, actor):
    return "SELF" if value == actor else str(value)


def parse_scene_scripts(raw, scene):
    pos = 56
    pos += 6
    actors = []
    track_size, pos = read_int(raw, pos, 2)
    track = raw[pos : pos + track_size]
    pos += track_size
    life_size, pos = read_int(raw, pos, 2)
    life = raw[pos : pos + life_size]
    pos += life_size
    actors.append((track, life))
    count, pos = read_int(raw, pos, 2)
    if count < 1:
        raise DecodeError("scene %d has invalid actor count %d" % (scene, count))
    for actor in range(1, count):
        if pos + 38 > len(raw):
            raise DecodeError("scene %d actor %d header runs past entry" % (scene, actor))
        flags = struct.unpack_from("<I", raw, pos)[0]
        pos += 38
        if flags & (1 << 18):
            pos += 6
        track_size, pos = read_int(raw, pos, 2)
        track = raw[pos : pos + track_size]
        pos += track_size
        life_size, pos = read_int(raw, pos, 2)
        life = raw[pos : pos + life_size]
        pos += life_size
        actors.append((track, life))
    return actors


def parse_track(raw):
    commands = []
    labels = {}
    pos = 0
    while pos < len(raw):
        offset = pos
        opcode = raw[pos]
        pos += 1
        if opcode >= len(TRACK_NAMES):
            raise DecodeError("unknown track opcode %d at offset %d" % (opcode, offset))
        sizes = TRACK_SIZES[opcode]
        if sizes is None:
            text, pos = read_cstring(raw, pos)
            params = [text]
        else:
            params = []
            for index, size in enumerate(sizes):
                signed = ((size == 1 and opcode in TRACK_SIGNED_8)
                          or (size == 2 and opcode in TRACK_SIGNED_16))
                value, pos = read_int(raw, pos, size, signed)
                if opcode == 10 and value == 65535:
                    value = -1
                params.append(value)
        commands.append([offset, opcode, params])
        if opcode == 9:
            labels[offset] = params[0]
        if opcode == 0:
            break
    for command in commands:
        if command[1] == 10 and command[2][0] != -1:
            target = command[2][0]
            if target not in labels:
                raise DecodeError("track GOTO at offset %d has no LABEL at %d"
                                  % (command[0], target))
            command[2][0] = labels[target]
    return commands, labels


def render_track(commands):
    out = []
    for _offset, opcode, params in commands:
        fields = [TRACK_NAMES[opcode]]
        if TRACK_SIZES[opcode] is None:
            fields.append(params[0])
        elif params and opcode not in TRACK_DUMMY_FIRST:
            fields.append(str(params[0]))
        indent = "" if opcode in {0, 9, 35} else "  "
        out.append(indent + " ".join(fields) + " \n")
    result = "".join(out)
    return result[:-1] if result.endswith("\n") else result


def parse_life(raw):
    commands = []
    comp_offsets = {}
    offset_stack = []
    pos = 0
    previous = "command"
    last_command = -1
    comp_index = 0
    switch_size = 0

    if len(raw) > 1 and (raw[-1] != 0 or raw[-2] == 35):
        commands.append({"offset": 0, "opcode": 32, "type": "virtual",
                         "params": [], "text": "main"})
        comp_offsets[0] = "main"

    while pos < len(raw):
        while offset_stack and pos >= offset_stack[-1]:
            commands.append({"offset": pos, "opcode": 16, "type": "virtual",
                             "params": []})
            offset_stack.pop()

        offset = pos
        opcode = raw[pos]
        pos += 1
        command_context = previous in {"command", "operator", "switch_var"}

        if command_context:
            if opcode >= len(LIFE_NAMES):
                raise DecodeError("unknown life opcode %d at offset %d" % (opcode, offset))
            last_command = opcode
            command_type = "if" if opcode in LIFE_TYPES_IF else "command"
            sizes = LIFE_SIZES[opcode]
            if sizes is None:
                text, pos = read_cstring(raw, pos)
                params = []
            else:
                text = None
                params = []
                for index, size in enumerate(sizes):
                    signed = index in LIFE_SIGNED_FIELDS.get(opcode, set())
                    value, pos = read_int(raw, pos, size, signed)
                    if opcode in {23, 33} and index == 0 and value == 65535:
                        value = -1
                    if opcode in {24, 34} and index == 1 and value == 65535:
                        value = -1
                    params.append(value)
                if opcode in {27, 28}:
                    mode = params[-1]
                    if mode in DIRMODE_REQUIRES_ACTOR:
                        value, pos = read_int(raw, pos, 1)
                        params.append(value)
                if opcode in {114, 115}:
                    value, pos = read_int(raw, pos, switch_size,
                                          opcode == 115 and switch_size == 2)
                    params.append(value)
            command = {"offset": offset, "opcode": opcode, "type": command_type,
                       "params": params}
            if text is not None:
                command["text"] = text
            commands.append(command)
            if opcode == 15 and offset_stack:
                offset_stack[-1] = params[0]
            previous = command_type

        elif previous == "if":
            if opcode >= len(VAR_NAMES):
                raise DecodeError("unknown life variable %d at offset %d" % (opcode, offset))
            params = []
            if opcode in VAR_HAS_PARAM:
                value, pos = read_int(raw, pos, 1)
                params.append(value)
            command_type = "switch_var" if last_command == 113 else "variable"
            commands.append({"offset": offset, "opcode": opcode, "type": command_type,
                             "params": params})
            if command_type == "switch_var":
                switch_size = VAR_RETURN_SIZE[opcode]
            previous = command_type

        elif previous == "variable":
            if opcode >= len(OPERATORS):
                raise DecodeError("unknown life operator %d at offset %d" % (opcode, offset))
            var_opcode = commands[-1]["opcode"]
            size = VAR_RETURN_SIZE[var_opcode]
            value, pos = read_int(raw, pos, size, var_opcode in VAR_SIGNED_RESULT)
            target, pos = read_int(raw, pos, 2)
            commands.append({"offset": offset, "opcode": opcode, "type": "operator",
                             "params": [value, target]})
            if (last_command not in {55, 112}
                    and (len(commands) <= 5
                         or commands[-6]["opcode"] != 55
                         or commands[-4]["params"][1] <= target)):
                offset_stack.append(target)
            previous = "operator"

        else:
            raise DecodeError("invalid life command sequence at offset %d" % offset)

        if command_context and opcode == 35 and pos < len(raw) - 1 and not offset_stack:
            comp_index += 1
            name = str(comp_index)
            commands.append({"offset": offset, "opcode": 32, "type": "virtual",
                             "params": [], "text": name})
            comp_offsets[pos] = name

        if command_context and opcode == 0:
            break

    return commands, comp_offsets


def resolve_life(commands_by_actor, track_labels, comp_offsets):
    inserted_index = 1
    for actor, commands in enumerate(commands_by_actor):
        for command in reversed(commands):
            if command["type"] != "command":
                continue
            opcode = command["opcode"]
            if opcode == 33:
                target_actor = actor
                target = command["params"][0]
            elif opcode == 34:
                target_actor = command["params"][0]
                target = command["params"][1]
            else:
                continue
            if target == -1 or target in comp_offsets[target_actor]:
                continue
            target_commands = commands_by_actor[target_actor]
            for position, target_command in enumerate(target_commands):
                if target_command["offset"] == target:
                    name = "inserted_%d" % inserted_index
                    inserted_index += 1
                    target_commands.insert(
                        position,
                        {"offset": target, "opcode": 32, "type": "virtual",
                         "params": [], "text": name},
                    )
                    comp_offsets[target_actor][target] = name
                    break

    for actor, commands in enumerate(commands_by_actor):
        for command in commands:
            if command["type"] != "command":
                continue
            opcode = command["opcode"]
            params = command["params"]
            if opcode == 23 and params[0] != -1:
                params[0] = track_labels[actor][params[0]]
            elif opcode == 24 and params[1] != -1:
                params[1] = track_labels[params[0]][params[1]]
            elif opcode == 33:
                command["text"] = ("break" if params[0] == -1
                                   else comp_offsets[actor][params[0]])
            elif opcode == 34:
                command["text"] = ("break" if params[1] == -1
                                   else comp_offsets[params[0]][params[1]])


def comment_text(text):
    return " ".join(text.replace("\r", " ").replace("\n", " ").split())


def game_var_comment(flag_id, value):
    flag = GAME_VARS.get(flag_id)
    if flag is None:
        return None
    comment = flag["name"]
    value_description = flag.get("values", {}).get(str(value))
    if value_description:
        comment += " (%d = %s)" % (value, value_description)
    return comment


def render_life(commands, actor, texts=None):
    out = []
    indent = 0
    if_stack = []
    switch_stack = []

    for index, command in enumerate(commands):
        opcode = command["opcode"]
        command_type = command["type"]
        single_indent = 0

        if command_type in {"command", "virtual", "if"}:
            if opcode in {2, 4, 12, 13, 14}:
                if_stack.append(indent)
            elif opcode == 16:
                if if_stack:
                    indent = if_stack.pop()
            elif opcode == 32:
                indent = 0
            elif opcode == 15:
                single_indent = (if_stack[-1] - indent) if if_stack else -2
            elif opcode == 113:
                switch_stack.append(indent)
            elif opcode == 118:
                if switch_stack:
                    indent = switch_stack.pop()
            elif opcode == 35:
                indent = 0

        fields = []
        if command_type in {"command", "virtual", "if"}:
            fields.append(LIFE_NAMES[opcode])
            params = command["params"]
            if opcode == 32:
                fields.append(command["text"])
            elif opcode == 30:
                fields.append(BEHAVIOURS[params[0]])
            elif opcode in {27, 28}:
                if opcode == 28:
                    fields.append(actor_name(params[0], actor))
                    mode = params[1]
                else:
                    mode = params[0]
                fields.append(DIRMODES[mode] if mode < len(DIRMODES)
                              else "(Bad dir mode: %d)" % mode)
                if mode in DIRMODE_REQUIRES_ACTOR:
                    fields.append(str(params[-1]))
            elif opcode in {114, 115}:
                fields.extend((OPERATORS[params[1]], str(params[2])))
            elif opcode == 33 and "text" in command:
                fields.append(command["text"])
            elif opcode == 34 and "text" in command:
                fields.extend((actor_name(params[0], actor), command["text"]))
            elif LIFE_SIZES[opcode] is None:
                fields.append(command["text"])
            elif opcode not in {15, 117}:
                for param_index, value in enumerate(params):
                    if param_index == 0 and opcode in LIFE_ACTOR_FIRST:
                        fields.append(actor_name(value, actor))
                    else:
                        fields.append(str(value))
            line = " " * (indent + single_indent) + " ".join(fields) + " "
            if texts is not None and opcode in LIFE_TEXT_PARAM:
                text_id = params[LIFE_TEXT_PARAM[opcode]]
                if text_id in texts:
                    line += "// " + comment_text(texts[text_id])
            elif opcode == 36:
                annotation = game_var_comment(params[0], params[1])
                if annotation:
                    line += "// " + annotation
            out.append(line)
        elif command_type in {"variable", "switch_var"}:
            fields.append(VAR_NAMES[opcode])
            if command["params"]:
                value = command["params"][0]
                fields.append(actor_name(value, actor) if opcode in VAR_PARAM_ACTOR
                              else str(value))
            out.append(" ".join(fields) + " ")
        elif command_type == "operator":
            value = command["params"][0]
            variable = commands[index - 1]
            var_opcode = variable["opcode"]
            if var_opcode == 20 and 0 <= value < len(BEHAVIOURS):
                rendered_value = BEHAVIOURS[value]
            elif var_opcode in VAR_COMPARE_ACTOR:
                rendered_value = actor_name(value, actor)
            else:
                rendered_value = str(value)
            line = " ".join((OPERATORS[opcode], rendered_value)) + " "
            if var_opcode == 15:
                annotation = game_var_comment(variable["params"][0], value)
                if annotation:
                    line += "// " + annotation
            out.append(line)

        if command_type in {"command", "virtual", "if"}:
            if opcode in {2, 4, 12, 13, 14, 32, 113, 115, 116}:
                indent += 2
            elif opcode == 117:
                indent -= 2

        if command_type in {"command", "virtual"}:
            out[-1] += "\n"
        elif command_type == "switch_var" and commands[index - 1]["opcode"] == 113:
            out[-1] += "\n"
        elif command_type == "operator":
            out[-1] += "\n"

    result = "".join(out)
    return result[:-1] if result.endswith("\n") else result


def decompile_entry(raw, scene, texts=None):
    actors = parse_scene_scripts(raw, scene)
    track_commands = []
    track_labels = []
    life_commands = []
    comp_offsets = []
    for track, life in actors:
        commands, labels = parse_track(track)
        track_commands.append(commands)
        track_labels.append(labels)
        commands, offsets = parse_life(life)
        life_commands.append(commands)
        comp_offsets.append(offsets)
    resolve_life(life_commands, track_labels, comp_offsets)
    return [
        (
            render_track(track_commands[actor]),
            render_life(life_commands[actor], actor, texts),
        )
        for actor in range(len(actors))
    ]


def main():
    parser = argparse.ArgumentParser(
        description="Bulk-decompile all LBA2 scene life and track scripts.")
    parser.add_argument("scene_hqr", help="Path to LBA2 SCENE.HQR")
    parser.add_argument(
        "output_dir",
        help="Directory for <scene>_<actor>_{life,track}.ls2s",
    )
    parser.add_argument(
        "--text-hqr",
        help="Optional TEXT.HQR path; adds dialogue text as inline comments",
    )
    parser.add_argument(
        "--language",
        default="en",
        help="TEXT.HQR language: en, fr, de, sp, it, po (default: en)",
    )
    args = parser.parse_args()

    entries, archive = hqr_inspect.entries(args.scene_hqr)
    language = text_hqr.parse_language(args.language)
    text_banks = {}
    os.makedirs(args.output_dir, exist_ok=True)
    written = 0
    metadata_scripts = 0
    for entry, offset, size, compressed_size, method in entries[1:]:
        if size is None:
            continue
        scene = entry - 1
        try:
            raw = hqr_inspect.decompress_entry(
                archive, offset, size, compressed_size, method)
            texts = None
            if args.text_hqr:
                island = raw[0]
                file_index = 3 if island == 12 else 3 + island
                if file_index not in text_banks:
                    bank = text_hqr.load_bank(
                        args.text_hqr, language, file_index)
                    text_banks[file_index] = {
                        row["id"]: row["text"] for row in bank["texts"]
                    }
                texts = text_banks[file_index]
            scripts = decompile_entry(raw, scene, texts)
        except (DecodeError, KeyError, IndexError) as exc:
            raise DecodeError("scene %d (HQR entry %d): %s" % (scene, entry, exc))
        life_paths = []
        for actor, (track, life) in enumerate(scripts):
            base = "%d_%d" % (scene, actor)
            description = (
                SCENE_DESCRIPTIONS[scene]
                if scene < len(SCENE_DESCRIPTIONS)
                else "Scene %d" % scene
            )
            header = "// %s, Actor %d\n\n" % (description, actor)
            with open(os.path.join(args.output_dir, base + "_track.ls2s"),
                      "w", encoding="utf-8", newline="\n") as out:
                out.write(header + track)
            life_path = os.path.join(
                args.output_dir, base + "_life.ls2s")
            with open(life_path,
                      "w", encoding="utf-8", newline="\n") as out:
                out.write(header + life)
            life_paths.append(life_path)
            written += 2
        import script_compile
        metadata_scripts += script_compile.annotate_life_targets(
            life_paths, [life for _track, life in parse_scene_scripts(raw, scene)])
    print(
        "wrote %d scripts to %s (%d life scripts with target metadata)"
        % (written, args.output_dir, metadata_scripts)
    )


if __name__ == "__main__":
    try:
        main()
    except (DecodeError, OSError, ValueError, struct.error) as exc:
        print("script_decompile.py: error: %s" % exc, file=sys.stderr)
        sys.exit(1)
