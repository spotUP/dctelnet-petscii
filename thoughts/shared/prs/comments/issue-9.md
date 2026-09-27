Fixed in #22: ibmcon's Insert Lines used `rows-1` -- a row count -- as the bottom **pixel** row of its scroll (Delete Lines correctly uses `rows*YSize-1`), so only the top pixel lines moved. Verified against a local server on an emulated A1200; could @Mariushl or you check the ABBS/MBBS editor on retro.mbbs.no with the stack's top branch? Test steps in #11.

🤖 Generated with [Claude Code](https://claude.com/claude-code)
