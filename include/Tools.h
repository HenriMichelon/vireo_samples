#pragma once

#define offsetof(s,m) ((std::uint32_t)&reinterpret_cast<char const volatile&>((((s*)0)->m)))