## What

Make doors draw correctly for byte-stream terminal callers (telnet/ssh clients such as DCTelnet on the Amiga), and keep LiveChat inside its panels for them.

1. **Fold 256-colour and 24-bit SGR to the 16 ANSI colours** for callers not known to handle them (`session.unicodeCapable !== true`). Door theme colours showed as white/grey in DCTelnet. `web/backend/src/utils/true-colour-fold.util.ts`, applied in `connection-emitter.ts`; reuses the SDK's `xterm256ToRgb`.
2. **Never write a control character into a blessed cell.** Element/list/log text is split on CR, LF and CRLF; the screen cell writer maps C0/DEL to a space. A bare CR in a chat line moved the real cursor to column 1 mid-row and broke the LiveChat layout.
3. **Draw terminal rows in fewer bytes.** The blessed screen coalesces changed runs in a row instead of one CUP per cell (about 29 KB / 606 CUPs per LiveChat session in the capture).
4. **No board-wide door effects for byte callers** (`boardEffectsAllowed()` in `web/backend/src/doors/door-effects-context.ts`) unless the door is a game.

Main is merged in (inline LiveChat pictures, a1fe835ba).

## Tests

- `sdk/tests/blessed-control-chars-in-cells.test.ts`
- `sdk/tests/blessed-render-coalesce-row.test.ts`
- `web/backend/tests/doors/door-effects-context.test.ts`
- `web/backend/tests/transport/door-colour-fold-16.test.ts`
- Telnet capture of LiveChat against a local board: 0 bare CRs, chat stays in its panel, sidebar intact, status bar at column 1.
- Kitty picture path: `web/backend/tests/transport/kitty-graphics-entry.test.ts` passes; the picture APC goes out after ESC7+CUP on the same emitter, outside the blessed cells.

## Known

- Full sdk jest had 4 failures in this worktree (theme-esm-runtime, one-spelling-for-one-kind-of-door, esm-conditions-resolve, petscii corpus); not confirmed whether they fail on main too.
- Hex scraps under shared pictures come from the Discord bridge appending CDN links; fixed separately in doorserver cdad0d1.

## Manual test after deploy

From DCTelnet on the live board: open LiveChat, check door colours, chat stays in panel, a picture shared from Discord shows inline under its `[PICTURE] #N` line.
