---
date: 2026-09-27
topic: Per-entry phone book / dialing directory fields in other BBS clients (input for DCTelnet per-BBS settings)
tags: [research, phonebook, syncterm, term, ncomm, per-bbs-settings]
status: final
---

# Per-entry phone book fields in other BBS/terminal clients

Documentary survey. What each client stores PER ENTRY versus GLOBALLY.

## Sources read

| Client | Version | Where |
|---|---|---|
| SyncTERM | sbbs git `531bd654b` (2026-09-22) | local sparse clone `.../aa2b6024-.../scratchpad/reference/sbbs/src/syncterm`, upstream https://gitlab.synchro.net/main/sbbs (src/syncterm). `bbslist.h:142-204` (struct bbslist), `bbslist.c:889-1030` (`read_item`, every `iniGet*` key), `syncterm.h:66` (global `struct syncterm_settings`), `conn.c:63-71` (conn types + default ports) |
| Term (Olaf Barthel) | 4.7 (20.10.96) - the last source on Aminet; 4.8 source not found | https://aminet.net/package/comm/term/term-source (nested `Extras/Source/term-source.lha`), docs https://aminet.net/package/comm/term/term-doc. `Global.h:1098-1539` (settings structs + `struct Configuration`), `Global.h:1963-2016` (PhoneHeader/PhoneEntry), `PhonePanelUI.c:117-566` (entry editor), `term.guide` nodes `Phonebook` (l.4738), `Phonebook_entry_panel` (l.4936), `Copy_panel`, `Trap_panel` (l.5321) |
| NComm | 3.0 doc (3.06 binary adds no doc) | https://aminet.net/package/comm/term/ncomm30 -> `NComm/Doc/NComm.doc` section 8.8 (l.2137-2262), 3.11 (l.909), 8.9 (PB password) |
| NetRunner / MagiTerm | - | NOT CONFIRMED. mysticbbs.com only says NetRunner has "multiple phone book, mTelnet and SyncTerm phone book importers"; no field list found. MagiTerm: no source located on GitHub in the time box (`apamment` repos not listed). Omitted. |

`github.com/obarthel/term` does not exist (checked the GitHub API repo list of `obarthel`).

## 1. SyncTERM - every per-entry key (`bbslist.c` read_item)

Storage: one INI section per entry, section name = entry name (max 30 chars, `LIST_NAME_MAX`).
Two lists exist: the user list and a system-wide list. Keys read with `sys ? NULL : section`
are IGNORED for system-list entries (they fall back to defaults) - marked (U) below:
credentials, paths, logs, TLS secrets and call statistics are user-list-only.

**Connection**
| Key | Meaning | Default |
|---|---|---|
| `Address` | host name / phone number / device | "" |
| `ConnectionType` | RLogin, RLoginReversed, Telnet, Raw, SSH, SSHNA, Modem, Serial, NoRTS, Shell, MBBSGhost, TelnetS, MQTT | SSH |
| `Port` | TCP port | per type: 513/23/22/992/8883 (`conn.c:71`) |
| `AddressFamily` | PerDNS / IPv4 / IPv6 | PerDNS |
| `BPSRate` | emulated line speed (throttle), 0 = full speed | 0 |
| `FlowControl` | RTSCTS / XONXOFF / RTSCTS_XONXOFF / None (serial) | 0 = RTSCTS |
| `DataBits`, `StopBits`, `Parity` | serial framing | 8, 1, None |
| `TelnetBrokenTextmode` | do not negotiate telnet BINARY | false |
| `TelnetDeferNegotiate` | wait for server to start option negotiation | false |
| `SSHFingerprint` | stored host key (SHA-256, legacy SHA-1 upgraded on connect) | none |
| `SFTPPublicKey`, `SSHAllowAES128CBC`, `SSHAcceptEarlyData` | SSH options | false |
| `TLSTrustWebPKI`(U), `TLSTrustedCertificate`(U), `TLSClientCertificate`(U), `TLSClientKey`(U), `TLSPSKIdentity`(U), `TLSPSK`(U), `TLSPSKVersion`(U), `TLSVersionFloor`(U) | TLS | true / "" / TLS1.3 / Unknown |
| `BeDumb` | legacy, read-only: maps to Raw | - |
| (none) `ghost_program` | MBBS GHost program; only filled from a URL (`syncterm.c:1314`), never saved | - |

