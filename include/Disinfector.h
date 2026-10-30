#pragma once
#include "Common.h"
#include "RegistryManager.h"

class Disinfector {
public:
    Disinfector(const TargetEnvironment& env, RegistryManager& regMgr);
    void Execute();

private:
    void CleanIFEO();
    void NeutralizeWinlogonHijacks();
    void CleanRunKeys();
    void NeutralizeMaliciousServices();
    void NeutralizeScheduledTasks();
    void SanitizeFile(const fs::path& path);

    TargetEnvironment m_env;
    RegistryManager& m_reg;
};
