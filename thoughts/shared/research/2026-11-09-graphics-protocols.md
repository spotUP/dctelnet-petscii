---
date: 2026-11-09
topic: graphics protocols for UP-Term (iTerm2 inline images, gap table, DEC graphics usage)
status: research
---

# Graphics protocols: iTerm2 images, UP-Term gap table, DEC usage

Paths below are under /Users/spot/Code/UP-Term/vtcon unless noted.

## 1. iTerm2 inline images (OSC 1337)

Source: https://iterm2.com/documentation-images.html

- Single form: `ESC ] 1337 ; File = [k=v;k=v...] : <base64> BEL` (ST `ESC \` also accepted).
- Multipart form (iTerm2 3.5+, made for tmux): `ESC ] 1337 ; MultipartFile = [args] BEL`, then N x `ESC ] 1337 ; FilePart = <base64 chunk> BEL`, then `ESC ] 1337 ; FileEnd BEL`. Each sequence is capped at 1,048,576 bytes (old tmux: 256).
- Args: `name` (base64 filename), `size` (bytes, only for the progress bar), `width`/`height` (`N` cells, `Npx`, `N%` of session width, `auto`), `preserveAspectRatio` (0 = stretch, default 1), `inline` (1 = show, else download to Downloads folder), `doNotMoveCursor` (exists in implementations, not on the doc page).
- Formats: "any image format macOS supports" (PNG, JPEG, GIF incl. animated, PDF, PICT). Non-image files are only transferred. Other terminals decode PNG/JPEG/GIF in practice.
- Cursor after image: the doc page does not say. In iTerm2 and WezTerm the cursor ends up below/right of the image (image occupies cells, cursor moves past it) unless `doNotMoveCursor=1`. Unverified against iTerm2 source; WezTerm doc: https://wezterm.org/imgcat.html
- Detection: no DA1 bit. iTerm2 doc recommends its Feature Reporting protocol (OSC 1337 ; Capabilities / ReportCellSize, see KDE request https://discuss.kde.org/t/feature-request-implement-iterm2-terminal-feature-reporting-osc-1337-capabilities/41615). Hosts in practice sniff TERM_PROGRAM (iTerm.app, WezTerm), LC_TERMINAL, or XTVERSION. A BBS host has none of these; it would have to be sent unconditionally or by user setting.
- Implementers: iTerm2, WezTerm (https://wezterm.org/imgcat.html), mintty (https://terminaltrove.com/compare/terminals/mintty-vs-wezterm/), Tabby, VS Code. Konsole: sources conflict. Windows Terminal: no IIP found (sixel only).
- BBS use: none found in web search (searched BBS + iTerm2/sixel). SyncTERM and BBS software use RIP, sixel, own APC/CTerm extensions, not OSC 1337. Treat as zero BBS traffic; value is for modern CLI tools (imgcat, yazi, chafa, httpie plugins) run over telnet/ssh.

## 2. Reuse estimate

- OSC dispatch: OSC strings are collected in `str`; long payloads stream past it via the osc52 mechanism (engine/vtengine.c:5826-5830, OSC 52/7/8 `osc52_begin`, `long_cmd`). `1337;File=` needs the same hook: keep header in `str`, stream the base64 after `:`.
- base64: kitty has its own streaming decoder `kit_b64` (engine/kitty.c:390) and the engine a second one for OSC 52 (engine/vtengine.c:250 `b64_acc`). Note kit_b64 accepts URL-safe chars; fine for iTerm2.
- Decode: PNG via `png_decode` (engine/png.c:94, called at engine/kitty.c:522); fallback to host hook `t->decode(raw, rlen, &argb, &w, &h)` (engine/kitty.c:530), implemented in handler/vtcon_handler.c:549-760 with Amiga picture.datatype V43 (reads PNG, and JPEG/GIF if the datatypes are installed on the Amiga). Engine has NO JPEG/GIF decoder (grep "jpeg|jpg|gif" in engine/*.c and *.h: no hits). JPEG/GIF therefore work only through the datatype hook, i.e. depend on the Amiga's installed datatypes. Animated GIF would show the first frame via datatypes (unverified).
- Placement: `kit_src`/`kit_picture`/`kit_make` (kitty.c:114-250), quantise to palette, `image_install`/`image_place` (vtengine.c, as used by sixel_end at ~5498-5530). `kit_show` (kitty.c:659) does placement with cell sizes.
- New code needed: the OSC 1337 header parser (width/height unit conversion to cells from cell pixel size, aspect ratio, inline flag), MultipartFile state, cursor advance rule, `File=` with inline=0 ignored/dropped, and reply side (feature report) if wanted.
- Estimate: roughly 60-70% reuse (decode, quantise, store, place, free, handler hook); new ~150-250 lines of C in a new engine file (e.g. iterm.c) plus 2-3 lines in the OSC dispatcher. Small task.

## 3. Gap table (UP-Term today)

Structures read: the parser state enum (vtengine.c:51-54: S_CSI_IGNORE, S_OSC, S_STRING, S_SIXEL), the string introducer dispatch (vtengine.c:5826-5840: OSC 7/8/52, APC G -> kitty_begin, DCS q -> sixel_begin), and the charset table. Greps over engine/ for "regis|tektronix|4014|drcs|decdld|ripscrip|1337" found nothing relevant (one hit was a false positive). Note: the first `grep -E` with "?38" errored and was redone without it; the final grep covered the other terms.

| Protocol | Parser present | Rendered on screen | Announced to host | Evidence |
|---|---|---|---|---|
| kitty | engine/kitty.c (kitty_begin at vtengine.c:5833, kitty_byte 5772) | Yes: image store, handler decode vtcon_handler.c:549+, PROF sentinels at :8411, :8536 | Replies to APC G queries (kit_reply kitty.c:437); no DA bit exists for it | works, per owner and rig sentinels |
| sixel | vtengine.c:4996 vt_sixel, sixel_begin 5273, sixel_end 5498, DECSDM 3206, ?1070 3209 | Yes, same image store via image_install/image_place (5498-5530) | DA1 `\033[?62;4;22c` at vtengine.c:3879 (4 = sixel) | works through shared image path (handler has no sixel-specific code) |
| iTerm2 OSC 1337 | none (osc_dispatch has OSC 9 notify only, vtengine.c:4736 is iTerm2's notification, not images) | none | none | grep "1337" in engine/ no hit |
| RIPscrip | none | none | none | no hits in engine/, thoughts/, src/ (UP-Term has no src/ dir with code; checked thoughts and vtcon) |
| ReGIS (DCS p) | none: only DCS q (sixel) is routed (vtengine.c:5836); other DCS go to S_STRING and are discarded | none | no (DA1 lacks ReGIS bit 3) | dispatch switch read |
| DECDLD/DRCS (DCS { ) | none (discarded as S_STRING) | none | no | same |
| Tektronix 4014 | none (no ESC [?38h mode, no ESC FF) | none | no | grep + dispatch read |
| DEC Special Graphics | yes: charset[] '0' table dec_graphics vtengine.c:952, charset state :144, :207 | yes (text glyph mapping) | n/a | present since core |

RIPscrip work/plan in UP-Term: none found (grep "rip", "ripscrip" in UP-Term thoughts/ and vtcon; one unrelated "regis" hit is "registers" in vtengine.c, RIPscrip not mentioned).

## 4. How much real traffic uses the DEC graphics

- ReGIS: effectively none on BBSes; only DEC VT24x/VT3xx hardware, Reflection/Rumba-class commercial emulators and xterm (`-ti vt340`) use it. Sources: https://vt100.net/shuford/terminal/regis_emulators_news.txt, https://vt100.net/shuford/terminal. SyncTERM's feature list (https://syncterm.net/) does not include ReGIS.
- DRCS / DECDLD: no BBS software found emitting it. Closest BBS analogue is SyncTERM's own font-switch/upload extension (https://sourceforge.net/projects/syncterm/), not DRCS. Used by VT220+ apps and mlterm/xterm; essentially unused today.
- Tektronix 4014: no BBS traffic; legacy scientific hosts and xterm's Tek mode (`ESC [?38h`). Not in SyncTERM's list.
- For comparison, what BBS terminals do support: SyncTERM lists RIP v1.54, sixel, JPEG XL/PPM, Prestel, Atari VT52 (https://syncterm.net/), and a BBS mod serves sixel to SyncTERM. RIPscrip and sixel are the BBS-relevant ones; "DEC graphics" most likely means sixel (done) or DEC Special Graphics line drawing (done).
- Caveat: negative claims come from web searches that found no evidence, not from a survey of BBS software.