**Login / credentials**
| `UserName`(U), `Password`(U), `SystemPassword`(U) | used by RLogin/SSH auth and sent by Alt-L (now implemented by the embedded Wren script `scripts/auto/connected/connected.wren`, `term.c:5496`); `UserName` also becomes the SAUCE author when capturing (`term.c:3018`) | "" |

**Terminal / emulation**
| `TerminalType` | string sent as terminal type (telnet TTYPE / SSH / rlogin); "" = derive from emulation | "" |
| `RIP` | Off / RIPv1 / RIPv3 | Off |
| `ANSIMusic` | 0 `ESC[\|` only, 1 BANSI style, 2 all ANSI music (disables DL) | BANSI |
| `ForceLCF` | force last-column-flag wrap behaviour | false |
| `LFExpand` | treat bare LF as CR+LF | false |
| `YellowIsYellow` | brown vs. yellow for colour 6 | false |

**Screen / font / palette**
| `ScreenMode` | Current, 80x25..132x60, C64, C128-40/80, Atari, Atari-XEP80, Custom, EGA/VGA 80x25, Prestel, BBC Mode 7, Atari ST (29 values) - also selects the emulation for C64/Atari/Prestel/BBC | Current |
| `Font` | font name | "Codepage 437 English" |
| `Palette` | comma list of RGB ints, cycled to fill 16 entries | empty = default |
| `NoStatus` | hide status line | false |
| `HidePopups` | suppress popup dialogs | false |

**Transfer / paths**: `DownloadPath`(U), `UploadPath`(U) - default = SyncTERM transfer dir.
No per-entry protocol choice (protocol is picked at transfer time / autodetected ZMODEM).

**Logging**: `LogFile`(U) (""), `AppendLogFile`(U) (true), `TransferLogLevel`(U) (Info),
`ProtocolLogLevel`(U) (legacy key `TelnetLogLevel`, default Error).

**Scripts**: `WrenScripts`(U) - comma list of Wren scripts attached to this entry (`bbslist.c:1016`).

**Misc / bookkeeping**: `Comment` (1024 chars), `Added`(U), `LastConnected`(U), `TotalCalls`(U), `SortOrder`.

**Keyboard**: nothing per entry. No macros/F-key tables in the entry.

**Global only** (`struct syncterm_settings`, `syncterm.h:66`): default `TERM`, modem init/dial
string + device + com rate, scrollback lines, startup video/output mode, custom mode
cols/rows/font height/aspect, window size + scaling, audio output modes, default cursor
type, confirm-close, save prompt, transfer keypress timeouts, UI theme colours, list path,
KDF iterations (list encryption), web list URLs.

## 2. Term 4.x - per-entry configuration override model

Entry = `PhoneHeader` (identity) + `struct Configuration *Config` (a full copy of the
program configuration, each sub-block optional). Editor: every settings group is a
checkbox + "Edit" button; checked = this entry carries its own copy, unchecked = use the
global one. On connect the dialer COPIES all checked groups into the global configuration
(`term.guide` Phonebook_entry_panel). A "Copy config" panel pushes selected global groups
into one/all/selected entries, or marks them "Defaults" (leave global unchanged).
"Use" applies an entry's settings to the global config without dialing.

**Header fields** (`Global.h:1963`): Name[40], Number[100] (multiple numbers separated by
`|`, each can pick its own init/exit/dial-prefix via matching `|` lists), Comment[100],
UserName[100], Password[100] (+ "Hide" switch in the editor), QuickMenu (quick-dial menu,
max 50), NoRates, AutoDial (dial at startup; + global Auto exit), ID, Creation time,
group membership (PhoneGroup), dial-order tag.

