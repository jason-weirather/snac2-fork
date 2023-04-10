/* copyright (c) 2022 - 2023 grunfink / MIT license */

#ifndef _XS_ENCDEC_H

#define _XS_ENCDEC_H

 xs_str *xs_hex_enc(const xs_val *data, int size);
 xs_val *xs_hex_dec(const xs_str *hex, int *size);
 int xs_is_hex(const char *str);
 xs_str *xs_base64_enc(const xs_val *data, int sz);
 xs_val *xs_base64_dec(const xs_str *data, int *size);
 xs_str *xs_utf8_enc(xs_str *str, unsigned int cpoint);


#ifdef XS_IMPLEMENTATION

xs_str *xs_hex_enc(const xs_val *data, int size)
/* returns an hexdump of data */
{
    xs_str *s;
    char *p;
    int n;

    p = s = xs_realloc(NULL, _xs_blk_size(size * 2 + 1));

    for (n = 0; n < size; n++) {
        sprintf(p, "%02x", (unsigned char)data[n]);
        p += 2;
    }

    *p = '\0';

    return s;
}


xs_val *xs_hex_dec(const xs_str *hex, int *size)
/* decodes an hexdump into data */
{
    int sz = strlen(hex);
    xs_val *s = NULL;
    char *p;
    int n;

    if (sz % 2)
        return NULL;

    p = s = xs_realloc(NULL, _xs_blk_size(sz / 2 + 1));

    for (n = 0; n < sz; n += 2) {
        int i;
        if (sscanf(&hex[n], "%02x", &i) == 0) {
            /* decoding error */
            return xs_free(s);
        }
        else
            *p = i;

        p++;
    }

    *p = '\0';
    *size = sz / 2;

    return s;
}


int xs_is_hex(const char *str)
/* returns 1 if str is an hex string */
{
    while (*str) {
        if (strchr("0123456789abcdefABCDEF", *str++) == NULL)
            return 0;
    }

    return 1;
}


xs_str *xs_base64_enc(const xs_val *data, int sz)
/* encodes data to base64 */
{
    xs_str *s;
    unsigned char *p;
    char *i;
    int bsz, n;
    static char *b64_tbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                           "abcdefghijklmnopqrstuvwxyz"
                           "0123456789+/";

    bsz = ((sz + 3 - 1) / 3) * 4;
    i = s = xs_realloc(NULL, _xs_blk_size(bsz + 1));
    p = (unsigned char *)data;

    for (n = 0; n < sz; n += 3) {
        int l = sz - n;

        if (l == 1) {
            *i++ = b64_tbl[(p[n] >> 2) & 0x3f];
            *i++ = b64_tbl[(p[n] << 4) & 0x3f];
            *i++ = '=';
            *i++ = '=';
        }
        else
        if (l == 2) {
            *i++ = b64_tbl[(p[n] >> 2) & 0x3f];
            *i++ = b64_tbl[(p[n] << 4 | p[n + 1] >> 4) & 0x3f];
            *i++ = b64_tbl[(p[n + 1] << 2) & 0x3f];
            *i++ = '=';
        }
        else {
            *i++ = b64_tbl[(p[n] >> 2) & 0x3f];
            *i++ = b64_tbl[(p[n] << 4 | p[n + 1] >> 4) & 0x3f];
            *i++ = b64_tbl[(p[n + 1] << 2 | p[n + 2] >> 6) & 0x3f];
            *i++ = b64_tbl[(p[n + 2]) & 0x3f];
        }
    }

    *i = '\0';

    return s;
}


xs_val *xs_base64_dec(const xs_str *data, int *size)
/* decodes data from base64 */
{
    xs_val *s = NULL;
    int sz = 0;
    char *p;
    static char *b64_tbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                           "abcdefghijklmnopqrstuvwxyz"
                           "0123456789+/=";

    p = (char *)data;

    /* size of data must be a multiple of 4 */
    if (strlen(p) % 4)
        return NULL;

    for (p = (char *)data; *p; p += 4) {
        int cs[4];
        int n;
        unsigned char tmp[3];

        for (n = 0; n < 4; n++) {
            char *ss = strchr(b64_tbl, p[n]);

            if (ss == NULL) {
                /* not a base64 char */
                return xs_free(s);
            }

            cs[n] = ss - b64_tbl;
        }

        n = 0;

        /* first byte */
        tmp[n++] = cs[0] << 2 | ((cs[1] >> 4) & 0x0f);

        /* second byte */
        if (cs[2] != 64)
            tmp[n++] = cs[1] << 4 | ((cs[2] >> 2) & 0x3f);

        /* third byte */
        if (cs[3] != 64)
            tmp[n++] = cs[2] << 6 | (cs[3] & 0x3f);

        /* must be done manually because data can be pure binary */
        s = xs_realloc(s, _xs_blk_size(sz + n));
        memcpy(s + sz, tmp, n);
        sz += n;
    }

    /* asciiz it to use it as a string */
    s = xs_realloc(s, _xs_blk_size(sz + 1));
    s[sz] = '\0';

    *size = sz;

    return s;
}


xs_str *xs_utf8_enc(xs_str *str, unsigned int cpoint)
/* encodes an Unicode codepoint to utf8 */
{
    unsigned char tmp[4];
    int n = 0;

    if (cpoint < 0x80)
        tmp[n++] = cpoint & 0xff;
    else
    if (cpoint < 0x800) {
        tmp[n++] = 0xc0 | (cpoint >> 6);
        tmp[n++] = 0x80 | (cpoint & 0x3f);
    }
    else
    if (cpoint < 0x10000) {
        tmp[n++] = 0xe0 | (cpoint >> 12);
        tmp[n++] = 0x80 | ((cpoint >> 6) & 0x3f);
        tmp[n++] = 0x80 | (cpoint & 0x3f);
    }
    else
    if (cpoint < 0x200000) {
        tmp[n++] = 0xf0 | (cpoint >> 18);
        tmp[n++] = 0x80 | ((cpoint >> 12) & 0x3f);
        tmp[n++] = 0x80 | ((cpoint >> 6) & 0x3f);
        tmp[n++] = 0x80 | (cpoint & 0x3f);
    }

    return xs_append_m(str, (char *)tmp, n);
}

#endif /* XS_IMPLEMENTATION */

#endif /* _XS_ENCDEC_H */
