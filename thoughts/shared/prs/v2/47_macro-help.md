---
date: 2026-09-28
topic: "PR #47: Login macro help: two lines under the field, with \\w"
tags: [dctelnet, pr, v2.0-dev, gui]
status: final
---

# #47 Login macro help

Head `feature/pr-macro-help` (8ab9e2e) on spotUP/dctelnet, base v2.0-dev, stacked on #44.

**Based on v2.0-dev, stacked on #44.** Only the last commit belongs here.

## Summary

The help under an Address Book entry's **Login Macro** field was one line starting at the window's left edge, wider than the field column. It also did not list `\w"text"` (wait until the BBS sends a text), which #34 added.

It is now two lines, right-aligned to the field's right edge:

```
      \u User name   \p Password   \r Return
      \d Wait 1 s   \w"text" Wait for the text
```

- Right alignment uses `GTTX_Justification`/`GTJ_RIGHT` (GadTools V39); on OS 2.x the lines are left-aligned in the same column.
- The Settings rows and the Ok/Cancel buttons move down one line, and the window grows to fit.

## Testing

- Builds with vbcc for 68000 and 68020, warnings as errors; `cd tests && make` passes.
- Emulated A1200 (FS-UAE, driven through amiagent), on the RTG Workbench: Address Book > Edit shows both lines right-aligned under the field and inside the bevel box, with the Settings rows and buttons below them.

Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
