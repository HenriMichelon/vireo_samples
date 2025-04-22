/*
 * Copyright (c) 2025-present Henri Michelon
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */
module;
#include "Libraries.h"
#include "Win32Libraries.h"
module samples.win32;

import vireo.tools;

namespace samples {

    HWND Win32Application::hwnd = nullptr;
    shared_ptr<Application> Win32Application::app{};

    struct MonitorEnumData {
        int  enumIndex{0};
        int  monitorIndex{0};
        RECT monitorRect{0};
    };
    BOOL CALLBACK MonitorEnumProc(HMONITOR, HDC , LPRECT lprcMonitor, LPARAM dwData) {
        const auto data = reinterpret_cast<MonitorEnumData*>(dwData);
        if (data->enumIndex == data->monitorIndex) {
            data->monitorRect = *lprcMonitor;
            return FALSE;
        }
        data->enumIndex++;
        return TRUE;
    }

    int Win32Application::run(const shared_ptr<Application>& app,
                              const UINT width,
                              const UINT height,
                              const wstring& name,
                              const HINSTANCE hInstance,
                              const int nCmdShow) {
        if (!dirExists("shaders")) {
            MessageBox(nullptr,
                       L"Shaders directory not found, please run the application from the root of the project",
                       L"Error",
                       MB_OK);
            return 0;
        }

        wstring title = name;
        title.append(L" : ");

        const auto windowClass = WNDCLASSEX{
            .cbSize = sizeof(WNDCLASSEX),
            .style = CS_HREDRAW | CS_VREDRAW,
            .lpfnWndProc = WindowProc,
            .hInstance = hInstance,
            .hCursor = LoadCursor(NULL, IDC_ARROW),
            .lpszClassName = L"VireoSampleClass",
        };
        RegisterClassEx(&windowClass);

        auto monitorData = MonitorEnumData {
        };

        int x = CW_USEDEFAULT;
        int y = CW_USEDEFAULT;
        int w = width;
        int h = height;
        DWORD style;
        DWORD exStyle;
        if (width == 0 || height == 0) {
            exStyle = WS_EX_APPWINDOW;
            style = WS_POPUP | WS_MAXIMIZE;
            EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(&monitorData));
            w = monitorData.monitorRect.right - monitorData.monitorRect.left;
            h = monitorData.monitorRect.bottom - monitorData.monitorRect.top;
            x = monitorData.monitorRect.left;
            y = monitorData.monitorRect.top;
        } else {
            style = WS_OVERLAPPEDWINDOW;
            exStyle = 0;
            auto windowRect = RECT{0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
            AdjustWindowRect(&windowRect, style, FALSE);
            x = (GetSystemMetrics(SM_CXSCREEN) - (windowRect.right - windowRect.left)) / 2;
            y = (GetSystemMetrics(SM_CYSCREEN) - (windowRect.bottom - windowRect.top)) / 2;
        }

        hwnd = CreateWindowEx(
            exStyle,
            windowClass.lpszClassName,
            title.c_str(),
            style,
            x,
            y,
            w,
            h,
            nullptr,
            nullptr,
            hInstance,
            nullptr);

        auto rect = RECT{};
        GetWindowRect(hwnd, &rect);

        Win32Application::app = app;
        const auto backend = backendSelectorDialog(hInstance, title);
        // const auto backend = vireo::Backend::VULKAN;
        // const auto backend = vireo::Backend::DIRECTX;
        if (backend == vireo::Backend::UNDEFINED) {
            return 0;
        }
        app->init(backend, hwnd);

        if (backend == vireo::Backend::VULKAN) {
            title.append(L"Vulkan 1.3");
        } else {
            title.append(L"DirectX 12");
        }
        SetWindowText(hwnd, title.c_str());

        try {
            app->onInit();
            ShowWindow(hwnd, nCmdShow);
            auto msg = MSG{};
            while (msg.message != WM_QUIT) {
                if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
            app->onDestroy();
            return static_cast<char>(msg.wParam);
        } catch (vireo::Exception e) {
            MessageBoxA(nullptr, e.what(), "Fatal error", MB_OK);
            return 1;
        }
    }

    LRESULT CALLBACK Win32Application::WindowProc(const HWND hWnd, const UINT message, const WPARAM wParam, const LPARAM lParam) {
        const auto& app = getApp();
        switch (message) {
            case WM_CREATE: {
                const auto pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
                SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
                }
                return 0;
            case WM_KEYDOWN:
                if (app) {
                    app->onKeyDown(static_cast<uint32_t>(wParam));
                }
                return 0;
            case WM_KEYUP:
                if (app) {
                    app->onKeyUp(static_cast<uint32_t>(wParam));
                }
                return 0;
            case WM_PAINT:
                if (app) {
                    app->onUpdate();
                    app->onRender();
                }
                return 0;
            case WM_SIZE:
                if (app) {
                    app->onResize();
                }
                return 0;
            case WM_CLOSE:
                PostQuitMessage(0);
                return 0;
            default:;
        }
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    LRESULT CALLBACK Win32Application::SelectorWindowProc(const HWND hWnd, const UINT msg, const WPARAM wParam, const LPARAM lParam) {
        switch (msg) {
            case WM_COMMAND:
                switch (LOWORD(wParam)) {
                case ID_VULKAN:
                    PostQuitMessage(ID_VULKAN);
                    break;
                case ID_DIRECTX:
                    PostQuitMessage(ID_DIRECTX);
                    break;
                default:;
                }
                break;
            case WM_CLOSE:
                PostQuitMessage(IDCANCEL);
                break;
            default:;
        }
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    bool Win32Application::dirExists(const string& dirName_in) {
        const DWORD ftyp = GetFileAttributesA(dirName_in.c_str());
        return (ftyp != INVALID_FILE_ATTRIBUTES) && (ftyp & FILE_ATTRIBUTE_DIRECTORY);
    }

    vireo::Backend Win32Application::backendSelectorDialog(const HINSTANCE hInstance, const wstring& title) {
        const auto className = L"ApiSelectorWindow";
        const WNDCLASS wc{
            .lpfnWndProc = SelectorWindowProc,
            .hInstance = hInstance,
            .lpszClassName = className,
        };
        RegisterClass(&wc);

        const HWND hWnd = CreateWindowEx(
            0,
            className,
            L"Select Graphics API",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            215,
            100,
            nullptr,
            nullptr,
            hInstance,
            nullptr);

        auto hFont = CreateFont(
            -MulDiv(9, GetDeviceCaps(GetDC(nullptr), LOGPIXELSY), 72),
            0,
            0,
            0,
            FW_NORMAL,
            FALSE,
            FALSE,
            FALSE,
            ANSI_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            L"Segoe UI");

        const auto hVulkan = CreateWindow(
            L"BUTTON",
            L"Vulkan",
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            20,
            15,
            70,
            25,
            hWnd,
            reinterpret_cast<HMENU>(ID_VULKAN),
            hInstance,
            nullptr);
        SendMessage(hVulkan, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

        const auto hDirectX = CreateWindow(
            L"BUTTON",
            L"DirectX",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            110,
            15,
            70,
            25,
            hWnd,
            reinterpret_cast<HMENU>(ID_DIRECTX),
            hInstance,
            nullptr);
        SendMessage(hDirectX, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

        auto rect = RECT{};
        GetWindowRect(hWnd, &rect);
        const auto x = (GetSystemMetrics(SM_CXSCREEN) - (rect.right - rect.left)) / 2;
        const auto y = (GetSystemMetrics(SM_CYSCREEN) - (rect.bottom - rect.top)) / 2;
        SetWindowPos(hWnd, nullptr, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

        ShowWindow(hWnd, SW_SHOW);
        UpdateWindow(hWnd);
        auto msg = MSG{};
        while (GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        DestroyWindow(hWnd);

        switch (static_cast<int>(msg.wParam)) {
            case ID_DIRECTX: {
                return vireo::Backend::DIRECTX;
            }
            case ID_VULKAN: {
                return vireo::Backend::VULKAN;
            }
            default: {
                return vireo::Backend::UNDEFINED;
            }
        }
    }
}
