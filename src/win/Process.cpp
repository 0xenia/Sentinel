//
// Created by Xenia on 1.09.2024.
//

#include "Process.h"
#include <iostream>
#include <map>
#include <winternl.h>
#pragma comment(lib, "ntdll.lib")

Logger Process::logger;

std::mutex logMutex;

bool Process::IsIntegrityCheckRunning = false;
std::atomic_bool Process::keep_running;


bool Process::ShouldIgnoreProcess(const std::string &ProcessName) {
    const std::vector<std::string> IgnoreList
    {
        "[System Process]",
        "svchost.exe",
        "explorer.exe",
        "conhost.exe",
        "System"
    };

    return std::any_of(IgnoreList.begin(), IgnoreList.end(), [&](const auto &Name) {
        return ProcessName == Name;
    });
}

bool Process::IsProcessRunning(const FProcessList &Proc) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, Proc.pid);
    if (hProcess) {
        CloseHandle(hProcess);
        return true;
    }
    return false;
}


std::vector<FProcessList> Process::GetProcessList() {
    std::vector<FProcessList> ProcessList;
    PROCESSENTRY32 ProcessEntry;

    HANDLE ProcessSnapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (ProcessSnapshotHandle == INVALID_HANDLE_VALUE) return ProcessList;

    ProcessEntry.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(ProcessSnapshotHandle, &ProcessEntry)) {
        CloseHandle(ProcessSnapshotHandle);
        return ProcessList;
    }

    do {
        FProcessList Proc;
        Proc.pid = ProcessEntry.th32ProcessID;
        Proc.name = ProcessEntry.szExeFile;
        Proc.processListName = std::to_string(Proc.pid) + " : " + Proc.name;

        // Create snapshot for modules
        HANDLE ModuleSnapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, Proc.pid);
        if (ModuleSnapshotHandle != INVALID_HANDLE_VALUE) {
            MODULEENTRY32 ModuleEntry;
            ModuleEntry.dwSize = sizeof(MODULEENTRY32);

            if (Module32First(ModuleSnapshotHandle, &ModuleEntry)) {
                Proc.path = ModuleEntry.szExePath;
            } else {
                Proc.path = "Unknown path";
            }
            CloseHandle(ModuleSnapshotHandle);
        } else {
            Proc.path = "Failed to get path";
        }

        if (!ShouldIgnoreProcess(Proc.name)) {
            ProcessList.push_back(Proc);
        }
    } while (Process32Next(ProcessSnapshotHandle, &ProcessEntry));

    CloseHandle(ProcessSnapshotHandle);
    return ProcessList;
}

bool GetTextSectionInfo(HANDLE hProcess, LPVOID &textSectionBase, SIZE_T &textSectionSize) {
    PROCESS_BASIC_INFORMATION pbi;
    ULONG returnLength = 0;
    NTSTATUS status = NtQueryInformationProcess(hProcess, ProcessBasicInformation, &pbi, sizeof(pbi), &returnLength);
    if (status != 0) {
        return false;
    }
    PEB peb;
    if (!ReadProcessMemory(hProcess, pbi.PebBaseAddress, &peb, sizeof(peb), nullptr)) {
        return false;
    }

    PEB_LDR_DATA ldr;
    if (!ReadProcessMemory(hProcess, peb.Ldr, &ldr, sizeof(ldr), nullptr)) {
        return false;
    }

    auto *moduleList = ldr.InMemoryOrderModuleList.Flink;
    LDR_DATA_TABLE_ENTRY firstEntry;

    if (!ReadProcessMemory(hProcess, CONTAINING_RECORD(moduleList, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks),
                           &firstEntry, sizeof(firstEntry), nullptr)) {
        return false;
    }

    PVOID imageBaseAddress = firstEntry.DllBase;

    IMAGE_DOS_HEADER dosHeader;
    if (!ReadProcessMemory(hProcess, imageBaseAddress, &dosHeader, sizeof(dosHeader), nullptr)) {
        return false;
    }

    IMAGE_NT_HEADERS64 ntHeaders;
    PVOID ntHeadersAddress = static_cast<PBYTE>(imageBaseAddress) + dosHeader.e_lfanew;
    if (!ReadProcessMemory(hProcess, ntHeadersAddress, &ntHeaders, sizeof(ntHeaders), nullptr)) {
        return false;
    }

    PVOID sectionHeaderAddress = static_cast<PBYTE>(ntHeadersAddress) + sizeof(ntHeaders);
    for (WORD i = 0; i < ntHeaders.FileHeader.NumberOfSections; ++i) {
        IMAGE_SECTION_HEADER sectionHeader;
        if (!ReadProcessMemory(hProcess, sectionHeaderAddress, &sectionHeader, sizeof(sectionHeader), nullptr)) {
            return false;
        }

        std::string sectionName = std::string(reinterpret_cast<char *>(sectionHeader.Name), 8);
        if (sectionName.find(".text") != std::string::npos) {
            textSectionBase = static_cast<PBYTE>(imageBaseAddress) + sectionHeader.VirtualAddress;
            textSectionSize = sectionHeader.Misc.VirtualSize;
            return true;
        }
        sectionHeaderAddress = static_cast<PBYTE>(sectionHeaderAddress) + sizeof(IMAGE_SECTION_HEADER);
    }

    return false;
}

