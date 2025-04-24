#pragma once
#include "Libraries.h"

#ifdef _WIN32
import samples.win32;
#define APP(_APP, _TITLE, _WIDTH, _HEIGHT) \
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszArgument, int nCmdShow) { \
return samples::Win32Application::run(_APP, _WIDTH, _HEIGHT, _TITLE, hInstance, nCmdShow); \
};
#endif
