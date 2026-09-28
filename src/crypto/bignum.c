/* src/crypto/bignum.c -- modular exponentiation for SSH (Montgomery). */
#include "bignum.h"

#ifdef BN_ASM
/* bn_muladd.s: the same loop with mulu.w (68000) and with mulu.l (68020+). */
bn_limb Bn_MulAdd000(__reg("a0") bn_limb *r, __reg("a1") const bn_limb *a,
                     __reg("d0") UWORD n, __reg("d1") bn_limb w);
bn_limb Bn_MulAdd020(__reg("a0") bn_limb *r, __reg("a1") const bn_limb *a,
                     __reg("d0") UWORD n, __reg("d1") bn_limb w);
#include <exec/execbase.h>
extern struct ExecBase *SysBase;

bn_limb Bn_MulAdd(bn_limb *r, const bn_limb *a, UWORD n, bn_limb w)
{
    return (SysBase->AttnFlags & AFF_68020) ? Bn_MulAdd020(r, a, n, w) : Bn_MulAdd000(r, a, n, w);
}
#else
/* In C without a 64-bit type: 16 x 16 bit products, as the 68000 does. */
bn_limb Bn_MulAdd(bn_limb *r, const bn_limb *a, UWORD n, bn_limb w)
{
    ULONG wl = w & 0xFFFF, wh = w >> 16, carry = 0;

    while (n--)
    {
        ULONG al = *a & 0xFFFF, ah = *a >> 16, lo, hi, mid, mid2, t;
        a++;
        lo = al * wl;
        hi = ah * wh;
        mid = al * wh;
        mid2 = ah * wl;
        mid += mid2;
        if (mid < mid2) hi += 0x10000UL;
        t = lo + (mid << 16);
        hi += (mid >> 16) + (t < lo);
        lo = t;
        lo += carry; hi += lo < carry;
        lo += *r;    hi += lo < *r;
        *r++ = lo;
        carry = hi;
    }
    return carry;
}
#endif

static void from_bytes(bn_limb *x, UWORD n, const UBYTE *b, size_t len)
{
    size_t i;

    memset(x, 0, n * sizeof(bn_limb));
    for (i = 0; i < len && i / 4 < n; i++)          /* from the least significant byte */
        x[i / 4] |= (bn_limb)b[len - 1 - i] << ((i & 3) * 8);
}

static void to_bytes(const bn_limb *x, UWORD n, UBYTE *b)
{
    UWORD i;

    for (i = 0; i < n; i++)
    {
        UBYTE *p = b + 4 * (n - 1 - i);
        p[0] = (UBYTE)(x[i] >> 24); p[1] = (UBYTE)(x[i] >> 16); p[2] = (UBYTE)(x[i] >> 8); p[3] = (UBYTE)x[i];
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
    bn_limb borrow = 0;
    UWORD i;

    for (i = 0; i < n; i++)
    {
        bn_limb d = x[i] - m[i] - borrow;
        borrow = (x[i] < m[i]) || (x[i] == m[i] && borrow);
        x[i] = d;
    }
}

/* r = x*y*R^-1 mod m, operand scanning: row i adds x*y[i] into t+i, then
 * the multiple of m that clears t[i]; the result is t[n..2n]. r may be x
 * or y. */
static void mont_mul(const struct BnMod *mod, bn_limb *r, const bn_limb *x, const bn_limb *y)
{
    static bn_limb t[2 * BN_MAX_LIMBS + 2];
    const bn_limb *m = mod->m;
    UWORD n = mod->n, i, k;

    memset(t, 0, (2 * n + 2) * sizeof(bn_limb));
    for (i = 0; i < n; i++)
    {
        bn_limb c = Bn_MulAdd(t + i, x, n, y[i]);
        for (k = i + n; c; k++) { t[k] += c; c = t[k] < c; }
        c = Bn_MulAdd(t + i, m, n, t[i] * mod->minv);
        for (k = i + n; c; k++) { t[k] += c; c = t[k] < c; }
    }
    if (t[2 * n] || geq(t + n, m, n))
        sub(t + n, m, n);
    memcpy(r, t + n, n * sizeof(bn_limb));
}

BOOL Bn_SetMod(struct BnMod *mod, const UBYTE *bytes, size_t len)
{
    UWORD n, i, bits, e;
    bn_limb inv = 1, top;

    while (len && !*bytes) { bytes++; len--; }
    if (!len || len * 8 > BN_MAX_BITS || !(bytes[len - 1] & 1))
        return FALSE;
    n = (UWORD)((len + 3) / 4);
    mod->n = n;
    from_bytes(mod->m, n, bytes, len);
    for (i = 0; i < 5; i++)                             /* Newton: m0^-1 mod 2^32 */
        inv = inv * (2 - mod->m[0] * inv);
    mod->minv = 0 - inv;

    /* R^2 mod m, R = 2^(32n). With b the bit length of m, 2^b mod m is
     * 2^b - m (m's two's complement cut to b bits); doubling it to 2^(33n)
     * and five Montgomery squarings (2^e -> 2^(2e - 32n)) give 2^(64n). */
    for (bits = 32 * n, top = mod->m[n - 1]; !(top & 0x80000000UL); top <<= 1) bits--;
    memset(mod->rr, 0, sizeof(mod->rr));
    sub(mod->rr, mod->m, n);                            /* 0 - m */
    if (bits % 32) mod->rr[n - 1] &= (1UL << (bits % 32)) - 1;
    for (e = bits; e < 33 * n; e++)
    {
        bn_limb carry = mod->rr[n - 1] >> 31;
        for (i = n - 1; i > 0; i--)
            mod->rr[i] = mod->rr[i] << 1 | mod->rr[i - 1] >> 31;
        mod->rr[0] <<= 1;
        if (carry || geq(mod->rr, mod->m, n))
            sub(mod->rr, mod->m, n);
    }
    for (i = 0; i < 5; i++)
        mont_mul(mod, mod->rr, mod->rr, mod->rr);
    return TRUE;
}

void Bn_ModExp(const struct BnMod *mod, const UBYTE *base, const UBYTE *exp, size_t expLen, UBYTE *out)
{
    static bn_limb x[BN_MAX_LIMBS], acc[BN_MAX_LIMBS], one[BN_MAX_LIMBS];
    UWORD n = mod->n;
    size_t i;
    int bit;
    BOOL started = FALSE;

    from_bytes(x, n, base, Bn_Bytes(mod));
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
