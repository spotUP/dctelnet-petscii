Fixes #9.

**Stacked on #21** (it needs the `ibmcon/` source added in #17). Only the last commit belongs here.

## Summary

@Mariushl's fullscreen-editor glitch on ABBS/MBBS, traced to `CSI L` (Insert Lines) in the issue, is in `ibmcon.device`: its Insert Lines used `rows-1` as the **bottom pixel row** of its scroll -- a row count where Delete Lines (`CSI M`) correctly uses `rows*YSize-1`. Only the top pixel lines moved, so an editor scrolling up left stale lines. Now `rows*YSize-1`. And `N` was clamped to the rows below the cursor, so an insert on the last row did nothing; the cursor's row counts too, as in `CSI M`.

ibmcon is now **1.7** (lib_Revision 7).

## Testing

On an emulated A1200 with a local server sending 25 numbered lines, then `ESC[5;1H ESC[3L`: rows 5-7 are blank and line 05 is on row 8 (before the fix nothing moved).

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
