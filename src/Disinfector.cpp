#include "Disinfector.h"
#include "Logger.h"

Disinfector::Disinfector(const TargetEnvironment& env, RegistryManager& regMgr)
    : m_env(env), m_reg(regMgr) {}

void Disinfector::SanitizeFile(const fs::path& path) {
    if (!fs::exists(path)) return;
    try {
        SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_NORMAL);
        fs::remove_all(path);
        Logger::Log(LogLevel::Success, L"Neutralized file: " + path.wstring());
    } catch (const std::exception& e) {
        Logger::Log(LogLevel::Error, L"Failed to remove file " + path.wstring() + L": " + ToWide(e.what()));
    }
}

void Disinfector::CleanIFEO() {
    Logger::Log(LogLevel::Info, L"Scanning Image File Execution Options (Debugger Hijacks)...");
    HKEY hSoftware = m_reg.GetSoftwareKey();
    if (!hSoftware) return;

    HKEY hIFEO = NULL;
    std::wstring ifeoPath = L"Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options";
    if (RegOpenKeyExW(hSoftware, ifeoPath.c_str(), 0, KEY_ALL_ACCESS, &hIFEO) != ERROR_SUCCESS) {
        Logger::Log(LogLevel::Warning, L"Unable to open IFEO registry branch.");
        return;
    }

    const std::vector<std::wstring> targets = {
        L"taskmgr.exe", L"cmd.exe", L"regedit.exe", L"msconfig.exe",
        L"powershell.exe", L"SecurityHealthSystray.exe", L"ProcessHacker.exe"
    };

    for (const auto& target : targets) {
        HKEY hTargetKey = NULL;
        if (RegOpenKeyExW(hIFEO, target.c_str(), 0, KEY_ALL_ACCESS, &hTargetKey) == ERROR_SUCCESS) {
            DWORD type = 0;
            if (RegQueryValueExW(hTargetKey, L"Debugger", NULL, &type, NULL, NULL) == ERROR_SUCCESS) {
                RegDeleteValueW(hTargetKey, L"Debugger");
                Logger::Log(LogLevel::Critical, L"REMOVED MALICIOUS DEBUGGER HIJACK: " + target);
            }
            RegCloseKey(hTargetKey);
        }
    }
    RegCloseKey(hIFEO);
}

void Disinfector::NeutralizeWinlogonHijacks() {
    Logger::Log(LogLevel::Info, L"Verifying Winlogon Core Integrity...");
    HKEY hSoftware = m_reg.GetSoftwareKey();
    if (!hSoftware) return;

    HKEY hWinlogon = NULL;
    std::wstring wlPath = L"Microsoft\\Windows NT\\CurrentVersion\\Winlogon";
    if (RegOpenKeyExW(hSoftware, wlPath.c_str(), 0, KEY_ALL_ACCESS, &hWinlogon) != ERROR_SUCCESS) {
        Logger::Log(LogLevel::Error, L"Failed to access Winlogon key.");
        return;
    }

    const wchar_t* expectedShell = L"explorer.exe";
    std::wstring defaultUserInit = m_env.targetSystem32Dir.wstring() + L"\\userinit.exe,";

    RegSetValueExW(hWinlogon, L"Shell", 0, REG_SZ, (const BYTE*)expectedShell, (DWORD)((wcslen(expectedShell) + 1) * sizeof(wchar_t)));
    RegSetValueExW(hWinlogon, L"Userinit", 0, REG_SZ, (const BYTE*)defaultUserInit.c_str(), (DWORD)((defaultUserInit.length() + 1) * sizeof(wchar_t)));
    
    RegDeleteValueW(hWinlogon, L"Taskman");
    RegCloseKey(hWinlogon);

    Logger::Log(LogLevel::Success, L"Winlogon Shell and Userinit parameters reset to baseline.");
}

