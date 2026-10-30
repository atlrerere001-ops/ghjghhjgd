#include "RegistryManager.h"
#include "Logger.h"

ScopedHive::ScopedHive(HKEY root, const std::wstring& subkey, const fs::path& filePath) 
    : m_root(root), m_subkey(subkey), m_hKey(NULL), m_mounted(false) 
{
    if (!fs::exists(filePath)) {
        Logger::Log(LogLevel::Error, L"Hive file not found: " + filePath.wstring());
        return;
    }

    LONG status = RegLoadKeyW(m_root, m_subkey.c_str(), filePath.c_str());
    if (status != ERROR_SUCCESS) {
        Logger::Log(LogLevel::Error, L"RegLoadKeyW failed for " + m_subkey + L". Error code: " + std::to_wstring(status));
        return;
    }

    status = RegOpenKeyExW(m_root, m_subkey.c_str(), 0, KEY_ALL_ACCESS, &m_hKey);
    if (status != ERROR_SUCCESS) {
        Logger::Log(LogLevel::Error, L"Failed to open loaded hive: " + m_subkey);
        RegUnLoadKeyW(m_root, m_subkey.c_str());
        return;
    }

    m_mounted = true;
    Logger::Log(LogLevel::Success, L"Successfully mounted hive: " + m_subkey);
}

ScopedHive::~ScopedHive() {
    if (m_mounted) {
        if (m_hKey) {
            RegCloseKey(m_hKey);
            m_hKey = NULL;
        }
        LONG status = RegUnLoadKeyW(m_root, m_subkey.c_str());
        if (status == ERROR_SUCCESS) {
            Logger::Log(LogLevel::Info, L"Unmounted hive: " + m_subkey);
        } else {
            Logger::Log(LogLevel::Warning, L"Failed to cleanly unmount hive: " + m_subkey + L". Err: " + std::to_wstring(status));
        }
    }
}

bool ScopedHive::IsMounted() const {
    return m_mounted;
}

HKEY ScopedHive::GetKey() const {
    return m_hKey;
}

RegistryManager::RegistryManager(const TargetEnvironment& env) 
    : m_env(env), m_hSoftware(NULL), m_hSystem(NULL) {}

RegistryManager::~RegistryManager() {
    UnmountTargetHives();
}

bool RegistryManager::MountTargetHives() {
    if (!m_env.isWinPE) {
        LONG res1 = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE", 0, KEY_ALL_ACCESS, &m_hSoftware);
        LONG res2 = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM", 0, KEY_ALL_ACCESS, &m_hSystem);
        return (res1 == ERROR_SUCCESS && res2 == ERROR_SUCCESS);
    }

    fs::path configDir = m_env.targetSystem32Dir / L"config";
    fs::path softwarePath = configDir / L"SOFTWARE";
    fs::path systemPath = configDir / L"SYSTEM";

    m_softwareHive = std::make_unique<ScopedHive>(HKEY_LOCAL_MACHINE, L"AEGIS_OFFLINE_SOFTWARE", softwarePath);
    m_systemHive = std::make_unique<ScopedHive>(HKEY_LOCAL_MACHINE, L"AEGIS_OFFLINE_SYSTEM", systemPath);

    if (m_softwareHive->IsMounted()) {
        m_hSoftware = m_softwareHive->GetKey();
    }
    if (m_systemHive->IsMounted()) {
        m_hSystem = m_systemHive->GetKey();
    }

    return (m_hSoftware != NULL && m_hSystem != NULL);
}

void RegistryManager::UnmountTargetHives() {
    if (!m_env.isWinPE) {
        if (m_hSoftware) RegCloseKey(m_hSoftware);
        if (m_hSystem) RegCloseKey(m_hSystem);
        m_hSoftware = NULL;
        m_hSystem = NULL;
        return;
    }

    m_softwareHive.reset();
    m_systemHive.reset();
    m_hSoftware = NULL;
    m_hSystem = NULL;
}
