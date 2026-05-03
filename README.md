# micro-json (ujson)

A tiny, byte-fed JSON parser for small CPU / MCU targets.

`micro-json` is intentionally narrow: it exists to read the kind of
small, flat, key-value JSON messages that tend to flow over UART, TCP
or USB on a microcontroller. The parser is a single-pass state machine
with no heap allocation, no globals, and no dependencies beyond
`stdint.h`, `stddef.h`, and `string.h`.

## What it parses

```
{"key1":"value1", "key2":"value2", ...}
```

- One **flat** object per message — no nested objects, no arrays.
- Both keys and values are **quoted strings**; no numbers / booleans /
  `null`, no escape sequences (`\"`, `\\`, `\uXXXX`, ...).
- Whitespace (` `, `\t`, `\r`, `\n`) is allowed between structural
  tokens.
- The empty object `{}` is rejected; at least one pair is required.

If you need full JSON, use a real JSON library — this is not it.

## API

```c
#include "ujson.h"

st_ujson_parser parser;

void on_message(st_ujson_parser *p) {
    for (uint8_t i = 0; i < p->pair_count; i++) {
        printf("%s = %s\n", p->key[i].str, p->value[i].str);
    }
}

ujson_init(&parser, on_message);

/* Feed bytes as they arrive from the transport. */
while (have_byte()) {
    ujson_feed(&parser, read_byte());
}
```

Three functions:

| Function | Purpose |
| --- | --- |
| `ujson_init(p, cb)`  | Reset the parser and install a completion callback (may be `NULL`). |
| `ujson_reset(p)`     | Reset state and zero buffers; preserves the callback. Called automatically on `}` and on parse errors. |
| `ujson_feed(p, b)`   | Feed one byte. Returns `1` on normal progress, `0` on a hard parse error (the parser auto-resets). Fires `cb(p)` once when `}` closes a non-empty object. |

Inside the callback, read `p->pair_count`, `p->key[i].str`,
`p->value[i].str`, and (if needed) the raw `p->msg` capture. Don't
retain pointers across the call — the parser resets immediately after.

## Footprint

Per-instance memory at default settings:

```
2 * UJSON_MAX_KV * UJSON_STR_MAX  +  UJSON_MSG_MAX  +  small fixed fields
   = 2 * 8 * 64                   +  256            +  ~12
   = 1280 bytes (approx.)
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

TBD — pick one before publishing (MIT and BSD-2-Clause are both good
fits for a single-file library like this).

## Origin

State-machine parser by **MaIII Themd**, originally written 26 Sep
2016 for IAR ARM 7.10.3 firmware. This release strips the original
project-specific protocol layer and ships only the generic JSON
parser.
