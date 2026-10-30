#pragma once
#include "Common.h"

enum class LogLevel {
    Info,
    Success,
    Warning,
    Error,
    Critical
};

class Logger {
public:
    static void Init(const fs::path& logPath);
    static void Log(LogLevel level, const std::wstring& message);
    static void Raw(const std::wstring& message);

private:
    static std::wofstream logFile;
    static HANDLE hConsole;
};
