**Stacked on #15**, which is stacked on #14. Only the commits after it belong here.

## Summary

With PETSCII Mode on, DCTelnet used the C64 fonts all the time, so its own texts (the start-up information, "Looking up ...", "Connection closed") were unreadable C64 graphics. Now the C64 display is used **only during a PETSCII session**: the normal font before connecting and after disconnecting.

- The switch reopens only the console in the same window (ibmcon reads the RastPort font live); the screen and window stay.
- DCTelnet's own messages printed while the C64 display is up are translated so they read correctly in the C64 upper/lower case sets.
- The "Connection closed" notice is printed again after the reopen that follows a disconnect.
- F1 in PETSCII Mode sent F2's code (off by one); fixed, and function-key macros win over the C64 F-codes.
- The console's I/O request is checked before use (it was dereferenced unchecked; the console is now also reopened at run time).

## Testing

- Host unit tests: `cd test && make`.
- Tested on an emulated A1200 and on real hardware against a PETSCII board (amiexpress-web).

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
