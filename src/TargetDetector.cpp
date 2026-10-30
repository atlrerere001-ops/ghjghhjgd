#include "TargetDetector.h"
#include "Logger.h"

bool TargetDetector::CheckWinPE() {
    HKEY hKey = NULL;
    LONG result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\MiniNT", 0, KEY_READ, &hKey);
    if (result == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return true;
    }
    return false;
}

bool TargetDetector::FindWindowsPartition(TargetEnvironment& env) {
    if (!env.isWinPE) {
        wchar_t winDir[MAX_PATH];
        if (GetWindowsDirectoryW(winDir, MAX_PATH) == 0) {
            return false;
        }
        env.targetWindowsDir = winDir;
        env.targetSystemDrive = env.targetWindowsDir.root_path();
        env.targetSystem32Dir = env.targetWindowsDir / L"System32";
        return true;
    }

    Logger::Log(LogLevel::Info, L"WinPE/WinRE environment detected. Scanning drives for offline Windows...");

    wchar_t driveLetter = L'C';
    for (; driveLetter <= L'Z'; ++driveLetter) {
        std::wstring drivePath = std::wstring(1, driveLetter) + L":\\";
        fs::path candidate = fs::path(drivePath) / L"Windows";
        fs::path kernelCandidate = candidate / L"System32" / L"ntoskrnl.exe";

        if (fs::exists(kernelCandidate)) {
            Logger::Log(LogLevel::Success, L"Found installed Windows OS at: " + candidate.wstring());
            env.targetSystemDrive = drivePath;
            env.targetWindowsDir = candidate;
            env.targetSystem32Dir = candidate / L"System32";
            return true;
        }
    }

    Logger::Log(LogLevel::Critical, L"No valid offline Windows installation could be located!");
    return false;
}

bool TargetDetector::Detect(TargetEnvironment& env) {
    env.isWinPE = CheckWinPE();
    return FindWindowsPartition(env);
}
