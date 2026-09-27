Thanks for the detailed analysis here -- it made these fixable. With the reassembled source of `ibmcon.device` (now in `ibmcon/`, see #17), every cause you listed has a fix in the PR stack:

| Cause | Fix |
|---|---|
| Insert Lines `CSI L` (Retro Community, #9) | #22 (ibmcon 1.7) |
| CTerm 24-bit colour `CSI Ps;r;g;b t` (A-Net main menu on the top line, Absinthe NORMAL LOGIN on two lines) | ibmcon took the first parameter as a row count; since 1.6 (#17) a multi-parameter `CSI t` is ignored |
| Device Status Report `CSI 6 n` / `CSI 5 n` (Absinthe animation stall, 20 For Beers countdown, Constructive Chaos) | #25: DCTelnet answers, with the cursor position from ibmcon 1.9 |
| Cursor Backward `CSI 79 D` (Absinthe ticker) | #25: CUB/CUF stop at the margins instead of wrapping |

We don't call these BBSes ourselves, so I could only verify against a local test server on an emulated A1200 (DSR answers, CUB/CUF margins, Insert Lines, 24-bit colour ignored). **Could you test them on the real boards?** The top of the stack has everything:

1. Branch `feature/pr-ansi-dsr-cursor` on spotUP/dctelnet (PR #25).
2. Build DCTelnet as usual, and `ibmcon.device` with `make` in `ibmcon/` (vasm). DCTelnet opens the newest ibmcon it finds in its own `Devs` drawer or `DEVS:`; the start-up screen shows which one (`ibmcon.device 1.9 from DCTelnet's drawer`).
3. What to check:
   - **Absinthe:** login with Animation = YES does not stall; NORMAL LOGIN uses the whole screen; the main-menu ticker is clean.
   - **20 For Beers:** the countdown on the animation screen.
   - **A-Net Online:** the main menu is laid out over the screen, not on the top line.
   - **Constructive Chaos:** the InterBBS Wall. If it still crashes, the cause is something other than DSR; the sequence it sends would help.
   - **Retro Community:** the ABBS/MBBS fullscreen editor scrolling up and down (#9).

🤖 Generated with [Claude Code](https://claude.com/claude-code)
