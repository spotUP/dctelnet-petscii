# EClock timing, A1200 68060, EClock 709379 Hz, 5 runs median, 4 x 1024-byte parts
```
eclock_hz 709379
plain run0 swap_ticks 466 swap+write_ticks 3267
plain run1 swap_ticks 467 swap+write_ticks 3204
plain run2 swap_ticks 469 swap+write_ticks 3921
plain run3 swap_ticks 475 swap+write_ticks 12177
plain run4 swap_ticks 466 swap+write_ticks 3216
plain MEDIAN swap_us 658 swap+write_us 4607
sgr-heavy run0 swap_ticks 2835 swap+write_ticks 41593
sgr-heavy run1 swap_ticks 2813 swap+write_ticks 8257
sgr-heavy run2 swap_ticks 2817 swap+write_ticks 8297
sgr-heavy run3 swap_ticks 2810 swap+write_ticks 8416
sgr-heavy run4 swap_ticks 2824 swap+write_ticks 7756
sgr-heavy MEDIAN swap_us 3973 swap+write_us 11702
sgr-light run0 swap_ticks 1528 swap+write_ticks 4335
sgr-light run1 swap_ticks 1237 swap+write_ticks 4453
sgr-light run2 swap_ticks 1246 swap+write_ticks 19673
sgr-light run3 swap_ticks 1237 swap+write_ticks 5120
sgr-light run4 swap_ticks 1247 swap+write_ticks 4262
sgr-light MEDIAN swap_us 1757 swap+write_us 6280
```
## Source (eclock.c), built: vc +aos68k -cpu=68020 -O3 -speed, image walkero/docker4amigavbcc:m68k-amigaos-2.0.0, linked with src/ansiscan.c
```c
#include <exec/types.h>
#include <exec/io.h>
#include <exec/memory.h>
#include <devices/console.h>
#include <devices/timer.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <proto/timer.h>
#include <string.h>
#include "ansiscan.h"

struct Device *TimerBase; struct IntuitionBase *IntuitionBase;
static UBYTE b1[4096], b2[4096], b3[4096], tmp[1024 + ANSI_SWAP17_HELD];

static ULONG freq;
static ULONG now(void){ struct EClockVal ev; freq = ReadEClock(&ev); return ev.ev_lo; }

static void fill(void)
{
    int i, c = 0, k;
    for (i = 0; i < 4096; ) { /* plain: CR LF every 80 */
        if (c == 80 && i + 2 <= 4096) { b1[i++] = '\r'; b1[i++] = '\n'; c = 0; }
        else { b1[i++] = 'A' + (i % 26); c++; }
    }
    for (i = 0; i + 9 <= 4096; ) { memcpy(b2 + i, "\033[1;33mabcd", 9); i += 9; }
    while (i < 4096) b2[i++] = 'x';
    for (i = 0, k = 0; i < 4096; ) {
        if (k == 12 && i + 4 <= 4096) { memcpy(b3 + i, "\033[0m", 4); i += 4; k = 0; }
        else { b3[i++] = 'a' + (i % 26); k++; }
    }
}

static int cmp(const void *a, const void *b){ ULONG x=*(ULONG*)a,y=*(ULONG*)b; return x<y?-1:x>y; }
static ULONG med(ULONG *v){ qsort(v,5,sizeof(ULONG),cmp); return v[2]; }

int main(void)
{
    struct MsgPort *tp, *cp; struct timerequest *tr; struct IOStdReq *io;
    struct Window *w; FILE *f; int b, r, p;
    UBYTE *bufs[3]; const char *nm[3] = {"plain","sgr-heavy","sgr-light"};
    struct AnsiSwap17 st; ULONG t0, t1, sw[5], cw[5];

    IntuitionBase = (struct IntuitionBase*)OpenLibrary("intuition.library", 37);
    fill(); bufs[0]=b1; bufs[1]=b2; bufs[2]=b3;
    tp = CreateMsgPort(); tr = (struct timerequest*)CreateIORequest(tp, sizeof *tr);
    if (!tr || OpenDevice("timer.device", UNIT_ECLOCK, (struct IORequest*)tr, 0)) return 20;
    TimerBase = tr->tr_node.io_Device;
    w = OpenWindowTags(NULL, WA_Left,0, WA_Top,20, WA_Width,640, WA_Height,200,
        WA_Title,(ULONG)"DCTelnet upstream session: console.device EClock timing for PR 44",
        WA_Flags, WFLG_ACTIVATE|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_SMART_REFRESH|WFLG_NOCAREREFRESH,
        TAG_DONE);
    cp = CreateMsgPort(); io = (struct IOStdReq*)CreateIORequest(cp, sizeof *io);
    if (!w || !io) return 20;
    io->io_Data = (APTR)w; io->io_Length = sizeof(struct Window);
    if (OpenDevice("console.device", 0, (struct IORequest*)io, 0)) return 20;
    f = fopen("DH1:Temp/eclock.txt", "w");
    now(); fprintf(f, "eclock_hz %lu\n", freq);
    for (b = 0; b < 3; b++) {
        for (r = 0; r < 5; r++) {
            memset(&st, 0, sizeof st);
            t0 = now();
            for (p = 0; p < 4; p++) Ansi_Swap17(&st, bufs[b] + p*1024, 1024, tmp);
            t1 = now(); sw[r] = t1 - t0;
            io->io_Command = CMD_WRITE; io->io_Data = (APTR)"\033[H\033[J"; io->io_Length = 6; DoIO((struct IORequest*)io);
            memset(&st, 0, sizeof st);
            t0 = now();
            for (p = 0; p < 4; p++) {
                size_t n = Ansi_Swap17(&st, bufs[b] + p*1024, 1024, tmp);
                io->io_Command = CMD_WRITE; io->io_Data = (APTR)tmp; io->io_Length = n; DoIO((struct IORequest*)io);
            }
            t1 = now(); cw[r] = t1 - t0;
            fprintf(f, "%s run%d swap_ticks %lu swap+write_ticks %lu\n", nm[b], r, sw[r], cw[r]);
        }
        fprintf(f, "%s MEDIAN swap_us %lu swap+write_us %lu\n", nm[b],
            med(sw)*1000/(freq/1000), med(cw)*1000/(freq/1000));
    }
    fclose(f);
    CloseDevice((struct IORequest*)io); DeleteIORequest((struct IORequest*)io); DeleteMsgPort(cp);
    CloseWindow(w); CloseDevice((struct IORequest*)tr); DeleteIORequest((struct IORequest*)tr); DeleteMsgPort(tp);
    return 0;
}
```
