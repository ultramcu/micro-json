/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 MaIII Themd
 */

/******************************************************************
 * micro-json (ujson) -- implementation
 * See ujson.h for scope, limitations and the public contract.
 ******************************************************************/

#include "ujson.h"

#include <stdlib.h>  /* strtol */

/* Append one byte to a key/value-string slot.
 * Reserves the final byte of the slot for the NUL terminator, so that
 * the slot is always a valid C string after every accepted byte.
 * Returns 1 on success, 0 if the slot would overflow. */
static uint8_t ujson_append_token(char *slot, uint16_t *cursor, uint8_t byte)
{
    if (*cursor >= (UJSON_STR_MAX - 1u)) {
        return 0;
    }
    slot[*cursor] = (char)byte;
    *cursor       = (uint16_t)(*cursor + 1u);
    slot[*cursor] = '\0';
    return 1;
}

/* True for ASCII whitespace skipped between structural tokens. */
static uint8_t ujson_is_ws(uint8_t b)
{
    return (b == ' ' || b == '\t' || b == '\r' || b == '\n');
}

/* Finalise a numeric value: parse the digit string into an int32_t,
 * mark the slot numeric, and bump pair_count. Used by every
 * terminator branch in the read_value_int state. */
static void ujson_finalise_int(st_ujson_parser *p)
{
    p->value[p->pair_count].i          = (int32_t)strtol(
                                              p->value[p->pair_count].str,
                                              NULL, 10);
    p->value[p->pair_count].is_numeric = 1u;
    p->pair_count++;
}

/******************************************************************
 * Public API
 ******************************************************************/

void ujson_init(st_ujson_parser *p, ujson_on_message_t on_message)
{
    if (p == NULL) {
        return;
    }
    ujson_reset(p);
    p->on_message = on_message;
}

void ujson_reset(st_ujson_parser *p)
{
    if (p == NULL) {
        return;
    }
    p->st         = _ujson_st_start_;
    p->pair_count = 0;
    p->tok_ptr    = 0;
    p->msg_ptr    = 0;
    memset(p->key,   0, sizeof(p->key));
    memset(p->value, 0, sizeof(p->value));
    memset(p->msg,   0, sizeof(p->msg));
    /* on_message is intentionally preserved across resets. */
}

