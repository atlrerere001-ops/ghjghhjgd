#include "Common.h"
#include "Logger.h"
#include "Privileges.h"
#include "TargetDetector.h"
#include "RegistryManager.h"
#include "Disinfector.h"
#include "SystemRepair.h"

void PrintBanner() {
    Logger::Raw(L"=============================================================");
    Logger::Raw(L"        AEGIS ULTIMATE RECOVERY & DISINFECTION ENGINE       ");
    Logger::Raw(L"              Built for Live OS & WinRE/WinPE               ");
    Logger::Raw(L"=============================================================");
}

int wmain(int argc, wchar_t* argv[]) {
    fs::path exeDir = fs::current_path();
    Logger::Init(exeDir / L"Aegis_Execution.log");

    PrintBanner();

    Logger::Log(LogLevel::Info, L"Initializing Aegis Core...");

    if (!Privileges::ElevateCurrentProcess()) {
        Logger::Log(LogLevel::Critical, L"Permission elevation phase could not be completed.");
    }

    TargetEnvironment env;
    if (!TargetDetector::Detect(env)) {
        Logger::Log(LogLevel::Critical, L"System target detection aborted. Terminating.");
        return 1;
    }

    Logger::Log(LogLevel::Info, L"Target System Drive: " + env.targetSystemDrive.wstring());
    Logger::Log(LogLevel::Info, L"Target Windows Directory: " + env.targetWindowsDir.wstring());

    RegistryManager regMgr(env);
    Logger::Log(LogLevel::Info, L"Mounting critical system registry structures...");
    if (!regMgr.MountTargetHives()) {
        Logger::Log(LogLevel::Critical, L"Failed to mount registry hives. Some malware might be locking hives exclusively.");
    } else {
        Logger::Log(LogLevel::Success, L"Hives successfully attached for surgery.");
        
        Logger::Log(LogLevel::Info, L"--- [PHASE 1: DISINFECTION & AUTORUN CLEANING] ---");
        Disinfector disinfector(env, regMgr);
        disinfector.Execute();
    }

    Logger::Log(LogLevel::Info, L"--- [PHASE 2: SUBSYSTEM RESTORATION & INTEGRITY] ---");
    SystemRepair repair(env);
    repair.Execute();

    Logger::Log(LogLevel::Info, L"Unmounting registry to commit transactions to physical storage...");
    regMgr.UnmountTargetHives();

    Logger::Log(LogLevel::Success, L"Operations fully finalized. The operating system is sanitized.");
    Logger::Log(LogLevel::Info, L"You may now safely reboot into normal Windows.");
    
    return 0;
}
