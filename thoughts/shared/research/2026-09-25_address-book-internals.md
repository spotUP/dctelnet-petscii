---
date: 2026-09-25
topic: Address Book internals (in-memory struct, DCTelnet.Book file format, UI, connect flow)
tags: [address-book, guis.c, file-format, gadtools, issue-10, issue-4]
status: final
---

# Address Book internals

Input for upstream issue #10 "Per-Address-Book entry configuration (fonts, terminal
settings, etc.)" (OPEN). Documents what exists on `feature/petscii-emulation`.
All address-book code lives in `src/guis.c`; nothing else touches the entry struct.

## 1. In-memory entry: `struct BookStruct` (src/guis.c:27-36)

```c
struct BookStruct
{
    char    name[32];
    char    host[52];
    UWORD    port;
    ULONG   lastConnect;
    char    username[42];
    char    password[42];
    char    res[82];
};
```

| offset | field | size | notes |
|---|---|---|---|
| 0 | `name` | 32 | list label; node `ln_Name` points here (guis.c:359) |
| 32 | `host` | 52 | hostname or IP, passed straight to connect |
| 84 | `port` | 2 | UWORD big-endian |
| 86 | `lastConnect` | 4 | ULONG, seconds since the Unix epoch (`mytime()`, utils.c:44-51) |
| 90 | `username` | 42 | |
| 132 | `password` | 42 | plain text |
| 174 | `res` | 82 | reserved; never read or written by code |
| | **total** | **256** | |

- The struct is private to guis.c (not in any header). `ln_Name == &book->name == book`,
  so code casts `ln_Name` back to `struct BookStruct *` (guis.c:268, 448, 482).
- Layout relies on 68k 2-byte alignment of the ULONG at offset 86 (vbcc `+aos68k` and the
  original SAS/C both do this). Commit `895ba12` added `#pragma dontwarn 306` ("padding bytes")
  around the system includes only (guis.c:5-16), not around the struct.
- Verified against the shipped starter book `build/package/DCTelnet/DCTelnet.Book`: 512 bytes =
  2 records; port 0x0539 (1337) at 0x54 and 0x0017 (23) at 0x154; `lastConnect` 0.
- History: original 1.5/1.6 source (`ca00d3e`) had `long last;` in the same slot with an
  Amiga-DateStamp-based `mytime()` that already added 252460800 (Unix epoch). `9e880ee`
  renamed it to `ULONG lastConnect` (2038 fix). Byte layout unchanged since the import.
- Stale bytes after the NUL are kept (e.g. `telehack.com\0*` in the starter file):
  `strlcpy` does not clear the tail of a field.

## 2. On-disk format

- Path: `PROGDIR:DCTelnet.Book` — `bookFilename` (DCTelnet.c:246, extern DCTelnet.h:174).
  Siblings: `PROGDIR:DCTelnet.Prefs`, `PROGDIR:DCTelnet.Keys` (DCTelnet.c:245, 247).
- Binary, raw dump of `struct BookStruct` records back-to-back, 256 bytes each, native
  big-endian. **No header, no magic, no version, no record count.**
- Load: `AddressBook()` guis.c:343-374, run every time the window opens (no caching).
  `Open(MODE_OLDFILE)`, then loop: `AllocMem(256, MEMF_PUBLIC)` (not cleared),
  `FRead(fh, book, 256, 1)`; a return of 0 (EOF, error, or a partial trailing block)
  frees the buffer and stops. Missing file = empty list.
- Tolerance to a changed record size: none. A file shorter than a whole record drops the
  partial last record silently. Records of any other size are mis-framed (every field after
  the first record lands at the wrong offset); nothing detects it. The `res[82]` tail is the
  only space that can gain fields without changing the 256-byte stride: it is read and
  written back byte-for-byte (FRead fills it, FWrite dumps it), and new entries get it zeroed
  via `MEMF_CLEAR` (guis.c:488).
- Save: guis.c:533-549, after the window closes. Only if `save` is TRUE (set by Connect,
  Delete, successful Edit, successful Add — guis.c:452, 470, 480, 497). `Open(MODE_NEWFILE)`
  truncates the file in place (no temp file / rename), then `FWrite(..., 256, 1)` per node in
  current list order; FWrite/Close results are not checked. Sort changes alone do not save
  (`3b3c94d`).
