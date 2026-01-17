#pragma once

#ifdef WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef UNICODE
        #define UNICODE
    #endif
    #ifndef _UNICODE
        #define _UNICODE
    #endif
    #define NOMINMAX
    #include <windows.h>
    #include <glm/glm.hpp>
#elifdef __linux__
    #include <QApplication>
#endif
#include <cstdint>
#include <cstddef>

import std;
import glm;
import vireo;
