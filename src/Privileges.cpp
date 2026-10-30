#include "Privileges.h"
#include "Logger.h"

bool Privileges::ElevateCurrentProcess() {
    HANDLE hToken = NULL;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        Logger::Log(LogLevel::Error, L"Failed to open process token.");
        return false;
    }

    const std::vector<std::wstring> privileges = {
        SE_BACKUP_NAME,
        SE_RESTORE_NAME,
        SE_SHUTDOWN_NAME,
        SE_DEBUG_NAME,
        SE_TAKE_OWNERSHIP_NAME,
        SE_SECURITY_NAME,
        SE_SYSTEM_ENVIRONMENT_NAME
    };

    bool allSuccess = true;
    for (const auto& priv : privileges) {
        TOKEN_PRIVILEGES tp;
        LUID luid;

        if (!LookupPrivilegeValueW(NULL, priv.c_str(), &luid)) {
            allSuccess = false;
            continue;
        }

        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luid;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL)) {
            allSuccess = false;
        }

        if (GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
            allSuccess = false;
        }
    }

    CloseHandle(hToken);
    if (allSuccess) {
        Logger::Log(LogLevel::Success, L"All critical security tokens successfully acquired.");
    } else {
        Logger::Log(LogLevel::Warning, L"Running with partially granted token privileges (normal if in Live OS without full elevation).");
    }
    return true;
}