void Process::EnumMemoryRegions(FProcessList &Proc) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, Proc.pid);
    if (hProcess == nullptr) {
        logger.log("Failed to open process");
        return;
    }

    Proc.memoryRegions.clear();


    // Get the .text section information
    LPVOID textSectionBase = nullptr;
    SIZE_T textSectionSize = 0;
    if (!GetTextSectionInfo(hProcess, textSectionBase, textSectionSize)) {
        logger.log("Failed to get text section");
        return;
    }

    MEMORY_BASIC_INFORMATION mbi;
    LPVOID addr = textSectionBase;
    while (VirtualQueryEx(hProcess, addr, &mbi, sizeof(mbi)) == sizeof(mbi)) {
        if (mbi.State == MEM_COMMIT &&
            mbi.Protect & PAGE_EXECUTE_READ) {
            // Check if the region overlaps with the .text section
            if (static_cast<LPBYTE>(mbi.BaseAddress) < static_cast<LPBYTE>(textSectionBase) + textSectionSize &&
                static_cast<LPBYTE>(mbi.BaseAddress) + mbi.RegionSize > static_cast<LPBYTE>(textSectionBase)) {
                MemoryRegion region;
                region.baseAddress = mbi.BaseAddress;
                region.size = mbi.RegionSize;

                // Optionally, you can read the memory contents here if you want to compare it later
                std::vector<BYTE> memoryContents(mbi.RegionSize);
                SIZE_T bytesRead;
                if (ReadProcessMemory(hProcess, mbi.BaseAddress, memoryContents.data(), mbi.RegionSize, &bytesRead)) {
                    memoryContents.resize(bytesRead);
                    region.contents = std::move(memoryContents);
                    region.hash = CRC32::CalculateCRC32(region.contents);
                    region.hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, Proc.pid);
                } else {
                    region.contents.clear();
                    region.hash = 0;
                }

                // Save the memory region information
                Proc.memoryRegions.push_back(std::move(region));
            }
        }

        // Move to the next region
        addr = static_cast<LPBYTE>(mbi.BaseAddress) + mbi.RegionSize;
    }

    CloseHandle(hProcess);
}


void Process::CheckIntegrity(const FProcessList &Proc) {
    for (const auto &region: Proc.memoryRegions) {
        std::vector<BYTE> currentMemory(region.size);
        SIZE_T bytesRead;

        if (ReadProcessMemory(region.hProcess, region.baseAddress, currentMemory.data(), region.size, &bytesRead) &&
            bytesRead == region.size) {
            if (memcmp(region.contents.data(), currentMemory.data(), region.size) != 0) {
                for (size_t i = 0; i < region.size; ++i) {
                    const uintptr_t address = reinterpret_cast<uintptr_t>(region.baseAddress) + i;

                    if (region.contents[i] != currentMemory[i]) {
                        std::ostringstream oss;
                        oss << "Memory change detected at address: 0x" << std::hex << address
                                << ". Original: 0x" << std::hex << static_cast<int>(region.contents[i])
                                << ", Current: 0x" << std::hex << static_cast<int>(currentMemory[i]);

                        logger.log(oss.str());
                    }
                }
            }
        } else {
            if (!IsProcessRunning(Proc)) {
                keep_running = false;
                logger.log("Process not running");
            }
            std::ostringstream oss;
            oss << "Failed to read memory at address: 0x" << std::hex << reinterpret_cast<uintptr_t>(region.
                baseAddress);
            logger.log(oss.str());
        }
    }
}


void Process::StartIntegrityCheck(const FProcessList &Proc) {
    std::chrono::milliseconds delayInterval(1000);

    logger.log("Integrity check started");
    keep_running = true;
    IsIntegrityCheckRunning = true;

    std::thread integrityCheckThread([Proc, delayInterval] {
        while (keep_running && IsProcessRunning(Proc)) {
            CheckIntegrity(Proc);
            std::this_thread::sleep_for(delayInterval);
        }
        IsIntegrityCheckRunning = false;
        logger.log("Integrity check stopped");
    });

    integrityCheckThread.detach();
}

void Process::StopIntegrityCheck() {
    keep_running = false;
}


