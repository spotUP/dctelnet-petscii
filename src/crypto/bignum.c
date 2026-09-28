/* src/crypto/bignum.c -- modular exponentiation for SSH (Montgomery). */
#include "bignum.h"

typedef ULONG bn_dlimb;
#define LIMB_BITS 16

static void from_bytes(bn_limb *x, UWORD n, const UBYTE *b, size_t len)
{
    size_t i;

    memset(x, 0, n * sizeof(bn_limb));
    for (i = 0; i < len && i / 2 < n; i++)          /* from the least significant byte */
        x[i / 2] |= (bn_limb)(b[len - 1 - i] << ((i & 1) * 8));
}

static void to_bytes(const bn_limb *x, UWORD n, UBYTE *b)
{
    UWORD i;

    for (i = 0; i < n; i++)
    {
        b[2 * n - 1 - 2 * i] = (UBYTE)x[i];
        b[2 * n - 2 - 2 * i] = (UBYTE)(x[i] >> 8);
    }
}

/* x >= m ? */
static BOOL geq(const bn_limb *x, const bn_limb *m, UWORD n)
{
    int i;

    for (i = n - 1; i >= 0; i--)
        if (x[i] != m[i]) return x[i] > m[i];
    return TRUE;
}

static void sub(bn_limb *x, const bn_limb *m, UWORD n)
{
    ULONG borrow = 0;
    UWORD i;

    for (i = 0; i < n; i++)
    {
        ULONG d = (ULONG)x[i] - m[i] - borrow;

        x[i] = (bn_limb)d;
        borrow = (d >> 16) & 1;
    }
}

/* r = x*y*R^-1 mod m (CIOS). r may be x or y. */
static void mont_mul(const struct BnMod *mod, bn_limb *r, const bn_limb *x, const bn_limb *y)
{
    static bn_limb t[BN_MAX_LIMBS + 2];
    const bn_limb *m = mod->m;
    UWORD n = mod->n, i, j;

    memset(t, 0, (n + 2) * sizeof(bn_limb));
    for (i = 0; i < n; i++)
    {
        bn_dlimb c = 0, yi = y[i];
        bn_limb u;

        for (j = 0; j < n; j++)
        {
            c = (bn_dlimb)x[j] * yi + t[j] + (c >> LIMB_BITS);
            t[j] = (bn_limb)c;
        }
        c = (bn_dlimb)t[n] + (c >> LIMB_BITS);
        t[n] = (bn_limb)c;
        t[n + 1] = (bn_limb)(c >> LIMB_BITS);
        u = (bn_limb)(t[0] * mod->minv);
        c = (bn_dlimb)u * m[0] + t[0];
        for (j = 1; j < n; j++)
        {
            c = (bn_dlimb)u * m[j] + t[j] + (c >> LIMB_BITS);
            t[j - 1] = (bn_limb)c;
        }
        c = (bn_dlimb)t[n] + (c >> LIMB_BITS);
        t[n - 1] = (bn_limb)c;
        t[n] = (bn_limb)(t[n + 1] + (c >> LIMB_BITS));
    }
    if (t[n] || geq(t, m, n))
        sub(t, m, n);
    memcpy(r, t, n * sizeof(bn_limb));
}

BOOL Bn_SetMod(struct BnMod *mod, const UBYTE *bytes, size_t len)
{
    UWORD n, i;
    bn_limb inv = 1;

    while (len && !*bytes) { bytes++; len--; }
    if (!len || len * 8 > BN_MAX_BITS || !(bytes[len - 1] & 1))
        return FALSE;
    n = (UWORD)((len + 1) / 2);
    mod->n = n;
    from_bytes(mod->m, n, bytes, len);
    for (i = 0; i < 5; i++)                             /* Newton: m0^-1 mod 2^16 */
        inv = (bn_limb)(inv * (2 - mod->m[0] * inv));
    mod->minv = (bn_limb)(0 - inv);
    /* R^2 mod m: 1 doubled 2*16n times, reduced as it goes. */
    memset(mod->rr, 0, sizeof(mod->rr));
    mod->rr[0] = 1;
    for (i = 0; i < 2 * LIMB_BITS * n; i++)
    {
        bn_limb top = (bn_limb)(mod->rr[n - 1] >> (LIMB_BITS - 1));
        UWORD k;

        for (k = n - 1; k > 0; k--)
            mod->rr[k] = (bn_limb)((mod->rr[k] << 1) | (mod->rr[k - 1] >> (LIMB_BITS - 1)));
        mod->rr[0] = (bn_limb)(mod->rr[0] << 1);
        if (top || geq(mod->rr, mod->m, n))
            sub(mod->rr, mod->m, n);
    }
    return TRUE;
}

void Bn_ModExp(const struct BnMod *mod, const UBYTE *base, const UBYTE *exp, size_t expLen, UBYTE *out)
{
    static bn_limb x[BN_MAX_LIMBS], acc[BN_MAX_LIMBS], one[BN_MAX_LIMBS];
    UWORD n = mod->n;
    size_t i;
    int bit;
    BOOL started = FALSE;

    from_bytes(x, n, base, (size_t)n * 2);
    mont_mul(mod, x, x, mod->rr);                       /* x * R mod m */
    memset(one, 0, sizeof(one));
    one[0] = 1;
    mont_mul(mod, acc, one, mod->rr);                   /* 1 * R mod m */
    for (i = 0; i < expLen; i++)
        for (bit = 7; bit >= 0; bit--)
        {
            if (started)
                mont_mul(mod, acc, acc, acc);
            if ((exp[i] >> bit) & 1)
            {
                mont_mul(mod, acc, acc, x);
                started = TRUE;
            }
        }
    mont_mul(mod, acc, acc, one);                       /* out of Montgomery form */
    to_bytes(acc, n, out);
}

void Bn_Fit(const UBYTE *in, size_t len, UBYTE *out, size_t width)
{
    while (len > width && !*in) { in++; len--; }
    memset(out, 0, width);
    if (len <= width)
        memcpy(out + width - len, in, len);
}
