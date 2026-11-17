---
date: 2026-10-29
topic: DCTelnet issue 58 - SSH crash only with VBCC -O (release)
status: inconclusive - crypto vectors pass at -O3; faulty function NOT identified
---
# Findings

Issue 58: crash after quitting, "Software Failure 8100 0005" = AN_MemCorrupt (exec memory list corrupted), i.e. a heap/stack overwrite, not a wrong-result bug in a pure function. Only release (-O3 -speed -maxoptpasses=100) build.

## Reproduction attempt (scratchpad ssh-o-crash/)
- Built tests/test_crypto.c + src/crypto/*.c + bn_muladd.s with host vbcc (Homebrew, +aos68k config adapted from src/aos68k, -cpu=68000 -DBN_ASM), once all -O0 and once all -O3 -speed -maxoptpasses=100. Script: .../scratchpad/ssh-o-crash/build.sh.
- Ran both under vamos (-C 68000 -m 8192 -s 64): BOTH print "crypto: all assertions passed" (x25519 incl. iterated, group14 modexp, RSA 4096, AES, SHA, HMAC). So no miscompile of X25519/bignum/sha/aes is visible in output on this vbcc. Host vbcc version not confirmed equal to 0.9h patch 3 (vbccm68k has no -v).
- Host ASAN+UBSAN build of tests/test_ssh.c + ssh.c + crypto (C bignum path, not asm): clean. No portable-C overflow or UB found in ssh.c/crypto.

## Strongest lead (unverified): stack size
Stack frames (sub.w #N,a7) release vs -O0: ssh.c largest 1536 vs 540; x25519.c 1400 and 880 vs 492; sha256 332 vs 308. Release frames are about 3x larger (inlining/unrolling). A call chain Ssh_Receive (1536) -> X25519 (1400) -> fe helpers + bignum + bsdsocket/dos calls is >3 KB, close to the 4096-byte default Workbench stack. A stack overflow trashes memory below the stack, matching AN_MemCorrupt appearing seconds later and "no crash in debug". Could not confirm: DCTelnet.info stack field not parsed reliably, no stack-high-water measurement done.

## Not checked
- sshconn.c / ssh.c compiled -O3 running the real handshake on m68k (test_ssh needs sockets; not portable to vamos).
- bn_muladd.s is hand asm, unaffected by -O; reviewed by eye, no register/loop bug found (n>=1 guaranteed by callers).
- Other files compiled -O3 in the release app.

## Proposed next steps
1. Measure stack: in DCTelnet fill stack with a pattern at start of SshConn_Start and report high-water at SshConn_End (or run under fs-uae with a 4096 stack and 8192/32768 stack, compare). 
2. Check the shipped DCTelnet.info StackSize and whether a stack check/StackSwap exists.
3. If stack: fix root cause = allocate big temporaries (fe arrays, X25519 state) on the heap or make them static, and ensure startup requests enough stack; do not rely on -O0.
