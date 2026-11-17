---
date: 2026-10-30
topic: "Cost of Ansi_Swap17() in ConWrite() on a 68000 (PR #44 reviewer question)"
status: measured instruction counts, estimated cycles; console.device figure is an assumption (unverified)
---

# Ansi_Swap17() cost on a 68000

Code: `fork/feature/pr-console-fixes`, `src/ansiscan.c` Ansi_Swap17 / Swap17Sgr, called from `src/DCTelnet.c` ConWrite() in 1024-byte parts before each `DoIO(CMD_WRITE)`.

## Method

- vbcc (host), `-cpu=68000 -O3 -speed -maxoptpasses=100`, ansiscan.c unmodified from the branch.
- Benchmark `swap17/bench.c` calls Ansi_Swap17 once over a 4096-byte buffer under vamos (68000).
- Exact instruction count: vamos instruction trace (`-I`), taken from the call to the return (includes libvc memcpy and the 32-bit multiply helpers).
- Cycles: each traced instruction priced with a 68000 timing table (`swap17/cyc.py`: register/EA modes, taken vs not-taken branches, mulu 46). This is a model, not a cycle-exact emulator: expect +/-10 %. No wait states, no chip-RAM contention (an A500 with only chip RAM runs code slower, up to roughly 1.3x).
- Inputs (no `.ans` file exists in the repo, so ANSI art is synthetic):
  - plain: random printable ASCII, 4096 bytes
  - sgr/12: a SGR sequence (`ESC[1;37;41m`, `ESC[0;31m`, `ESC[1;33;44m`, `ESC[0m`, ...) after every 12 text bytes (typical ANSI art)
  - sgr/4: a SGR sequence after every 4 text bytes (worst realistic case)
- Files: `/private/tmp/claude-501/-Users-spot-Code-DCTelnet/c0a23933-ed3d-4dee-a857-a826fcab1366/scratchpad/swap17/` (bench.c, cyc.py, traces).

## Result (A500, 7.09 MHz)

console.device time assumed 3 KB/s (range 2-8 KB/s, see below), i.e. 1365 ms per 4 KB; in brackets the overhead if console.device does 8 KB/s (512 ms).

| input | bytes | instr/byte | cycles/byte | ms per 4 KB | KB/s | % of console.device time |
|---|---|---|---|---|---|---|
| plain text | 4096 | 15.0 | 156 | 90 | 45 | 6.6 % (18 %) |
| SGR every 12 bytes | 4096 | 33.0 | 338 | 195 | 21 | 14 % (38 %) |
| SGR every 4 bytes | 4096 | 44.5 | 455 | 263 | 15.6 | 19 % (51 %) |

Per ANSI-art screen (4-8 KB, colour density like sgr/12): about 195-390 ms of Ansi_Swap17.

## Where the time goes

- Plain text: 15 instructions per byte; state, input index and length live in memory/stack, so every byte pays several memory loads and a branch chain (cmp/tst on state, ESC, 0x9B).
- SGR: Swap17Sgr multiplies by 10 per digit (`mulu`, ~46 cycles each, 14-19 % of the SGR cost) plus the memcpy of the held sequence.

## console.device reference (assumption)

No measured console.device throughput exists in `thoughts/` (grep for speed/benchmark figures found none). console.device renders via graphics.library Text() per run, with scroll blits; on a stock 68000/OCS Amiga it is commonly in the low KB/s range, 2-8 KB/s depending on font, depth and scrolling. This is an estimate, not a measurement; to close it, time `Ansi_Swap17` against a DoIO CMD_WRITE of the same buffer on the target with `timer.device` EClock.

ConWrite is synchronous (DoIO), so Swap17 time adds to the console.device time; it does not overlap.

## When the call runs

Only for `APP_RENDERER_CONSOLE_DEVICE`. The built-in renderer (term_feed) returns earlier; ibmcon (`APP_RENDERER_IBMCON_DEVICE`) writes `data` directly with no swap. So ibmcon and the built-in renderer pay nothing.

