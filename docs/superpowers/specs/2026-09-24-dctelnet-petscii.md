---
date: 2026-09-24
topic: "PETSCII terminal emulation + telnet TTYPE negotiation for DCTelnet"
tags: [amiga, dctelnet, petscii, telnet, xem, amiexpress-web]
status: draft
---

# Goal

Add real Commodore 64/128 PETSCII terminal emulation to DCTelnet (a native
Amiga telnet/BBS client), so it can connect to a PETSCII-capable BBS —
concretely, `amiexpress-web` — and negotiate into genuine PETSCII mode
(correct C64 glyphs, colors, cursor behavior), not an ANSI/CP437
approximation.

This is sub-project #1 of a larger effort (native AGA/RTG display backend
and hardware-banging fast screen mode are #2, deferred; WiFi-modem serial
connectivity is #3, deferred; further SyncTerm-parity emulations are
backlog).

# Background / why this base

- **CGTerm 2.0** (the original ask) targets Commodore 8-bit hardware
  (6502/6510, VIC-II, CIA userport RS-232) — nothing shares code, ABI, or
  chipset with Amiga/68k. A "port" of it would have meant a from-scratch
  reimplementation to its feature spec, not a code port.
- **SyncTerm** (Synchronet, GPLv2, actively maintained) was considered as an
  alternative base: portable C, CIOLIB/COMIO backend abstraction. Good
  reference architecture, not chosen as the base because DCTelnet already
  solves the two hardest parts for us (native Intuition GUI, direct
  bsdsocket.library TCP/IP) and we have the maintainer's permission to build
  on it directly.
