#pragma once
#include "Common.h"

class ScopedHive {
public:
    ScopedHive(HKEY root, const std::wstring& subkey, const fs::path& filePath);
    ~ScopedHive();
    bool IsMounted() const;
    HKEY GetKey() const;

private:
    HKEY m_root;
    std::wstring m_subkey;
    HKEY m_hKey;
    bool m_mounted;
};

class RegistryManager {
public:
    RegistryManager(const TargetEnvironment& env);
    ~RegistryManager();

    bool MountTargetHives();
    void UnmountTargetHives();

    HKEY GetSoftwareKey() const { return m_hSoftware; }
    HKEY GetSystemKey() const { return m_hSystem; }

private:
    TargetEnvironment m_env;
    std::unique_ptr<ScopedHive> m_softwareHive;
    std::unique_ptr<ScopedHive> m_systemHive;
    HKEY m_hSoftware;
    HKEY m_hSystem;
};
