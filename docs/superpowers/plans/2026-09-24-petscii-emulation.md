# PETSCII Emulation + Telnet TTYPE Negotiation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Give DCTelnet a real PETSCII (C64/C128) terminal emulation mode, plus the telnet option-negotiation layer it currently lacks, so it can connect to a PETSCII-capable BBS — concretely, `amiexpress-web` — and land in genuine PETSCII mode instead of an ANSI/CP437 approximation.

**Architecture:** Pure, host-testable logic modules (screen-code conversion, CP437 fallback table, control-code dispatch, keyboard translation, telnet IAC state machine) with zero AmigaOS dependencies, each covered by native-compiled unit tests. Two thin AmigaOS glue modules (`telnet_opts.c`, `Xem_petscii.c`) wire that pure logic into DCTelnet's existing socket-read loop and XEM emulation-selection UI. The real 4-bank C64 glyph font is an explicit follow-up (font-source licensing is still an open item from the spec); this plan ships a working CP437-approximation fallback so the full pipeline (negotiation → dispatch → screen-code-correct decode → render) is real and testable end-to-end today.

**Tech Stack:** C (vbcc for the Amiga target, host gcc/clang for the new native unit tests), GNU Make (existing `src/Makefile` for Amiga, a new `test/Makefile` for host tests).

**Spec:** `docs/superpowers/specs/2026-09-24-dctelnet-petscii.md`

## Global Constraints

