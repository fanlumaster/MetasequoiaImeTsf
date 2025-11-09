#pragma once

#include <windows.h>
#include <string>
#include <fmt/xchar.h>

template <typename... Args> //
void DebugLog(std::wstring format_str, Args &&...args)
{
#ifdef FANY_DEBUG
    auto msg = fmt::format(format_str, std::forward<Args>(args)...);
    OutputDebugString(msg.c_str());
#endif
}