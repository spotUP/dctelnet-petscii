**Stacked on #18.** Only the commits after it belong here.

## Summary

- **Newest copy of ibmcon and the libraries.** DCTelnet ships `Devs/ibmcon.device` and `Libs/` (reqtools, XEM, XPR) in its own drawer but only opened `DEVS:`/`LIBS:`, so an older ibmcon installed there was used instead. `OpenNewestDevice()`/`OpenNewestLibrary()` compare the `$VER` of both copies and open the newer (DCTelnet's on a tie); an older copy already in memory is expunged first when nothing uses it. Fonts are tried from DCTelnet's `Fonts` drawer first.
- **Start-up screen**: one labelled line per fact (Display engine, Screen, Text area), none wider than 80 columns; it says where ibmcon came from.
- **Font fallback**: a font that cannot be opened falls back per pixel shape and the start-up screen names it; Settings > Screen Font switches in place on the Workbench.
- **Title bar**: windows and the title-bar LEDs are placed by the title bar the screen actually draws, not by the font size setting (with Topaz Pro 16 the first rows went under the bar).

## Testing

- Host unit tests (`ProgDir_Path`, `$VER` parsing and comparison): `cd test && make`.
- Tested on an emulated A1200, including an older ibmcon left in memory by another program: DCTelnet expunged it and opened its own newer copy.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