- **DCTelnet** (bruno-frederic/dctelnet, forked with the maintainer's direct
  permission — he's a personal contact of the project owner) is the chosen
  base: native AmigaOS app, Kickstart 2.0+/68000+, vbcc + GNU Make hosted
  build (real intuition.library/bsdsocket.library linking), pluggable
  terminal emulation via **XEM** libraries and pluggable file transfer via
  **XPR** libraries — both are Amiga-ecosystem-standard plugin interfaces,
  not DCTelnet-specific, so a PETSCII XEM module has value beyond this one
  client.
  - Original DCTelnet code (Zed, 1998-2008) and Bruno's 2026 maintenance
    carry no formal open-source license; we're building on it under direct
    permission from the maintainer, not a public license grant. Do not
    represent this fork as generally open-source.

# Reference material consulted (not copied wholesale)

- **SyncTerm** (`src/conio/petscii.c`, `cterm_petscii.c/h`,
  `cterm_atascii.c` for the parallel case) — **LGPLv2+**, Rob Swindell.
  Architecture: a 32-byte control-code bitmap plus a 256-entry
  function-pointer handler table, dispatched from a generic byte loop —
  adding an emulation mode means adding a table, not touching the core.
  Also: a flat `display_petscii[]` PETSCII→CP437 fallback table, and an
  `input_petscii[]` keyboard translation table (C64 case-swap on letters;
  cursor/F-keys/RUN-STOP mapped to PETSCII control bytes). These specific
  files may be ported directly (not just referenced) since LGPL permits
  linking into non-LGPL code, provided the LGPL notice stays on those
  files.
- **amiexpress-web** (`web/backend/src/utils/petscii.util.ts`,
  `petscii-session-model.ts`, `c64-detect.util.ts`,
  `telnet-petscii-port.util.ts`) — the actual interop target's own
  implementation. Key correctness lesson SyncTerm's flat table doesn't
  capture: **wire PETSCII byte ≠ C64 screen code** — decode must go
  byte→screen-code→glyph, not byte→glyph directly
  (`petsciiToScreenCode()`), because the video chip's internal numbering
  differs from the wire/keyboard numbering. Glyph selection needs 4 font
  banks (unshifted/shifted × normal/reverse-video) — SyncTerm arrives at
  the same 4-bank need independently via spare attribute bits, which
  cross-validates the shape. Also surfaced: a configurable
  Swedish/Finnish charset-ROM variant (bracket-trio ↔ å/ä/ö) is a real,
  user-set option, not a universal constant.
- **amiexpress-web's telnet server**
  (`web/backend/src/server/telnet-server.ts`,
  `handlers/command-handler/pre-login.ts`) — defines the actual
  interop contract we must hit:
  - Server sends `IAC DO TELOPT_TTYPE` (RFC 1091) on connect.
  - `classifyTerminalType()` checks the reported TTYPE string: containing
    `C64`/`COMMODORE`/`PETSCII` → `isC64 = true` → server sets
    `session.terminalType = 'c64'`, which on the next keypress after the
    connect screen auto-sets `petsciiMode = true`, `ansiEnabled = false`,
    40×25 screen, skips the graphics (`ANSI, RIP or No graphics`) prompt
    entirely, and switches to raw `petscii-output` byte events.
  - A separate `isAmiga` classification (bare `TERM`, `AMIGA*` prefix,
    `NCOMM`, `JRCOMM`, `VT52`, or unanswered) forces ANSI-without-Unicode
    mode — this is what DCTelnet gets today by default.
  - A dedicated `TELNET_PETSCII_PORT` (opt-in per board) exists as a
    negotiation-free fallback, not needed if we implement TTYPE properly.

# Gap found in DCTelnet itself

`DCTelnet.c`, `connect.c`, `DCTelnet-protocol.h` implement **no telnet
option negotiation at all** — no IAC/DO/WILL/SB/SE handling anywhere. A
vendored `third_party/netinclude/arpa/telnet.h` (standard BSD telnet
constants) exists but is unused. `FLAG_SIMPLE_TELNET` /
`FLAG_RAW_CONNECTION` are menu-visible options with no corresponding
negotiation engine behind them. This means triggering amiexpress-web's
TTYPE-based auto-PETSCII-detect requires building that engine first — it
is a prerequisite, not an extension of existing code.

# Architecture

Two new modules, both new code:

## 1. `telnet_opts.c/h` — minimal telnet option negotiation

- Wires in the existing (unused) `arpa/telnet.h` constants.
- Sits in `connect.c`'s socket-read path, ahead of the XEM/emulation
  layer: consumes and answers IAC sequences, passes all other bytes
  through unchanged.
- Behavior:
  - `IAC DO TELOPT_TTYPE` → reply `IAC WILL TELOPT_TTYPE`.
  - `IAC SB TELOPT_TTYPE SEND IAC SE` → reply
    `IAC SB TELOPT_TTYPE IS "<mode-string>" IAC SE`, where mode-string is
    `"PETSCII"` when the user has selected PETSCII emulation, else
    `"AMIGA"`.
  - Any other `DO`/`WILL` we don't implement → reply `WONT`/`DONT`
    (refuse-by-default; never hang waiting for a capability we don't
    have).
- Must handle a `SB ... SE` sequence split across two `recv()` calls —
  buffer partial subnegotiation state across reads.

## 2. `Xem_petscii.c/h` — the PETSCII emulation

Same shape as SyncTerm's `cterm_petscii.c`:

- `petscii_ctrl_bitmap[32]` — which of 256 byte values are control codes.
- `petscii_handlers[256]` — function-pointer table, one per control byte:
  cursor movement, the 16 C64 color control codes, home/clear, reverse
  video on/off, charset (shift) toggle.
- Decode path is screen-code-correct: incoming byte → C64 screen code →
  glyph index into one of 4 font banks (unshifted/shifted ×
  normal/reverse), selected by persistent per-connection state (current
  shift, reverse-video, background color) — not a flat lookup table.
- Charset-ROM variant (standard vs Swedish/Finnish) as a new DCTelnet
  Prefs option.
- Keyboard path: a translation table (case-swap on letters;
  cursor/F-keys/RUN-STOP → PETSCII control bytes), same shape as
  SyncTerm's `input_petscii[]`, written fresh for DCTelnet's key-event
  model.
- Selecting "PETSCII" in DCTelnet's existing emulation-selection UI both
  activates this module's byte-dispatch table for rendering/input AND
  sets `telnet_opts.c`'s negotiation string to `"PETSCII"`.

# Data flow

1. Connect → `telnet_opts.c` watches the read stream for IAC bytes,
   handles negotiation inline, passes everything else through.
2. Server (having received our TTYPE) flips to PETSCII mode and starts
   sending raw PETSCII bytes → `Xem_petscii.c`'s dispatch loop: control
   bitmap check → handler, or printable path → screen-code lookup → glyph
   from the active font bank → blit into the console window's RasterPort
   (via the existing XEM `Text()`-based path for v1; the later
   hardware-banging screen backend, sub-project #2, is a separate output
   path this module doesn't need to know about).
3. Local keypress → `input_petscii[]`-style table → byte(s) written to
   the socket.

# Open items (must close before/alongside implementation, not blocking this spec)

1. **Font source.** Need the actual 4-bank C64 glyph bitmap. In order of
   preference: (a) an already-open-licensed C64 font clone — needs a real
   license check, not an assumption; (b) SyncTerm's `allfonts.c`
   Commodore entries, once the "imported from FreeBSD syscons" provenance
   is confirmed permissive for that specific glyph subset; (c) hand-drawn
   as last resort.
2. **No test harness in DCTelnet at all.** New code (`telnet_opts.c`,
   `Xem_petscii.c`) needs unit tests added from scratch — no existing
   suite to extend. At minimum: byte-dispatch table completeness (every
   control byte 0-31/128-159 handled or explicit no-op), screen-code
   conversion cross-checked against amiexpress-web's own table on a fixed
   byte set, and a telnet negotiation test with a canned IAC sequence
   including one split mid-`SB...SE`.

# Testing / done-criterion

Reachability, not unit-only: boot the built DCTelnet exe under Amiberry
(or real hardware), connect to a running `amiexpress-web` instance, select
PETSCII emulation, confirm the board's log shows `[C64] Real C64 terminal
detected`, confirm the rendered screen shows correct PETSCII glyphs (not
garbage or the CP437 fallback), and complete a login. This is the
top-level entry point proof required before calling this feature done —
matching the reachability standard used on the sibling demo-system
project.

# Explicitly out of scope for this spec

- Native AGA/RTG display backend and hardware-banging fast screen mode
  (sub-project #2).
- WiFi-modem (AT-command) serial connectivity (sub-project #3).
- ATASCII, RIP, Avatar, and other SyncTerm-parity emulations (backlog,
  spec'd individually later if wanted).
- SDL/SDL_AGA — dropped; not used anywhere in this design.
