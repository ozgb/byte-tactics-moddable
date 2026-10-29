#include "format.h"
#include "json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char* text;
    size_t size;
    size_t at;
    const char* name;
    char* error;
    size_t error_size;
    int failed;
} reader;

static void fail(reader* r, const char* message)
{
    size_t i;
    int line = 1;
    if (r->failed) {
        return;
    }
    r->failed = 1;
    for (i = 0; i < r->at && i < r->size; i++) {
        if (r->text[i] == '\n') {
            line++;
        }
    }
    wl_format(r->error, r->error_size, "%s:%d: %s", r->name, line, message);
}

static void skip_space(reader* r)
{
    while (r->at < r->size && (r->text[r->at] == ' ' || r->text[r->at] == '\t' || r->text[r->at] == '\n' || r->text[r->at] == '\r')) {
        r->at++;
    }
}

static int peek(reader* r)
{
    skip_space(r);
    return r->at < r->size ? (unsigned char)r->text[r->at] : -1;
}

void wl_json_free(wl_json* node)
{
    while (node) {
        wl_json* next = node->next;
        wl_json_free(node->first);
        free(node->text);
        free(node->key);
        free(node);
        node = next;
    }
}

static char* parse_string(reader* r)
{
    size_t capacity = 16;
    size_t length = 0;
    char* out = (char*)malloc(capacity);
    r->at++;
    while (1) {
        int c;
        if (r->at >= r->size) {
            fail(r, "unterminated string");
            free(out);
            return NULL;
        }
        c = (unsigned char)r->text[r->at++];
        if (c == '"') {
            break;
        }
        if (c < 0x20) {
            fail(r, "control character in string");
            free(out);
            return NULL;
        }
        if (c == '\\') {
            if (r->at >= r->size) {
                fail(r, "unterminated string");
                free(out);
                return NULL;
            }
            c = (unsigned char)r->text[r->at++];
            switch (c) {
            case '"':
            case '\\':
            case '/':
                break;
            case 'n':
                c = '\n';
                break;
            case 't':
                c = '\t';
                break;
            default:
                r->at--;
                fail(r, "unsupported escape in string");
                free(out);
                return NULL;
            }
        }
        if (length + 2 > capacity) {
            capacity *= 2;
            out = (char*)realloc(out, capacity);
        }
        out[length++] = (char)c;
    }
    out[length] = 0;
    return out;
}

static wl_json* parse_value(reader* r, int depth);

static wl_json* parse_container(reader* r, int depth, int object)
{
    wl_json* node = (wl_json*)calloc(1, sizeof *node);
    wl_json** tail = &node->first;
    int close = object ? '}' : ']';
    node->kind = object ? WL_JSON_OBJECT : WL_JSON_ARRAY;
    r->at++;
    if (peek(r) == close) {
        r->at++;
        return node;
    }
    while (1) {
        wl_json* item;
        char* key = NULL;
        if (object) {
            if (peek(r) != '"') {
                fail(r, "expected a quoted key");
                break;
            }
            key = parse_string(r);
            if (!key) {
                break;
            }
            if (peek(r) != ':') {
                free(key);
                fail(r, "expected ':' after a key");
                break;
            }
            r->at++;
        }
        item = parse_value(r, depth + 1);
        if (!item) {
            free(key);
            break;
        }
        if (object) {
            wl_json* seen;
            for (seen = node->first; seen; seen = seen->next) {
                if (!strcmp(seen->key, key)) {
                    fail(r, "duplicate key");
                    break;
                }
            }
        }
        item->key = key;
        *tail = item;
        tail = &item->next;
        if (r->failed) {
            break;
        }
        {
            int c = peek(r);
            if (c == ',') {
                r->at++;
                continue;
            }
            if (c == close) {
                r->at++;
                return node;
            }
            fail(r, object ? "expected ',' or '}'" : "expected ',' or ']'");
            break;
        }
    }
    wl_json_free(node);
    return NULL;
}

static wl_json* parse_value(reader* r, int depth)
{
    wl_json* node;
    int c = peek(r);
    if (depth > 16) {
        fail(r, "nested too deeply");
        return NULL;
    }
    if (c == '{') {
        return parse_container(r, depth, 1);
    }
    if (c == '[') {
        return parse_container(r, depth, 0);
    }
    node = (wl_json*)calloc(1, sizeof *node);
    if (c == '"') {
        node->kind = WL_JSON_STRING;
        node->text = parse_string(r);
        if (node->text) {
            return node;
        }
    } else if (c == '-' || (c >= '0' && c <= '9')) {
        long value = 0;
        int negative = 0;
        if (c == '-') {
            negative = 1;
            r->at++;
        }
        if (r->at >= r->size || r->text[r->at] < '0' || r->text[r->at] > '9') {
            fail(r, "malformed number");
        } else if (r->text[r->at] == '0' && r->at + 1 < r->size && r->text[r->at + 1] >= '0' && r->text[r->at + 1] <= '9') {
            fail(r, "number with a leading zero");
        } else {
            while (r->at < r->size && r->text[r->at] >= '0' && r->text[r->at] <= '9') {
                if (value > (0x7fffffffL - 9) / 10) {
                    fail(r, "number out of range");
                    break;
                }
                value = value * 10 + (r->text[r->at++] - '0');
            }
            if (!r->failed && r->at < r->size && (r->text[r->at] == '.' || r->text[r->at] == 'e' || r->text[r->at] == 'E')) {
                fail(r, "only integers are allowed");
            }
        }
        if (!r->failed) {
            node->kind = WL_JSON_NUMBER;
            node->number = negative ? -value : value;
            return node;
        }
    } else if (c == 't' && r->size - r->at >= 4 && !memcmp(r->text + r->at, "true", 4)) {
        r->at += 4;
        node->kind = WL_JSON_BOOL;
        node->number = 1;
        return node;
    } else if (c == 'f' && r->size - r->at >= 5 && !memcmp(r->text + r->at, "false", 5)) {
        r->at += 5;
        node->kind = WL_JSON_BOOL;
        return node;
    } else if (c == 'n' && r->size - r->at >= 4 && !memcmp(r->text + r->at, "null", 4)) {
        r->at += 4;
        node->kind = WL_JSON_NULL;
        return node;
    } else {
        fail(r, "unexpected character");
    }
    wl_json_free(node);
    return NULL;
}

wl_json* wl_json_parse(const char* text, size_t size, const char* name, char* error, size_t error_size)
{
    reader r;
    wl_json* root;
    r.text = text;
    r.size = size;
    r.at = 0;
    r.name = name;
    r.error = error;
    r.error_size = error_size;
    r.failed = 0;
    root = parse_value(&r, 0);
    if (root && peek(&r) != -1) {
        fail(&r, "text after the end of the document");
        wl_json_free(root);
        root = NULL;
    }
    if (!root && !r.failed) {
        fail(&r, "malformed document");
    }
    return root;
}

const wl_json* wl_json_get(const wl_json* object, const char* key)
{
    const wl_json* item;
    if (!object || object->kind != WL_JSON_OBJECT) {
        return NULL;
    }
    for (item = object->first; item; item = item->next) {
        if (!strcmp(item->key, key)) {
            return item;
        }
    }
    return NULL;
}

size_t wl_json_count(const wl_json* node)
{
    size_t n = 0;
    const wl_json* item;
    for (item = node ? node->first : NULL; item; item = item->next) {
        n++;
    }
    return n;
}
