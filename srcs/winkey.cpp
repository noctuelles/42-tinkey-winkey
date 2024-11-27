#include <windows.h>
#include <iostream>

int main()
{
    MSG msg = {0};

    std::cout << "Hello, World!" << std::endl;

    while (GetMessage(&msg, NULL, 0, 0) != 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (1);
}