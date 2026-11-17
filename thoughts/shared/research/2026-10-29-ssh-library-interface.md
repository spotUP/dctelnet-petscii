---
date: 2026-10-29
topic: ssh.library interface for upstream PR #42 (Bruno's split request)
status: draft - decisions made 2026-10-29
---

# ssh.library interface (task 3 in thoughts/spot/todos-upstream-prs.md)

Source read: dctelnet-ssh worktree, branch pr/ssh: src/sshconn.h, src/sshconn.c,
src/ssh.h, src/Xfer.h, src/Xem_wrapper.c; Bruno's 2026-10-23 comment on #42.

## What exists today

- `src/ssh.c` + `src/crypto/*` is already a pure protocol engine. It has no socket
  calls, no file I/O, no requesters, no malloc/printf. Its only includes are its own
  headers and `exec/execbase.h`. The caller hands it socket bytes (`Ssh_Receive`),
  takes bytes for the socket (`Ssh_TakeOutput`) and text for the terminal
  (`Ssh_TakeText`). User interaction goes through three callbacks in `struct Ssh`:
  `hostKey`, `ask`, `note`.
- `src/sshconn.c` (259 lines) is the Amiga glue: `send()`/`recv()` on `tcpSocket`,
  host key requester and `PROGDIR:KnownHosts`, login requesters, random seeding from
  `SysBase->IdleCount`/`DispCount` and `VBeamPos()`.
