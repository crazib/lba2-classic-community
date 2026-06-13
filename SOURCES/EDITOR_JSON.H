#ifndef EDITOR_JSON_H
#define EDITOR_JSON_H

#include <SYSTEM/ADELINE_TYPES.H>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define JSMN_STATIC
#define JSMN_PARENT_LINKS
#include "../LIB386/jsmn.h"

#define EDITOR_JSON_MAX_TOKENS 16384 * 64

static S32 EditorJsonTokenEnd(const jsmntok_t *tokens, S32 count, S32 index) {
    S32 end;

    if (index < 0 OR index >= count)
        return index;
    if (tokens[index].type == JSMN_OBJECT OR tokens[index].type == JSMN_ARRAY) {
        end = index + 1;
        while (end < count AND tokens[end].start < tokens[index].end)
            end++;
        return end;
    }
    return index + 1;
}

static S32 EditorJsonEquals(const char *json, const jsmntok_t *tok, const char *text) {
    S32 len;

    if (tok->type != JSMN_STRING)
        return FALSE;
    len = (S32)strlen(text);
    if (tok->end - tok->start != len)
        return FALSE;
    return strncmp(json + tok->start, text, len) == 0;
}

static S32 EditorJsonFindValue(const char *json, jsmntok_t *tokens, S32 count, S32 object, const char *key) {
    S32 i;
    S32 end;

    if (object < 0 OR object >= count OR tokens[object].type != JSMN_OBJECT)
        return -1;
    end = EditorJsonTokenEnd(tokens, count, object);
    i = object + 1;
    while (i < end) {
        S32 value = i + 1;

        if (tokens[i].parent == object AND value < count AND EditorJsonEquals(json, &tokens[i], key))
            return value;
        i = EditorJsonTokenEnd(tokens, count, value);
    }
    return -1;
}

static S32 EditorJsonParseInt(const char *json, jsmntok_t *tokens, S32 count, S32 index, S32 *value) {
    char buf[32];
    S32 len;
    char *endptr;
    long v;

    if (index < 0 OR index >= count OR tokens[index].type != JSMN_PRIMITIVE)
        return FALSE;
    len = tokens[index].end - tokens[index].start;
    if (len <= 0 OR len >= (S32)sizeof(buf))
        return FALSE;
    memcpy(buf, json + tokens[index].start, len);
    buf[len] = '\0';
    v = strtol(buf, &endptr, 10);
    if (*endptr)
        return FALSE;
    *value = (S32)v;
    return TRUE;
}

static S32 EditorJsonParseBool(const char *json, jsmntok_t *tokens, S32 count, S32 index, S32 *value) {
    S32 len;

    if (index < 0 OR index >= count OR tokens[index].type != JSMN_PRIMITIVE)
        return FALSE;
    len = tokens[index].end - tokens[index].start;
    if (len == 4 AND strncmp(json + tokens[index].start, "true", 4) == 0) {
        *value = TRUE;
        return TRUE;
    }
    if (len == 5 AND strncmp(json + tokens[index].start, "false", 5) == 0) {
        *value = FALSE;
        return TRUE;
    }
    return FALSE;
}

static S32 EditorJsonParseIntArray(const char *json, jsmntok_t *tokens, S32 count, S32 array, S32 *values, S32 expected) {
    S32 i;
    S32 found = 0;
    S32 end;

    if (array < 0 OR array >= count OR tokens[array].type != JSMN_ARRAY)
        return FALSE;
    end = EditorJsonTokenEnd(tokens, count, array);
    for (i = array + 1; i < end AND found < expected; i = EditorJsonTokenEnd(tokens, count, i)) {
        if (tokens[i].parent != array)
            continue;
        if (!EditorJsonParseInt(json, tokens, count, i, &values[found]))
            return FALSE;
        found++;
    }
    return found == expected;
}

static S32 EditorJsonReadFile(const char *path, char **out_text) {
    FILE *f;
    long size;
    char *text;

    *out_text = NULL;
    f = fopen(path, "rb");
    if (!f)
        return FALSE;
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return FALSE;
    }
    size = ftell(f);
    if (size < 0) {
        fclose(f);
        return FALSE;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return FALSE;
    }
    text = (char *)malloc((size_t)size + 1);
    if (!text) {
        fclose(f);
        return FALSE;
    }
    if (size > 0 AND fread(text, 1, (size_t)size, f) != (size_t)size) {
        free(text);
        fclose(f);
        return FALSE;
    }
    text[size] = '\0';
    fclose(f);
    *out_text = text;
    return TRUE;
}

static S32 EditorJsonFileExists(const char *path) {
    FILE *f = fopen(path, "rb");

    if (!f)
        return FALSE;
    fclose(f);
    return TRUE;
}

static S32 EditorJsonParseFile(const char *path, char **out_json, jsmntok_t **out_tokens, S32 *out_count) {
    char *json;
    jsmntok_t *tokens;
    jsmn_parser parser;
    S32 count;

    *out_json = NULL;
    *out_tokens = NULL;
    *out_count = 0;

    if (!EditorJsonReadFile(path, &json))
        return FALSE;

    tokens = (jsmntok_t *)malloc(sizeof(jsmntok_t) * EDITOR_JSON_MAX_TOKENS);
    if (!tokens) {
        free(json);
        return FALSE;
    }

    jsmn_init(&parser);
    count = jsmn_parse(&parser, json, strlen(json), tokens, EDITOR_JSON_MAX_TOKENS);
    if (count <= 0 OR tokens[0].type != JSMN_OBJECT) {
        free(tokens);
        free(json);
        return FALSE;
    }

    *out_json = json;
    *out_tokens = tokens;
    *out_count = count;
    return TRUE;
}

#endif /* EDITOR_JSON_H */
