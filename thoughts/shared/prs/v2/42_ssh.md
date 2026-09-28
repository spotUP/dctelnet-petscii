---
date: 2026-09-28
topic: "PR #42: SSH connections"
tags: [dctelnet, pr, v2.0-dev]
status: final
---

# #42 SSH connections

Head `feature/pr-ssh` on spotUP/dctelnet, base v2.0-dev.

**Based on v2.0-dev, stacked on #40.** Only the last commit belongs here.

## Summary

Terminal > SSH (and an SSH box in the Terminal group of an Address Book
entry's settings) turns a connection into an SSH login: everything sent
both ways is encrypted, the password too. The SSH-2 client is written for
DCTelnet, with no library and no AmiSSL:

- src/ssh.c: the protocol as a pure state machine (socket bytes in,
  socket bytes and terminal text out), host-tested by replaying two
  logins recorded from a paramiko server (tools/ssh_test_server.py).
- src/crypto/: SHA-256 and HMAC, AES-128-CTR, a Montgomery bignum, RSA
  PKCS#1 v1.5 verification, X25519 and a random pool. All multiplying
  goes through Bn_MulAdd, 68000 or 68020 assembly picked at run time
  (crypto/bn_muladd.s, vbcc only; GCC and SAS/C build the C loop).
- Algorithms: curve25519-sha256 key exchange, and
  diffie-hellman-group14-sha256 on a 68020 or better; rsa-sha2-256 host
  keys; aes128-ctr with hmac-sha2-256; password and keyboard-interactive
  login; one session channel with a pty, window-change, flow control and
  server rekeying.
- Each message is taken only where the session is: before the first
  NEWKEYS nothing but the key exchange (a man in the middle cannot say
  "logged in" in the clear and get the password), authentication only
  while logging in, channel traffic only once the channel is open.
- src/sshconn.c: every socket read and write of an SSH session goes
  through the engine -- the terminal, Zmodem/XPR transfers, the anti-idle
  keep-alive (SSH_MSG_IGNORE) and window-size changes. The main loop does
  not sleep in WaitSelect while decrypted bytes wait.
- Host keys are kept in PROGDIR:KnownHosts (Trust, Just This Time,
  Cancel; a changed key gets a warning), read whole and rewritten only
  from the whole file. The password requester shows
  stars (GetSecretRequester).
- A SyncTERM phone book's SSH entries are imported (port 22).

The setting is State bit 14 (APP_SSH), part of TELNET_DATA() and of the
Terminal group; an old DCTelnet.Prefs keeps it (old flag bit 20). The SSH
sources build with -maxoptpasses=100: at v2.0's -O3 -speed, vbcc needs
more than its default 30 passes for them (warning 172).

Also: the guide's Rlogin line names Terminal > Rlogin.

## Testing on v2.0-dev

- Builds with vbcc for 68000 and 68020, warnings as errors; `cd tests && make` passes.
- Emulated A1200 (FS-UAE, driven through amiagent): An entry with SSH in its settings: host key requester, Trust (KnownHosts written), login with the entry's user name and password, pty VT102 80x25, a 40000-byte stream; with no user name DCTelnet asks (the password shows stars). Also with the built-in renderer on the own screen and with PETSCII Mode (pty PETSCII 40x30, C64 font), on ibmcon, console.device and the built-in renderer.

Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01DmFJdpcHF7rhRU6XnPvz6P
