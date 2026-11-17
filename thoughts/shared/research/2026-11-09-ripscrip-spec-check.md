---
date: 2026-11-09
topic: RIPscrip 1.54 spec check of plan 2026-11-08-ripscrip.md (phase RIP-0)
status: partial (agent ran past its tool budget; items marked UNKNOWN were not reached)
---

# RIPscrip spec check (RIP-0)

Sources: RIPscrip 1.54 text (TeleGrafix, 19 July 1993; section names below are the spec's own headings,
line numbers refer to a local text extraction kept in the scratchpad, not in the repo); SyncTERM
`src/syncterm/ripper.c` read online only (no code copied); live capture of connect.serionbbs.com.
Nothing from the spec or from GPL code is copied into the repo.

## Counts

CONFIRMED 14, CORRECTED 4, RESOLVED by RIP-0 closure 5 (one with a conflict, one SyncTERM-only), STILL UNKNOWN 0.

## Per item

| Plan item | Verdict | Value / source |
|---|---|---|
| Line starts with `!` at column 0, commands introduced by `|` | CONFIRMED | spec "General structure" rules 1-4 |
| Text after a command on the same line is ignored; line not starting `!|` is raw text | CONFIRMED | rule 6 |
| `^A` / `^B` replace `!` to start a command mid-line | CONFIRMED | rule 12 (SOH or STX; host-only) |
| `\` at end of line continues; `\!` `\|` `\\` escapes | CONFIRMED | rules 5, 11, 13. Extra: a literal `\\` as last item needs three backslashes when also continuing (rule 13) |
| Level digits then letter; sub-levels `|15L`, up to 9 digits | CONFIRMED + detail | rule 8: first char 1-9 is level, a second digit is sub-level, up to 9 levels. Command char is any printable non-digit (rule 9). Plan only has levels 0/1/9; parser must allow the general form |
| MegaNum base-36 0-9A-Z | CONFIRMED | rule 10 |
| MegaNum widths "1-4 digits" | CORRECTED | Width is fixed per command and per parameter: 2 for coordinates, colours, angles, counts; 1 for flags (`1M` clk, clr); 3 for `1D` flags and `1<ESC>` res; 4 for `=` user pattern and `1B` flags; 5 for `1M` res. A variable-width number is allowed only as the LAST parameter. Parser needs a per-command width table (below), not a generic 1-4 |
| Coordinate space 640x350 EGA 16 colours from 64 | CONFIRMED | `Q` takes 16 x 2-digit values, `a` color:2 value:2 |
| Default EGA palette values | RESOLVED (see closure section) | spec RIP_SET_PALETTE table; one spec-vs-SyncTERM conflict on indices 6/7 |
| Command letters (plan section 1) | CONFIRMED with one fix | all letters in the plan exist. Fix: `1E` is END_TEXT (no arguments), `1T` is BEGIN_TEXT (x0 y0 x1 y1 res:2), `1t` is REGION_TEXT (justify:1 + text). Plan wrote "`1T`/`1t`/`1E` region text": it is a three-command group, `1E` closes it |
| Write mode `W` | CONFIRMED | mode:2, 00 overwrite, 01 XOR |
| Line style `=` | CONFIRMED | style:2 user_pat:4 thick:2; presets 0000 FFFF solid, 3333 dotted, 1E3F centre, 1F1F dashed; 04 custom; thickness 01 or 03 |
| Fill style `S` | CONFIRMED | pattern:2 color:2; 00 background, 01 solid, then 02-0B in order: line, light slash, normal slash, normal backslash, light backslash, light hatch, heavy cross hatch, interleaving line, wide dot, close dot. 0C user-defined via `s` |
| Query answer string | CORRECTED | RIPterm answers `RIPSCRIP015410` (vendor code 1 = RIPterm, sub 0), not `RIPSCRIP015400`. Vendor 0 = generic. Spec "ANSI sequences". Live check: Serion sent RIP to both answers, so a generic `015400` also works there. SyncTERM sends `RIPSCRIP015410` (v1) and `RIPSCRIP030001` (v3); see below. Plan section 2 text must change. Decision for D10-D14: answer `RIPSCRIP015410` |
| `ESC[0!` same as `ESC[!` | CONFIRMED | spec |
| `ESC[1!` RIP off, `ESC[2!` RIP on | CONFIRMED | spec: off means RIP sequences are plain text |
| Mouse limit 128 | CONFIRMED + detail | 128 fields and buttons COMBINED (spec "Miscellaneous notes") |
| Overlap: last defined wins | CONFIRMED | regions scanned most recent first |
| `1M` layout | CONFIRMED | num:2 x0:2 y0:2 x1:2 y1:2 clk:1 clr:1 res:5 text |
| `1U` layout | CONFIRMED | x0 y0 x1 y1 (2 each) hotkey:2 flags:1 res:1 text. Live sample from Serion matches |
| `1B` button style | CONFIRMED | wid:2 hgt:2 orient:2 flags:4 size:2 then more fields; live Serion sample seen |
| Host commands: `^x` control chars and backquote alternative | CONFIRMED | spec "Control characters": `^M` sends CR, backquote is an alternative to caret, `^^` for a literal caret on some hosts, special keys written `^[[A` etc. |
| `$VAR$` list | RESOLVED (see closure section) | 56 headings; closure table |
| Host command templates, pick lists, command blocks | RESOLVED (see closure section) | spec HOST COMMANDS section |
| `.ICN` layout | RESOLVED (see closure section) | NOT in the spec text; layout from SyncTERM loader only, so unverified against real boards |
| Level 9 block transfer `9<ESC>` | CONFIRMED (args only) | mode:1 proto:1 file_type:2 res:4. Protocol list (Xmodem/Ymodem/Zmodem/Kermit) not re-checked |
| `1<ESC>` query | CONFIRMED | mode:1 res:3 text |
| `1D` define | CONFIRMED | flags:3 res:2 text |
| Text window `w` | CONFIRMED | x0 y0 x1 y1 (2 each) wrap:1 size:1 |
| Font behaviour / stroke fonts | RESOLVED (see closure section) | `Y` values and font 0 metrics; stroke font .CHR rendering still RIP-1 work |

Command parameter widths seen (for the parser table): `w` 2,2,2,2,1,1; `v` 2,2,2,2; `g` 2,2; `c` 2; `Q` 16x2;
`a` 2,2; `W` 2; `=` 2,4,2; `S` 2,2; `Y` 2,2,2,2; `L R B` 4x2; `C` 3x2; `O o` 4x2; `A` 6x2; `V I i` 6x2 variants;
`Z` 9x2; `P p l` count:2 then pairs; `F` 3x2; `1M` above; `1U` above; `1I` x:2 y:2 mode:2 clipboard:1 res:2 filename.

## Capture facts (connect.serionbbs.com:23, 2026-11-09)

Synchronet board. At connect it sends DA, DSR and `ESC[!` queries; answers must be in stream order or the board does not
send RIP. After answering, the pre-login screen is 27 RIP lines, one `1U` button ("Continue"), one `1B` button style,
no `1M`, no `1I`, no level 9. Without a RIP answer it sends ANSI (1994 bytes). Both `RIPSCRIP015410` and
`RIPSCRIP015400` produced the same 1507 bytes.

## SyncTERM (read online, ripper.c and neighbours)

- Licence headers: `ripper.c`, `ripper.h`, `rip_icons.h` carry NO licence text. `syncterm.c`, `term.c`, `fonts.c` carry only
  `Copyright (C), 2007 by Stephen Hurd`. The directory `src/syncterm/LICENCE` says GPL "either version 2 of the License, or
  (at your option) any later version"; the comio, conio, smblib, uifc and xpdev libraries are LGPL. Treat RIP files as
  GPL v2+. Do not copy; behaviour facts only.
- Versions: answer table is `RIPSCRIP015410` (RIP v1) and `RIPSCRIP030001` (RIP v3). ESC[! handling is in the connection
  interposer; block mode (`9<ESC>`) case exists.
- Differences from the 1.54 doc: not itemised (agent out of budget). To do in RIP-1: compare palette, font set and the
  text-window handling in ripper.c against the spec.

## Design-changing corrections

1. Answer `RIPSCRIP015410`, not `015400`.
2. MegaNum widths are per-command fixed; parser needs a width table; variable width only for the last parameter.
3. `1E` is END_TEXT; `1T`/`1t`/`1E` is a group.
4. Level syntax is general (`|15L`, up to 9 digits); a 0/1/9 only parser is too narrow.
5. Mouse limit 128 is fields plus buttons combined.

## Fixture ideas (hand-written, own content)

1. Line grammar: `!` start, two commands per line, trailing junk text ignored, `^A` start mid-line, `^B` start.
2. Continuation: `\` split inside a MegaNum pair, and the three-backslash last-position case.
3. Escapes: `\|`, `\!`, `\\` inside a `T` text string.
4. Primitives sheet: one of each `L R B C O A P F` at known pixels, with a checked pixel list.
5. Palette: `Q` all 16, then `a` one entry; read back pixel colours.
6. Fill styles: 12 fill patterns in a grid; pixel checks for solid, line, dots.
7. Buttons: `1B` style, three `1U` with hotkeys; click and hotkey test; 129th region rejected; overlap picks last.
8. Detection: stream with `ESC[!`, `ESC[1!`, `ESC[2!`; assert answer string and that RIP is ignored between 1! and 2!.


## RIP-0 closure (2026-11-09)

Source caveat: the only spec text findable online is textfiles.com/computers/ripscrip.txt, which is **Revision 1.53.00
(12 May 1993)**, not 1.54. `$RIPVER$` there returns RIPSCRIP015300. Everything below is 1.53 text; 1.54 differences are
unknown (the earlier session's 1.54 extraction is not in the repo). SyncTERM read from
github.com/SynchronetBBS/sbbs `src/syncterm/ripper.c` (GPL v2+, facts only, nothing copied).

| Item | Value | Source |
|---|---|---|
| 1. Default 16-colour palette (index -> master EGA 6-bit rgbRGB) | 0:0 1:1 2:2 3:3 4:4 5:5 6:20 7:7 8:56 9:57 10:58 11:59 12:60 13:61 14:62 15:63. Names: Black, Blue, Green, Cyan, Red, Magenta, Brown, Light Gray, Dark Gray, Light Blue, Light Green, Light Cyan, Light Red, Light Magenta, Yellow, White. 6-bit value bits: 0 blue 2/3, 1 green 2/3, 2 red 2/3, 3 blue 1/3, 4 green 1/3, 5 red 1/3 (SyncTERM master table uses levels 0x00/0x55/0xAA/0xFF). `*` (RIP_RESET_WINDOWS) restores this table; `$RESET$` is identical. Colour 0 is always the background. | Spec: RIP_SET_PALETTE, RIP_ONE_PALETTE, RIP_RESET_WINDOWS. SyncTERM: `default_ega_palette`, `ega_palette[64][3]`, reset path in the RESET_WINDOWS handler |
| 1b. CONFLICT on indices 6 and 7 | Spec table lists 06 -> master 7 "Brown" and 07 -> master 20 "Light Gray", but master 7 is gray and master 20 is brown (standard EGA). SyncTERM uses 6 -> 20, 7 -> 7. The spec table is internally inconsistent with its own colour names. **RIP-1 follows SyncTERM / the colour names (6 = brown 0x14, 7 = light gray 0x07)**; verify against a real board scene if one with brown is available. | Spec RIP_SET_PALETTE table vs RIP_COLOR names; SyncTERM `default_ega_palette` |
| 2. `$VAR$` list | See the variable list below. 56 spec headings (SAVE/SAVEx, RESTORE/RESTOREx, APP0-APP9 count once each; plus `$?$`, `$?x$` template inserts). | Spec: HOST COMMANDS > TEXT VARIABLES; SyncTERM not consulted for the list |
| 3. Host command syntax | See the expansion rules below. | Spec: CONTROL CHARACTERS, POP-UP LISTS, HOST COMMAND "TEMPLATES" |
| 4. `.ICN` layout | NOT described in the spec text (RIP_WRITE_ICON / RIP_LOAD_ICON give only the command, modes 0-4 COPY/XOR/OR/AND/NOT, and that `.ICN` is appended when no extension). SyncTERM loader: header = two 16-bit little-endian words, width-1 then height-1 (4 bytes, no further header read); then per scanline, 4 bit-planes each `ceil(width/8)` bytes, MSB = leftmost pixel, rows padded to a byte, top row first. Plane order in the file as SyncTERM reads it: first plane = colour bit 3 (intensity), then bit 2, bit 1, last plane = bit 0 (blue). Colour is an index into the 16-colour RIP palette. This is the Borland BGI `getimage` EGA format (SyncTERM calls it that and applies the 65535-byte BGI imagesize limit: `ceil(w/8)*4*h + 6`). Native Borland getimage normally stores plane 0 = blue first; SyncTERM reads the reverse. **Unverified against a real .ICN file: RIP-1 must test with a known icon and pick the order that matches.** | Spec: RIP_WRITE_ICON, RIP_LOAD_ICON. SyncTERM: icon load function reading `.ICN` (rows via 4 plane reads), `bgi_imagesize_ok` |
| 5a. `Y` font numbers | 0 Default 8x8 (bit-mapped); 1 Triplex; 2 Small; 3 Sans Serif; 4 Gothic (Old English); 5 Script; 6 Simplex; 7 Triplex Script; 8 Complex; 9 European; 10 (0A) Bold. 1-10 are scalable stroke fonts (SyncTERM embeds Borland-style TRIP, LITT, SANS, GOTH, SCRI, SIMP, TSCR, LCOM, EURO, BOLD .CHR data). Argument widths: font:2 direction:2 size:2 res:2. | Spec RIP_FONT_STYLE; SyncTERM font arrays near `rip_fonts[]` |
| 5b. direction / size | Direction 00 horizontal, 01 vertical (text runs bottom to top, rotated 90 degrees; SyncTERM draws font 0 vertical that way). Size 01 normal, 02 = x2 ... 0A = x10 (valid 1-10; SyncTERM indexes metrics by size-1). | Spec RIP_FONT_STYLE; SyncTERM font-0 vertical branch in the glyph drawer |
| 5c. Font 0 cell metrics | Cell = 8*size wide, 8*size tall; advance 8*size per character; glyph pixels are replicated size x size. Spec metric table (top, bow, base, drop) for font 0: size1 0,2,6,7; 2: 0,4,13,15; 3: 0,6,20,23; 4: 0,8,27,31; 5: 0,10,34,39; 6: 0,12,41,47; 7: 0,14,48,55; 8: 0,16,55,63; 9: 0,18,62,71; 10: 0,20,69,79. SyncTERM carries the same table inside `#if 0` marked "from the doc and wrong, we use a different font". Text window sizes (separate from `Y`): 0 8x8, 1 7x8, 2 8x14, 3 7x14, 4 16x14. **Conflict: RIP-1 should use the 8*size cell for layout and the spec metric table only if baseline-relative placement is needed; the spec table values do not match an 8x8 bitmap (size 1 drop 7 is fine, size 2 base 13 is not 2x6=12).** Stroke-font tables for fonts 1-10 are in the spec (same section) and in SyncTERM; copy from the spec when implementing. | Spec: font metrics tables and METRIC struct; RIP_TEXT_WINDOW size table. SyncTERM: `font_metrics[11][10]` (inside disabled block for font 0), bitmap glyph loop for `font.num == 0` |

### Item 2: text variables (56 headings)

Query variables (return a value to the host): `$RIPVER$` (RIPSCRIP + version, 015300 in 1.53; answer 015410 per the earlier decision),
`$DATE$` MM/DD/YY, `$MONTH$` full name, `$MONTHNUM$` 01-12, `$DAY$` 01-31, `$DOY$` 001-366, `$YEAR$` 2 digits,
`$FYEAR$` 4 digits, `$TIME$` HH:MM:SS 24h, `$HOUR$` 01-12, `$MHOUR$` 00-23, `$MIN$`, `$SEC$`, `$AMPM$`, `$DATETIME$`
(Unix style "Sat Dec 19 14:38:50 1993"), `$TIMEZONE$` or NONE, `$DOW$` full day, `$ADOW$` Sun..Sat, `$WDAY$` 0-6 Sunday=0,
`$WOY$` week 00-53 Sunday-first, `$WOYM$` Monday-first, `$X$` `$Y$` `$XY$` mouse position (0000-9999), `$XYM$` position and
buttons, `$M$` button status LMR, `$MSTAT$` YES/NO mouse installed, `$TWIN$` YES/NO text window exists, `$TWFONT$` text font,
`$TWH$` `$TWW$` window height/width, `$TWX0$` `$TWY0$` `$TWX1$` `$TWY1$` window corners, `$CURX$` `$CURY$` text cursor,
`$CURSOR$` cursor on/off status, `$STATBAR$` status bar status.

Side-effect (Active) variables, return nothing: sound `$BEEP$` (1000 Hz 75 ms), `$BLIP$` (barrier), `$MUSIC$` (success jingle),
`$ALARM$` (failure), `$PHASER$`, `$REVPHASER$`; display `$RESET$` (same as `!|*`), `$EGW$` erase graphics window,
`$ETW$` erase text window, `$DTW$` disable text window, `$CON$` `$COFF$` text cursor on/off, `$SBARON$` `$SBAROFF$` status bar;
state save/restore `$SAVEALL$` `$RESTOREALL$` (all attributes), `$SAVE$` `$SAVEx$` `$RESTORE$` `$RESTOREx$` (graphics screen;
`$RESTORE0$`-`$RESTORE9$` delete the file after restore), `$SMF$` `$RMF$` save/restore mouse fields, `$MKILL$` kill mouse
fields, `$STW$` `$RTW$` save/restore text window, `$SCB$` `$RCB$` `$PCB$` save/restore/paste clipboard; modes `$VT102ON$`
`$VT102OFF$`, `$DWAYON$` `$DWAYOFF$` (Doorway), `$HKEYON$` `$HKEYOFF$` (button hotkeys), `$TABON$` `$TABOFF$` (TAB field
select); `$APP0$`-`$APP9$` external application call (recognise and ignore). User variables (`1D` define, `1<ESC>` query) are
a third class, e.g. `$FULL_NAME$`. Terminal that lacks a variable must still recognise it and do nothing (earlier finding).

### Item 3: host command expansion rules

- Control characters: `^X` (caret + letter) or backquote + letter becomes the control byte: `^G` beep, `^L` clear, `^M` CR,
  `^C` break, `^H` backspace, `^[` ESC, `^S` / `^Q` XOFF/XON. Special keys: `^[[A` `^[[B` `^[[C` `^[[D` arrows, `^[[H` home,
  `^[[K` end, `^[[L` control-home. `^^` yields a literal caret on hosts that allow it. Translation is done by the terminal;
  a caret is not sent.
- Escapes inside host strings: `\!`, `\|`, `\\` for literal `!`, `|`, `\`.
- Pick list: `((question::entry,entry,...))`. Question optional (default "Choose one of the following:"); leading `*` in the
  question makes a pick mandatory (ESC otherwise inserts nothing). Entry `text@description` shows description, inserts
  text. `_x_` or `~x~` in a description marks a hotkey (first one active; a missing closing mark highlights the rest). Max 20
  entries; total length 256 bytes (1.52 and earlier) or 1024 bytes (1.53+). Pick lists do not nest.
- Templates: up to 36 (ids 0-9, A-Z). Define with `[n:]text` where `$?$` marks where other button text is inserted. Use with
  `[n]text` (no colon) to feed text into template n. Chain `[0372]HELLO`: result of 0 feeds 3, then 7, then 2; the same
  template may repeat; 0-36 per chain. `[]text` or no prefix = no template. Final buffer 0-4095 characters. `$?x$` embeds
  template x into another template or command; an embedded template inside an embedded one gets no variable, pick-list or
  control-character expansion (verbatim). Undefined embed expands to empty (except radio-group dependencies). Variables,
  lists and control characters expand when each template is processed; they cannot construct another template's list.
- "Command block" (`[`...`]`-style): the only bracket construct in the spec is the template prefix above; no separate
  command-block syntax found in 1.53.
