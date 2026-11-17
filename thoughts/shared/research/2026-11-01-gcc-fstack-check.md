---
date: 2026-11-01
topic: Does gcc (bebbo/AmigaPorts, libnix nix20) -fstack-check share vbcc -stack-check's second-process failure?
status: complete (source-read; no binary built, unverified by disassembly)
---

# Findings

Sources: github.com/AmigaPorts/gcc branch amiga10.4 (bebbo/* repos 404; AmigaPorts is the canonical fork), github.com/AmigaPorts/libnix master.

## Q1 What -fstack-check emits
- The Amiga-specific stack check (`__stkovf` / `__stkchk_d0`, compare against global `___stk_limit`) lives in
  gcc/config/m68k/amigaos.c:61-80 (`amigaos_prologue_begin_hook`, `if (TARGET_STACKCHECK)`), but that whole block
  is inside `#if 0` (amigaos.c:57), i.e. dead. It is driven by -mstackcheck / -mstackextend (amigaos.h:93-99), not -fstack-check.
- m68k.c has no `-fstack-check` hook (no probe_stack / STACK_CHECK_* in m68k.c, m68k.h, m68k.md, amigaos.h; grep empty).
  The only limit compare in the backend is for explicit `-fstack-limit-symbol/-register` (m68k.c:1110-1118, ctrap against stack_limit_rtx).
  So plain -fstack-check is the generic GCC mode: no call to __stkchk_*, no compare against a limit (we do not pass -fstack-limit-*).
  Caveat: generic GCC probing behaviour inferred from absence of target support, not from disassembly.
- If the check WERE active (-mstackcheck): libnix `sources/stack/init_stk.c:7,22-31` sets one global `__stk_limit` once at startup from
  `FindTask(NULL)->pr_Task.tc_SPLower` (+ safezone). Same single-global design as vbcc.
  Runtime tool `sources/nix/misc/stkchk.c:35` (`__checkstack`) uses FindTask-style per-process tc_SPLower but only measures usage at exit.

## Q2 Second task in DCTelnet
- YES. src/DCTelnet.c:4985 (BeginServerConnection): `CreateNewProcTags(NP_Entry, HandleConnectingWindowTask, TAG_DONE)`.
  HandleConnectingWindowTask (src/connect.c:138) opens the "Connecting..." window, timer.device, handles Abort; it runs on that second process.
  Everything else (EstablishTCPConnection, UI, terminal) runs on the main task. The task's grep missed it: DCTelnet.c is treated as binary
  by grep; use `grep -a`. No other CreateTask/AddTask/SystemTags/RunCommand in src (third_party excluded).

## Q3 Verdict
- gcc debug build does NOT share the vbcc failure mode via -fstack-check: no global-limit compare is emitted (dead code / generic mode).
  Residual risk only if someone adds -mstackcheck/-mstackextend (then same single global __stk_limit, second process would trip it).
- 7a0ceb7 rationale (second process) is correct for vbcc and matches the code (CreateNewProcTags is present).
- Not tested here: the gcc debug build on the rig. Keeping -fstack-check on gcc is harmless-to-useless; cheap confirmation = one rig run of the gcc debug build with a host on the command line.
