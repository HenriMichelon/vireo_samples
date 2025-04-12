/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Libraries.h"
#include "Win32Libraries.h"
export module samples.win32;

import glm;
import samples.app;
import vireo.backend;
using namespace vireo;

namespace samples {

    export class Win32Application {
    public:
        static int run(
            std::shared_ptr<Application> app,
            UINT width, UINT height,
            const std::wstring& name, HINSTANCE hInstance, int nCmdShow);
        static HWND getHwnd() { return hwnd; }
        static auto& getApp() { return app; }

    protected:
        static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    private:
        static HWND hwnd;
        static std::shared_ptr<Application> app;
    };
}
