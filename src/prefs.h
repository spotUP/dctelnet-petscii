#ifndef PREFS_H
#define PREFS_H

/* DCTelnet's settings, saved as a raw dump in PROGDIR:DCTelnet.Prefs. Kept
 * free of any AmigaOS header beyond exec/types.h so host-built modules
 * (site_prefs.c) and their tests can use it. */

#include <exec/types.h>
#include <string.h>     // size_t

struct PrefsStruct
{
    ULONG DisplayID;
    UWORD DisplayWidth,
          DisplayHeight,
          DisplayDepth,
          fontsize;
    char  fontname[32],
          downloadpath[52],
          xferlibrary[52],
          xferinit[52];
    UWORD color[16];
    ULONG flags;
    UWORD win_left,
          win_top,
          win_width,
          win_height,
          sb_left,
          sb_top,
          sb_width,
          sb_height;
    char  uploadpath[52],
          displaydriver[32];
    ULONG sb_lines;
    char  displayidstr[32];
    UWORD toolBarWin_left,
          toolBarWin_top;
    /* Appended for 256-colour and RTG screens -- the fields above are the
     * 376-byte layout every older DCTelnet reads and writes (PREFS_V1_SIZE).
     * The 16 ANSI colours as 0x00RRGGBB in ANSI order (0 black, 1 red ...
     * 7 white, 8-15 bright). Valid when ansiMagic == PREFS_ANSI_MAGIC; an
     * older file loads with 0 here and gets them from color[] (RGB4, in
     * ibmcon's unit-1 order: pen 1 white, pen 7 red). */
    ULONG ansiMagic;
    ULONG ansiRGB[16];
    /* Appended for the settings that came after (PREFS_V2_SIZE above this).
     * Every field is 0 for its default, so an older, shorter file loads with
     * the defaults. A new field takes bytes from reserved: the size of the
     * struct -- and of every settings file -- does not change again. On/off
     * settings are flags bits instead. (Plain fields, not a nested struct:
     * vbcc's offsetof takes no member path, and site_prefs.c lists each
     * field by offset.) */
    UBYTE redialTries;          /* failed connects tried again; 0: none */
    UBYTE redialDelay;          /* seconds between tries; 0: 10 */
    UBYTE antiIdleMinutes;      /* a telnet NOP after this long without a
                                   key sent; 0: never */
    UBYTE connectTimeout;       /* seconds a connect may take; 0: the TCP
                                   stack's own limit */
    UBYTE charset;              /* what the BBS sends: CHARSET_CP437 (0),
                                   _LATIN1, _UTF8 (charset.h) */
    UBYTE bell;                 /* BEL: BELL_FLASH (0), BELL_SOUND, BELL_OFF */
    UBYTE ansiMusic;            /* <>0: play ANSI music (ansimusic.h) */
    UBYTE extReserved[57];
};

#define BELL_FLASH 0     /* prefs.bell: the screen flashes (DisplayBeep) */
#define BELL_SOUND 1     /*   a short tone */
#define BELL_OFF   2

#define PREFS_V1_SIZE     376
#define PREFS_V2_SIZE     444   /* up to and with ansiRGB */
#define PREFS_SIZE        508   /* checked in site_prefs.c */
#define PREFS_ANSI_MAGIC  0x52474231UL    /* 'RGB1' */

/*
* Bit flags stored in PrefsStruct->flags.
* These defines describe the meaning of each individual bit.
* These flags map directly to the checkable items in the "Options" pull-down menu
*/
#define FLAG_HIDE_TITLEBAR       (1UL << 0)   // BIT 0  = Hide Title Bar
#define FLAG_CRLF_CORRECTION     (1UL << 1)   // BIT 1  = CRLF Correction
#define FLAG_HIDE_LEDS           (1UL << 2)   // BIT 2  = Hide LEDS
#define FLAG_USE_WORKBENCH       (1UL << 3)   // BIT 3  = Use Workbench
#define FLAG_BS_DEL_SWAP         (1UL << 4)   // BIT 4  = BS/DEL Swap
#define FLAG_DISABLE_SCROLLBACK  (1UL << 5)   // BIT 5  = Disable Scrollback
#define FLAG_STRIP_COLOUR        (1UL << 6)   // BIT 6  = Strip Colour
#define FLAG_SIMPLE_TELNET       (1UL << 7)   // BIT 7  = Very simple telnet negotiation.
#define FLAG_PACKET_WINDOW       (1UL << 8)   // BIT 8  = Packet Window
#define FLAG_USE_XEM_LIBRARY     (1UL << 9)   // BIT 9  = Use XEM Library
#define FLAG_TOOL_BAR            (1UL << 10)  // BIT 10 = Tool bar
#define FLAG_RETURN_CRLF         (1UL << 11)  // BIT 11 = Return = CR&LF
#define FLAG_LOCAL_ECHO          (1UL << 12)  // BIT 12 = Local Echoback
#define FLAG_RAW_CONNECTION      (1UL << 13)  // BIT 13 = Raw Connection (NO telnet negotiation data)
#define FLAG_JUMP_SCROLL         (1UL << 14)  // BIT 14 = Jump Scroll
#define FLAG_PETSCII_MODE        (1UL << 15)  // BIT 15 = PETSCII emulation (C64/C128)
#define FLAG_WINDOW_SNAPSHOT     (1UL << 16)  // BIT 16 = win_* hold a Snapshot Windows size;
                                              //          clear: the Workbench window opens 80x25
#define FLAG_132_COLUMNS         (1UL << 19)  // BIT 19 = the terminal is 132 columns wide, not 80
#define FLAG_RLOGIN              (1UL << 18)  // BIT 18 = Rlogin (RFC 1282): a raw byte stream after
                                              //          the login message, no telnet codes
#define FLAG_SSH                 (1UL << 20)  // BIT 20 = SSH (encrypted login, sshconn.c): no telnet codes
#define FLAG_VT_KEYS             (1UL << 17)  // BIT 17 = Home/End/Page/Insert send VT codes
                                              //          (ESC[1~ ...), clear: ANSI-BBS (ESC[H ...)

#endif /* PREFS_H */
