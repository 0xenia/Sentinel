//
// Created by Xenia on 1.09.2024.
//

#include "Menu.h"

#include <dwmapi.h>

#include "Process.h"
#include <regex>
#include "Lexend.h"

WNDCLASSEX Menu::wc = {};

HWND Menu::hwnd = nullptr;

int selected_process = -1;

std::vector<FProcessList> processes;
std::vector<std::string> processNames;

bool auto_scroll;

std::vector<std::string> filteredProcessNames;

void Menu::DrawUI() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(300, ImGui::GetIO().DisplaySize.y)); // Sol panel
    ImGui::Begin("Process Selector", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    if (processes.empty()) {
        processes = Process::GetProcessList();
    }

    if (processNames.empty() && !processes.empty()) {
        for (const auto &proc: processes) {
            processNames.emplace_back(proc.processListName.c_str());
        }
    }


    static char filterBuffer[256] = "";
    static int selectedProcessIndex = -1;
    bool comboOpen = ImGui::BeginCombo("##processlist",
                                       selectedProcessIndex >= 0
                                           ? processNames[selectedProcessIndex].c_str()
                                           : "Select Process");

    if (comboOpen) {
        ImGui::InputText("##Filter", filterBuffer, IM_ARRAYSIZE(filterBuffer));


        const std::string pattern(filterBuffer);
        const std::regex regexPattern(pattern, std::regex_constants::icase);


        filteredProcessNames.clear();


        for (size_t i = 0; i < processNames.size(); i++) {
            if (std::regex_search(processNames[i], regexPattern)) {
                if (ImGui::Selectable(processNames[i].c_str(), selectedProcessIndex == i)) {
                    selectedProcessIndex = static_cast<int>(i);
                    selected_process = selectedProcessIndex;
                }
            }
        }

        ImGui::EndCombo();
    }

    ImGui::SameLine();
    if (ImGui::Button("Refresh")) {
        selected_process = -1;
        processNames.clear();
        processes.clear();
    }

    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(300, 0));
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x - 300, ImGui::GetIO().DisplaySize.y - 150));
    ImGui::Begin("Main Content", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    ImGui::Text("Process Details and Actions");
    ImGui::Separator();

    if (!processes.empty() && selected_process < processes.size()) {
        FProcessList &selectedProc = processes[selected_process];
        ImGui::TextWrapped("Process ID: %d", selectedProc.pid);
        ImGui::TextWrapped("Process Name: %s", selectedProc.name.c_str());
        ImGui::TextWrapped("Process Path: %s", selectedProc.path.c_str());

        ImGui::Separator();

        if (ImGui::Button("Take Memory Snapshot")) {
            Process::EnumMemoryRegions(selectedProc);
        }
        ImGui::SameLine();
        if (!Process::IsIntegrityCheckRunning) {
            if (ImGui::Button("Start Memory Watcher")) {
                Process::StartIntegrityCheck(selectedProc);
            }
        } else {
            if (ImGui::Button("Stop Memory Watcher")) {
                Process::StopIntegrityCheck();
            }
        }

        ImGui::SameLine();
        ImGui::Checkbox("Auto Scroll Logs", &auto_scroll);
        ImGui::SameLine();
        if (ImGui::Button("Copy Logs to Clipboard"))
        {
            if (!Process::logger.getLogs().empty())
            {
                std::string allLogs;
                for (const auto& Logs : Process::logger.getLogs()) {
                    allLogs += Logs + "\n";
                }
                ImGui::SetClipboardText(allLogs.c_str());
            }
        }

        ImGui::Separator();

        ImGui::Text("Memory Regions Being Monitored:");
        for (const auto &region: selectedProc.memoryRegions) {
            ImGui::TextWrapped("Base Address: 0x%p, Size: 0x%lx, Hash: 0x%lx",
                               region.baseAddress, static_cast<uint32_t>(region.size), region.hash);
        }
    } else {
        ImGui::Text("No process selected.");
    }

    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(300, ImGui::GetIO().DisplaySize.y - 150));
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x - 300, 150));
    ImGui::Begin("Logs", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    ImGui::Separator();
    ImGui::Text("Logs:");
    for (const auto &Logs: Process::logger.getLogs()) {
        ImGui::TextWrapped(Logs.c_str());
    }


    if (auto_scroll) {
        float scrollY = ImGui::GetScrollY();
        if (ImGui::GetScrollMaxY() > scrollY) {
            ImGui::SetScrollHereY(1.0f); // Scroll to bottom
        }
    }


    ImGui::End();
}

void Menu::Setup() {
    wc = {
        sizeof(WNDCLASSEX), CS_CLASSDC, UI::WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr,
        nullptr,
        _T("Sentinel"), nullptr
    };
    ::RegisterClassEx(&wc);
    hwnd = CreateWindowExA(WS_EX_DLGMODALFRAME, wc.lpszClassName, _T("Sentinel"), WS_OVERLAPPEDWINDOW, 100, 100,
                                  1280, 800, nullptr, nullptr, wc.hInstance, nullptr);
    BOOL USE_DARK_MODE = true;
    BOOL SET_IMMERSIVE_DARK_MODE_SUCCESS = SUCCEEDED(DwmSetWindowAttribute(
        hwnd, DWMWINDOWATTRIBUTE::DWMWA_USE_IMMERSIVE_DARK_MODE,
        &USE_DARK_MODE, sizeof(USE_DARK_MODE)));

    COLORREF DARK_COLOR = 0x00505050;
    BOOL SET_CAPTION_COLOR = SUCCEEDED(DwmSetWindowAttribute(
        hwnd, DWMWINDOWATTRIBUTE::DWMWA_CAPTION_COLOR,
        &DARK_COLOR, sizeof(DARK_COLOR)));

    // Initialize Direct3D
    if (!UI::CreateDeviceD3D(hwnd)) {
        UI::CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return;
    }

    // Show the window
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    const ImGuiIO &io = ImGui::GetIO();
    (void) io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    //    ImGui::StyleColorsDark();
    ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX12_Init(UI::g_pd3dDevice, UI::NUM_FRAMES_IN_FLIGHT,
                        DXGI_FORMAT_R8G8B8A8_UNORM, UI::g_pd3dSrvDescHeap,
                        UI::g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
                        UI::g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart());

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //    io.Fonts->AddFontDefault();
    io.Fonts->AddFontFromMemoryCompressedTTF(&Lexend_compressed_data, Lexend_compressed_size, 15.0f, nullptr,
                                             io.Fonts->GetGlyphRangesDefault());
}

