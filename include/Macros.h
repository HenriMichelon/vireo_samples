#pragma once

#ifdef _WIN32
#include <windows.h>
import std;
import vireo;
import samples.win32;
#define APP(_APP, _TITLE, _WIDTH, _HEIGHT) \
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszArgument, int nCmdShow) { \
return samples::Win32Application::run(_APP, _WIDTH, _HEIGHT, std::to_wstring(_TITLE), hInstance, nCmdShow); \
};
#elifdef __linux__
import samples.sdl;
#define APP(_APP, _TITLE, _WIDTH, _HEIGHT) \
int main(int argc, char** argv) { \
return samples::SDLApplication::run(_APP, _WIDTH, _HEIGHT, _TITLE); \
};
#endif
