#pragma once
#include "Common.h"

class SystemRepair {
public:
    SystemRepair(const TargetEnvironment& env);
    void Execute();

private:
    void ResetHostsFile();
    void ResetWinsock();
    void ExecuteOfflineSFC();
    void ClearSuspiciousTempPaths();

    TargetEnvironment m_env;
};