- Order on disk = displayed order at close. `SortABook()` (guis.c:241-306) is a bubble sort
  that swaps the `ln_Name` pointers (i.e. the records) between nodes. Modes are the cycle
  gadget index (labels guis.c:51-57): 0 recent first (default, applied on every load,
  guis.c:377), 1 oldest first, 2 name A-Z, 3 name Z-A (`stricmp`).
- Delete (guis.c:458-473): `ConfirmRequester`, `Remove` + `FreeMem` record and node,
  `lastcode--` (UWORD: deleting index 0 wraps to 65535; `FindNode` then returns NULL).
- Installer: `build/package/DCTelnet/Install_DCTelnet:97-118` installs the starter book only if
  none exists, else as `DCTelnet-New.Book` (`2d0bec7`, `fcb0b7c`).

## 3. UI

### Generated vs hand-written
- `src/abook.gui`, `edit.gui`, `connect.gui`, `fkey.gui` are GadToolsBox V2.0b IFF projects
  (`FORM GXUI`). `abook.h` / `edit.h` keep the GTB-style `GD_*` / `GDX_*` IDs and `*_CNT`.
- The C in guis.c is hand-maintained, not regenerated: GTB's NewGadget/tag tables were
  reduced to `struct MyNewGadget` (guis.h:7-12) + a shared builder `MakeGadgets()`
  (guis.c:98-126) + font scaling `ComputeFont/ComputeX/ComputeY` (guis.c:88-148), and windows
  open with the global `newWin` + `OpenWindow` (OpenWindowTags variants left commented out).
- The .gui files are out of date with the code: abook.gui cycle labels are
  "Name|Name Reverse|Last Connect" (code has 4 modes); edit.gui title "Edit Site Profile"
  (code: "Edit Address Book Profile", renamed in `65a8f0f`); `GDX_DELETE` (abook.h) and
  `GDX_PORT` (edit.h) are missing. None of the .gui files changed since the import (`ca00d3e`).

### List window — `OpenABookWindow()` guis.c:150-213, loop in `AddressBook()` guis.c:383-514
- 420x132 design units, title "DCTelnet: Address Book". Gadgets (guis.c:61-86, IDs abook.h):
  LISTVIEW `GD_LIST` (labels = the `struct List`, patched into `aBookGTags[1]` guis.c:380),
  BUTTON `_Connect` / `_Edit` / `_Add` / `_Delete`, CYCLE "List Sorted By:".
- Selection tracked in `lastcode` (row index); double-click on the same row = Connect
  (guis.c:433-444). Vanilla keys C/E/A/D `goto` the button handlers (guis.c:398-411).
- Modal: called via `WindowSub(AddressBook)` (DCTelnet.c:382-392) from the tool-bar button
  `BUTTON_ADDRESS_BOOK` (DCTelnet.c:1959-1960) and menu `MENU_ADDRESS_BOOK` "Address Book",
  shortcut B (DCTelnet.c:109, 2275-2276). Prototype guis.h:44.

### Edit/New window — `OpenEditProfileWindow()` guis.c:586-638, `EditProfile()` guis.c:649-770
- 450x103 units. Gadgets (guis.c:559-583, IDs edit.h):

| ID | kind | label | field | limit |
|---|---|---|---|---|
| GD_SITE 0 | STRING | _Site Name: | name | MaxChars 31 |
| GD_ADDRESS 1 | STRING | _Address: | host | 51 |
| GD_LAST 2 | TEXT (read-only) | Last Called: | lastConnect via `myctime()` | |
| GD_OK 3 / GD_CANCEL 4 | BUTTON | _Ok / _Cancel | | |
| GD_PORT 5 | INTEGER | _Port: | port | 9 digits, no range check |
| GD_USERNAME 6 | STRING | _Username: | username | 41 |
| GD_PASSWORD 7 | STRING | Pass_word: | password | 41 (not masked) |

- Initial values are poked into the flat tag array by hard-coded index:
  `editProfileGTags[1, 8, 15, 26, 33, 40]` (guis.c:668-675). Adding/reordering a gadget
  shifts those indexes.
- Read-back happens once, after the loop, only on OK (guis.c:743-759): `StringInfo->Buffer`
  strlcpy'd into each field; `StringInfo->LongInt` truncated into UWORD `port`. This is the
  fix for issue #4 (`2a50c53`); before, values were copied only on each gadget's GADGETUP,
  so fields left with the mouse were lost.
