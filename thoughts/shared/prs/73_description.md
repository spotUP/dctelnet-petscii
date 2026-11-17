Stacks on #72 (feature/pr-ssh-terminal); the two new commits are the last two on the branch.

## Problem

PETSCII Mode was a separate on/off setting (State bit 15, a menu item and a checkbox in the site settings) beside Terminal Type (Automatic / BBS / Unix). The two could disagree: PETSCII Mode on with Terminal Type Unix made no sense, and every place that named the terminal (SSH TERM, telnet TTYPE, rlogin, the site summary) had its own PETSCII special case.

## Change

- Terminal Type gets a fourth value, PETSCII (`TERMTYPE_PETSCII`, 3). The Terminal > Terminal Type submenu and the site settings cycle gadget both list it. The "PETSCII Mode" menu item and checkbox are gone.
- `Prefs_IsPetscii()` is the only way the code asks whether a session is PETSCII.
- State bit 15 is retired. A prefs file or Address Book entry that still has it set loads as Terminal Type PETSCII with the bit cleared (`Prefs_MigratePetscii`). Saving never writes the bit. The load clamp now accepts type 3.
- PETSCII uses the BBS personality. `Vt_SshTermName` returns "PETSCII", and `Prefs_TelnetTermName` gives the telnet TTYPE, rlogin and site summary name from one place.
- A tests/Makefile source check (`check-petscii-termtype`) fails if the old flag, menu item or checkbox comes back outside the migration code.
- The guide, the missing-font notice ("PETSCII:") and code comments call it Terminal Type PETSCII. The change log and credits keep the old name.

Compatibility: an older DCTelnet reading prefs saved by this one sees Terminal Type 3 and clamps it to Automatic.

## Tested

- Host tests pass (`make -C tests`). New tests: prefs migration (bit 15 loads as PETSCII, TermType 3 survives a load, the bit is not saved), site prefs (Address Book migration and summary text), vtpersonality (PETSCII acts as BBS; SSH and telnet names). They do not compile on #72 without the change.
- Real A1200 (68060, AmigaOS 3.2): prefs saved by the previous build with PETSCII Mode on load as Terminal Type PETSCII (menu check and cycle gadget). The cycle gadget lists Automatic, BBS, Unix and PETSCII, and "PETSCII" fits its 96 pixels. The checkbox and menu item are gone. A PETSCII board (Particles! BBS) shows in the C64 font with PETSCII graphics. Switching to BBS moves the menu check, and only one of the four items stays checked.
