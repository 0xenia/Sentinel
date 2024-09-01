
#include <algorithm>
#include "UI.h"
#include "Lexend.h"
#include <vector>
#include <string>
#include <Windows.h>
#include <winternl.h>

#include <DbgHelp.h>

#include <dwmapi.h>
#include <regex>

#include "Process.h"

#include "Menu.h"

#ifdef _DEBUG
#define DX12_ENABLE_DEBUG_LAYER
#endif

#ifdef DX12_ENABLE_DEBUG_LAYER
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif


// Helper functions


int main() {
    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();

    Process::logger.log("Program started.");

    Menu::Setup();

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    bool done = false;
    while (!done) {
        // Poll and handle messages (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Start the Dear ImGui frame
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        //Rendering
        Menu::DrawUI();

        ImGui::Render();

        FrameContext *frameCtx = UI::WaitForNextFrameResources();
        UINT backBufferIdx = UI::g_pSwapChain->GetCurrentBackBufferIndex();
        frameCtx->CommandAllocator->Reset();

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = UI::g_mainRenderTargetResource[backBufferIdx];
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        UI::g_pd3dCommandList->Reset(frameCtx->CommandAllocator, nullptr);
        UI::g_pd3dCommandList->ResourceBarrier(1, &barrier);

        // Render Dear ImGui graphics
        const float clear_color_with_alpha[4] = {
            clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w
        };
        UI::g_pd3dCommandList->ClearRenderTargetView(UI::g_mainRenderTargetDescriptor[backBufferIdx],
                                                     clear_color_with_alpha, 0,
                                                     nullptr);
        UI::g_pd3dCommandList->OMSetRenderTargets(1, &UI::g_mainRenderTargetDescriptor[backBufferIdx], FALSE, nullptr);
        UI::g_pd3dCommandList->SetDescriptorHeaps(1, &UI::g_pd3dSrvDescHeap);
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), UI::g_pd3dCommandList);
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        UI::g_pd3dCommandList->ResourceBarrier(1, &barrier);
        UI::g_pd3dCommandList->Close();

        UI::g_pd3dCommandQueue->ExecuteCommandLists(
            1, reinterpret_cast<ID3D12CommandList * const*>(&UI::g_pd3dCommandList));

        UI::g_pSwapChain->Present(1, 0); // Present with vsync
        //g_pSwapChain->Present(0, 0); // Present without vsync

        UINT64 fenceValue = UI::g_fenceLastSignaledValue + 1;
        UI::g_pd3dCommandQueue->Signal(UI::g_fence, fenceValue);
        UI::g_fenceLastSignaledValue = fenceValue;
        frameCtx->FenceValue = fenceValue;
    }

    UI::WaitForLastSubmittedFrame();

    // Cleanup
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    UI::CleanupDeviceD3D();
    DestroyWindow(Menu::hwnd);
    ::UnregisterClass(Menu::wc.lpszClassName, Menu::wc.hInstance);

    return 0;
}
