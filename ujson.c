/******************************************************************
 * micro-json (ujson) -- implementation
 * See ujson.h for scope, limitations and the public contract.
 ******************************************************************/

#include "ujson.h"

/* Append one byte to a key/value slot.
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
