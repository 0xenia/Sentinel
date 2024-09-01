//
// Created by Xenia on 1.09.2024.
//

#ifndef PROCESS_H
#define PROCESS_H
#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <TlHelp32.h>
#include <psapi.h>
#include <algorithm>
#include "CRC32.h"
#include "Logger.h"
#include <mutex>
#include <thread>
#include <future>
#include <atomic>



struct MemoryRegion {
    LPVOID baseAddress;
    SIZE_T size;
    std::vector<BYTE> contents;
    DWORD hash;
    HANDLE hProcess;
};

struct FProcessList {
    DWORD pid{};
    std::string name;
    std::string processListName;
    std::string path;
    std::vector<MemoryRegion> memoryRegions;
};


class Process {
public:

    static Logger logger;

    static bool ShouldIgnoreProcess(const std::string &ProcessName);

    static std::vector<FProcessList> GetProcessList();

    static void EnumMemoryRegions(FProcessList& Proc);

    static void CheckIntegrity(const FProcessList& Proc);

    static void StartIntegrityCheck(const FProcessList &Proc);

    static void StopIntegrityCheck();

    static bool IsProcessRunning(const FProcessList &Proc);

    static bool IsIntegrityCheckRunning;

    static std::atomic_bool keep_running;
};


#endif //PROCESS_H
