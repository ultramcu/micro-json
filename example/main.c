/* micro-json example: feed several JSON messages through one parser. */

#include <stdio.h>
#include <string.h>
#include "../ujson.h"

static const char *MSG[] = {
    "{ \"pt\":\"hb\", \"id\":\"node-01\" }",
    "{ \"tid\":\"01001\", \"pt\":\"cmd\", \"act\":\"start\", \"ref\":\"1234567890\" }",
    "{\"k1\":\"v1\",\"k2\":\"v2\",\"k3\":\"v3\"}",
    "{ \"ver\" : \"01.02.03\" , \"upfw\" : \"yes\" }",
    /* should fail (missing closing quote on key) */
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
