# 2026-11-11 RMB menu bisect: INCONCLUSIVE, no first bad commit found

Result: no commit identified. The rig could not reproduce the symptom with the injected right button, even on a
build the owner calls good, so there was nothing to bisect against.

## What was tested (rig 3, 68060 max, 640x256 depth 4 DCTelnet screen, unconnected)
- HEAD 7c08df3 (rig binary, md5 9a7456a9...): right button held 1.5-3 s at several positions, then screen grab
  (ami.py shot) and a host capture of the emulator window (screencapture of the FS-UAE window): no menu strip drawn at all.
- dc29c0a built from a scratch worktree (make 68020 with the host toolchain env of host-ci, 40 s, output
  build/vbcc-68020-release/DCTelnet.68020): identical result, no strip drawn.
- Control: Workbench (Directory Opus screen) with the same injected hold: no menu bar drawn either.
  So an injected right button (wasabi MOUSE action 2, button 1 = right) does not open the Intuition menu strip on this
  rig, for any program. The tests above therefore say nothing about DCTelnet.
- Both grab and emulator-window capture agree, so the strip is not just missing from wasabid's grab.

## Side findings
- DCTelnet starts unconnected with a blank "Processor: / Kickstart: / Renderer: / TCP Stack:" banner (values empty)
  on the 5 builds/runs seen; unverified whether that is normal for the 7c08df3 build.
- A left click inside the DCTelnet window opens "Connection Options" (its interior stayed blank in one emulator
  capture, gadgets were present in the uitree): possibly a stale-redraw sign, not investigated.
- Ctrl-C (Break) does not quit DCTelnet; Right-Amiga+Q (ami.py key 10 80) does. A second Right-Amiga+Q with no
  DCTelnet running reaches Directory Opus ("end your session" requester); dismissed with Cancel, DOpus still up.

## Static scan (not a verdict)
Since dc29c0a, src/vtbuiltin.c grew by ~780 lines (kitty picture / RIP rendering, commits 9af2a9c f5fe155 a4ea15a
0f1cef4 3a0da7a 290297f fbe0463 7c08df3); vtpens.c +14, DCTelnet.c +22. I did not read the diff for unlocked draws
into VtImgRp/term_rp: that is the next step, together with a way to open the strip.

## Next step to unblock
Open the strip some other way: a real mouse (owner), or a rig helper that sends the right button as raw input events
with IEQUALIFIER_RBUTTON, then bisect between dc29c0a (claimed good) and 7c08df3 by building with the recipe above
(env VBCC/PATH/NDK32_INC/NET_INC from build host-ci, `make TOOLCHAIN=vbcc BUILD_HASH=x 68020`, ~40 s a build).
Candidate order for the bisect: 3a0da7a, 290297f/7c08df3 (kitty palette), 0bd337a (PR 87 pens), f5fe155/9af2a9c (RIP draws).