**Overridable groups in the entry editor** (`PhonePanelUI.c:205-566`), notable fields
from `Global.h`:
| Group | Notable fields |
|---|---|
| Serial | baud, bits/parity/stop, handshaking, xON/xOFF, duplex, strip bit 8, device + unit, shared, carrier check, OwnDevUnit, break length |
| Modem | init/exit/hangup strings, dial prefix/suffix, PBX prefix, result strings (CONNECT/BUSY/...), redial delay/retries, dial timeout, tone/pulse, connect limit + macro, char send delay |
| Screen | display mode id, width/height/depth, overscan, colour mode, 16-colour palette (12-bit and 96-bit), UI font, public screen / use Workbench, title bar, status line, split status, blinking, pen array |
| Terminal | bell/alert mode, emulation mode, font mode, send CR/LF + receive CR/LF translation, columns/lines + auto size, keymap file, external emulation (XEM) file, beep sound file, text font + IBM font name/height |
| Emulation | cursor/keypad application mode, wrap, insert, newline mode, destructive BS, swap BS/DEL, answerback string, VT100/VT200 identity, locks (font/colour/style/wrap), attribute and pen mapping |
| Clipboard | unit, line/char delay, insert prefix/suffix, line prompt, pacing, LF->CR |
| Capture | log actions, call log, log file name, buffer size/enabled, auto-capture on connect, capture filter, capture path, buffer path, IBM char conversion |
| Command | Startup, Login, Logoff, Upload-done, Download-done macros |
| Misc | priority, backup config, transfer/emulation server, override path, auto-upload, icons, I/O buffer, protective mode, wait string/delay |
| Path | ASCII/text/binary upload + download paths, config storage, editor, help file |
| Transfer | XPR library per direction and type (default/ASCII/text/binary x up/down), internal ASCII up/down, quiet, pacing, signatures for auto-detect, identify command, error notification |
| Translation | per-entry character translation table file (send + receive) |
| Macro keys | per-entry function-key macro file |
| Cursor keys | per-entry cursor key definitions |
| Fast macros | per-entry fast-macro list |
| Rates | call-cost tables (per day / date / time) |

**Global only** (not offered in the entry editor): Speech, Sound, Hotkeys, Traps (auto-login
triggers), area codes, phonebook file. `struct Configuration` carries file-name slots for
these (`Config.c:1763-1788`) but the entry editor has no checkbox for them - per-entry use
NOT CONFIRMED.

**Auto-login**: user name/password are consumed by command sequences `\u` and `\p`, typically
in (global) Traps: `User name:` -> `\u\r`, `Password:` -> `\p\r` (term.guide Trap_panel).
The per-entry Login macro runs after connect (after Startup macro, sequentially since 4.x);
the dial panel has "Skip login macro" (`Dial.c:606`).

## 3. NComm 3.0 - fixed per-entry field set (`NComm.doc` 8.8)

Name (40, 17 shown), Phone # (60, `|`-separated multiple numbers, or FidoNet node via
traplist.library), Comment (40), **Config** (per-entry config file: colours, screen mode,
menus; entry fields win over it), **Macros** (per-entry macro-key file), **Script** (NComm
script run after dial), **ARexx** (ARexx script run after dial), Username, Password
(used as `\u` / `\p` in scripts and macro keys), Baud rate, Data length 7/8, Parity,
Stop bits, Duplex, **Character set** (IBN/IBP/US7/UK7/GE7/FR7/SP7/IT7/DE7/NO7/SF7/ISO/IBM;
IBM/IBN switch to an IBM font), **Swap Del/BS**, **Xfer protocol** (internal list or
"External XPR"), Show in menu, Dial Cmd (prefix 1-4), last-dialled date + call count
(only written when global "Log Phone Info" is on, 3.11). Global: modem strings, "PB
Password" (8.9, password-protects the phonebook), logging to `NComm.log`.

## 4. Comparison (P = per entry, G = global only, - = absent)

