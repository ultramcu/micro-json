/******************************************************************
 * micro-json (ujson)
 * Minimal byte-fed JSON parser for small CPU / MCU targets.
 *
 * Origin : MaIII Themd, 26/9/2016 (state-machine design)
 *
 * Scope (intentional):
 *   - Parses ONE flat object: {"k1":"v1","k2":"v2",...}
 *   - All keys and values are quoted strings.
 *   - No nested objects, no arrays, no numbers / booleans / null.
 *   - No escape sequences (\", \\, \n, \uXXXX, ...).
 *   - The empty object {} is rejected; at least one pair is required.
 *
 * Memory model:
 *   - All state lives inside st_ujson_parser. No heap, no globals.
 *   - One parser instance is independent from any other.
 *
 * Compile-time tunables (override with -D...):
 *   UJSON_MAX_KV   -- max key/value pairs per object   (default 8)
 *   UJSON_STR_MAX  -- max bytes per key OR value slot  (default 64,
 *                     including the trailing NUL)
 *   UJSON_MSG_MAX  -- raw message capture size in bytes (default 256,
 *                     including the trailing NUL)
 ******************************************************************/

#ifndef UJSON_H
#define UJSON_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UJSON_MAX_KV
#define UJSON_MAX_KV   8u
#endif

#ifndef UJSON_STR_MAX
#define UJSON_STR_MAX  64u
#endif

#ifndef UJSON_MSG_MAX
#define UJSON_MSG_MAX  256u
#endif

/* Internal state-machine cursor values. */
enum {
    _ujson_st_start_ = 0,
    _ujson_st_wait_dq_start_,
    _ujson_st_read_key_,
    _ujson_st_wait_colon_,
    _ujson_st_wait_dq_value_,
    _ujson_st_read_value_,
    _ujson_st_wait_comma_or_end_
};

/* Fixed-size string slot used for both keys and values.
 * Always NUL-terminated after every accepted byte. */
typedef struct {
    char str[UJSON_STR_MAX];
} st_ujson_string;

typedef struct st_ujson_parser st_ujson_parser;

/* User-supplied handler invoked once per successfully parsed object,
 * right before the parser auto-resets. Read key[]/value[]/pair_count
 * here; do not retain pointers across the call. */
typedef void (*ujson_on_message_t)(st_ujson_parser *parser);

struct st_ujson_parser {
    uint8_t  st;                    /* state-machine cursor */
    uint8_t  pair_count;            /* completed key/value pairs */
    uint16_t tok_ptr;               /* write cursor in current key/value slot */
    uint16_t msg_ptr;               /* write cursor in raw msg capture */

    st_ujson_string key  [UJSON_MAX_KV];
    st_ujson_string value[UJSON_MAX_KV];

    char msg[UJSON_MSG_MAX];        /* raw capture of bytes since '{' */

    ujson_on_message_t on_message;  /* may be NULL */
};

/******************************************************************
 * Initialise a parser.
 *   p          : parser instance (must not be NULL).
 *   on_message : callback fired on '}'; pass NULL to disable.
 * Calls ujson_reset() internally and stores on_message.
 ******************************************************************/
void ujson_init(st_ujson_parser *p, ujson_on_message_t on_message);

/******************************************************************
 * Reset parser state and zero all buffers. The on_message handler
 * installed by ujson_init() is preserved across resets. Called
 * automatically on '}' completion and on every parse error.
 ******************************************************************/
void ujson_reset(st_ujson_parser *p);

/******************************************************************
 * Feed one byte from the transport (UART, TCP, USB, ...).
 * Returns 1 on normal progress (incl. completion), 0 on hard parse
 * error -- the parser was reset; the caller may discard accumulated
 * state. Fires on_message(p) once when '}' closes a non-empty object.
 ******************************************************************/
uint8_t ujson_feed(st_ujson_parser *p, uint8_t byte);

#ifdef __cplusplus
}
#endif

#endif /* UJSON_H */
