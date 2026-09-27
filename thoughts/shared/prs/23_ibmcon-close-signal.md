Fixes #3.

**Stacked on #22.** Only the last commit belongs here.

## Summary

You traced #3 to ibmcon freeing signal bit 31 on close; with the source in `ibmcon/` the cause is visible:

- ibmcon's handler process creates its command port with `CreatePort()`, so the port's signal is allocated **in the handler's task** -- a new process's first `AllocSignal(-1)` returns 31.
- `UnitClose` (run by `CloseDevice`, i.e. **in the opener's task**) deleted that port with `DeletePort()`, whose `FreeSignal(mp_SigBit)` freed the **opener's** bit 31 -- bsdsocket.library's, when it holds it. `WaitSelect()` then stopped reporting data after a display change.

ibmcon **1.8**: the handler deletes its own port before it replies to `CMD_DIE`; `UnitClose` no longer touches it.

DCTelnet's `AllocSignal(31)` reservation stays, with an updated comment: it only matters when an ibmcon older than 1.8 is still in memory and in use elsewhere (DCTelnet opens the newest copy it finds, and 1.8 ships in its `Devs` drawer).

## Testing

On an emulated A1200, a test program holds bit 31, opens ibmcon on a window and closes it:

```
ibmcon 1.7: bit 31 before close: allocated, after close: FREED
ibmcon 1.8: bit 31 before close: allocated, after close: allocated
```

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
