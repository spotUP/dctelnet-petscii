---
date: 2026-09-28
topic: "SSH for DCTelnet on AmigaOS 3 / 68k: feasibility (plan item P9.3)"
tags: [dctelnet, ssh, research]
status: final
---

# SSH for DCTelnet -- feasibility

Web research by a sub-agent, 2026-09-28. Marks: [confirmed] = source read;
[unconfirmed] = inferred or source not reachable.

## Findings

1. **AmiSSL 5.x** (OpenSSL 3.x, Apache-2.0): needs **68020+**, no 68000 build
   [confirmed, release notes]. SDK `AmiSSL-5.x-SDK.lha` on
   https://github.com/jens-maus/amissl/releases; vbcc includes and an autoinit
   source ship [confirmed]; a prebuilt vbcc OS3 link library [unconfirmed].
   Library size "larger than 3MB" [confirmed, franke.ms].
2. **Existing SSH on OS3/m68k**
   - BebboSSH / AmigaSSH (Stefan Franke, GPLv3, **68000+**, own C++ crypto,
     client + libcryptossh.library) -- only curve25519-sha256, ssh-ed25519,
     aes128-gcm / chacha20-poly1305 [confirmed].
     https://franke.ms/git/bebbo/bebbossh, Aminet comm/net/amigassh
   - ssh2fs (salass00): libssh2 + AmiSSL on OS3 with gcc [confirmed].
   - SSHTerm (salass00): libssh2 + AmiSSL, OS4 only [confirmed].
   - No maintained dropbear/libssh OS3 port found [unconfirmed absence].
3. **Speed**: BebboSSH X25519 keypair ~0.9 s on a 25 MHz 68030; an A500
   (68000) reaches the password prompt in under 50 s [confirmed, HN thread
   46053262]. RSA/DH-2048 on 68000/020: no numbers found [unconfirmed].
4. **BBS servers**: Synchronet and Mystic use cryptlib -- DH group14/GEX,
   RSA host keys, aes-ctr/cbc, hmac-sha2-256/sha1; **no curve25519/ed25519
   seen** [partly confirmed]. ENiGMA½ (node ssh2) offers curve25519 but an
   RSA host key [confirmed]. So BebboSSH's ed25519-only host key support
   most likely cannot talk to any of the three [inferred, untested].

## Recommendation

**No-go for AmiSSL + libssh2**: it drops 68000 users, adds a 3 MB+
dependency and ~150-250 KB of code, and libssh2 has no vbcc build.

Feasible only as a scoped project: a small BBS-targeted SSH client (own
code, or BebboSSH's if DCTelnet can be GPLv3) with curve25519 +
dh-group14-sha256, RSA verify, aes128-ctr + hmac-sha2-256, password auth,
one pty channel. Rough effort 4-8 weeks; the main risk is 2048-bit modexp
speed on a 68000. **Measure one 2048-bit modexp on 68000 and 68020 first.**

Owner decision pending (go/no-go, and the license question).
