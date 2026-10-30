#include "Logger.h"

std::wofstream Logger::logFile;
HANDLE Logger::hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

void Logger::Init(const fs::path& logPath) {
    logFile.open(logPath, std::ios::out | std::ios::app);
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
}

void Logger::Log(LogLevel level, const std::wstring& message) {
    WORD color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    std::wstring prefix = L"[INFO] ";

    switch (level) {
    case LogLevel::Info:
        color = FOREGROUND_INTENSITY | FOREGROUND_BLUE | FOREGROUND_GREEN;
        prefix = L"[*] ";
        break;
    case LogLevel::Success:
        color = FOREGROUND_INTENSITY | FOREGROUND_GREEN;
        prefix = L"[+] ";
        break;
    case LogLevel::Warning:
        color = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN;
        prefix = L"[!] ";
        break;
    case LogLevel::Error:
        color = FOREGROUND_INTENSITY | FOREGROUND_RED;
        prefix = L"[-] ";
        break;
    case LogLevel::Critical:
        color = BACKGROUND_RED | FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        prefix = L"[CRITICAL] ";
        break;
    }

    SetConsoleTextAttribute(hConsole, color);
    std::wcout << prefix;
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    std::wcout << message << std::endl;

    if (logFile.is_open()) {
        logFile << prefix << message << std::endl;
        logFile.flush();
    }
}

void Logger::Raw(const std::wstring& message) {
    std::wcout << message << std::endl;
    if (logFile.is_open()) {
        logFile << message << std::endl;
        logFile.flush();
    }
}