## Reply draft for Bruno

I measured Ansi_Swap17 on a 68000 (vbcc -O3, instruction-counted, 7.09 MHz A500 timings): plain text costs about 156 cycles per byte (90 ms per 4 KB), colour-heavy ANSI art 340-455 cycles per byte (200-260 ms per 4 KB). Against console.device, which draws at roughly 2-8 KB/s on a stock Amiga, that is about 7-20 % extra at 3 KB/s, more if console.device is fast. It only runs for the console.device renderer; ibmcon and the built-in renderer are untouched. I can make the plain-text path cheaper if you want (a tight scan to the next ESC), but I did not want to tune it before you decided it matters.

## Optimisation 2026-10-30

Change (src/ansiscan.c, worktree dctelnet-console, uncommitted): in state 0 a local-pointer loop copies bytes straight to the output until the next ESC or CSI (0x1B/0x9B); the state byte lives in a local variable across the loop and is written back once at exit. A fused scan-and-copy loop was used instead of scan then memcpy: it avoids a library call per run, which matters when runs are 4 to 12 bytes. Behaviour unchanged: the new test `test_swap_long_mixed_input_split_at_every_offset` (plain runs, SGR, non-SGR CSI, lone ESC, ESC ESC, 8-bit CSI, a broken sequence; whole and split at every offset into two calls, compared with a string literal) passes on both the old and the new code, and the full test_ansiscan builds clean with -Wall -Wextra -Werror -std=c99.

Method as above (same flags, same bench, same three inputs; bench folder `scratchpad/swap17b/`, traces `tr_old_*`/`tr_new_*`, cyc2.py = cyc.py with the call/return addresses given by environment).

| input | before cycles/byte | after cycles/byte | before instr/byte | after instr/byte |
|---|---|---|---|---|
| plain text | 156 | 72 | 15.0 | 9.0 |
| SGR every 12 bytes | 338 | 303 | 33.0 | 30.7 |
| SGR every 4 bytes | 455 | 450 | 44.5 | 44.7 |

Plain text is now about 2.2 times cheaper (about 42 ms per 4 KB instead of 90 ms). Colour-heavy art gains little, because the cost there is in the held-sequence path (Swap17Sgr and the memcpy of each sequence), which this change does not touch.

Draft reply for Bruno:
I made the plain-text path of the colour fix about twice as cheap: it now costs roughly 72 cycles per byte on a stock 68000 instead of 156, so about 42 ms per 4 KB. ANSI art with a colour change every few bytes improves only a little (about 300 to 450 cycles per byte), since that cost is in rewriting the sequences themselves. Output is byte-for-byte the same as before, including sequences split between reads, and a new test checks that at every split point.

## Measured on real hardware (2026-11-01)

Owner's A1200 (68060, AmigaOS 3.2), EClock 709379 Hz, median of 5 runs, 4096 bytes written as 4 x 1024 like ConWrite(), Workbench console window 640x200 topaz 8. Console write = (swap + write) minus the separately timed swap, so it includes scrolling and drawing.

| Buffer | Ansi_Swap17 us | Console write us | Console KB/s | Swap share |
|---|---|---|---|---|
| plain text | 658 | ~3950 | ~1040 | 14% |
| SGR-heavy (ESC[1;33m + 4 chars) | 3973 | ~7730 | ~530 | 34% |
| SGR-light (ESC[0m every ~12 chars) | 1757 | ~4520 | ~900 | 28% |

console.device on this machine is ~100x faster than the 2-8 KB/s assumed above, so the earlier "overhead as a fraction of console time" figures are wrong for fast machines; on a 060 the swap is 14-34% of the write path. A 68000 figure still needs a 68000 (or a cycle model of console.device). Outliers up to 12x the median were seen (multitasking). The heavy buffer uses colour 33, which Ansi_Swap17 scans but does not rewrite. Tool source and raw data: 2026-11-01-console-eclock-a1200.md.
