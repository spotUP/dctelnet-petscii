---
date: 2026-09-28
topic: "SSH for DCTelnet: a small BBS-targeted SSH-2 client of its own"
tags: [dctelnet, ssh, plan]
status: draft
---

# SSH client for DCTelnet

Owner decision 2026-09-28: SSH is a go; target 68020+ for every key
exchange, 68000 through curve25519 only; probing live BBSes allowed.

## Measurements (cycle-exact FS-UAE, compiled C, 16-bit limbs)

One 2048-bit Montgomery multiplication: **A500 68000 7 MHz 1525 ms,
A1200 68020 14 MHz 175 ms** (32-bit limbs in C: 226 ms -- vbcc calls a
library for the 64-bit product; hand asm needed). dh-group14 needs about
2 x 400 of them: A500 ~7 min (not viable), 68020 ~20-30 s after asm.

## What the servers offer (live probe, handshake only)

| Server | KEX | Host key | Cipher / MAC |
|---|---|---|---|
| Synchronet 3.x (cryptlib, vert.synchro.net) | dh-gex-sha256/sha1, group14-sha256/sha1 | rsa-sha2-256, ssh-rsa | aes128/256-ctr/cbc, 3des; hmac-sha2-256, hmac-sha1 |
| ENiGMA 0.5.1 (xibalba.l33t.codes) | curve25519-sha256, ecdh-nistp*, group14-sha1, group1-sha1 | rsa-sha2-512/256, ssh-rsa | aes-ctr/gcm/cbc; hmac-sha2-256/512, sha1 |
| OpenSSH 9.6 (bbs.uprough.net) | sntrup/curve25519/ecdh/dh | rsa-sha2, ecdsa, ed25519 | chacha20, aes-ctr/gcm; etm MACs |

**Minimal set reaching all three:** kex curve25519-sha256 (+@libssh.org)
and diffie-hellman-group14-sha256; host key rsa-sha2-256; aes128-ctr;
hmac-sha2-256. (ENiGMA's DH is group14-*sha1* only: with curve25519 first
it is not needed.) 68000: curve25519 servers only (not Synchronet).

## Design

- Own code, no GPL: SHA-256/HMAC, AES-128 (encrypt, CTR), bignum
  (Montgomery modexp; C for 68000, 68020 asm inner loop), X25519 (from
  TweetNaCl, public domain), RSA PKCS#1 v1.5 SHA-256 verify.
- `src/ssh.c`: the SSH-2 transport, auth and channel as a pure state
  machine: bytes in from the socket -> plaintext out to the terminal,
  keys in -> bytes out. No socket calls inside, so the whole protocol is
  host-tested against the Mac's own sshd (an OpenSSH on a local port,
  password auth, forced command) and then against the live BBSes.
- Randomness: SHA-256 pool fed by EClock, beam position, timer, mouse and
  key timing, DateStamp; host key fingerprints in PROGDIR:KnownHosts,
  asked on first connect (as ssh does).
- DCTelnet: Protocol SSH (flag like Rlogin, per entry), user/password
  from the entry or asked, pty "ansi" at the terminal's size,
  window-change on resize, the key exchange shown as progress.

## Checklist

- [ ] S1.1 SHA-256 + HMAC-SHA256 (FIPS 180-4 / RFC 4231 vectors)
- [ ] S1.2 AES-128 encrypt + CTR (FIPS 197 / SP 800-38A vectors)
- [ ] S1.3 bignum: Montgomery modexp 2048, RSA verify (a real signature)
- [ ] S1.4 X25519 (RFC 7748 vectors)
- [ ] S1.5 random pool (Amiga sources; host: /dev/urandom)
- [ ] S2.1 packets, version exchange, KEXINIT negotiation
- [ ] S2.2 curve25519-sha256 and group14-sha256 kex, exchange hash, keys
- [ ] S2.3 encryption/MAC after NEWKEYS, sequence numbers, rekey refused
- [ ] S2.4 userauth password (+ keyboard-interactive for Synchronet?)
- [ ] S2.5 session channel, pty-req, shell, data, window adjust, window-change, EOF/close
- [ ] S2.6 host test: log in to a local sshd, run a command, read output
- [ ] S3.1 DCTelnet: SSH protocol option (Options + entry Terminal group), port 22
- [ ] S3.2 connect path, progress, known hosts prompt, password prompt
- [ ] S3.3 rig test against the Mac's sshd; owner test against live BBSes
- [ ] S4.1 68020 asm Montgomery inner loop; timings A500 (curve25519) / A1200 (group14)
- [ ] S4.2 optional: dh-gex asking for 1024 bits (Synchronet on 68000?) -- measure
- [ ] S5 guide, PR
