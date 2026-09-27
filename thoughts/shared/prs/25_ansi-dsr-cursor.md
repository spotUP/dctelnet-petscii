Addresses #11. **Stacked on #24.** Only the last commit belongs here.

## Summary

Your analysis in #11 named the causes; with the ibmcon source they are fixable:

| Cause | Status |
|---|---|
| **Insert Lines** (Retro Community, #9) | fixed in #22 (ibmcon 1.7) |
| **CTerm 24-bit colour** `CSI Ps;r;g;b t` (A-Net main menu on the top line, Absinthe NORMAL LOGIN on two lines) | ibmcon took the first parameter as a **row count**; since 1.6 (#17) a `CSI t` with more than one parameter is ignored, as terminals without 24-bit colour should |
| **Device Status Report** `CSI 6 n` / `CSI 5 n` (Absinthe animation stall, 20 For Beers countdown, Constructive Chaos) | **this PR** |
| **Cursor Backward** `CSI 79 D` (Absinthe ticker) | **this PR** |

- **DSR.** The console only draws, so DCTelnet answers. A scanner (`src/dsr.c`, host-tested, including a request split between two network reads) finds `CSI 5 n` and `CSI 6 n` in the BBS's text. `BbsWrite()` draws the text up to the request, asks ibmcon where the cursor is (new `IBMCMD_GETCURSOR`, ibmcon **1.9**) and sends `ESC[row;colR`, or `ESC[0n` for `CSI 5 n`.
- **CUB/CUF.** ibmcon's Cursor Backward at column 1 went up to the end of the previous line, and Cursor Forward at the right edge wrapped to the next one. So `CSI 79 D` from short of column 80 landed a line too high. Both now stop at the margin, as in ANSI.SYS and VT100.

Not checked against the live BBSes yet: the Constructive Chaos crash may have another cause.

## Testing

- Host unit tests: `cd test && make` (`test_dsr`).
- On an emulated A1200 with a local server: `CSI 6 n` after `here` at row 5, column 10 is answered `ESC[5;14R`; `CSI 5 n` is answered `ESC[0n`; `CSI 79 D` and `CSI 20 C` stop at columns 1 and 80 of their row.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