- Keys: O=Ok, C=Cancel, S/A/P/U/W focus a gadget (guis.c:704-723). Close gadget = cancel.
- Add (guis.c:484-503): `MEMF_CLEAR` record, defaults name "*new site*", host
  "*ip/host here*", port 23; appended to the list only if EditProfile returns TRUE.

## 4. Connect flow from the Address Book

1. Connect button / key C / double-click sets `conbook = (struct BookStruct *)ln_Name`,
   `ret = save = TRUE`, closes the loop (guis.c:445-455).
2. After the window is closed (guis.c:516-531): `tcpPort = conbook->port;`
   `BeginServerConnection(conbook->host, conbook->port)`.
3. On `RETURN_OK`: `conbook->lastConnect = mytime()`, and `username` / `password` are copied
   into the globals `char username[42], password[42]` (DCTelnet.c:211). They are used only by
   the menu items `MENU_SEND_USERNAME` / `MENU_SEND_PASSWORD` (DCTelnet.c:2497-2505); there is
   no automatic login.
4. The book is then saved (always, because Connect sets `save`), and all records are freed.
   Nothing from the entry survives except `server` (copied in EstablishTCPConnection),
   `tcpPort`, `username`, `password`.
- `BeginServerConnection()` DCTelnet.c:2609-2630 spawns `HandleConnectingWindowTask`
  (connect.c:136, "Connecting" window from connect.gui) and calls
  `EstablishTCPConnection()` DCTelnet.c:2643-2773: opens bsdsocket, `DisConnect`, copies the
  host into global `server`, `gethostbyname`, `connect`, then telnet/zmodem context reset
  (non-legacy path), `conectionTime = mytime()`, `petscii_dispatch_init(&g_petsciiState,40,25)`
  and the PETSCII font warning if `prefs.flags & FLAG_PETSCII_MODE` (DCTelnet.c:2755-2766).
- All terminal behaviour is global: `struct PrefsStruct prefs` (DCTelnet.h:8-35; `fontname[32]`,
  `fontsize`, `flags`, colours, display ID) and `FLAG_*` bits DCTelnet.h:42-57 (e.g.
  `FLAG_CRLF_CORRECTION` bit 1, `FLAG_BS_DEL_SWAP` 4, `FLAG_STRIP_COLOUR` 6,
  `FLAG_RETURN_CRLF` 11, `FLAG_LOCAL_ECHO` 12, `FLAG_RAW_CONNECTION` 13, `FLAG_PETSCII_MODE` 15).
  No entry field feeds `prefs`.
- Other connect entry points that bypass the book: `OnConnectClicked()` host:port requester
  (DCTelnet.c:1683-1737), command-line `argv` (DCTelnet.c:1299, 1415), finger (DCTelnet.c:1137).

## 5. History

- `git log --oneline -- src/guis.c`: 31 commits, all upstream refactors on top of the import
  `ca00d3e` (DCTelnet 1.5/1.6 from Aminet `dct16_src`). None changed the record layout.
- Address-book-relevant: `2a50c53` (issue #4 fix, EditProfile read-back on OK), `9e880ee`
  (`long last` -> `ULONG lastConnect`, myctime), `3b3c94d` (recent-first default sort, sort no
  longer triggers a save), `242fcc5` (removed impossible `lastcode < 0` checks),
  `1ec155d` / `4b585c9` (starter book 768 -> 512 bytes = 3 -> 2 records), `2d0bec7` /
  `fcb0b7c` (installer preserves an existing book).
- Issue #4 "Address Book may not be saved correctly": CLOSED 2026-01-31 (mouse-validated
  fields lost). Commit `2a50c53` references it (author date 2026-02-22).
- Issue #10: OPEN, from EAB user n3m3; asks for per-entry fonts, terminal type, CRLF correction.

## Constraints for issue #10 (facts, not design)

- Record stride 256 is implicit and unversioned; existing books and the starter book use it.
- 82 reserved bytes at offset 174 are round-tripped untouched and zero in new entries.
- Edit window init uses hard-coded tag-array indexes; window size is fixed design units.
- Entry data is discarded after connect; per-entry settings would have to be applied between
  `BeginServerConnection` and the free loop, or copied into globals, since `prefs` is the only
  runtime source of terminal settings.
