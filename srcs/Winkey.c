#include <Windows.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <psapi.h>

#include <strsafe.h>
#include <stdlib.h>

#include "Winkey.h"

HWND g_hWnd;
HWND g_hWndCurrentFocus;

UINT UWM_WINKEY = 0;

void WinEventproc(
    HWINEVENTHOOK hWinEventHook,
    DWORD event,
    HWND hwnd,
    LONG idObject,
    LONG idChild,
    DWORD idEventThread,
    DWORD dwmsEventTime)
{
    if (event == EVENT_OBJECT_FOCUS)
    {
        TCHAR wndText[256];

        g_hWndCurrentFocus = hwnd;

        if (GetWindowText(hwnd, wndText, 256))
        {
            SetWindowText(g_hWnd, wndText);
        }
    }

    UNREFERENCED_PARAMETER(hWinEventHook);
    UNREFERENCED_PARAMETER(idObject);
    UNREFERENCED_PARAMETER(idChild);
    UNREFERENCED_PARAMETER(idEventThread);
    UNREFERENCED_PARAMETER(dwmsEventTime);
}

/*
 * @brief Get the clipboard text.
 *
 * @param hwnd The window handle.
 * @return const TCHAR* The clipboard text or NULL.
 */
const TCHAR *GetClipboardText(HWND hwnd)
{
    TCHAR *result = NULL;
    TCHAR *pchData = NULL;
    HANDLE hData = NULL;
    size_t szData = 0;

    OpenClipboard(hwnd);
#ifdef UNICODE
    hData = GetClipboardData(CF_UNICODETEXT);
#else
    hData = GetClipboardData(CF_TEXT);
#endif
    if (hData == NULL)
    {
        goto cleanClipboard;
    }
    pchData = (TCHAR *)GlobalLock(hData);

    if (StringCchLength(pchData, STRSAFE_MAX_CCH, &szData) != S_OK)
    {
        goto globalUnlock;
    }
    result = malloc(szData * sizeof(TCHAR));
    if (result == NULL)
    {
        goto globalUnlock;
    }
    StringCchCopy(result, szData, pchData);

globalUnlock:
    GlobalUnlock(hData);
cleanClipboard:
    CloseClipboard();
    return result;
}

const TCHAR *GetCrtlText(TCHAR c)
{
    const TCHAR format[] = TEXT("[CRTL + %c]");
    size_t szResult = sizeof(format) + sizeof(TCHAR);
    TCHAR *result = NULL;

    result = malloc(szResult);
    if (result == NULL)
    {
        return NULL;
    }

    StringCchPrintfEx(result, szResult, NULL, NULL, 0, format, c + 'A' - 1);

    return result;
}

/**
 * @brief
 *
 * @param c
 * @param isAllocated
 * @return const TCHAR*
 */
static const TCHAR *GetSpecialKeyValue(TCHAR c)
{
    const TCHAR *result = NULL;
    SHORT crtlKeyState = 0;

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
        const TCHAR *result = NULL;

        result = GetSpecialKeyValue((TCHAR)wParam);
        if (result)
        {
            MessageBox(hwnd, result, TEXT("Winkey"), MB_OK);
            free((void *)result);
        }
        else
        {
            MessageBox(hwnd, (LPCTSTR)&wParam, TEXT("Winkey"), MB_OK);
        }
    }
    else
    {
        switch (uMsg)
        {
        case WM_CREATE:
            break;
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

            EndPaint(hwnd, &ps);
            break;
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

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    const TCHAR CLASS_NAME[] = TEXT("WINKEY");
    MSG msg = {0};
    WNDCLASS wc = {0};
    HWINEVENTHOOK hWinEventHook;
    HHOOK hHook;
    HINSTANCE hHookDll;
    FARPROC setInstallingHwnd;
    FARPROC GetMessageProc;

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    g_hWnd = CreateWindowEx(
        0,
        CLASS_NAME,
        TEXT("WINKEY"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL,
        NULL,
        hInstance,
        NULL);

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
    GetMessageProc = GetProcAddress(hHookDll, "GetMessageProc");
    setInstallingHwnd = GetProcAddress(hHookDll, "setInstallingHwnd");
    if (GetMessageProc == NULL || setInstallingHwnd == NULL)
    {
        return 0;
    }
    hHook = SetWindowsHookEx(WH_GETMESSAGE, (HOOKPROC)GetMessageProc, hHookDll, 0);
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

    /* Install WinEvent hook for retrieving the current window that has keyboard focus. */

    hWinEventHook = SetWinEventHook(
        EVENT_OBJECT_FOCUS,
        EVENT_OBJECT_FOCUS,
        NULL,
        WinEventproc,
        0,
        0,
        WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS | WINEVENT_SKIPOWNTHREAD);

    if (hWinEventHook == NULL)
    {
        return 0;
    }

    ShowWindow(g_hWnd, nCmdShow);

    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    FreeLibrary(hHookDll);
    UnhookWinEvent(hWinEventHook);
    UnhookWindowsHookEx(hHook);

    return 0;

    UNREFERENCED_PARAMETER(pCmdLine);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nCmdShow);
}