# micro-json — moved

> This repository has moved.
>
> `micro-json` is now the **`ujson` module** of
> [**`c-ulib`**](https://github.com/ultramcu/c-ulib), a small
> umbrella of C99 libraries for MCU / embedded use.

## New home

- **Module:** <https://github.com/ultramcu/c-ulib/tree/main/ujson>
- **Header / source:** [`ujson.h`](https://github.com/ultramcu/c-ulib/blob/main/ujson/ujson.h)
  / [`ujson.c`](https://github.com/ultramcu/c-ulib/blob/main/ujson/ujson.c)
- **API and usage:** see the
  [ujson README](https://github.com/ultramcu/c-ulib/blob/main/ujson/README.md)

The code is identical — same byte-fed state machine, same int + string
value support, same lookup helpers (`ujson_find` / `ujson_get_str` /
`ujson_get_int` / `ujson_value_eq`).

## If you're already using this repo

The last release as a stand-alone repository remains tagged at
[`v0.3.0`](https://github.com/ultramcu/micro-json/releases/tag/v0.3.0).
Code that pins that exact tag will keep working — the tag is
preserved.

For new projects, prefer the `c-ulib` location.

## License

[MIT](LICENSE) — `SPDX-License-Identifier: MIT`. (Unchanged.)
