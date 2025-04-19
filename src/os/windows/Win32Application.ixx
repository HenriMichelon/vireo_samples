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

import samples.app;

export namespace samples {

    class Win32Application {
    public:
        static int run(
            const shared_ptr<Application>& app,
            UINT width, UINT height,
            const wstring& name,
            HINSTANCE hInstance,
            int nCmdShow);
        static HWND getHwnd() { return hwnd; }
        static auto& getApp() { return app; }

    private:
        static constexpr auto ID_VULKAN{1001};
        static constexpr auto ID_DIRECTX{1002};

        static HWND hwnd;
        static shared_ptr<Application> app;

        static bool dirExists(const string& dirName_in);
        static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
        static LRESULT CALLBACK SelectorWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
        static vireo::Backend backendSelectorDialog(HINSTANCE hInstance, const wstring& title);
    };
}
