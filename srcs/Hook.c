#include <Windows.h>
#include "Winkey.h"

#pragma data_seg(".WINKEY")

HWND installingHwnd = NULL;

#pragma data_seg()
#pragma comment(linker, "/SECTION:.WINKEY,RWS")

UINT UWM_WINKEY = 0;

__declspec(dllexport) LRESULT CALLBACK GetMessageProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    MSG *msg;

    if (nCode >= HC_ACTION)
    {
        msg = (MSG *)lParam;

        if (LOWORD(msg->message) == WM_CHAR)
        {
            PostMessage(installingHwnd, UWM_WINKEY, msg->wParam, msg->lParam);
        }
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);

    UNREFERENCED_PARAMETER(wParam);
}

__declspec(dllexport) int __cdecl setInstallingHwnd(HWND hwnd)
{
    installingHwnd = hwnd;
    return 0;
}

BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        UWM_WINKEY = RegisterWindowMessage(WINKEY_CHAR_WSTR);
        if (UWM_WINKEY == 0)
        {
            return (FALSE);
        }
        break;
    case DLL_PROCESS_DETACH:
        break;
    default:
        break;
    }

    return (TRUE);

    UNREFERENCED_PARAMETER(lpvReserved);
    UNREFERENCED_PARAMETER(hinstDLL);
}