- Call sites outside sshconn.c (git grep `SshConn_[A-Za-z]+\(`):
  DCTelnet.c 494 Write, 762 End, 1231 Start, 1321/1323 Active+KeepAlive,
  2068 Active+Read, 2081 ClosedByServer, 2760/2793/2811/2843 Buffered;
  DCTelnet-protocol.h 422/425 Active+WindowChange;
  **Xfer.c 68 Active+Read, 289 Buffered, 862/864/869 Active+Buffered+Peek**
  (Bruno's list misses the three Xfer.c sites: file transfers over SSH).

## Proposed split

The library boundary goes where the code already has one: between the engine and
the glue.

- **ssh.library** = `ssh.c` + `crypto/*` (+ `bn_muladd.s`). Freestanding: exec only.
- **DCTelnet keeps sshconn.c**: sockets, requesters, KnownHosts, entropy sources.
  It opens ssh.library in `SshConn_Start()` only (`OpenLibrary("ssh.library", 1)`),
  closes it in `SshConn_End()`. No SSH use = no library loaded = no RAM used.
- Why the socket stays in DCTelnet: a bsdsocket.library base belongs to the opener,
  and socket descriptors are per base. A library that called `send()`/`recv()` would
  need DCTelnet's SocketBase passed in and would tie ssh.library to one TCP/IP stack
  ABI. With I/O in the app, the library never touches the network and a security fix
  in the protocol or crypto ships as a library update alone (Bruno's goal 2).

## Hot paths (Bruno's concern)

`SshConn_Active()` and `SshConn_Buffered()` stay in DCTelnet and read fields, no
library call:

```c
extern struct SshSession *sshSession;           /* sshconn.c; NULL = no SSH */
#define SshConn_Active()   (sshSession != NULL)
#define SshConn_Buffered() (sshSession && sshSession->textLen)
```

Same cost as today's function bodies, minus the call.

## Library interface

Opaque session allocated by the library, with a small public head that the app may
read. The private tail (keys, buffers, ~30 KB today) never becomes ABI.

```c
struct SshSession                       /* public head, read-only for the app */
{
    UWORD  version;                     /* SSH_SESSION_VERSION; grows only at the end */
    UWORD  headSize;                    /* sizeof the public head the library filled */
    LONG   state;                       /* SSH_* states from ssh.h */
    ULONG  textLen;                     /* terminal bytes waiting (SshConn_Buffered) */
    BOOL   serverClosed;
    char   error[160];
    APTR   userData;                    /* for the app's callbacks */
    /* private fields follow, owned by the library */
};

struct SshCallbacks                     /* app side, register args like XEM_IO */
{
    BOOL (__ASM__ *hostKey)(__REG__(a0, struct SshSession *s), __REG__(a1, const UBYTE *blob),
                            __REG__(d0, ULONG len), __REG__(a2, const UBYTE *fp));
    BOOL (__ASM__ *ask)(__REG__(a0, struct SshSession *s), __REG__(a1, const char *prompt),
                        __REG__(d0, BOOL echo), __REG__(a2, char *out), __REG__(d1, ULONG max));
    void (__ASM__ *note)(__REG__(a0, struct SshSession *s), __REG__(a1, const char *text));
};
```

LVOs (one per engine function today, nothing new):

| LVO | Function | Replaces |
| --- | --- | --- |
| -30 | `SshSession *Ssh_Create(a0 SshNewSession *)` | AllocVec + `Ssh_Init` |
| -36 | `void Ssh_Delete(a0 s)` | memset + FreeVec |
| -42 | `ULONG Ssh_Receive(a0 s, a1 data, d0 len)` | same |
| -48 | `void Ssh_Send(a0 s, a1 data, d0 len)` | same |
| -54 | `ULONG Ssh_TakeOutput(a0 s, a1 out, d0 max)` | same |
| -60 | `ULONG Ssh_TakeText(a0 s, a1 out, d0 max)` | same |
| -66 | `void Ssh_WindowChange(a0 s, d0 cols, d1 rows)` | same |
| -72 | `void Ssh_KeepAlive(a0 s)` | same |
| -78 | `void Ssh_Disconnect(a0 s)` | same |
| -84 | `void Ssh_Fingerprint(a0 fp, a1 out)` | same |
| -90 | `void Ssh_AddEntropy(a0 data, d0 len)` | `Random_Add` (pool moves into the library) |

`SshNewSession` carries user, pass, term, cols, rows, slowCpu, callbacks, userData
and its own `version` so later fields can be added without breaking old callers.

Callbacks use the `__SAVE_DS__ __ASM__ __REG__` macros DCTelnet already defines for
XEM (`src/Xfer.h:15`), so an app built with another compiler can call in.

## Effects

- sshconn.c changes: the `ssh` pointer type, `Ssh_*` calls through `SshBase`,
  `Random_Add` -> `Ssh_AddEntropy`, OpenLibrary/CloseLibrary in Start/End, and a
  clear message if ssh.library is missing ("SSH needs LIBS:ssh.library").
- DCTelnet.c, DCTelnet-protocol.h, Xfer.c: no change (macros keep the names).
- Tests: tests/test_ssh.c and test_crypto.c keep linking the engine sources directly;
  one new test opens the built library under vamos and drives Create/Receive/Delete.
- Packaging: installer copies ssh.library to LIBS:.

## Interaction with #58 (VBCC -O crash)

The split does not fix #58 by itself. If the cause is stack depth (lead under
measurement: thoughts/shared/research/2026-10-29-ssh-stack-highwater.md), library
calls run on the caller's stack, so the fix (big temporaries off the stack, or a
stack request) must land in the engine either way. Fix #58 first, as decided.

## Decisions (user, 2026-10-29)

1. Split line: engine only. ssh.c + crypto in ssh.library; sockets, requesters,
   KnownHosts and entropy sources stay in sshconn.c.
2. CPU builds: 68000 and 68020 variants, plus FPU/no-FPU and 68040/68060 variants.
   Facts for the build matrix (checked in pr/ssh):
   - ssh.c and crypto/*.c use no float or double, so an FPU build compiles the same
     integer code; the FPU axis only matters if the matrix must mirror DCTelnet's.
   - src/crypto/bn_muladd.s uses 64-bit `mulu.l d1,d3:d2` (lines 65, 149). The 68060
     has no 64-bit MULU.L in hardware: it traps to the 68060 support library on every
     limb multiply. The 060 variant needs a 32x32->64 path from 32-bit multiplies.
     This is the variant with real speed value.
3. Missing library: connect fails with "SSH needs LIBS:ssh.library"; Telnet unaffected.
