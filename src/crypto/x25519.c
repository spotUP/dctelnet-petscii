/* src/crypto/x25519.c -- X25519 (RFC 7748), TweetNaCl's field arithmetic
 * (public domain): 16 limbs of 16 bits in 64-bit integers. */
#include "x25519.h"

typedef long long i64;
typedef i64 gf[16];

static const gf k121665 = { 0xDB41, 1 };

static void car25519(gf o)
{
    int i;
    i64 c;

    for (i = 0; i < 16; i++)
    {
        o[i] += (i64)1 << 16;
        c = o[i] >> 16;
        o[(i + 1) * (i < 15)] += c - 1 + 37 * (c - 1) * (i == 15);
        o[i] -= c << 16;
    }
}

static void sel25519(gf p, gf q, int b)
{
    i64 t, c = ~(i64)(b - 1);
    int i;

    for (i = 0; i < 16; i++)
    {
        t = c & (p[i] ^ q[i]);
        p[i] ^= t;
        q[i] ^= t;
    }
}

static void pack25519(UBYTE *o, const gf n)
{
    int i, j, b;
    gf m, t;

    for (i = 0; i < 16; i++) t[i] = n[i];
    car25519(t); car25519(t); car25519(t);
    for (j = 0; j < 2; j++)
    {
        m[0] = t[0] - 0xffed;
        for (i = 1; i < 15; i++)
        {
            m[i] = t[i] - 0xffff - ((m[i - 1] >> 16) & 1);
            m[i - 1] &= 0xffff;
        }
        m[15] = t[15] - 0x7fff - ((m[14] >> 16) & 1);
        b = (int)((m[15] >> 16) & 1);
        m[14] &= 0xffff;
        sel25519(t, m, 1 - b);
    }
    for (i = 0; i < 16; i++)
    {
        o[2 * i] = (UBYTE)(t[i] & 0xff);
        o[2 * i + 1] = (UBYTE)(t[i] >> 8);
    }
}

static void unpack25519(gf o, const UBYTE *n)
{
    int i;

    for (i = 0; i < 16; i++) o[i] = n[2 * i] + ((i64)n[2 * i + 1] << 8);
    o[15] &= 0x7fff;
}

static void A(gf o, const gf a, const gf b) { int i; for (i = 0; i < 16; i++) o[i] = a[i] + b[i]; }
static void Z(gf o, const gf a, const gf b) { int i; for (i = 0; i < 16; i++) o[i] = a[i] - b[i]; }

static void M(gf o, const gf a, const gf b)
{
    i64 t[31];
    int i, j;

    for (i = 0; i < 31; i++) t[i] = 0;
    for (i = 0; i < 16; i++)
        for (j = 0; j < 16; j++)
            t[i + j] += a[i] * b[j];
    for (i = 0; i < 15; i++) t[i] += 38 * t[i + 16];
    for (i = 0; i < 16; i++) o[i] = t[i];
    car25519(o);
    car25519(o);
}

static void S(gf o, const gf a) { M(o, a, a); }

static void inv25519(gf o, const gf in)
{
    gf c;
    int a;

    for (a = 0; a < 16; a++) c[a] = in[a];
    for (a = 253; a >= 0; a--)
    {
        S(c, c);
        if (a != 2 && a != 4) M(c, c, in);
    }
    for (a = 0; a < 16; a++) o[a] = c[a];
}

void X25519(UBYTE out[32], const UBYTE scalar[32], const UBYTE point[32])
{
    UBYTE z[32];
    gf x, a, b, c, d, e, f, x16, x32;
    int i, r;

    for (i = 0; i < 31; i++) z[i] = scalar[i];
    z[31] = (UBYTE)((scalar[31] & 127) | 64);
    z[0] &= 248;
    unpack25519(x, point);
    for (i = 0; i < 16; i++) { b[i] = x[i]; d[i] = a[i] = c[i] = 0; }
    a[0] = d[0] = 1;
    for (i = 254; i >= 0; --i)
    {
        r = (z[i >> 3] >> (i & 7)) & 1;
        sel25519(a, b, r);
        sel25519(c, d, r);
        A(e, a, c); Z(a, a, c); A(c, b, d); Z(b, b, d);
        S(d, e); S(f, a); M(a, c, a); M(c, b, e);
        A(e, a, c); Z(a, a, c); S(b, a); Z(c, d, f);
        M(a, c, k121665); A(a, a, d); M(c, c, a); M(a, d, f);
        M(d, b, x); S(b, e);
        sel25519(a, b, r);
        sel25519(c, d, r);
    }
    for (i = 0; i < 16; i++) { x16[i] = a[i]; x32[i] = c[i]; }
    inv25519(x32, x32);
    M(x16, x16, x32);
    pack25519(out, x16);
}

void X25519_Base(UBYTE out[32], const UBYTE scalar[32])
{
    static const UBYTE nine[32] = { 9 };

    X25519(out, scalar, nine);
}
