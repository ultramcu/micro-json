# micro-json (ujson)

A tiny, byte-fed JSON parser for small CPU / MCU targets.

`micro-json` is intentionally narrow: it exists to read the kind of
small, flat, key-value JSON messages that tend to flow over UART, TCP
or USB on a microcontroller. The parser is a single-pass state machine
with no heap allocation, no globals, and no dependencies beyond
`stdint.h`, `stddef.h`, and `string.h`.

## What it parses

```
{"key1":"value1", "key2":42, "key3":-7, "key4":"value4"}
```

- One **flat** object per message — no nested objects, no arrays.
- **Keys** are always quoted strings.
- **Values** can be either a quoted string *or* a signed 32-bit
  integer. The string form is always preserved in `value[i].str`;
  for integer values, `value[i].is_numeric` is set and `value[i].i`
  holds the parsed `int32_t`.
- Whitespace (` `, `\t`, `\r`, `\n`) is allowed between structural
  tokens.
- The empty object `{}` is rejected; at least one pair is required.
- No floats / exponents, no booleans / `null`, no escape sequences
  (`\"`, `\\`, `\uXXXX`, ...).

If you need full JSON, use a real JSON library — this is not it.

## API

```c
#include "ujson.h"

st_ujson_parser parser;

void on_message(st_ujson_parser *p) {
    for (uint8_t i = 0; i < p->pair_count; i++) {
        if (p->value[i].is_numeric) {
            printf("%s = %d (int)\n", p->key[i].str, p->value[i].i);
        } else {
            printf("%s = %s\n",       p->key[i].str, p->value[i].str);
        }
    }

    /* Or look up a specific key without scanning yourself: */
    int32_t v;
    if (ujson_get_int(p, "value", &v)) { /* ... */ }
}

ujson_init(&parser, on_message);

/* Feed bytes as they arrive from the transport. */
while (have_byte()) {
    ujson_feed(&parser, read_byte());
}
```

### Core functions

| Function | Purpose |
| --- | --- |
| `ujson_init(p, cb)`  | Reset the parser and install a completion callback (may be `NULL`). |
| `ujson_reset(p)`     | Reset state and zero buffers; preserves the callback. Called automatically on `}` and on parse errors. |
| `ujson_feed(p, b)`   | Feed one byte. Returns `1` on normal progress, `0` on a hard parse error (the parser auto-resets). Fires `cb(p)` once when `}` closes a non-empty object. |

Inside the callback, read `p->pair_count`, `p->key[i].str`,
`p->value[i].str` (and `value[i].i` / `is_numeric` for integer
values), and the raw `p->msg` capture if you need the original bytes.
Don't retain pointers across the call — the parser resets immediately
after.

### Lookup helpers

| Function | Purpose |
| --- | --- |
| `ujson_find(p, "name") -> int` | Index of the first key matching `name` (exact match), or `-1` if missing. |
| `ujson_get_str(p, "name", &s) -> 1/0` | Sets `*s` to point at `value[i].str` if found. |
| `ujson_get_int(p, "name", &out) -> 1/0` | Returns `1` only if found AND the value was an unquoted integer; writes the parsed value to `*out`. |
| `ujson_value_eq(p, "name", "exp")` | `1` if the key exists with that exact string value. |

## Footprint

Per-instance memory at default settings:

```
UJSON_MAX_KV * (UJSON_STR_MAX           // key
                + UJSON_STR_MAX + 5)    // value (str + i + is_numeric)
              + UJSON_MSG_MAX
              + small fixed fields
   = 8 * (64 + 64 + 5)                  // ≈ 1064
              + 256
              + ~12
   = ~1330 bytes
```

Tune with compile-time defines:

| Macro | Default | Meaning |
| --- | --- | --- |
| `UJSON_MAX_KV`  | `8`   | Max key/value pairs per object |
| `UJSON_STR_MAX` | `64`  | Max bytes per key OR value (incl. NUL) |
| `UJSON_MSG_MAX` | `256` | Raw-message capture buffer (incl. NUL) |

For example, on a very small target:
`-DUJSON_MAX_KV=4 -DUJSON_STR_MAX=24 -DUJSON_MSG_MAX=128` brings each
parser instance below 320 bytes.

## Error handling

`ujson_feed()` returns `0` on a structural parse error. The parser is
reset before returning, so the next call resumes from the top-level
`start` state and is ready for a new `{`. Common error causes:

- A non-whitespace byte where one isn't allowed (e.g. between keys
  and `:`).
- A key or value longer than `UJSON_STR_MAX - 1`.
- A new pair past `UJSON_MAX_KV`.

Bytes that come in *before* the opening `{` are silently ignored, so
framing-level garbage (line noise, log prefixes) doesn't trip the
parser.

## Building and running the example

The `example/` folder contains a self-contained demo:

```sh
cc -std=c99 -Wall -Wextra -O2 ujson.c example/main.c -o ujson_demo
./ujson_demo
```

There is no Makefile and no CMake by design — drop `ujson.c` /
`ujson.h` into your project and compile alongside your other
translation units.

## License

[MIT](LICENSE) — `SPDX-License-Identifier: MIT`.

Drop `ujson.c` / `ujson.h` into any project (open or closed,
commercial or otherwise). Just keep the copyright + permission
notice with the source.

