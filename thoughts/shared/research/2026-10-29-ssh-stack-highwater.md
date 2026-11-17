---
date: 2026-10-29
topic: DCTelnet issue 58 - SSH stack high-water at -O0 vs release (vamos)
status: partial - primitives measured; full-engine number includes harness overhead; call-site depth (step 3) NOT done
---
# Method
Harness (scratchpad .../c0a23933-ed3d-4dee-a857-a826fcab1366/scratchpad/stack/{probe.c,harness.c,ssh_replay.c,build.sh,runall.sh}): Probe_Fill() paints 7900 words (31.6 KB) of 0xDEADBEEF below SP, the entry runs, Probe_Scan() (same frame layout, -O0) finds the lowest overwritten word. Accuracy about +-20 bytes. vamos -C 68000 -m 8192 -s 64. Host vbcc V0.9h (vbccm68k banner; Makefile says 0.9h patch 3, patch level not verifiable). Release = -O3 -speed -maxoptpasses=100, -cpu=68000, -DBN_ASM, crypto files + ssh.c.

# Stack high-water (bytes)
| entry point | -O0 | release |
|---|---|---|
| X25519 scalarmult | 1168 | 2688 |
| X25519_Base | 1148 | 2664 |
| group14 modexp (test fn, 2048-bit) | 312 | 496 |
| RSA 4096 verify | 564 | 684 |
| RSA SHA256 verify (2048) | 564 | 684 |
| AES-128 CTR | 380 | 668 |
| SHA256 | 568 | 672 |
| HMAC-SHA256 | 756 | 852 |
| ssh replay curve25519 (Drive+Ssh_Receive+kex, see caveat) | 4176 | 4964 |
| ssh replay group14 | 4176 | 4172 |

Release/-O0 ratio: x25519 2.3x, AES 1.8x, others 1.1-1.6x.

Caveat on the SSH rows: the replay runs via test_ssh.c's Drive() (copy, sockets stubbed) plus libc stdio (fopen'd before the fill, fread/printf inside). Harness+stdio frames are NOT separated from the engine, so 4.2-5.0 KB is an upper bound for Ssh_Receive-down, not the engine alone. Recorded sessions cannot replay on m68k: the client's random cookie (byte 28) differs (host-endian counter hashing in Random_Bytes), so the run ends at "host key signature is wrong" after X25519/group14 + RSA verify ran; post-newkeys decrypt was not reached. ssh.c has no large local arrays (grep), so the depth is frames, not buffers.

