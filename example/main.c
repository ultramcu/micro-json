/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 MaIII Themd
 */

/* micro-json example: feed several JSON messages through one parser. */

#include <stdio.h>
#include <string.h>
#include "../ujson.h"

static const char *MSG[] = {
    /* simple two-pair object, all-string */
    "{ \"name\":\"hello\", \"mode\":\"demo\" }",
    /* mixed: a string and several integers (positive + negative) */
    "{ \"sensor\":\"temp\", \"value\":235, \"offset\":-3, \"ok\":1 }",
    /* compact, no whitespace */
    "{\"a\":\"1\",\"b\":\"2\",\"c\":\"3\"}",
    /* generous whitespace + tabs around tokens */
    "{ \"user\"\t:\t\"alice\" ,\r\n  \"count\" : 42 }",
    /* malformed: missing closing quote on the key -- should auto-reset */
    "{ \"bad : \"value\" }",
};

static void on_message(st_ujson_parser *p)
{
    printf("  -> %u pair(s):\n", p->pair_count);
    for (uint8_t i = 0; i < p->pair_count; i++) {
        if (p->value[i].is_numeric) {
            printf("     [%u] %-12s = %d  (int)\n",
                   i, p->key[i].str, p->value[i].i);
        } else {
            printf("     [%u] %-12s = %s\n",
                   i, p->key[i].str, p->value[i].str);
        }
    }

    /* Lookup helpers: pull specific keys by name out of whatever
     * arrived. Both forms gracefully no-op when the key isn't there. */
    int32_t v;
    if (ujson_get_int(p, "value", &v)) {
        printf("     ujson_get_int(\"value\")  -> %d\n", v);
    }
    const char *s;
    if (ujson_get_str(p, "name", &s)) {
        printf("     ujson_get_str(\"name\")   -> %s\n", s);
    }
    if (ujson_value_eq(p, "mode", "demo")) {
        printf("     ujson_value_eq(\"mode\",\"demo\") -> match\n");
    }
}

static void run(st_ujson_parser *p, const char *s)
{
    printf("IN : %s\n", s);
    size_t n = strlen(s);
    uint8_t ok = 1;
    for (size_t i = 0; i < n; i++) {
        if (!ujson_feed(p, (uint8_t)s[i])) {
            ok = 0;
        }
    }
    if (!ok) {
        printf("  -> parse error (parser auto-reset)\n");
    }
    printf("\n");
}

int main(void)
{
    st_ujson_parser parser;
    ujson_init(&parser, on_message);

    for (size_t i = 0; i < sizeof(MSG) / sizeof(MSG[0]); i++) {
        run(&parser, MSG[i]);
    }
    return 0;
}