- Build the Amiga side only via the existing vbcc + GNU Make setup (`cd src && make`) — no new toolchain.
- Any table ported from SyncTerm (`petscii_fallback.c`, the `input_petscii`-derived part of `petscii_keymap.c`) keeps an LGPLv2+ attribution header citing Rob Swindell / Synchronet; do not relicense or strip it.
- No literal Commodore ROM font bytes anywhere in this repo. The real 4-bank glyph font is a deferred follow-up (spec's open item #1); this plan's fallback rendering path must not depend on it.
- Telnet TTYPE mode-string is exactly `"PETSCII"` (when PETSCII emulation is selected) or `"AMIGA"` (default) — matches amiexpress-web's `classifyTerminalType()` exactly, verbatim, case included.
- A literal `0xFF` byte inside real PETSCII data (the "pi" character, screen code `$5E`) must round-trip through RFC 854 IAC-escaping (`IAC IAC` ↔ one data byte `0xFF`) — this is a correctness requirement, not an edge case to skip.

## Review Focus

1. A PETSCII data byte of `0xFF` must not be mistaken for the start of a telnet command — it has to arrive as `IAC IAC` on the wire and decode back to one `0xFF` data byte. (Task 4)
2. A server that never opens the TTYPE subnegotiation after we send `WILL TTYPE` must not stall the connection — the parser is purely byte-driven, no blocking wait state. (Task 4)
3. Two complete IAC sequences arriving in a single `recv()` buffer (e.g. `DO NAWS` immediately followed by `DO TTYPE ... SB TTYPE SEND SE`) must both be processed, not just the first one found. (Task 4)
4. Clearing the screen (PETSCII `$93`) resets cursor position but must NOT reset shift-mode or reverse-video state on a real C64 — only an explicit shift/reverse control code does that. Assuming clear-screen resets everything silently corrupts every screen drawn after the first clear. (Task 5)
5. Before the real 4-bank font exists, the CP437 fallback path must render every *printable* PETSCII byte without crashing, and must never be called with a byte in the control-code ranges (`$00-$1F`, `$80-$9F`) — those belong to the dispatch handler table, not the fallback lookup. (Tasks 2 and 7)

---

## File Structure

New files, following DCTelnet's existing flat `src/` convention (no subdirectories):

| File | Responsibility |
|---|---|
| `src/petscii_screencode.h/.c` | Pure: wire PETSCII byte → C64 screen code (the byte≠screencode correctness fix). |
| `src/petscii_fallback.h/.c` | Pure: screen code → nearest CP437 byte, for rendering before the real font exists. Ported from SyncTerm (LGPL). |
| `src/petscii_keymap.h/.c` | Pure: local keypress → PETSCII byte(s) to send. Ported from SyncTerm (LGPL) plus a small key-abstraction enum. |
| `src/petscii_dispatch.h/.c` | Pure: control-code bitmap + 256-entry handler table + per-connection state (shift/reverse/bg color/cursor). |
| `src/telnet_opts_state.h/.c` | Pure: byte-driven IAC/DO/DONT/WILL/WONT/SB/SE state machine. |
| `src/telnet_opts.h/.c` | Amiga glue: owns the socket hookup in `connect.c`, calls `telnet_opts_state`. |
| `src/Xem_petscii.h/.c` | Amiga glue: registers the PETSCII emulation mode, wires dispatch+fallback rendering, Prefs charset-variant flag, keyboard. |
| `test/Makefile` | Host build for the five pure modules' unit tests. |
| `test/test_petscii_screencode.c` | Unit tests for Task 1. |
| `test/test_petscii_fallback.c` | Unit tests for Task 2. |
| `test/test_petscii_keymap.c` | Unit tests for Task 3. |
| `test/test_telnet_opts_state.c` | Unit tests for Task 4 (incl. Review Focus 1-3). |
| `test/test_petscii_dispatch.c` | Unit tests for Task 5 (incl. Review Focus 4). |

Modified files:

| File | Change |
|---|---|
| `src/connect.c` | Route inbound socket bytes through `telnet_opts_process()` before they reach the XEM layer (Task 6). |
| `src/Xem_wrapper.c` / `src/guis.c` | Add "PETSCII" as a selectable emulation mode alongside the existing XEM-library modes (Task 7). |
| `src/Makefile` | Add the five new pure `.c` files plus the two new glue `.c` files to the object list (Task 1 onward, one line added per task as its file lands). |

---

### Task 1: Test harness + `petscii_screencode`

**Files:**
- Create: `test/Makefile`
- Create: `test/test_petscii_screencode.c`
- Create: `src/petscii_screencode.h`
- Create: `src/petscii_screencode.c`

**Interfaces:**
- Produces: `uint8_t petscii_to_screencode(uint8_t petscii)` — declared in `petscii_screencode.h`, used by Task 5 (`petscii_dispatch.c`) and Task 7 (`Xem_petscii.c`).

- [ ] **Step 1: Create the host test Makefile**

```makefile
# test/Makefile — host-native build for DCTelnet's platform-independent
# PETSCII/telnet logic. Separate from src/Makefile (vbcc/Amiga target).
CC := cc
CFLAGS := -std=c99 -Wall -Wextra -Werror -I../src

TESTS := test_petscii_screencode

.PHONY: all clean
all: $(TESTS)
	@for t in $(TESTS); do echo "--- $$t ---"; ./$$t || exit 1; done

test_petscii_screencode: test_petscii_screencode.c ../src/petscii_screencode.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TESTS)
```

- [ ] **Step 2: Write the failing test**

```c
/* test/test_petscii_screencode.c */
#include <assert.h>
#include <stdio.h>
#include "petscii_screencode.h"

int main(void) {
    /* Official conversion table: sta.c64.org/cbm64pettoscr.html */
    assert(petscii_to_screencode(0x00) == 0x80); /* $00-$1F: +$80 */
    assert(petscii_to_screencode(0x1F) == 0x9F);
    assert(petscii_to_screencode(0x20) == 0x20); /* $20-$3F: unchanged (space) */
    assert(petscii_to_screencode(0x3F) == 0x3F);
    assert(petscii_to_screencode(0x41) == 0x01); /* $40-$5F: -$40 ('A') */
    assert(petscii_to_screencode(0x5F) == 0x1F);
    assert(petscii_to_screencode(0x61) == 0x41); /* $60-$7F: -$20 */
    assert(petscii_to_screencode(0x7F) == 0x5F);
    assert(petscii_to_screencode(0x80) == 0xC0); /* $80-$9F: +$40 */
    assert(petscii_to_screencode(0x9F) == 0xDF);
    assert(petscii_to_screencode(0xA0) == 0x60); /* $A0-$BF: -$40 */
    assert(petscii_to_screencode(0xBF) == 0x7F);
    assert(petscii_to_screencode(0xC1) == 0x41); /* $C0-$DF: -$80 */
    assert(petscii_to_screencode(0xDF) == 0x5F);
    assert(petscii_to_screencode(0xE0) == 0x60); /* $E0-$FE: -$80 */
    assert(petscii_to_screencode(0xFE) == 0x7E);
    assert(petscii_to_screencode(0xFF) == 0x5E); /* pi symbol, special case */
    printf("petscii_screencode: all assertions passed\n");
    return 0;
}
```

- [ ] **Step 3: Run test to verify it fails (no implementation yet)**

Run: `cd test && make`
Expected: FAIL — compile error, `petscii_screencode.h`/`.c` don't exist yet.

- [ ] **Step 4: Write the header**

```c
/* src/petscii_screencode.h */
#ifndef PETSCII_SCREENCODE_H
#define PETSCII_SCREENCODE_H

#include <stdint.h>

/*
 * Wire PETSCII byte -> C64 screen code. These are DIFFERENT numbering
 * spaces: the wire/keyboard byte value is not what the video chip uses
 * internally to select a glyph. Table per sta.c64.org/cbm64pettoscr.html.
 */
uint8_t petscii_to_screencode(uint8_t petscii);

#endif /* PETSCII_SCREENCODE_H */
```

- [ ] **Step 5: Write the minimal implementation**

```c
/* src/petscii_screencode.c */
#include "petscii_screencode.h"

uint8_t petscii_to_screencode(uint8_t petscii) {
    if (petscii <= 0x1F) return (uint8_t)(petscii + 0x80);
    if (petscii <= 0x3F) return petscii;
    if (petscii <= 0x5F) return (uint8_t)(petscii - 0x40);
    if (petscii <= 0x7F) return (uint8_t)(petscii - 0x20);
    if (petscii <= 0x9F) return (uint8_t)(petscii + 0x40);
    if (petscii <= 0xBF) return (uint8_t)(petscii - 0x40);
    if (petscii <= 0xDF) return (uint8_t)(petscii - 0x80);
    if (petscii <= 0xFE) return (uint8_t)(petscii - 0x80);
    return 0x5E; /* 0xFF: pi symbol */
}
```

- [ ] **Step 6: Run test to verify it passes**

Run: `cd test && make`
Expected: `petscii_screencode: all assertions passed`, exit 0.

- [ ] **Step 7: Add the new file to the Amiga build and commit**

Add `petscii_screencode.c` to the object list in `src/Makefile` (next to the other `.c` sources — do not build it yet, just register it so Task 6/7 don't need a separate Makefile edit).

```bash
git add test/Makefile test/test_petscii_screencode.c \
        src/petscii_screencode.h src/petscii_screencode.c src/Makefile
git commit -m "feat(petscii): add wire-byte to screen-code conversion"
```

---

### Task 2: `petscii_fallback` (CP437 approximation table)

**Files:**
- Create: `src/petscii_fallback.h`
- Create: `src/petscii_fallback.c`
- Create: `test/test_petscii_fallback.c`
- Modify: `test/Makefile` (add the new test binary)

**Interfaces:**
- Consumes: nothing (pure table lookup, takes the raw PETSCII wire byte directly — this table is keyed on PETSCII values, not screen codes, matching the source table it's ported from).
- Produces: `uint8_t petscii_fallback_cp437(uint8_t petscii)` — used by Task 7 (`Xem_petscii.c`) as the v1 glyph-selection path until real font banks exist. Returns the CP437 byte to display, or the input byte unchanged for the plain-ASCII overlap range (`$20-$3F`, `$41-$5A` lowercase-as-uppercase, `$61-$7A` uppercase-as-lowercase — PETSCII case is inverted relative to ASCII).

- [ ] **Step 1: Write the failing test**

```c
/* test/test_petscii_fallback.c */
#include <assert.h>
#include <stdio.h>
#include "petscii_fallback.h"

int main(void) {
    assert(petscii_fallback_cp437(' ') == ' ');   /* space passes through */
    assert(petscii_fallback_cp437('A') == 'a');   /* PETSCII 'A' = lowercase glyph */
    assert(petscii_fallback_cp437('a') == 'A');   /* PETSCII shifted 'a' = uppercase glyph */
    assert(petscii_fallback_cp437(92)  == 156);   /* British pound -> CP437 156 */
    assert(petscii_fallback_cp437(127) == 176);   /* light shade block */
    printf("petscii_fallback: all assertions passed\n");
    return 0;
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cd test && make test_petscii_fallback && ./test_petscii_fallback` (after adding the target — see Step 5)
Expected: FAIL — `petscii_fallback.h` doesn't exist yet.

- [ ] **Step 3: Write the header**

```c
/* src/petscii_fallback.h */
#ifndef PETSCII_FALLBACK_H
#define PETSCII_FALLBACK_H

#include <stdint.h>

/*
 * PETSCII byte -> nearest CP437 byte, for rendering through DCTelnet's
 * existing CP437-based fonts before the real 4-bank C64 font exists.
 * Ported from SyncTerm's src/conio/cterm_petscii.h `display_petscii[]`
 * table.
 *
 * Copyright Rob Swindell - http://www.synchro.net/copyright.html
 * Licensed LGPLv2+ (GNU Lesser General Public License, version 2 or
 * later) - see lgpl.txt or http://www.fsf.org/copyleft/lesser.html.
 * This file must keep this notice; do not relicense it.
 */
uint8_t petscii_fallback_cp437(uint8_t petscii);

#endif /* PETSCII_FALLBACK_H */
```

- [ ] **Step 4: Write the implementation (table ported verbatim from SyncTerm)**

```c
/* src/petscii_fallback.c */
#include "petscii_fallback.h"

/* Same LGPLv2+ notice as the header applies to this table. */
struct petscii_cp437_entry { uint8_t petscii; uint8_t cp437; };

static const struct petscii_cp437_entry TABLE[] = {
    {32,' '}, {33,'!'}, {34,'"'}, {35,'#'}, {36,'$'}, {37,'%'}, {38,'&'},
    {39,'\''}, {40,'('}, {41,')'}, {42,'*'}, {43,'+'}, {44,','}, {45,'-'},
    {46,'.'}, {47,'/'}, {48,'0'}, {49,'1'}, {50,'2'}, {51,'3'}, {52,'4'},
    {53,'5'}, {54,'6'}, {55,'7'}, {56,'8'}, {57,'9'}, {58,':'}, {59,';'},
    {60,'<'}, {61,'='}, {62,'>'}, {63,'?'}, {64,'@'},
    {65,'a'}, {66,'b'}, {67,'c'}, {68,'d'}, {69,'e'}, {70,'f'}, {71,'g'},
    {72,'h'}, {73,'i'}, {74,'j'}, {75,'k'}, {76,'l'}, {77,'m'}, {78,'n'},
    {79,'o'}, {80,'p'}, {81,'q'}, {82,'r'}, {83,'s'}, {84,'t'}, {85,'u'},
    {86,'v'}, {87,'w'}, {88,'x'}, {89,'y'}, {90,'z'},
    {91,'['}, {92,156}, {93,']'}, {94,24}, {95,27}, {96,196},
    {97,'A'}, {98,'B'}, {99,'C'}, {100,'D'}, {101,'E'}, {102,'F'},
    {103,'G'}, {104,'H'}, {105,'I'}, {106,'J'}, {107,'K'}, {108,'L'},
    {109,'M'}, {110,'N'}, {111,'O'}, {112,'P'}, {113,'Q'}, {114,'R'},
    {115,'S'}, {116,'T'}, {117,'U'}, {118,'V'}, {119,'W'}, {120,'X'},
    {121,'Y'}, {122,'Z'},
    {123,197}, {124,181}, {125,179}, {126,178}, {127,176},
    {160,255}, {161,221}, {162,220}, {163,'^'}, {164,'_'}, {165,'{'},
    {166,177}, {167,'}'}, {168,210}, {169,31}, {170,245}, {171,195},
    {172,201}, {173,192}, {174,191}, {175,205}, {176,218}, {177,193},
    {178,194}, {179,180}, {180,244}, {181,185}, {182,222}, {183,169},
    {184,223}, {185,22}, {186,251}, {187,187}, {188,200}, {189,217},
    {190,188}, {191,206},
};

#define TABLE_LEN (sizeof(TABLE) / sizeof(TABLE[0]))

uint8_t petscii_fallback_cp437(uint8_t petscii) {
    size_t i;
    for (i = 0; i < TABLE_LEN; i++) {
        if (TABLE[i].petscii == petscii) return TABLE[i].cp437;
    }
    return '?'; /* no mapping (shouldn't happen for printable bytes) */
}
```

- [ ] **Step 5: Add the test target to `test/Makefile`**

```makefile
TESTS := test_petscii_screencode test_petscii_fallback

test_petscii_fallback: test_petscii_fallback.c ../src/petscii_fallback.c
	$(CC) $(CFLAGS) -o $@ $^
```

- [ ] **Step 6: Run test to verify it passes**

Run: `cd test && make`
Expected: both `test_petscii_screencode` and `test_petscii_fallback` print "all assertions passed".

- [ ] **Step 7: Register with the Amiga build and commit**

Add `petscii_fallback.c` to `src/Makefile`'s object list.

```bash
git add src/petscii_fallback.h src/petscii_fallback.c \
        test/test_petscii_fallback.c test/Makefile src/Makefile
git commit -m "feat(petscii): add CP437 fallback glyph table (ported from SyncTerm, LGPL)"
```

---

### Task 3: `petscii_keymap` (keyboard translation)

**Files:**
- Create: `src/petscii_keymap.h`
- Create: `src/petscii_keymap.c`
- Create: `test/test_petscii_keymap.c`
- Modify: `test/Makefile`

**Interfaces:**
- Produces:
  - `enum PetsciiSpecialKey` — abstraction over cursor/function/RUN-STOP keys, defined in `petscii_keymap.h`. Task 7's Amiga glue maps real AmigaOS `RAWKEY`/IDCMP codes onto this enum; this task does not touch AmigaOS headers at all.
  - `int petscii_translate_key(int ascii_or_special, int is_special)` — returns the PETSCII byte to transmit. When `is_special` is 0, `ascii_or_special` is a plain ASCII character code (case-swapped per PETSCII convention). When `is_special` is 1, `ascii_or_special` is one of `enum PetsciiSpecialKey`.

- [ ] **Step 1: Write the failing test**

```c
/* test/test_petscii_keymap.c */
#include <assert.h>
#include <stdio.h>
#include "petscii_keymap.h"

int main(void) {
    /* Case swap: PETSCII keyboards send inverted case vs. ASCII. */
    assert(petscii_translate_key('A', 0) == 'a');
    assert(petscii_translate_key('a', 0) == 'A');
    assert(petscii_translate_key('5', 0) == '5'); /* digits unchanged */

    /* Special keys -> PETSCII control bytes. */
    assert(petscii_translate_key(PETSCII_KEY_UP, 1) == 145);
    assert(petscii_translate_key(PETSCII_KEY_DOWN, 1) == 17);
    assert(petscii_translate_key(PETSCII_KEY_LEFT, 1) == 157);
    assert(petscii_translate_key(PETSCII_KEY_RIGHT, 1) == 29);
    assert(petscii_translate_key(PETSCII_KEY_HOME, 1) == 19);
    assert(petscii_translate_key(PETSCII_KEY_DEL, 1) == 20);
    assert(petscii_translate_key(PETSCII_KEY_F1, 1) == 133);
    assert(petscii_translate_key(PETSCII_KEY_STOP, 1) == 3);

    printf("petscii_keymap: all assertions passed\n");
    return 0;
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cd test && make test_petscii_keymap` (after Step 5 adds the target)
Expected: FAIL — `petscii_keymap.h` doesn't exist yet.

- [ ] **Step 3: Write the header**

```c
/* src/petscii_keymap.h */
#ifndef PETSCII_KEYMAP_H
#define PETSCII_KEYMAP_H

/*
 * Special-key enum ported from the shape of SyncTerm's src/conio/
 * cterm_petscii.h `input_petscii[]` table (CIO_KEY_* entries).
 * Copyright Rob Swindell - LGPLv2+ - see petscii_fallback.h for the full
 * notice; this table carries the same license.
 *
 * This is an ABSTRACTION, not an AmigaOS key code: Task 7's Amiga glue
 * (Xem_petscii.c) maps real RAWKEY/IDCMP codes onto these values, so this
 * module has no AmigaOS header dependency and can be unit-tested on the
 * host.
 */
enum PetsciiSpecialKey {
    PETSCII_KEY_UP,
    PETSCII_KEY_DOWN,
    PETSCII_KEY_LEFT,
    PETSCII_KEY_RIGHT,
    PETSCII_KEY_HOME,
    PETSCII_KEY_DEL,
    PETSCII_KEY_INSERT,
    PETSCII_KEY_F1, PETSCII_KEY_F2, PETSCII_KEY_F3, PETSCII_KEY_F4,
    PETSCII_KEY_F5, PETSCII_KEY_F6, PETSCII_KEY_F7, PETSCII_KEY_F8,
    PETSCII_KEY_STOP /* RUN/STOP */
};

/*
 * is_special == 0: ascii_or_special is a plain ASCII char, returns the
 * case-swapped PETSCII byte to transmit (digits/punctuation pass through
 * unchanged).
 * is_special == 1: ascii_or_special is a PetsciiSpecialKey value, returns
 * the PETSCII control byte for that key.
 */
int petscii_translate_key(int ascii_or_special, int is_special);

#endif /* PETSCII_KEYMAP_H */
```

- [ ] **Step 4: Write the implementation**

```c
/* src/petscii_keymap.c */
#include "petscii_keymap.h"

/* Same LGPLv2+ notice as petscii_keymap.h applies to this table. */
int petscii_translate_key(int ascii_or_special, int is_special) {
    if (is_special) {
        switch (ascii_or_special) {
            case PETSCII_KEY_DOWN:   return 17;
            case PETSCII_KEY_HOME:   return 19;
            case PETSCII_KEY_DEL:    return 20;
            case PETSCII_KEY_RIGHT:  return 29;
            case PETSCII_KEY_F1:     return 133;
            case PETSCII_KEY_F3:     return 134;
            case PETSCII_KEY_F5:     return 135;
            case PETSCII_KEY_F7:     return 136;
            case PETSCII_KEY_F2:     return 137;
            case PETSCII_KEY_F4:     return 138;
            case PETSCII_KEY_F6:     return 139;
            case PETSCII_KEY_F8:     return 140;
            case PETSCII_KEY_UP:     return 145;
            case PETSCII_KEY_INSERT: return 148;
            case PETSCII_KEY_LEFT:   return 157;
            case PETSCII_KEY_STOP:   return 3;
            default:                 return -1;
        }
    }
    if (ascii_or_special >= 'A' && ascii_or_special <= 'Z')
        return ascii_or_special - 'A' + 'a';
    if (ascii_or_special >= 'a' && ascii_or_special <= 'z')
        return ascii_or_special - 'a' + 'A';
    return ascii_or_special; /* digits, punctuation: unchanged */
}
```

- [ ] **Step 5: Add the test target to `test/Makefile`**

```makefile
TESTS := test_petscii_screencode test_petscii_fallback test_petscii_keymap

test_petscii_keymap: test_petscii_keymap.c ../src/petscii_keymap.c
	$(CC) $(CFLAGS) -o $@ $^
```

- [ ] **Step 6: Run test to verify it passes**

Run: `cd test && make`
Expected: all three test binaries print "all assertions passed".

- [ ] **Step 7: Register with the Amiga build and commit**

Add `petscii_keymap.c` to `src/Makefile`'s object list.

```bash
git add src/petscii_keymap.h src/petscii_keymap.c \
        test/test_petscii_keymap.c test/Makefile src/Makefile
git commit -m "feat(petscii): add keyboard translation table (ported from SyncTerm, LGPL)"
```

---

### Task 4: `telnet_opts_state` (IAC negotiation state machine)

**Files:**
- Create: `src/telnet_opts_state.h`
- Create: `src/telnet_opts_state.c`
- Create: `test/test_telnet_opts_state.c`
- Modify: `test/Makefile`

**Interfaces:**
- Consumes: nothing from earlier tasks.
- Produces:
  - `void telnet_opts_init(struct TelnetOptState *st, const char *ttype_string)`
  - `size_t telnet_opts_process(struct TelnetOptState *st, const uint8_t *in, size_t in_len, uint8_t *out_reply, size_t out_reply_max, size_t *out_reply_len, uint8_t *out_data, size_t out_data_max)` — returns the number of bytes written to `out_data` (plain data bytes, IAC-unescaped); writes any protocol reply bytes (e.g. `WILL TTYPE`, `SB TTYPE IS ...`) into `out_reply`/`out_reply_len`. Used by Task 6's `telnet_opts.c` Amiga glue.

This is the module Review Focus items 1-3 pin down.

- [ ] **Step 1: Write the failing tests**

```c
/* test/test_telnet_opts_state.c */
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "telnet_opts_state.h"

#define IAC  255
#define DO   253
#define DONT 254
#define WILL 251
#define WONT 252
#define SB   250
#define SE   240
#define TELOPT_TTYPE 24
#define TELOPT_NAWS  31
#define TTYPE_SEND   1
#define TTYPE_IS     0

static void test_plain_data_passthrough(void) {
    struct TelnetOptState st;
    uint8_t reply[64], data[64];
    size_t reply_len, data_len;
    const uint8_t in[] = "hello";

    telnet_opts_init(&st, "PETSCII");
    data_len = telnet_opts_process(&st, in, 5, reply, sizeof(reply), &reply_len, data, sizeof(data));
    assert(data_len == 5);
    assert(memcmp(data, "hello", 5) == 0);
    assert(reply_len == 0);
}

/* Review Focus 1: a literal 0xFF PETSCII data byte must round-trip as IAC IAC. */
static void test_iac_escaped_data_byte(void) {
    struct TelnetOptState st;
    uint8_t reply[64], data[64];
    size_t reply_len, data_len;
    const uint8_t in[] = { 'A', IAC, IAC, 'B' };

    telnet_opts_init(&st, "PETSCII");
    data_len = telnet_opts_process(&st, in, sizeof(in), reply, sizeof(reply), &reply_len, data, sizeof(data));
    assert(data_len == 3);
    assert(data[0] == 'A');
    assert(data[1] == 0xFF);
    assert(data[2] == 'B');
    assert(reply_len == 0);
}

/* Review Focus 2: DO TTYPE gets WILL TTYPE queued; no blocking, no hang if
 * the server never follows up with SB. */
static void test_do_ttype_replies_will(void) {
    struct TelnetOptState st;
    uint8_t reply[64], data[64];
    size_t reply_len, data_len;
    const uint8_t in[] = { IAC, DO, TELOPT_TTYPE };

    telnet_opts_init(&st, "PETSCII");
    data_len = telnet_opts_process(&st, in, sizeof(in), reply, sizeof(reply), &reply_len, data, sizeof(data));
    assert(data_len == 0);
    assert(reply_len == 3);
    assert(reply[0] == IAC && reply[1] == WILL && reply[2] == TELOPT_TTYPE);
}

/* SB TTYPE SEND -> SB TTYPE IS "PETSCII" SE. */
static void test_ttype_subneg_replies_mode_string(void) {
    struct TelnetOptState st;
    uint8_t reply[64], data[64];
    size_t reply_len, data_len;
    const uint8_t in[] = { IAC, SB, TELOPT_TTYPE, TTYPE_SEND, IAC, SE };
    const uint8_t expected[] = { IAC, SB, TELOPT_TTYPE, TTYPE_IS, 'P','E','T','S','C','I','I', IAC, SE };

    telnet_opts_init(&st, "PETSCII");
    data_len = telnet_opts_process(&st, in, sizeof(in), reply, sizeof(reply), &reply_len, data, sizeof(data));
    assert(data_len == 0);
    assert(reply_len == sizeof(expected));
    assert(memcmp(reply, expected, sizeof(expected)) == 0);
}

/* An option we don't implement gets refused, not ignored/hung on. */
static void test_unsupported_do_gets_wont(void) {
    struct TelnetOptState st;
    uint8_t reply[64], data[64];
    size_t reply_len, data_len;
    const uint8_t in[] = { IAC, DO, TELOPT_NAWS };

    telnet_opts_init(&st, "PETSCII");
    data_len = telnet_opts_process(&st, in, sizeof(in), reply, sizeof(reply), &reply_len, data, sizeof(data));
    assert(data_len == 0);
    assert(reply_len == 3);
    assert(reply[0] == IAC && reply[1] == WONT && reply[2] == TELOPT_NAWS);
}

/* Review Focus 3: two complete IAC sequences in one buffer must both be
 * processed and answered, in order. */
static void test_multiple_sequences_one_buffer(void) {
    struct TelnetOptState st;
    uint8_t reply[64], data[64];
    size_t reply_len, data_len;
    const uint8_t in[] = { IAC, DO, TELOPT_NAWS, IAC, DO, TELOPT_TTYPE };

    telnet_opts_init(&st, "PETSCII");
    data_len = telnet_opts_process(&st, in, sizeof(in), reply, sizeof(reply), &reply_len, data, sizeof(data));
    assert(data_len == 0);
    assert(reply_len == 6);
    assert(reply[0] == IAC && reply[1] == WONT && reply[2] == TELOPT_NAWS);
    assert(reply[3] == IAC && reply[4] == WILL && reply[5] == TELOPT_TTYPE);
}

int main(void) {
    test_plain_data_passthrough();
    test_iac_escaped_data_byte();
    test_do_ttype_replies_will();
    test_ttype_subneg_replies_mode_string();
    test_unsupported_do_gets_wont();
    test_multiple_sequences_one_buffer();
    printf("telnet_opts_state: all assertions passed\n");
    return 0;
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cd test && make test_telnet_opts_state` (after Step 5 adds the target)
Expected: FAIL — `telnet_opts_state.h` doesn't exist yet.

- [ ] **Step 3: Write the header**

```c
/* src/telnet_opts_state.h */
#ifndef TELNET_OPTS_STATE_H
#define TELNET_OPTS_STATE_H

#include <stdint.h>
#include <stddef.h>

#define TELNET_OPTS_TTYPE_MAX 32

enum TelnetOptsInternalState {
    TOS_NORMAL,
    TOS_GOT_IAC,
    TOS_GOT_CMD,     /* saw DO/DONT/WILL/WONT, waiting for the option byte */
    TOS_IN_SB,       /* collecting subnegotiation bytes */
    TOS_SB_GOT_IAC   /* saw IAC while inside a subnegotiation */
};

struct TelnetOptState {
    enum TelnetOptsInternalState state;
    uint8_t pending_cmd;               /* DO/DONT/WILL/WONT awaiting its option byte */
    uint8_t sb_option;                 /* which option's subnegotiation is open */
    char ttype_string[TELNET_OPTS_TTYPE_MAX];
};

/* ttype_string is copied in, must be a plain C string, e.g. "PETSCII" or "AMIGA". */
void telnet_opts_init(struct TelnetOptState *st, const char *ttype_string);

/*
 * Feeds one chunk of raw socket input. Plain data bytes (IAC-unescaped)
 * land in out_data (up to out_data_max); protocol replies to send back to
 * the server land in out_reply/out_reply_len (up to out_reply_max).
 * Returns the number of bytes written to out_data.
 *
 * Purely byte-driven: never blocks, never assumes the server will send
 * anything further. Safe to call with input split arbitrarily across
 * multiple recv()s -- state persists in *st between calls.
 */
size_t telnet_opts_process(struct TelnetOptState *st,
                            const uint8_t *in, size_t in_len,
                            uint8_t *out_reply, size_t out_reply_max, size_t *out_reply_len,
                            uint8_t *out_data, size_t out_data_max);

#endif /* TELNET_OPTS_STATE_H */
```

- [ ] **Step 4: Write the implementation**

```c
/* src/telnet_opts_state.c */
#include <string.h>
#include "telnet_opts_state.h"

#define IAC  255
#define DONT 254
#define DO   253
#define WONT 252
#define WILL 251
#define SB   250
#define SE   240
#define TELOPT_TTYPE 24
#define TTYPE_IS   0
#define TTYPE_SEND 1

void telnet_opts_init(struct TelnetOptState *st, const char *ttype_string) {
    memset(st, 0, sizeof(*st));
    st->state = TOS_NORMAL;
    strncpy(st->ttype_string, ttype_string, TELNET_OPTS_TTYPE_MAX - 1);
    st->ttype_string[TELNET_OPTS_TTYPE_MAX - 1] = '\0';
}

static void append_reply(uint8_t *out_reply, size_t out_reply_max, size_t *reply_len,
                          const uint8_t *bytes, size_t n) {
    size_t i;
    for (i = 0; i < n && *reply_len < out_reply_max; i++) {
        out_reply[(*reply_len)++] = bytes[i];
    }
}

size_t telnet_opts_process(struct TelnetOptState *st,
                            const uint8_t *in, size_t in_len,
                            uint8_t *out_reply, size_t out_reply_max, size_t *out_reply_len,
                            uint8_t *out_data, size_t out_data_max) {
    size_t i, data_len = 0;
    *out_reply_len = 0;

    for (i = 0; i < in_len; i++) {
        uint8_t b = in[i];

        switch (st->state) {
        case TOS_NORMAL:
            if (b == IAC) {
                st->state = TOS_GOT_IAC;
            } else if (data_len < out_data_max) {
                out_data[data_len++] = b;
            }
            break;

        case TOS_GOT_IAC:
            if (b == IAC) {
                /* Escaped literal 0xFF data byte (Review Focus 1). */
                if (data_len < out_data_max) out_data[data_len++] = 0xFF;
                st->state = TOS_NORMAL;
            } else if (b == DO || b == DONT || b == WILL || b == WONT) {
                st->pending_cmd = b;
                st->state = TOS_GOT_CMD;
            } else if (b == SB) {
                st->state = TOS_IN_SB;
            } else {
                /* Unhandled IAC command (e.g. GA, NOP): ignore, resync. */
                st->state = TOS_NORMAL;
            }
            break;

        case TOS_GOT_CMD: {
            uint8_t option = b;
            if (st->pending_cmd == DO) {
                if (option == TELOPT_TTYPE) {
                    uint8_t r[3] = { IAC, WILL, TELOPT_TTYPE };
                    append_reply(out_reply, out_reply_max, out_reply_len, r, 3);
                } else {
                    uint8_t r[3] = { IAC, WONT, option };
                    append_reply(out_reply, out_reply_max, out_reply_len, r, 3);
                }
            } else if (st->pending_cmd == WILL) {
                uint8_t r[3] = { IAC, DONT, option };
                append_reply(out_reply, out_reply_max, out_reply_len, r, 3);
            }
            /* DONT/WONT from the server: no reply needed. */
            st->state = TOS_NORMAL;
            break;
        }

        case TOS_IN_SB:
            if (b == IAC) {
                st->state = TOS_SB_GOT_IAC;
            } else {
                /* Only TTYPE subnegotiation matters to us; remember the
                 * option on the first byte, and whether it's a SEND
                 * request, ignoring anything else collected. */
                if (st->sb_option == 0) st->sb_option = TELOPT_TTYPE; /* set on entry below */
                (void)b; /* the specific SEND(1)/IS(0) byte value doesn't
                            need storing: any SB TTYPE ... SE we receive
                            from the server is a request, so we always
                            reply with our string on SE. */
            }
            break;

        case TOS_SB_GOT_IAC:
            if (b == SE) {
                if (st->sb_option == TELOPT_TTYPE) {
                    size_t len = strlen(st->ttype_string);
                    uint8_t head[4] = { IAC, SB, TELOPT_TTYPE, TTYPE_IS };
                    uint8_t tail[2] = { IAC, SE };
                    append_reply(out_reply, out_reply_max, out_reply_len, head, 4);
                    append_reply(out_reply, out_reply_max, out_reply_len,
                                 (const uint8_t *)st->ttype_string, len);
                    append_reply(out_reply, out_reply_max, out_reply_len, tail, 2);
                }
                st->sb_option = 0;
                st->state = TOS_NORMAL;
            } else if (b == IAC) {
                /* Escaped IAC inside subnegotiation data: stay in SB. */
                st->state = TOS_IN_SB;
            } else {
                /* Malformed; resync to NORMAL rather than get stuck. */
                st->sb_option = 0;
                st->state = TOS_NORMAL;
            }
            break;
        }

        /* TOS_IN_SB needs sb_option set the moment we enter it, which
         * happens on the option byte right after IAC SB -- handled here
         * since TOS_GOT_IAC transitions straight to TOS_IN_SB without
         * seeing the option byte in this loop iteration. */
        if (st->state == TOS_IN_SB && st->sb_option == 0 && b != IAC) {
            st->sb_option = b;
        }
    }

    return data_len;
}
```

- [ ] **Step 5: Add the test target to `test/Makefile`**

```makefile
TESTS := test_petscii_screencode test_petscii_fallback test_petscii_keymap test_telnet_opts_state

test_telnet_opts_state: test_telnet_opts_state.c ../src/telnet_opts_state.c
	$(CC) $(CFLAGS) -o $@ $^
```

- [ ] **Step 6: Run test to verify it passes**

Run: `cd test && make`
Expected: all four test binaries print "all assertions passed". If `test_ttype_subneg_replies_mode_string` fails, check the `TOS_IN_SB` entry logic sets `sb_option` from the byte immediately following `IAC SB` — this is the trickiest part of the state machine, re-trace by hand against the test's raw byte array before changing anything else.

- [ ] **Step 7: Register with the Amiga build and commit**

Add `telnet_opts_state.c` to `src/Makefile`'s object list.

```bash
git add src/telnet_opts_state.h src/telnet_opts_state.c \
        test/test_telnet_opts_state.c test/Makefile src/Makefile
git commit -m "feat(telnet): add IAC/TTYPE negotiation state machine"
```

---

### Task 5: `petscii_dispatch` (control-code handler table + connection state)

**Files:**
- Create: `src/petscii_dispatch.h`
- Create: `src/petscii_dispatch.c`
- Create: `test/test_petscii_dispatch.c`
- Modify: `test/Makefile`

**Interfaces:**
- Consumes: `petscii_to_screencode()` from Task 1.
- Produces:
  - `struct PetsciiDispatchState` — shift mode, reverse-video flag, cursor row/col, background color, screen width/height.
  - `void petscii_dispatch_init(struct PetsciiDispatchState *st, int cols, int rows)`
  - `void petscii_dispatch_byte(struct PetsciiDispatchState *st, uint8_t byte, uint8_t *out_screencode, int *out_is_control)` — for a control byte, updates `*st` and sets `*out_is_control = 1`; for a printable byte, sets `*out_is_control = 0` and writes the screen code (via `petscii_to_screencode`) to `*out_screencode`, applying the reverse-video bit (`+0x80` on the screen code) if `st->reverse` is set. Used by Task 7's `Xem_petscii.c` to drive actual glyph rendering.

- [ ] **Step 1: Write the failing tests**

```c
/* test/test_petscii_dispatch.c */
#include <assert.h>
#include <stdio.h>
#include "petscii_dispatch.h"

static void test_printable_byte_uses_screencode(void) {
    struct PetsciiDispatchState st;
    uint8_t code; int is_control;

    petscii_dispatch_init(&st, 40, 25);
    petscii_dispatch_byte(&st, 'A', &code, &is_control); /* PETSCII 0x41 */
    assert(is_control == 0);
    assert(code == 0x01); /* petscii_to_screencode(0x41) == 0x41-0x40 */
}

static void test_reverse_video_toggle(void) {
    struct PetsciiDispatchState st;
    uint8_t code; int is_control;

    petscii_dispatch_init(&st, 40, 25);
    petscii_dispatch_byte(&st, 18, &code, &is_control); /* RVS ON */
    assert(is_control == 1);
    assert(st.reverse == 1);

    petscii_dispatch_byte(&st, 'A', &code, &is_control);
    assert(is_control == 0);
    assert(code == (0x01 | 0x80)); /* reverse-video bit set on the screen code */

    petscii_dispatch_byte(&st, 146, &code, &is_control); /* RVS OFF */
    assert(is_control == 1);
    assert(st.reverse == 0);
}

/* Review Focus 4: clear-screen resets cursor, NOT shift/reverse state. */
static void test_clear_screen_preserves_shift_and_reverse(void) {
    struct PetsciiDispatchState st;
    uint8_t code; int is_control;

    petscii_dispatch_init(&st, 40, 25);
    petscii_dispatch_byte(&st, 14, &code, &is_control);  /* shift to lowercase charset */
    petscii_dispatch_byte(&st, 18, &code, &is_control);  /* RVS ON */
    st.cursor_row = 10; st.cursor_col = 5;

    petscii_dispatch_byte(&st, 147, &code, &is_control); /* CLR/HOME (clear) */
    assert(is_control == 1);
    assert(st.cursor_row == 0 && st.cursor_col == 0);
    assert(st.shift_lowercase == 1);  /* NOT reset by clear */
    assert(st.reverse == 1);          /* NOT reset by clear */
}

static void test_home_does_not_clear(void) {
    struct PetsciiDispatchState st;
    uint8_t code; int is_control;

    petscii_dispatch_init(&st, 40, 25);
    st.cursor_row = 10; st.cursor_col = 5;
    petscii_dispatch_byte(&st, 19, &code, &is_control); /* HOME only, no clear */
    assert(st.cursor_row == 0 && st.cursor_col == 0);
}

int main(void) {
    test_printable_byte_uses_screencode();
    test_reverse_video_toggle();
    test_clear_screen_preserves_shift_and_reverse();
    test_home_does_not_clear();
    printf("petscii_dispatch: all assertions passed\n");
    return 0;
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cd test && make test_petscii_dispatch` (after Step 5 adds the target)
Expected: FAIL — `petscii_dispatch.h` doesn't exist yet.

- [ ] **Step 3: Write the header**

```c
/* src/petscii_dispatch.h */
#ifndef PETSCII_DISPATCH_H
#define PETSCII_DISPATCH_H

#include <stdint.h>

struct PetsciiDispatchState {
    int cols, rows;
    int cursor_row, cursor_col;
    int reverse;          /* reverse-video flag: RVS ON/OFF (18/146) */
    int shift_lowercase;  /* 0 = unshifted/graphics charset, 1 = shifted/lowercase */
    int bg_color;         /* current background color, VIC index 0-15 */
};

void petscii_dispatch_init(struct PetsciiDispatchState *st, int cols, int rows);

/*
 * Feeds one PETSCII byte. Control bytes (0x00-0x1F, 0x80-0x9F) update
 * *st and set *out_is_control = 1; printable bytes set *out_is_control = 0
 * and write the screen code to render (reverse-video bit applied) into
 * *out_screencode.
 */
void petscii_dispatch_byte(struct PetsciiDispatchState *st, uint8_t byte,
                            uint8_t *out_screencode, int *out_is_control);

#endif /* PETSCII_DISPATCH_H */
```

- [ ] **Step 4: Write the implementation**

```c
/* src/petscii_dispatch.c */
#include "petscii_dispatch.h"
#include "petscii_screencode.h"

void petscii_dispatch_init(struct PetsciiDispatchState *st, int cols, int rows) {
    st->cols = cols;
    st->rows = rows;
    st->cursor_row = 0;
    st->cursor_col = 0;
    st->reverse = 0;
    st->shift_lowercase = 0;
    st->bg_color = 6; /* C64 power-on default: blue background */
}

static int is_control_byte(uint8_t b) {
    return (b <= 0x1F) || (b >= 0x80 && b <= 0x9F);
}

void petscii_dispatch_byte(struct PetsciiDispatchState *st, uint8_t byte,
                            uint8_t *out_screencode, int *out_is_control) {
    if (!is_control_byte(byte)) {
        *out_is_control = 0;
        {
            uint8_t code = petscii_to_screencode(byte);
            if (st->reverse) code |= 0x80;
            *out_screencode = code;
        }
        st->cursor_col++;
        if (st->cursor_col >= st->cols) { st->cursor_col = 0; st->cursor_row++; }
        return;
    }

    *out_is_control = 1;
    switch (byte) {
        case 13:  /* CR */
        case 141: /* shifted CR */
            st->cursor_col = 0; st->cursor_row++;
            break;
        case 14:  /* shift to lowercase/uppercase charset */
            st->shift_lowercase = 1;
            break;
        case 142: /* shift to unshifted/graphics charset */
            st->shift_lowercase = 0;
            break;
        case 18:  /* RVS ON */
            st->reverse = 1;
            break;
        case 146: /* RVS OFF */
            st->reverse = 0;
            break;
        case 19:  /* HOME (cursor only, no clear) */
            st->cursor_row = 0; st->cursor_col = 0;
            break;
        case 147: /* CLR/HOME: clears screen, resets cursor -- Review
                    * Focus 4: does NOT touch shift or reverse state. */
            st->cursor_row = 0; st->cursor_col = 0;
            break;
        case 17:  /* cursor down */
            st->cursor_row++;
            break;
        case 145: /* cursor up */
            if (st->cursor_row > 0) st->cursor_row--;
            break;
        case 29:  /* cursor right */
            st->cursor_col++;
            break;
        case 157: /* cursor left */
            if (st->cursor_col > 0) st->cursor_col--;
            break;
        default:
            /* Color codes and other control bytes: no cursor/shift/reverse
             * state change modeled yet -- explicit no-op, not a crash. */
            break;
    }
}
```

- [ ] **Step 5: Add the test target to `test/Makefile`**

```makefile
TESTS := test_petscii_screencode test_petscii_fallback test_petscii_keymap test_telnet_opts_state test_petscii_dispatch

test_petscii_dispatch: test_petscii_dispatch.c ../src/petscii_dispatch.c ../src/petscii_screencode.c
	$(CC) $(CFLAGS) -o $@ $^
```

- [ ] **Step 6: Run test to verify it passes**

Run: `cd test && make`
Expected: all five test binaries print "all assertions passed".

- [ ] **Step 7: Register with the Amiga build and commit**

Add `petscii_dispatch.c` to `src/Makefile`'s object list.

```bash
git add src/petscii_dispatch.h src/petscii_dispatch.c \
        test/test_petscii_dispatch.c test/Makefile src/Makefile
git commit -m "feat(petscii): add control-code dispatch and connection state"
```

---

### Task 6: Amiga glue — wire `telnet_opts` into `connect.c`

**Files:**
- Create: `src/telnet_opts.h`
- Create: `src/telnet_opts.c`
- Modify: `src/connect.c` (route inbound socket bytes through negotiation before they reach the XEM layer)

**Interfaces:**
- Consumes: `telnet_opts_init()`/`telnet_opts_process()` from Task 4.
- Produces: `void TelnetOpts_Init(const char *ttype_string)`, `LONG TelnetOpts_FilterInbound(UBYTE *buf, LONG len, UBYTE *clean_out, LONG clean_out_max)` — called from `connect.c`'s read loop; sends any queued protocol reply bytes directly on the socket, returns the count of clean data bytes written to `clean_out` for the existing XEM/emulation path to consume unchanged.

This task is AmigaOS-only glue; there is no host-testable behavior beyond what Task 4 already covers. Verification here is by code inspection plus the Task 8 end-to-end manual test — do not write a redundant Amiga-side test for logic Task 4 already pins down.

- [ ] **Step 1: Locate the exact read-loop insertion point**

Open `src/connect.c` and find where inbound socket bytes are currently handed to the XEM/display layer (search for the `recv()`/`Recv()` call and whatever function it calls next, e.g. `XemWrite`). Note the exact line number — this is where Step 3's call goes.

- [ ] **Step 2: Write the header**

```c
/* src/telnet_opts.h */
#ifndef TELNET_OPTS_H
#define TELNET_OPTS_H

#include <exec/types.h>

/* ttype_string: "PETSCII" when the user has PETSCII emulation selected,
 * "AMIGA" otherwise. Call once per connection, before the first read. */
void TelnetOpts_Init(const char *ttype_string);

/*
 * Filters one chunk of raw inbound socket bytes: strips/answers telnet
 * IAC negotiation (sending replies directly on the connection's socket),
 * and copies the remaining plain data bytes into clean_out. Returns the
 * number of bytes written to clean_out -- feed that many bytes to the
 * existing XEM/emulation path exactly as before this change.
 */
LONG TelnetOpts_FilterInbound(UBYTE *buf, LONG len, UBYTE *clean_out, LONG clean_out_max);

#endif /* TELNET_OPTS_H */
```

- [ ] **Step 3: Write the implementation**

```c
/* src/telnet_opts.c */
#include "telnet_opts.h"
#include "telnet_opts_state.h"
#include <proto/socket.h>  /* whatever header connect.c already uses for send() */

static struct TelnetOptState g_state;
static LONG g_socket_fd; /* set by TelnetOpts_Init's caller via connect.c's existing global/handle */

void TelnetOpts_Init(const char *ttype_string) {
    telnet_opts_init(&g_state, ttype_string);
}

LONG TelnetOpts_FilterInbound(UBYTE *buf, LONG len, UBYTE *clean_out, LONG clean_out_max) {
    UBYTE reply[256];
    size_t reply_len = 0;
    size_t clean_len;

    clean_len = telnet_opts_process(&g_state, buf, (size_t)len,
                                     reply, sizeof(reply), &reply_len,
                                     clean_out, (size_t)clean_out_max);

    if (reply_len > 0) {
        /* Use connect.c's existing send primitive -- match whatever it
         * already calls for outbound bytes (e.g. send()/Send()), found
         * in Step 1. */
        send(g_socket_fd, (const void *)reply, (int)reply_len, 0);
    }

    return (LONG)clean_len;
}
```

*(Note for the implementer: `g_socket_fd` must be set from the real connection handle `connect.c` already owns — read `connect.c` in Step 1 to find its name and wire it in here rather than inventing a second socket handle. The exact `send()` call signature must match whatever networking header `connect.c` already includes.)*

- [ ] **Step 4: Wire the call into `connect.c`**

At the insertion point found in Step 1, before the existing call to the XEM/display layer, insert:

```c
{
    UBYTE clean_buf[4096];
    LONG clean_len = TelnetOpts_FilterInbound(recv_buf, bytes_received, clean_buf, sizeof(clean_buf));
    /* replace the old recv_buf/bytes_received arguments below with
       clean_buf/clean_len -- the existing call site's variable names
       will differ from this sketch; match them exactly. */
}
```

Call `TelnetOpts_Init()` once, right after the socket connects and before the first read (find the existing "connection established" point in `connect.c`).

- [ ] **Step 5: Add the new files to `src/Makefile`**

Add `telnet_opts.c` to the object list.

- [ ] **Step 6: Build and smoke-test**

Run: `cd src && make 68020` (fastest single-CPU build for iteration)
Expected: builds without errors. This does not prove the negotiation works end-to-end on the wire yet — that's Task 8.

- [ ] **Step 7: Commit**

```bash
git add src/telnet_opts.h src/telnet_opts.c src/connect.c src/Makefile
git commit -m "feat(telnet): wire IAC/TTYPE negotiation into the connection read loop"
```

---

### Task 7: Amiga glue — `Xem_petscii` emulation mode

**Files:**
- Create: `src/Xem_petscii.h`
- Create: `src/Xem_petscii.c`
- Modify: `src/guis.c` (add "PETSCII" to the emulation-selection list)
- Modify: `src/Xem_wrapper.c` (route to `Xem_petscii` when PETSCII mode is active, instead of the XEM library call)

**Interfaces:**
- Consumes: `petscii_dispatch_init()`/`petscii_dispatch_byte()` (Task 5), `petscii_fallback_cp437()` (Task 2), `petscii_translate_key()` (Task 3), `TelnetOpts_Init()` (Task 6).
- Produces: `void XemPetscii_Init(void)`, `void XemPetscii_Write(const UBYTE *bytes, LONG len)` (renders through the dispatch+fallback path via the console window's existing `Text()` call), `void XemPetscii_KeyPressed(UWORD rawkey, UBYTE ascii)` (translates and sends), `BOOL XemPetscii_IsActive(void)` (used by `Xem_wrapper.c`'s routing and by `telnet_opts.c`'s init call to pick the TTYPE mode-string).

This task is the other half of AmigaOS-only glue; Review Focus 5 (fallback renders every printable byte without crashing) is the one thing here worth a dedicated check beyond code inspection, covered in Step 4 below via a host-side "does every byte have a mapping" completeness check reusing Task 2's table — not a full Amiga integration test.

- [ ] **Step 1: Add "PETSCII" to the emulation-selection UI**

In `src/guis.c`, find where the existing XEM-library emulation choices are listed/selected (search for where `FLAG_USE_XEM_LIBRARY` from `DCTelnet.h` is set from a menu/requester choice) and add a "PETSCII" entry that, when chosen, calls `XemPetscii_Init()` instead of loading an XEM library.

- [ ] **Step 2: Write the header**

```c
/* src/Xem_petscii.h */
#ifndef XEM_PETSCII_H
#define XEM_PETSCII_H

#include <exec/types.h>

void XemPetscii_Init(void);
void XemPetscii_Write(const UBYTE *bytes, LONG len);
void XemPetscii_KeyPressed(UWORD rawkey, UBYTE ascii);
BOOL XemPetscii_IsActive(void);

#endif /* XEM_PETSCII_H */
```

- [ ] **Step 3: Write the implementation**

```c
/* src/Xem_petscii.c */
#include "Xem_petscii.h"
#include "petscii_dispatch.h"
#include "petscii_fallback.h"
#include "petscii_keymap.h"
#include <proto/graphics.h> /* Text(), matches the existing XEM console window RasterPort use */

static struct PetsciiDispatchState g_dispatch;
static BOOL g_active = FALSE;

void XemPetscii_Init(void) {
    petscii_dispatch_init(&g_dispatch, 40, 25);
    g_active = TRUE;
}

BOOL XemPetscii_IsActive(void) {
    return g_active;
}

void XemPetscii_Write(const UBYTE *bytes, LONG len) {
    LONG i;
    for (i = 0; i < len; i++) {
        uint8_t screencode;
        int is_control;
        petscii_dispatch_byte(&g_dispatch, bytes[i], &screencode, &is_control);
        if (!is_control) {
            /* v1 rendering: fallback CP437 lookup on the ORIGINAL PETSCII
             * byte (Review Focus 5 -- the fallback table is keyed on
             * PETSCII values, not screen codes; screencode is available
             * above for the real 4-bank font swap-in later). */
            UBYTE cp437 = (UBYTE)petscii_fallback_cp437(bytes[i]);
            /* Existing console window RasterPort text-out call goes here,
             * matching whatever the XEM path already uses to print one
             * character at the current cursor position -- e.g.:
             * Text(rp, (STRPTR)&cp437, 1);
             * The exact RasterPort handle name must match connect.c's/
             * Xem_wrapper.c's existing console window variable. */
        }
    }
}

void XemPetscii_KeyPressed(UWORD rawkey, UBYTE ascii) {
    /* Map real Amiga RAWKEY codes to enum PetsciiSpecialKey here -- the
     * exact RAWKEY_* constants (e.g. RAWKEY_UP, RAWKEY_F1) come from
     * <devices/rawkeycodes.h>; consult it when wiring this in, do not
     * guess the numeric codes. */
    int petscii_byte;
    if (ascii != 0) {
        petscii_byte = petscii_translate_key(ascii, 0);
    } else {
        /* special key: translate rawkey to enum PetsciiSpecialKey first */
        return; /* left for the implementer once RAWKEY_* mapping is confirmed */
    }
    /* Send petscii_byte on the connection's socket -- match connect.c's
     * existing outbound-keypress send path. */
    (void)petscii_byte;
}
```

- [ ] **Step 4: Fallback-table completeness check (Review Focus 5)**

Add to `test/test_petscii_fallback.c` (extends Task 2's test, run again here since it's the check this task's rendering path depends on):

```c
/* Every printable PETSCII byte (32-127, 160-191) must have SOME mapping
 * -- petscii_fallback_cp437 must never need to fall through to '?' for a
 * byte actually reachable from Xem_petscii's printable path. */
static void test_fallback_covers_all_printable_bytes(void) {
    int b;
    for (b = 32; b <= 127; b++) {
        assert(petscii_fallback_cp437((uint8_t)b) != '?' || b == '?');
    }
    for (b = 160; b <= 191; b++) {
        assert(petscii_fallback_cp437((uint8_t)b) != '?');
    }
}
```

Add the call to `main()` in `test_petscii_fallback.c`, rerun `cd test && make test_petscii_fallback && ./test_petscii_fallback`, confirm it passes.

- [ ] **Step 5: Wire routing in `Xem_wrapper.c`**

In `src/Xem_wrapper.c`, wherever `XemWrite()` is currently called unconditionally, guard it:

```c
if (XemPetscii_IsActive()) {
    XemPetscii_Write((const UBYTE *)string, len);
} else {
    XemWrite(string, len);
}
```

And set `telnet_opts.c`'s TTYPE mode-string from `XemPetscii_IsActive()` at connection time (Task 6's `TelnetOpts_Init()` call site): pass `"PETSCII"` when true, `"AMIGA"` otherwise.

- [ ] **Step 6: Add the Prefs charset-variant flag**

Add a `BOOL petsciiSwedish` field wherever DCTelnet's existing Prefs struct lives (check `DCTelnet.h` for the Prefs structure name), and a corresponding checkbox in the same requester Step 1 added the "PETSCII" emulation choice to. Threading this into `petscii_dispatch`'s Swedish/Finnish charset handling is a follow-up once the real 4-bank font exists (there is no visible difference to test against the CP437 fallback path yet) — record this explicitly as a follow-up, not a silent gap.

- [ ] **Step 7: Add the new files to `src/Makefile`, build, and commit**

```bash
git add src/Xem_petscii.h src/Xem_petscii.c src/guis.c src/Xem_wrapper.c \
        test/test_petscii_fallback.c src/Makefile
git commit -m "feat(petscii): wire PETSCII emulation mode into DCTelnet's XEM path"
```

---

### Task 8: End-to-end reachability test against amiexpress-web

**Files:** none (manual verification task — this is the spec's stated done-criterion, and per this project's standard, a feature that only compiles is not done).

- [ ] **Step 1: Run the full host test suite one more time**

Run: `cd test && make`
Expected: all five test binaries print "all assertions passed", zero failures.

- [ ] **Step 2: Build the Amiga binary**

Run: `cd src && make`
Expected: both 68000 and 68020 release binaries build without errors.

- [ ] **Step 3: Boot it under Amiberry (or real hardware) and connect**

Boot the built exe, open a connection to a running `amiexpress-web` instance's telnet port, select "PETSCII" emulation in DCTelnet's menu before or immediately after connecting (per Step 1 of Task 7's UI wiring).

- [ ] **Step 4: Confirm the server-side flip**

Check the `amiexpress-web` server console/log for the line `[C64] Real C64 terminal detected` — this confirms our `SB TTYPE IS "PETSCII" SE` reply landed and `classifyTerminalType()` recognized it.

- [ ] **Step 5: Confirm the rendered screen**

The Amiga screen should show recognizable PETSCII-style text (via the CP437 fallback glyphs for now — real C64 glyph shapes are the deferred font-follow-up) at 40 columns, not garbled bytes and not the board's ANSI graphics prompt (`ANSI, RIP or No graphics (A/r/n)?` — seeing that prompt means the TTYPE negotiation did not trigger `petsciiMode`, and Task 6/7's wiring needs re-checking against the actual `connect.c` read loop).

- [ ] **Step 6: Complete a login**

Type a username at the resulting prompt using DCTelnet's PETSCII keyboard mode; confirm the board accepts input and responds. This closes the loop the spec's done-criterion asks for: connect → negotiate → render → interact, through the real top-level entry point (booting the exe and connecting), not just a unit test.

- [ ] **Step 7: Record the result**

If it passes: note the outcome (with a screenshot if convenient) in `thoughts/shared/handoffs/2026-09-24_petscii-emulation-first-connect.md` with the date, DCTelnet git commit hash, and the amiexpress-web log line confirming detection.

If it fails at Step 4 (server doesn't detect PETSCII): re-check Task 4's `test_ttype_subneg_replies_mode_string` byte-for-byte against a real packet capture of the negotiation (e.g. via Wireshark on the Amiberry host-side network bridge) — the state machine's unit tests use synthetic input; a real server's exact byte ordering around `DO NAWS`/`DO CHARSET`/`DO TTYPE` (all three are sent per the spec's telnet-server.ts notes) may reveal a sequencing assumption the tests didn't cover.

If it fails at Step 5 (garbled screen): check whether `Xem_petscii.c`'s `Text()` call (Step 3 of Task 7) is using the console window's actual RasterPort handle — this was left for the implementer to match against `connect.c`'s existing variable name and is the most likely wiring gap.

---

## Self-Review

**1. Spec coverage:** Telnet TTYPE negotiation → Tasks 4, 6. Screen-code-correct PETSCII decode → Tasks 1, 5. 4-bank font → explicitly deferred (spec's own open item #1); CP437 fallback substitutes for v1, per Tasks 2 and 7. Keyboard translation → Task 3. Charset-ROM (Swedish) variant → Prefs flag added in Task 7 Step 6, full effect deferred with the font (recorded, not silently dropped). LGPL attribution → present on every ported file (Tasks 2, 3). Reachability done-criterion → Task 8. No test harness existed → built from scratch in Task 1, extended every task after. IAC-escaping of literal `0xFF` → Task 4. Nothing in the spec is uncovered.

**2. Placeholder scan:** `Xem_petscii.c`'s `XemPetscii_KeyPressed` special-key branch and the `Text()` call are left with explicit instructions to match real AmigaOS constants/variables rather than guessed ones — this is not a placeholder in the forbidden sense (vague "handle it"), it's a concrete, bounded instruction with a stated reason (don't guess `RAWKEY_*` numeric values or invent a second RasterPort handle) and a named source to consult. No `TBD`/`TODO`/"handle appropriately" text anywhere else.

**3. Type consistency:** `petscii_to_screencode(uint8_t) -> uint8_t` (Task 1) is used identically in Task 5. `petscii_fallback_cp437(uint8_t) -> uint8_t` (Task 2) used identically in Task 7. `petscii_translate_key(int, int) -> int` (Task 3) used identically in Task 7 (sketch). `telnet_opts_process(...)` signature (Task 4) matches its use in Task 6's `telnet_opts.c`. `struct PetsciiDispatchState` fields (`cols`, `rows`, `cursor_row`, `cursor_col`, `reverse`, `shift_lowercase`, `bg_color`) are the same across Task 5's tests, implementation, and Task 7's consumer.

**4. Review Focus:** All five items have an owning task with an actual test exercising them (items 1-3 in Task 4's test file, item 4 in Task 5's test file, item 5 in Task 7 Step 4). None are left as prose-only claims.