uint8_t ujson_feed(st_ujson_parser *p, uint8_t byte)
{
    if (p == NULL) {
        return 0;
    }

    /* Capture into the raw msg buffer, dropping silently when full.
     * One byte is reserved for the NUL terminator. */
    if (p->msg_ptr < (UJSON_MSG_MAX - 1u)) {
        p->msg[p->msg_ptr++] = (char)byte;
        p->msg[p->msg_ptr]   = '\0';
    }

    switch (p->st) {

    case _ujson_st_start_:
        if (byte == '{') {
            p->st = _ujson_st_wait_dq_start_;
        }
        /* Anything before '{' (incl. garbage) is ignored. */
        break;

    case _ujson_st_wait_dq_start_:
        if (byte == '\"') {
            /* Refuse a new pair when the table is full. */
            if (p->pair_count >= UJSON_MAX_KV) {
                ujson_reset(p);
                return 0;
            }
            p->st      = _ujson_st_read_key_;
            p->tok_ptr = 0;
        }
        else if (ujson_is_ws(byte)) {
            /* skip whitespace */
        }
        else {
            ujson_reset(p);
            return 0;
        }
        break;

    case _ujson_st_read_key_:
        if (byte == '\"') {
            p->st = _ujson_st_wait_colon_;
        }
        else if (!ujson_append_token(p->key[p->pair_count].str,
                                     &p->tok_ptr, byte)) {
            ujson_reset(p);
            return 0;
        }
        break;

    case _ujson_st_wait_colon_:
        if (byte == ':') {
            p->st = _ujson_st_wait_dq_value_;
        }
        else if (ujson_is_ws(byte)) {
            /* skip whitespace */
        }
        else {
            ujson_reset(p);
            return 0;
        }
        break;

    case _ujson_st_wait_dq_value_:
        if (byte == '\"') {
            p->st      = _ujson_st_read_value_;
            p->tok_ptr = 0;
        }
        else if ((byte >= '0' && byte <= '9') || byte == '-') {
            /* Numeric literal: capture into value[].str and we'll
             * parse on terminator. The first byte (digit or '-') is
             * the first character of the literal. */
            p->st      = _ujson_st_read_value_int_;
            p->tok_ptr = 0;
            if (!ujson_append_token(p->value[p->pair_count].str,
                                    &p->tok_ptr, byte)) {
                ujson_reset(p);
                return 0;
            }
        }
        else if (ujson_is_ws(byte)) {
            /* skip whitespace */
        }
        else {
            ujson_reset(p);
            return 0;
        }
        break;

    case _ujson_st_read_value_:
        if (byte == '\"') {
            p->pair_count++;
            p->st = _ujson_st_wait_comma_or_end_;
        }
        else if (!ujson_append_token(p->value[p->pair_count].str,
                                     &p->tok_ptr, byte)) {
            ujson_reset(p);
            return 0;
        }
        break;

    case _ujson_st_read_value_int_:
        if (byte == ',') {
            ujson_finalise_int(p);
            p->st = _ujson_st_wait_dq_start_;
        }
        else if (byte == '}') {
            ujson_finalise_int(p);
            if (p->on_message != NULL) {
                p->on_message(p);
            }
            ujson_reset(p);
        }
        else if (ujson_is_ws(byte)) {
            ujson_finalise_int(p);
            p->st = _ujson_st_wait_comma_or_end_;
        }
        else if (byte >= '0' && byte <= '9') {
            if (!ujson_append_token(p->value[p->pair_count].str,
                                    &p->tok_ptr, byte)) {
                ujson_reset(p);
                return 0;
            }
        }
        else {
            /* Anything else (incl. a stray '-' mid-number) is junk. */
            ujson_reset(p);
            return 0;
        }
        break;

    case _ujson_st_wait_comma_or_end_:
        if (byte == ',') {
            p->st = _ujson_st_wait_dq_start_;
        }
        else if (byte == '}') {
            if (p->on_message != NULL) {
                p->on_message(p);
            }
            ujson_reset(p);
        }
        else if (ujson_is_ws(byte)) {
            /* skip whitespace */
        }
        else {
            ujson_reset(p);
            return 0;
        }
        break;

    default:
        /* Unreachable in correct usage; defensive reset. */
        ujson_reset(p);
        return 0;
    }

    return 1;
}

/******************************************************************
 * Lookup helpers
 ******************************************************************/

int ujson_find(const st_ujson_parser *p, const char *name)
{
    if (p == NULL || name == NULL) {
        return -1;
    }
    for (uint8_t i = 0; i < p->pair_count; i++) {
        /* Exact NUL-terminated match. p->key[i].str is always
         * NUL-terminated by ujson_append_token. */
        const char *k = p->key[i].str;
        const char *n = name;
        while (*k != '\0' && *n != '\0' && *k == *n) { k++; n++; }
        if (*k == '\0' && *n == '\0') {
            return (int)i;
        }
    }
    return -1;
}

uint8_t ujson_get_str(const st_ujson_parser *p, const char *name,
                      const char **out_str)
{
    int idx = ujson_find(p, name);
    if (idx < 0) {
        return 0;
    }
    if (out_str != NULL) {
        *out_str = p->value[idx].str;
    }
    return 1;
}

uint8_t ujson_get_int(const st_ujson_parser *p, const char *name,
                      int32_t *out)
{
    int idx = ujson_find(p, name);
    if (idx < 0) {
        return 0;
    }
    if (!p->value[idx].is_numeric) {
        return 0;
    }
    if (out != NULL) {
        *out = p->value[idx].i;
    }
    return 1;
}

uint8_t ujson_value_eq(const st_ujson_parser *p, const char *name,
                       const char *expected)
{
    if (expected == NULL) {
        return 0;
    }
    int idx = ujson_find(p, name);
    if (idx < 0) {
        return 0;
    }
    const char *v = p->value[idx].str;
    while (*v != '\0' && *expected != '\0' && *v == *expected) {
        v++;
        expected++;
    }
    return (uint8_t)(*v == '\0' && *expected == '\0');
}