void Disinfector::CleanRunKeys() {
    Logger::Log(LogLevel::Info, L"Scanning Persistence Run/RunOnce Registry Keys...");
    HKEY hSoftware = m_reg.GetSoftwareKey();
    if (!hSoftware) return;

    std::vector<std::wstring> runBranches = {
        L"Microsoft\\Windows\\CurrentVersion\\Run",
        L"Microsoft\\Windows\\CurrentVersion\\RunOnce",
        L"Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\Run"
    };

    for (const auto& branch : runBranches) {
        HKEY hKey = NULL;
        if (RegOpenKeyExW(hSoftware, branch.c_str(), 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {
            DWORD valueCount = 0;
            DWORD maxValueNameLen = 0;
            DWORD maxValueDataLen = 0;

            if (RegQueryInfoKeyW(hKey, NULL, NULL, NULL, NULL, NULL, NULL, &valueCount, &maxValueNameLen, &maxValueDataLen, NULL, NULL) == ERROR_SUCCESS) {
                std::vector<wchar_t> nameBuf(maxValueNameLen + 1);
                std::vector<BYTE> dataBuf(maxValueDataLen + 1);

                for (DWORD i = 0; i < valueCount;) {
                    DWORD nameLen = (DWORD)nameBuf.size();
                    DWORD dataLen = (DWORD)dataBuf.size();
                    DWORD type = 0;

                    LONG status = RegEnumValueW(hKey, i, nameBuf.data(), &nameLen, NULL, &type, dataBuf.data(), &dataLen);
                    if (status != ERROR_SUCCESS) {
                        ++i;
                        continue;
                    }

                    std::wstring valName(nameBuf.data(), nameLen);
                    std::wstring valData = (type == REG_SZ || type == REG_EXPAND_SZ) ? reinterpret_cast<wchar_t*>(dataBuf.data()) : L"";

                    bool suspicious = false;
                    if (valData.find(L"AppData") != std::wstring::npos ||
                        valData.find(L"Temp") != std::wstring::npos ||
                        valData.find(L"wscript") != std::wstring::npos ||
                        valData.find(L"powershell") != std::wstring::npos ||
                        valData.find(L".vbs") != std::wstring::npos) {
                        suspicious = true;
                    }

                    if (suspicious) {
                        Logger::Log(LogLevel::Critical, L"Eliminating malicious Run payload: " + valName + L" -> " + valData);
                        RegDeleteValueW(hKey, valName.c_str());
                    } else {
                        ++i;
                    }
                }
            }
            RegCloseKey(hKey);
        }
    }
}

void Disinfector::NeutralizeMaliciousServices() {
    Logger::Log(LogLevel::Info, L"Inspecting Services hive entries...");
    HKEY hSystem = m_reg.GetSystemKey();
    if (!hSystem) return;

    HKEY hServices = NULL;
    std::wstring servicesPath = L"ControlSet001\\Services";
    if (RegOpenKeyExW(hSystem, servicesPath.c_str(), 0, KEY_ALL_ACCESS, &hServices) != ERROR_SUCCESS) {
        servicesPath = L"CurrentControlSet\\Services";
        if (RegOpenKeyExW(hSystem, servicesPath.c_str(), 0, KEY_ALL_ACCESS, &hServices) != ERROR_SUCCESS) {
            Logger::Log(LogLevel::Warning, L"Services registry node not found in loaded SYSTEM hive.");
            return;
        }
    }

    DWORD subKeys = 0;
    DWORD maxSubKeyLen = 0;
    if (RegQueryInfoKeyW(hServices, NULL, NULL, NULL, &subKeys, &maxSubKeyLen, NULL, NULL, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
        std::vector<wchar_t> keyNameBuf(maxSubKeyLen + 1);

        for (DWORD i = 0; i < subKeys; ++i) {
            DWORD len = (DWORD)keyNameBuf.size();
            if (RegEnumKeyExW(hServices, i, keyNameBuf.data(), &len, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
                std::wstring srvName(keyNameBuf.data(), len);
                HKEY hSrvInstance = NULL;
                if (RegOpenKeyExW(hServices, srvName.c_str(), 0, KEY_ALL_ACCESS, &hSrvInstance) == ERROR_SUCCESS) {
                    wchar_t imagePath[MAX_PATH * 2] = {0};
                    DWORD dataSize = sizeof(imagePath);
                    DWORD type = 0;

                    if (RegQueryValueExW(hSrvInstance, L"ImagePath", NULL, &type, (LPBYTE)imagePath, &dataSize) == ERROR_SUCCESS) {
                        std::wstring pathStr = imagePath;
                        if (pathStr.find(L"AppData\\Local\\Temp") != std::wstring::npos ||
                            pathStr.find(L"Users\\Public") != std::wstring::npos) {
                            Logger::Log(LogLevel::Critical, L"Disabling Rogue Service: " + srvName + L" (" + pathStr + L")");
                            DWORD disabledState = 4; // Disabled
                            RegSetValueExW(hSrvInstance, L"Start", 0, REG_DWORD, (const BYTE*)&disabledState, sizeof(DWORD));
                        }
                    }
                    RegCloseKey(hSrvInstance);
                }
            }
        }
    }
    RegCloseKey(hServices);
}

void Disinfector::NeutralizeScheduledTasks() {
    Logger::Log(LogLevel::Info, L"Auditing Windows Task Scheduler Files directly on disk...");
    fs::path tasksFolder = m_env.targetSystem32Dir / L"Tasks";
    if (!fs::exists(tasksFolder)) return;

    try {
        for (const auto& entry : fs::recursive_directory_iterator(tasksFolder, fs::directory_options::skip_permission_denied)) {
            if (entry.is_regular_file()) {
                std::ifstream file(entry.path(), std::ios::binary);
                std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                file.close();

                if (content.find(".vbs") != std::string::npos || 
                    content.find(".bat") != std::string::npos ||
                    content.find("powershell.exe -enc") != std::string::npos ||
                    content.find("AppData\\Roaming") != std::string::npos) {
                    Logger::Log(LogLevel::Critical, L"Removing Infected Scheduled Task File: " + entry.path().filename().wstring());
                    SanitizeFile(entry.path());
                }
            }
        }
    } catch (const std::exception& e) {
        Logger::Log(LogLevel::Warning, L"Error traversing scheduled tasks: " + ToWide(e.what()));
    }
}

void Disinfector::Execute() {
    CleanIFEO();
    NeutralizeWinlogonHijacks();
    CleanRunKeys();
    NeutralizeMaliciousServices();
    NeutralizeScheduledTasks();
}
