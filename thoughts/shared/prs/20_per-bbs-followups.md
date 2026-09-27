**Stacked on #19.** Follow-ups to the per-BBS settings (#10), found in testing.

## Summary

- **Screen mode and Use Workbench are global.** An entry saved in one mode moved DCTelnet between the Workbench and its own screen, or changed the screen mode, on every connect. The Screen group now holds the font and palette only; values stored in older entry files are ignored.
- **A setting changed while connected** lasts for the rest of the run (it is no longer reverted at disconnect) but is never written to DCTelnet.Prefs; changed again while not connected, it is saved.
- **Address Book**: deleting the first entry left no selection (UWORD underflow) and changed the list while attached to the listview; typing in a field entered with Tab now replaces its value (string edit hook); the edit window's frame encloses every field.
- **Finger**: during an entry session the query waits for the display reopen (its answer was wiped), and the reply's bare-LF lines no longer come out as a staircase (newline mode for the reply only).

## Testing

- Host unit tests: `cd test && make` (group members, hand changes and what is saved, list selection after delete, first-key replace).
- Tested on an emulated A1200 and by the reporter on FS-UAE.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
