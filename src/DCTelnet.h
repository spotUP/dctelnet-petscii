#ifndef DCTELNET_H
#define DCTELNET_H

#include <exec/types.h>
#include <string.h>     // size_t

#include "prefs.h"         // struct PrefsStruct, FLAG_* bits


// ID of the gadget in top right corner when title bar is hidden in full screen
#define GADGET_SCREEN_TO_BACK  20

/**
 * @brief Stable identifiers for entries in the main menu.
 *
 * Each identifier is stored in the corresponding NewMenu structure's nm_UserData field.
 * Values must remain unique.
 */
enum MenuItemID
{
    MENU_DCTELNET,
    MENU_ABOUT,
    MENU_BAR0,
    MENU_SCROLLBACK,
    MENU_CAPTURE,
    MENU_SAVE_SCREEN,
    MENU_CONNECTION_OPTIONS,
    MENU_ICONIFY,
    MENU_DISPLAY_SPEED_TEST,
    MENU_FINGER,
    MENU_BAR1,
    MENU_RESET_SCREEN,
    MENU_QUIT,

    MENU_TRANSFER,
    MENU_UPLOAD,
    MENU_BAR2,
    MENU_DOWNLOAD,
    MENU_BAR3,
    MENU_ASCII_SEND,

    MENU_CONNECTION,
    MENU_CONNECT,
    MENU_CONNECT_NEW_INSTANCE,
    MENU_DISCONNECT,
    MENU_BAR4,
    MENU_ADDRESS_BOOK,
    MENU_BAR5,
    MENU_INFORMATION,

    MENU_OPTIONS,
    MENU_USE_WORKBENCH,
    MENU_DISABLE_LEDS,
    MENU_HIDE_TITLEBAR,
    MENU_UNUSED_CRLF,
    MENU_BS_DEL_SWAP,
    MENU_DISABLE_SCROLLBACK,
    MENU_STRIP_ANSI_CODES,
    MENU_UNUSED_SIMPLE_TELNET,
    MENU_PACKET_WINDOW,
    MENU_USE_XEM_LIBRARY,
    MENU_TOOLBAR,
    MENU_RETURN_CRLF,
    MENU_VT_KEYS,
    MENU_EDIT,
    MENU_PASTE,
    MENU_COPY_SCREEN,
    MENU_LOCAL_ECHOBACK,
    MENU_RAW_CONNECTION,
    MENU_JUMP_SCROLL,
    MENU_PETSCII_MODE,

    MENU_SETTINGS,
    MENU_SCREEN_MODE,
    MENU_SCREEN_FONT,
    MENU_SCREEN_PALETTE,
    MENU_DOWNLOAD_PATH,
    MENU_TRANSFER_PROTOCOL,
    MENU_PROTOCOL_OPTIONS,
    MENU_FUNCTION_KEYS,
    MENU_XEM_LIBRARY,
    MENU_XEM_LIB_OPTIONS,
    MENU_TELNET_DISPLAY_ID,
    MENU_SCROLLBACK_LINES,
    MENU_SNAPSHOT_WINDOWS,
    MENU_SAVE_ENTRY_SETTINGS,

    MENU_LOGIN,
    MENU_SEND_USERNAME,
    MENU_SEND_PASSWORD,

    MENU_END
};

// Global variables exported
extern char server[64];
extern long nScrollbackLines;
extern long tcpSocket, nBytesReceived;
extern struct DrawInfo *drawInfo;
extern UWORD modeResX, modeResY;
extern struct Menu *mainMenuStrip;
extern struct MsgPort *iconPort;
extern struct NewWindow newWin;
extern struct PrefsStruct prefs;
extern struct PrefsStruct globalPrefs;   // global settings during an entry session
extern ULONG sessionSettingsId;          // 0 = no entry session
struct SiteSettings;
BOOL BeginEntrySession(ULONG settingsId, const struct SiteSettings *entry);
void EndEntrySession(void);
void DeferConnect(const char *name, const char *host, UWORD port, ULONG settingsId,
                  const char *user, const char *pass, const char *loginMacro);
void SendLoginMacro(const char *macro);
BOOL SessionOverridesKeyboard(void);
void AdoptEntrySession(ULONG settingsId, ULONG groups);
BOOL ScreenModeInto(struct PrefsStruct *target);
const struct PrefsStruct *GlobalSettings(void);
const TEXT *GlobalFKeys(void);
const struct PrefsStruct *ConnectBaseSettings(void);
const TEXT *ConnectBaseFKeys(void);
BOOL EditPalette(struct PrefsStruct *target);
UWORD AppScreenDepth(struct Screen *s);
extern struct List *scrollbackList;
extern struct Screen *scr;
extern struct TextFont *ansiFont;
extern struct Window *win, *scrollbackWin, *toolBarWin;

extern struct NewGadget newGadget;
extern BOOL shouldQuitApp;          // program finished
extern BOOL isRunningOnWB;          // running in wb
extern BOOL isAppIconified;         // iconified
extern BOOL shouldUniconify;        // must shouldUniconifyify
// Used in Receive(), xpr_sflush(). Beware: each call to this function destroy the content
extern UBYTE recvBuffer[4096];
extern unsigned char buf[2048];
#define F_KEY_COUNT 10
#define F_KEY_SIZE  152  // 151 chars + '\0'
extern TEXT fKeys[F_KEY_COUNT * F_KEY_SIZE];
extern UWORD winTop;                // WinTop topEdge (titlebar height)
extern struct Task *mainTask;       // An AmigaOS Task is roughly equivalent to a thread

// This flag is set by the "Connecting..." window task when the user cancels the operation:
extern BOOL isConnectionAborted;       // CONNECT_ABORTED or CONNECT_TIMED_OUT
#define CONNECT_ABORTED    1                // the user clicked Abort
#define CONNECT_TIMED_OUT  2                // Settings > Connection Options timeout
extern UWORD connectMsgType;
extern char *connectString;

extern UWORD tcpPort;    // current tcp port

extern const char prefsFilename[];
extern const char keysFilename[];
extern const char bookFilename[];

extern void *visualInfos;
extern char username[42], password[42];


// Functions exported
BOOL OpenDisplay(void);
void CloseDisplay(BOOL manageScreen);
long TCPSend(const char *buf, long len);
void CloseIcon(void);                                   // Uniconify the application
void LEDs(void);
void LocalFmt(char *ctl, ...);
void TextFmt(struct RastPort *rP, char *ctl, ...);
void LocalPrint(char *data);
void OpenIcon(void);                                    // inconify the application
void SavePrefs(void);
void SimpleReq(char *str);
void SetWaitPointer(struct Window * window);
UWORD BeginServerConnection(char *servername, UWORD port);
struct MenuItem *GetMenuItemFromID(enum MenuItemID id);
WORD GetMenuNumberFromID(enum MenuItemID id);

#endif /* DCTELNET_H */
