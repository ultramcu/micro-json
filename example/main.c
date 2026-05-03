/* micro-json example: feed several JSON messages through one parser. */

#include <stdio.h>
#include <string.h>
#include "../ujson.h"

static const char *MSG[] = {
    /* simple two-pair object */
    "{ \"name\":\"hello\", \"mode\":\"demo\" }",
    /* sensor-style reading, four pairs */
    "{ \"sensor\":\"temp\", \"value\":\"23.5\", \"unit\":\"C\", \"ok\":\"true\" }",
    /* compact, no whitespace */
    "{\"a\":\"1\",\"b\":\"2\",\"c\":\"3\"}",
    /* generous whitespace + tabs to exercise the skip set */
    "{ \"user\"\t:\t\"alice\" ,\r\n  \"action\" : \"login\" }",
    /* malformed: missing closing quote on the key -- should auto-reset */
    "{ \"bad : \"value\" }",
};

static void on_message(st_ujson_parser *p)
{
    printf("  -> %u pair(s):\n", p->pair_count);
    for (uint8_t i = 0; i < p->pair_count; i++) {
        printf("     [%u] %-12s = %s\n", i, p->key[i].str, p->value[i].str);
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