# -s 4 (4 KB stack) crypto vectors (test_crypto, vamos)
| build | stack | result |
|---|---|---|
| -O0, 68000 | 4 KB | all assertions passed |
| release, 68000 | 4 KB | all assertions passed |
| release, 68020 | 64 KB | passed |
| release, 68020 | 4 KB | passed |
| release, 68040 -fpu=68040 | 64 KB | passed |
x25519 does NOT fail its self-test at 68000/68020/68040 on this vbcc (the codewiz footnote's V0.9i pre miscompile is not reproduced). Primitives alone peak at 2.7 KB, so they fit 4 KB; vamos with -s 4 does not guard overflow below the stack anyway, so a pass proves little.

# Verdict
Stack overflow NOT confirmed, NOT refuted. Evidence for: release X25519 alone needs 2688 B (2.3x -O0); the engine path through it measured about 4.2-5.0 KB including harness overhead; that exceeds 4096 once DCTelnet's own frames above SshConn_Read/Fill (main loop, intuition handlers, bsdsocket) are added. Evidence against: -O0 ssh replay also reads 4176 incl. harness, so the engine-only delta between flags is not isolated. Needed next: measure inside DCTelnet (fill at SshConn_Start, report at SshConn_End) or a harness that calls Ssh_Receive with the stack painted just before it.

# Call-site depth (step 3)
Not done (budget). vc -S of sshconn.c/DCTelnet.c failed in my invocation (-S needs different flags); no grep hit for SshConn_Read callers outside sshconn.c in src/*.c, so the caller chain is unlocated.

# __stack
Release uses vbcc targets/m68k-amigaos/lib/startup.o, which exports ___stack (C name __stack), ___stackLower, ___stackPtr, ___stack_usage and contains exec StackSwap (-732) and AllocMem calls. So `long __stack = N;` is the startup's stack-request hook (symbol evidence only; its exact compare logic and a run on AmigaOS were not verified, vamos not tested). Source sets neither __stack nor a StackSwap; DCTelnet.info do_StackSize=0 means the 4096 default applies.

## Follow-up 2026-10-30
Method change: Note/Ask callbacks made silent (no strcat/printf/assert/strcmp; HostKey keeps Ssh_Fingerprint, an engine call). File I/O, malloc and the closing printf moved out of the measured window: harness does Replay_Prep (fopen/fread/malloc) BEFORE Probe_Fill and Replay_Finish (fclose/printf/free) AFTER Probe_Scan, so the window is p_ssh_* -> Replay_Run -> Drive -> Ssh_Receive and below. The earlier 4172-4176 was mostly stdio (fread/printf frames), not the engine. vamos -C 68000 -s 64, -cpu=68000, same flags as before. Tracked files untouched (modified copies only in the scratchpad stack/ dir: ssh_replay.c, harness.c).

| path | -O0 bytes | release bytes | caller frames (-O0 / rel) | engine-only -O0 | engine-only release |
|---|---|---|---|---|---|
| ssh replay curve25519 | 2020 | 4732 | 52 / 92 | ~1968 | ~4640 |
| ssh replay group14 | 1976 | 3376 | 52 / 92 | ~1924 | ~3284 |

Caller frames (from `vc -S` of the harness, includes return addresses and pushed args): jsr into p_ssh_* 4 + p_ssh_*->Replay_Run 4 + Replay_Run pushes 3 args (12) + jsr Drive 4 + Drive frame (-O0: sub.w #16, no saved regs = 16; release: sub.w #16 + movem of 10 regs a2-a5/d2-d7 = 56) + 12 B of args pushed for Ssh_Receive = 52 (-O0) / 92 (release). p_ssh_* and Replay_Run have no locals. Release figures are +-8 B (vbcc may fold arg pops).
Release curve25519 engine path is 4640 B: more than the 4096 B default stack by itself, before DCTelnet's own frames (main loop, SshConn_*, bsdsocket) are added. group14 release 3284 B fits only if less than ~800 B sits above it. -O0 fits (~2 KB).

### Deepest frame at release (ssh.c, -O3 -speed -maxoptpasses=100, -cpu=68000)
Frames (sub.w #N,a7 + saved regs): 1536 (+44 regs +4 ret = 1584) is OnKexReply (ssh.c:359; label l243 in `vc -S`; only caller is the kex dispatch). It holds the inlined key derivation buffers (the many Sha256_Add/Done, Aes128Ctr_Init, Rsa_VerifySha256, Bn_ModExp, memcmp calls are all in this one function). YES: X25519 is called beneath it (ssh.c:379 `X25519(secret, s->xPriv, f)`, one jsr _X25519 in the body), so the curve25519 peak stacks OnKexReply 1584 + X25519 and its callees. Within X25519 (x25519.c): X25519 itself sub.w #880, and its ladder helper (l74, calls Bn_Square 17x) sub.w #1400, each +44 regs +4 ret, plus a 64 B leaf (l52); that is the 2688 B standalone figure. Next largest ssh.c frames: 384 (l390), 332 (l150), 264, 252; all much smaller. So engine peak ~= dispatch frames (~370) + OnKexReply 1584 + X25519 chain 2688 = ~4640, which matches the measurement. group14 swaps the X25519 chain for Bn_ModExp (496 B standalone) under the same 1584 B frame: 3284.
Fix lever: OnKexReply's 1536 B of locals (move the hash/derivation scratch into struct Ssh or heap) and X25519's 1400+880 B (field elements as locals). Both are single-owner and static, so a persistent scratch in struct Ssh removes ~3 KB from the peak.

### Q2: does vbcc startup honour `long __stack = N;`
VERDICT: YES, and vamos runs the swap. Test q2/stk2.c (scratchpad stack/q2/) with `long __stack = 65536;`, vamos -C 68000 -m 8192 -s 4:
`&here=15024 stackLower=5050 stackPtr=15040 usable(here-lower)=65492`
Without __stack (stk_no, -s 4) &here=0x6010; with __stack=65536 (-s 4) &here=0x15010, identical to the plain build at -s 64: the stack moved to a freshly allocated 64 KB block. Allocated size stackPtr-stackLower = 65520 B; usable below main's locals 65492 B = 63.96 KB. Strict ">= 64 KB (65536)": short by 44 B, so request N + a few hundred bytes margin (e.g. 65536 + 512) if 64 KB usable is the requirement; for engine needs (~4.6 KB + DCTelnet frames) 16384 is plenty.
FindTask(NULL)->tc_SPLower/SPUpper read 0 under vamos (not populated), so the tc_ based report is meaningless there; the startup's own ___stackLower/___stackPtr are used. vamos did execute the StackSwap call (exec LVO -732).
Startup logic (startup.o, `hunktool -A info .../lib/startup.o`, offsets in the object):
```
1ca move.l ($3e,A3),D0     ; tc_SPUpper
1ce movea.l ($3a,A3),A0    ; tc_SPLower
1d2 sub.l A0,D0            ; current stack size
1d4 move.l ___stack,D4
1da cmp.l D4,D0
1dc bcc $22c               ; already >= __stack: no swap
1de cmpi.w #$24,($14,A6)   ; SysBase->LibNode.lib_Version >= 36
1e4 bcs $1fa               ; older: no swap, exit code 20
1ea add.l D4,D0 (D0=12+__stack) ; addq #8
1ee moveq #1,D1            ; MEMF_PUBLIC
1f0 jsr (-$2ac,A6)         ; AllocMem(__stack+20, ...)
...
216 movea.l A7,A1 ; 224 copies 12 bytes (return addr + args) to new top-16
226 jsr (-$2dc,A6)         ; StackSwap  (-732)
22c move.l A0,___stackLower ; 230 move.l A7,___stackPtr
```
Real AmigaOS: only swaps when the process stack is smaller than __stack and OS >= 2.0; project DCTelnet.info do_StackSize=0 means the 4096 default, so setting `long __stack` in the source is honoured by this startup (also needs linking with startup.o, not minstart.o).

## Fix 2026-10-30

Branch pr/ssh (worktree dctelnet-ssh). Root cause: big locals on the stack in the SSH engine and X25519.

What moved:
- struct Ssh gained hmac, mac[32], kx {Sha256, fp, iv, key}, mp[261] and a work union (X25519Work | DH {BnMod, two, fb, secret, pm1} | keyboard-interactive text/answer).
- X25519 / X25519_Base take a caller-owned struct X25519Work (field elements, 9-entry invert chain, 16-limb mul scratch). Mul/Sq/MulSmall take a scratch pointer; vbcc had inlined a 1128 B t[16] frame per call site.
- Former function-local statics mod/two/fb/secret in OnKexInit/OnKexReply now in struct Ssh (were shared across openers). Work union wiped before NEWKEYS.

Frames (gcc-12 -O0, before to after): OnKexReply 688 to 304, X25519 544 to 192, OnInfoRequest 416 to 112, OnKexInit 384 to 160. gcc -O2: X25519 976 to 256, Ssh_Receive 736 to 240.
vbcc (-O3 -speed, vamos 68000, engine-only = total minus 92 B caller frames): curve25519 4640 to 1488, group14 3284 to 1488. Standalone x25519 2688 to 500, x25519_base 2664 to 480.

Test proof: tests/Makefile check-stack (gcc -Wstack-usage=400, skipped on clang). With the src fix reverted it fails (ssh.c:359 688 B, ssh.c:550 416 B); with the fix it passes. make -B -C tests all green.

Still open: statics remain in bignum.c (t, x, acc, one, table), rsa.c (mod) and ssh.c (Choose pick, small text[]), all shared across openers; not stack related.
