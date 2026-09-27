**Stacked on #16.** Only the commits after it belong here.

## Summary

DCTelnet opened at most 16 colours on its own screen, and the ANSI colours took over the pens the system UI uses. This PR adds **256-colour AGA and RTG (Picasso96/CyberGraphX) screens** with the ANSI colours on pens of their own:

- The screen mode requester allows up to 32-bit depths.
- On 32+ colours the UI keeps Intuition's pens and colours; the 16 ANSI colours get pens 8-15 and 20-27 (the AGA pointer pens 16-19 are left alone) and a 24-bit palette in ANSI order in the settings (appended to the prefs file; older files still load).
- On the **Workbench**, the ANSI colours take 16 shared pens of exactly their colours (`ObtainBestPen`, released on close), so BBS colours are right there too. A colour that gets no pen uses the nearest one on the screen, and only obtained pens are released.
- Exposed window areas and the screen border are terminal black, not the UI's grey pen 0.
- The start-up screen says which ibmcon runs and how it draws colours.

## ibmcon.device

This needs `ibmcon.device` 1.5 or later (`IBMCMD_SETPENS`). ibmcon was freeware shipped as a 1998 binary only; this PR adds its **reassembled and annotated source** in `ibmcon/` (with the untouched reassembler dump for provenance, a Makefile and a README with the version history 1.4 bug-fixed to 1.6). `make` in `ibmcon/` builds the device; the release ships it in DCTelnet's `Devs` drawer.

## Testing

- Host unit tests (palette conversion, pen layout): `cd test && make`.
- `tools/rtgprobe` measures depth queries and COMPLEMENT on real screen modes (used to find that an RTG bitmap's `bm_Depth` is not its depth).
- Tested on an emulated A1200: AGA 256 colours, Picasso96 8/16/24-bit, 16-colour and the Workbench.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
