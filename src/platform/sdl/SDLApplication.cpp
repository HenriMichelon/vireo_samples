/*
 * Copyright (c) 2025-present Henri Michelon
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */
module;
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "Libraries.h"
module samples.sdl;

import vireo.tools;

namespace samples {

    std::shared_ptr<Application> SDLApplication::app{};
    vireo::PlatformWindowHandle SDLApplication::windowHandle{};

    int SDLApplication::run(
        std::shared_ptr<Application> app,
        const uint32_t width,
        const uint32_t height,
        const std::string& name) {
        SDL_Init(SDL_INIT_VIDEO);

        if (!dirExists("shaders")) {
            SDL_ShowSimpleMessageBox(
                SDL_MESSAGEBOX_ERROR,
                    "Shaders directory not found, please run the application from the root of the project and build the 'shaders' target",
                       "Error",
                       nullptr);
            SDL_Quit();
            return 1;
        }

        SDL_WindowFlags flags = SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN;
        if (width == 0 || height == 0) {
            flags |= SDL_WINDOW_FULLSCREEN;
        } else {
            flags |= SDL_WINDOW_RESIZABLE;
        }
        if (!(windowHandle = SDL_CreateWindow(name.c_str(),width,height,flags))) {
            throw vireo::Exception("Error creating SDL window : ", SDL_GetError());
        }

        app->init(vireo::Backend::VULKAN, windowHandle);
        try {
            app->onInit();
            SDL_ShowWindow(windowHandle);
            auto quit{false};
            while (!quit) {
                SDL_Event event;
                while (SDL_PollEvent(&event)) {
                    switch (event.type) {
                    case SDL_EVENT_QUIT:
                    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                        quit = true;
                        break;
                    case SDL_EVENT_WINDOW_RESIZED:
                        app->onResize();
                        break;
                    case SDL_EVENT_KEY_DOWN:
                        app->onKeyDown(static_cast<uint32_t>(event.key.scancode));
                        break;
                    case SDL_EVENT_KEY_UP:
                        app->onKeyUp(static_cast<uint32_t>(event.key.scancode));
                        break;
                    default:
                        break;
                    }
                }
                app->onUpdate();
                app->onRender();
            }
            app->onDestroy();
            SDL_DestroyWindow(windowHandle);
        } catch (vireo::Exception& e) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, e.what(), "Fatal error", nullptr);
            return 1;
        }
        app.reset();
        SDL_Quit();
        return 0;
    }

    bool SDLApplication::dirExists(const std::string& path) {
        namespace fs = std::filesystem;
        return fs::exists(path) && fs::is_directory(path);
    }

}
