#pragma once

#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <memory>
#include <sstream>
#include <fstream>

namespace fs = std::filesystem;

struct TargetEnvironment {
    bool isWinPE;
    fs::path targetSystemDrive;
    fs::path targetWindowsDir;
    fs::path targetSystem32Dir;
};

std::wstring ToWide(const std::string& str);
std::string ToNarrow(const std::wstring& wstr);
