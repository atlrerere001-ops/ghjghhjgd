#pragma once
#include "Common.h"

class TargetDetector {
public:
    static bool Detect(TargetEnvironment& env);

private:
    static bool CheckWinPE();
    static bool FindWindowsPartition(TargetEnvironment& env);
};