| Setting | SyncTERM | Term 4.7 | NComm 3.0 |
|---|---|---|---|
| Name | P | P | P |
| Address / number (multiple) | P (single) | P (`\|` list) | P (`\|` list) |
| Port / connection type | P | - (serial only) | - |
| Comment | P (1024) | P (100) | P (40) |
| Username / password | P | P (+Hide) | P |
| Second (system) password | P | - | - |
| Line settings (baud/bits/parity/stop/flow) | P | P (Serial group) | P |
| Emulated line speed | P (`BPSRate`) | - | - |
| Terminal type string sent to host | P | P (VT100/VT200 id, Emulation) | - |
| Emulation choice | P (via ScreenMode/RIP) | P (Terminal: mode + XEM file) | - (charset only) |
| Emulation quirks (LF expand, wrap, BS/DEL, ANSI music) | P | P | P (Swap Del/BS) |
| Character set / translation | P (`Font` = codepage) | P (Translation table file) | P (Character set) |
| Screen mode / size | P | P | P (via Config file) |
| Font | P | P (text + IBM font) | via Config file |
| Palette | P (16) | P (16) | via Config file |
| Status line | P (`NoStatus`) | P | - |
| Function-key macros | - | P (file) | P (file) |
| Cursor keys / keymap | - | P | - |
| Transfer protocol | - | P (XPR per direction/type) | P |
| Upload / download paths | P (U) | P | G |
| Capture / session log | P (`LogFile`) | P (Capture group) | G |
| Login automation | Alt-L + per-entry Wren scripts | Login/Startup/Logoff macros P; traps G | Script + ARexx P |
| Post-transfer macros | - | P | - |
| Clipboard paste pacing | - | P | - |
| Last called / call count / added | P | P (creation) | P (if enabled) |
| Quick menu / favourites | - | P | P |
| Autodial at startup | - | P | - |
| Groups / sort order | P (`SortOrder`) | P (groups) | - |
| Call-cost rates | - | P | G (locked rate) |
| SSH/TLS keys, fingerprint | P | - | - |
| Hide popups | P | - | - |
| Sound / speech / hotkeys | G | G | - |
| Scrollback size | G | P (Capture: buffer size) | G |
| Phonebook password/encryption | G (KDF) | - | G (PB Password) |

## 5. Consensus set (per-entry in 2+ clients)

1. Name, address/number, free-text comment.
2. Username + password for login automation (all three; Term and NComm expose them as
   `\u` / `\p` substitutions usable in macros/scripts).
3. Line parameters (speed, data bits, parity, stop bits, flow control).
4. Character set / translation (SyncTERM font-codepage, Term translation table, NComm charset).
5. Screen mode/size, font and 16-colour palette (NComm via a per-entry config file).
6. Emulation behaviour switches (BS/DEL swap, LF handling, wrap).
7. Terminal type identity sent to the host (SyncTERM, Term).
8. Function-key macro set (Term, NComm - both as a per-entry macro FILE reference).
9. Transfer protocol (Term, NComm) and upload/download paths (SyncTERM, Term).
10. Per-entry capture/log file (SyncTERM, Term).
11. A per-entry login action run after connect: Wren scripts (SyncTERM), Login macro (Term),
    Script/ARexx file (NComm).
12. Statistics: last connected, call count (SyncTERM, NComm; Term has creation time).
13. Status-line visibility (SyncTERM, Term).

## 6. Notable design points

- **Two override models.** SyncTERM and NComm have a FIXED per-entry field list. Term makes
  every settings group optionally per-entry (checkbox = "this entry overrides"), copying the
  entry's groups over the global config on connect; a "Copy config" panel seeds entries from
  the global set. NComm is a hybrid: fixed fields + an optional per-entry config FILE for
  everything else, with the entry's own fields taking priority.
- **Referencing files instead of embedding.** Term and NComm store macro keys, translation
  tables, cursor keys and config as file references per entry; SyncTERM embeds values in INI.
- **System vs user list** (SyncTERM): a shared read-only list whose entries cannot carry
  credentials, paths, logs or TLS secrets.
- **Secrets**: Term has a "Hide" switch; NComm password-protects the whole phonebook;
  SyncTERM can encrypt the list (KDF setting, global).
- **Multiple numbers per entry** with `|` (Term, NComm), each paired with its own dial prefix.
- **Per-entry character translation tables** exist in Term (file) and NComm (13 national
  7-bit sets + ISO + IBM); SyncTERM expresses the same through the font/codepage choice.
- **Per-entry comments** are universal; SyncTERM allows 1024 chars, the Amiga clients 40-100.
- Nothing in any of the three stores per-entry sound, speech or hotkeys.
