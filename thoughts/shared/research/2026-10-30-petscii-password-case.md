# PETSCII mixed-case password rejected (UPTown BBS / AmiExpress)

Date 2026-10-30. Branch inspected: feature/pr-petscii-case-switch (dctelnet-v2, HEAD 2448df3). The recovered tree's
petscii_keymap.c is the same function (it only lacks the later fkey/digit helpers).

## 1. Bytes sent (independent of charset state)
petscii_translate_key(), src/petscii_keymap.c:26-29 swaps ASCII letter case, nothing else:
- 'a' -> 0x41, 'A' -> 0x61, 'z' -> 0x5A, 'Z' -> 0x7A, '1' -> 0x31, '!' -> 0x21.
The same bytes go out in both charset states: the function takes no state, and g_petsciiState.shift_lowercase
(src/petscii_dispatch.c:117-122) is only read for display (DCTelnet.c:2290).
Callers: SendTypedChar DCTelnet.c:3496 (every typed char except CR), login macro text DCTelnet.c:2414.
Both reach OutKey -> TCPSend. Test pinning the behaviour: tests/test_petscii_modules.c:23-24.
(Line numbers in DCTelnet.c are from `git show HEAD:src/DCTelnet.c`.)

## 2. What a real C64 sends (from knowledge, unverified here)
KERNAL keyboard: in both charsets the unshifted letter key = 0x41-0x5A, SHIFT+letter = 0xC1-0xDA.
0x61-0x7A is never produced by the keyboard (those are graphics codes, screen codes 0x41-0x5A).
In the lowercase charset 0x41 displays as 'a' (so BBSes treat 0x41-0x5A as lowercase and 0xC1-0xDA as uppercase);
in the upper/graphics charset the same bytes display as capitals / graphics.
AmiExpress source not checked (unverified). From the symptom: AmiExpress Web PETSCII works with the same password,
so it sends 0x41-0x5A for lowercase and 0xC1-0xDA for uppercase (the C64 convention; unverified), and the BBS
normalises that to ASCII.

## 3. Root cause (confidence: high that the swap is wrong, medium on the exact BBS-side symptom)
src/petscii_keymap.c:26-29 models typed input as "swap case", the inverse of what a C64 sends. Result:
- lowercase typed 'a' -> 0x41: a PETSCII-aware BBS reads it as 'a' (correct by luck of convention) - lowercase letters survive.
- uppercase typed 'A' -> 0x61: not a letter in PETSCII. The BBS either passes it through as ASCII 'a' (case lost) or
  drops it as a graphics code. A password with any capital letter therefore no longer matches. Digits and punctuation are unchanged.
- ANSI mode sends ASCII: works. Web PETSCII sends 0xC1-0xDA for capitals: works.
Answer to Bruno: yes, mixed case is the issue; all-lowercase passwords should log in, any capital fails.
Initial charset: shift_lowercase = 0 (upper/graphics) at start, petscii_dispatch.c:26. It does not matter for the keymap
(the bytes are state-independent), but it explains the display: typed letters appear as capitals until the BBS sends 0x0E
(byte 14). The C64 keyboard would also send 0x41/0xC1 in that charset, so no state-dependent mapping is needed.
The swap was likely copied from the receive-side fallback (petscii_fallback_cp437, 0x41->'a') and applied to the wrong direction.

## Fix
petscii_translate_key non-special branch: 'a'-'z' -> 0x41-0x5A (c - 0x20), 'A'-'Z' -> 0xC1-0xDA (c + 0x80), others unchanged.
Header comment petscii_keymap.h:30-33 and the test lines 23-24 change to match. Macro path (DCTelnet.c:2414) follows for free
since it calls the same function. Option to confirm before shipping: capture bytes from AmiExpress Web (or a CGTerm/SyncTERM
session) for 'aA' and compare with the new output.

## 4. Tests
Existing: tests/test_petscii_modules.c:22-30 (test_keymap) asserts the current swap; no test reaches SendTypedChar.
Host test shape: replace the two asserts with 'a'==0x41, 'z'==0x5A, 'A'==0xC1, 'Z'==0xDA, '1'=='1', '!'=='!'; add a
round trip loop over all 26 letters asserting the output is never in 0x61-0x7A. Reachability: SendTypedChar is static in
DCTelnet.c (Amiga-only), so the entry-point test needs OutKey exposed or the char-to-byte step extracted into the keymap
module and the macro and typed paths both calling it.
