/* src/crypto/bignum.h -- modular exponentiation for SSH: the RSA host key
 * check (e = 65537) and Diffie-Hellman group14. Montgomery multiplication
 * on 16-bit limbs (the 68000's mulu.w). Numbers are little-endian limb
 * arrays of a fixed width; bytes in and out are big-endian, as in SSH.
 * Pure, unit-tested. */
#ifndef BIGNUM_H
#define BIGNUM_H

#include <exec/types.h>
#include <string.h>

#define BN_MAX_BITS  4096
#define BN_MAX_LIMBS (BN_MAX_BITS / 16 + 1)

typedef UWORD bn_limb;

struct BnMod                  /* a modulus, ready for Montgomery arithmetic */
{
    bn_limb m[BN_MAX_LIMBS];
    bn_limb rr[BN_MAX_LIMBS]; /* R^2 mod m, R = 2^(16 n) */
    UWORD   n;                /* limbs */
    bn_limb minv;             /* -m^-1 mod 2^16 */
};

/* The modulus from its big-endian bytes (odd, at most BN_MAX_BITS). FALSE
 * for an even or too long one. */
BOOL Bn_SetMod(struct BnMod *mod, const UBYTE *bytes, size_t len);

/* out = base^exp mod m. base and out are big-endian, mod->n * 2 bytes long
 * (base must be smaller than m); exp is big-endian of any length. */
void Bn_ModExp(const struct BnMod *mod, const UBYTE *base, const UBYTE *exp, size_t expLen, UBYTE *out);

/* Big-endian bytes (any length) padded or cut to width bytes. */
void Bn_Fit(const UBYTE *in, size_t len, UBYTE *out, size_t width);

#endif /* BIGNUM_H */
