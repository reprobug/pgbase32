#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"
#include <string.h>
#include <ctype.h>
#include <stdint.h>

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(base32_encode_pg);
PG_FUNCTION_INFO_V1(base32_decode_pg);
PG_FUNCTION_INFO_V1(base32_is_valid_pg);

static const char *BASE32_ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

/*
 * base32_encode_pg(bytea) text
 */
Datum
base32_encode_pg(PG_FUNCTION_ARGS)
{
    bytea *input;
    size_t inlen;
    const unsigned char *in;
    size_t outlen;
    char *out;
    size_t i, j;
    text *result;

    if (PG_ARGISNULL(0))
        PG_RETURN_NULL();

    input = PG_GETARG_BYTEA_PP(0);
    inlen = (size_t) VARSIZE_ANY_EXHDR(input);

    if (inlen == 0)
    {
        result = cstring_to_text("");
        PG_RETURN_TEXT_P(result);
    }

    in = (const unsigned char *) VARDATA_ANY(input);
    outlen = ((inlen + 4) / 5) * 8;
    out = (char *) palloc(outlen + 1);

    i = 0;
    j = 0;

    while (i < inlen)
    {
        uint64_t buffer = 0;
        int bytes = 0;
        int k, useful_groups;

        for (k = 0; k < 5; k++)
        {
            buffer <<= 8;
            if (i < inlen)
            {
                buffer |= in[i++];
                bytes++;
            }
        }

        useful_groups = (bytes * 8 + 4) / 5;

        for (k = 0; k < 8; k++)
        {
            if (k < useful_groups)
            {
                int shift = (7 - k) * 5;
                int idx = (buffer >> shift) & 0x1F;
                out[j++] = BASE32_ALPHABET[idx];
            }
            else
            {
                out[j++] = '=';
            }
        }
    }

    out[j] = '\0';
    result = cstring_to_text(out);
    pfree(out);
    PG_RETURN_TEXT_P(result);
}

/*
 * base32_decode_pg(text) bytea
 */
Datum
base32_decode_pg(PG_FUNCTION_ARGS)
{
    text *txt;
    int tlen;
    const char *t;
    unsigned char rev[256];
    int i;
    char *clean;
    int clen;
    int blocks, max_out;
    unsigned char *obuf;
    int opos;
    bytea *out;
    bool in_padding = false;

    if (PG_ARGISNULL(0))
        PG_RETURN_NULL();

    txt = PG_GETARG_TEXT_PP(0);
    tlen = VARSIZE_ANY_EXHDR(txt);
    t = VARDATA_ANY(txt);

    /* Inicializar tabla de reversión */
    for (i = 0; i < 256; i++)
        rev[i] = 0xFF;

    for (i = 0; i < 32; i++)
    {
        rev[(unsigned char) BASE32_ALPHABET[i]] = i;
        rev[(unsigned char) tolower(BASE32_ALPHABET[i])] = i;
    }

    clean = (char *) palloc(tlen + 1);
    clen = 0;
    in_padding = false;

    for (i = 0; i < tlen; i++)
    {
        unsigned char c = (unsigned char) t[i];

        if (isspace(c))
            continue;

        if (c == '=')
        {
            in_padding = true;
            clean[clen++] = c;
        }
        else
        {
            if (in_padding)
            {
                pfree(clean);
                ereport(ERROR,
                        (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                         errmsg("Invalid Base32: padding character '=' in middle of input")));
            }

            if (rev[c] == 0xFF)
            {
                pfree(clean);
                ereport(ERROR,
                        (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                         errmsg("Invalid Base32: invalid character '%c'", c)));
            }
            clean[clen++] = c;
        }
    }

    if (clen == 0)
    {
        pfree(clean);
        out = (bytea *) palloc(VARHDRSZ);
        SET_VARSIZE(out, VARHDRSZ);
        PG_RETURN_BYTEA_P(out);
    }

    if (clen % 8 != 0)
    {
        pfree(clean);
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                 errmsg("Invalid Base32: length must be multiple of 8, got %d", clen)));
    }

    /* Decodificar */
    blocks = clen / 8;
    max_out = blocks * 5;
    obuf = (unsigned char *) palloc(max_out);
    opos = 0;

    for (i = 0; i < blocks; i++)
    {
        uint64_t buffer = 0;
        int pad_count = 0;
        int k;
        int outbytes;

        /* Construir buffer de 40 bits */
        for (k = 0; k < 8; k++)
        {
            char c = clean[i * 8 + k];
            if (c == '=')
            {
                pad_count++;
                buffer <<= 5;
            }
            else
            {
                buffer = (buffer << 5) | rev[(unsigned char)c];
            }
        }

        /* Validar padding */
        switch (pad_count)
        {
            case 0: outbytes = 5; break;
            case 1: outbytes = 4; break;
            case 3: outbytes = 3; break;
            case 4: outbytes = 2; break;
            case 6: outbytes = 1; break;
            default:
                pfree(clean);
                pfree(obuf);
                ereport(ERROR,
                        (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                         errmsg("Invalid Base32: invalid padding pattern")));
        }

        for (k = 0; k < outbytes; k++)
        {
            int shift = (4 - k) * 8;
            obuf[opos++] = (buffer >> shift) & 0xFF;
        }
    }

    out = (bytea *) palloc(opos + VARHDRSZ);
    SET_VARSIZE(out, opos + VARHDRSZ);
    memcpy(VARDATA(out), obuf, opos);

    pfree(clean);
    pfree(obuf);
    PG_RETURN_BYTEA_P(out);
}

/*
 * base32_is_valid_pg(text) boolean
 */
Datum
base32_is_valid_pg(PG_FUNCTION_ARGS)
{
    text *txt;
    int tlen;
    const char *t;
    int i;
    unsigned char rev[256];
    bool in_padding = false;
    int data_chars = 0;

    if (PG_ARGISNULL(0))
        PG_RETURN_BOOL(false);

    txt = PG_GETARG_TEXT_PP(0);
    tlen = VARSIZE_ANY_EXHDR(txt);
    t = VARDATA_ANY(txt);

    for (i = 0; i < 256; i++)
        rev[i] = 0xFF;

    for (i = 0; i < 32; i++)
    {
        rev[(unsigned char) BASE32_ALPHABET[i]] = i;
        rev[(unsigned char) tolower(BASE32_ALPHABET[i])] = i;
    }

    data_chars = 0;
    in_padding = false;

    for (i = 0; i < tlen; i++)
    {
        unsigned char c = (unsigned char) t[i];

        if (isspace(c))
            continue;

        if (c == '=')
        {
            in_padding = true;
            data_chars++;
        }
        else
        {
            if (in_padding)
                PG_RETURN_BOOL(false);

            if (rev[c] == 0xFF)
                PG_RETURN_BOOL(false);

            data_chars++;
        }
    }

    if (data_chars == 0 || data_chars % 8 != 0)
        PG_RETURN_BOOL(false);

    PG_RETURN_BOOL(true);
}
