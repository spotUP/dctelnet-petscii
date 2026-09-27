# ibmcon.device 1.4 -- annotated disassembly

Source: reassembled dump of `ibmcon.device` 1.4 (Mar 9 1998) from
https://www.computerdevices.it/vintage_amiga_page/download/ibmcon.device.asm

| File | Content |
|------|---------|
| `ibmcon.device.original.asm` | Untouched In_Go Reassembler dump (auto labels, no comments) |
| `ibmcon.device.readable.asm` | Restructured version: reconstructed label names, struct/offset EQUs, full commentary, and 13 bug fixes (each marked `FIXED:` in the code, list in the file header) |

## What it is

An Exec device (SAS/C-compiled C code) that emulates an IBM-PC style
ANSI.SYS text console inside an Intuition window.

* `OpenDevice("ibmcon.device", unit, ioreq, 0)` with `io_Data` = an open
  `struct Window *`.
* Only `CMD_WRITE` is supported (`io_Length = -1` means NUL-terminated).
* The write stream is interpreted: CSI sequences (`ESC[` / 8-bit `$9B`),
  IBM control characters, SO/SI switching into the high-bit half of an
  IBM font (CP437 graphics).
* Rendering goes straight into the window RastPort via graphics.library.
* Each opener gets a private clone of the device base and a private
  handler process (`IBMCON_Handler`); `CloseDevice` kills it with the
  private command `$7FF0`.
* Unit number: 0 = Amiga pen order; >=1 swaps pens 1/7 (IBM colour
  order).

Full architecture description, struct layouts, the SAS/C autoinit
`$FC`-byte relocation trick, and the list of original bugs (fixed in
the readable version) are in the header of `ibmcon.device.readable.asm`.

Fixed bugs, in short: CSI `S` scroll direction; SetSoftStyle italic
enable mask; erasing now uses the SGR background pen instead of pen 0;
text placed on the font's real baseline; UnitOpen failure handling +
port leak; DevOpen sets `io_Error`/`io_Device` properly; CreatePort
signal-allocation check sign-extended; column clamp + text-run flush
against 200-byte buffer overflows; CSI parameters saturate at 255
instead of wrapping; unit >=2 two-line bottom-scroll special case
removed; top-margin scroll no longer one line short; CSI `J` mode 1
no longer erases part of the line twice; CSI `r` is now a real
DECSTBM vertical scroll region (validated top;bottom, cursor homed,
bounds the margin scrolling and therefore LF and wrapping; IL/DL,
SU/SD and erases deliberately stay whole-window, ANSI.SYS style).

## Verification

`ibmcon.device.readable.asm` assembles with vasm straight to an
installable device binary (`ibmcon.device` in this directory, copy to
`DEVS:`):

    vasmm68k_mot -Fhunkexe -kick1hunks -nosym -o ibmcon.device ibmcon.device.readable.asm

(`-kick1hunks` keeps the relocations as classic HUNK_RELOC32 like the
original SAS/C binary instead of the V37+ RELOC32SHORT form; `-nosym`
drops the symbol table hunks.)

Data sections assemble byte-exact to the original layout (Segment3 =
$140 bytes, all magic offsets preserved). The code section matches the
original except for the marked bug fixes.

No runtime test environment exists here (would need an Amiga emulator
with a client that opens the device); verification of the fixes is
assemble-clean plus a manual register/stack audit of each changed
routine.

## 1.5 (2026-09-27, for DCTelnet's RTG/256-colour work)

* **Depth 5 and deeper get colours.** The depth switch handled 1-4 only;
  256-colour AGA and RTG screens fell into the monochrome case.
* **Pen table (`IBMCMD_SETPENS`, io_Command `$7FE0`).** A client sends
  io_Data -> `UBYTE pens[16]` in ANSI order. Colour codes then stay logical
  0-15 and are mapped at every SetAPen/SetBPen/SetRast; the unit-1 pen 1<->7
  swap and the depth check of background pens no longer apply; clears use
  the mapped ANSI background instead of pen 0. Not reachable from the
  written byte stream, so a BBS cannot change it. Without the command the
  device behaves as 1.4 (plus the depth fix).
* **Cursor with a pen table:** COMPLEMENT with write mask fg^bg, so the
  cell swaps foreground and background exactly wherever the pens sit.
* Background-pen check: depth 32 no longer rejects every pen (bset is
  modulo 32).
* `make check` proves the code is plain 68000 despite the MC68020 directive.
