//
// Created by Xenia on 1.09.2024.
//

#ifndef UI_H
#define UI_H

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#include <d3d12.h>
#include <dxgi1_4.h>
#include <tchar.h>

struct FrameContext
{
    ID3D12CommandAllocator* CommandAllocator;
    UINT64 FenceValue;
};



class UI {
    public:
    // Data
    static constexpr int NUM_FRAMES_IN_FLIGHT = 3;
    static FrameContext g_frameContext[NUM_FRAMES_IN_FLIGHT];
    static UINT g_frameIndex;

    static constexpr int NUM_BACK_BUFFERS = 3;
    static ID3D12Device* g_pd3dDevice;
    static ID3D12DescriptorHeap* g_pd3dRtvDescHeap;
    static ID3D12DescriptorHeap* g_pd3dSrvDescHeap;
    static ID3D12CommandQueue* g_pd3dCommandQueue;
    static ID3D12GraphicsCommandList* g_pd3dCommandList;
    static ID3D12Fence* g_fence;
    static HANDLE g_fenceEvent;
    static UINT64 g_fenceLastSignaledValue;
    static IDXGISwapChain3* g_pSwapChain;
    static HANDLE g_hSwapChainWaitableObject;

    static ID3D12Resource* g_mainRenderTargetResource[NUM_BACK_BUFFERS];
    static D3D12_CPU_DESCRIPTOR_HANDLE g_mainRenderTargetDescriptor[NUM_BACK_BUFFERS];

    // Forward declarations of helper functions
    static bool CreateDeviceD3D(HWND hWnd);
    static void CleanupDeviceD3D();
    static void CreateRenderTarget();
    static void CleanupRenderTarget();
    static void WaitForLastSubmittedFrame();
    static FrameContext* WaitForNextFrameResources();
    static void ResizeSwapChain(HWND hWnd, int width, int height);
    static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};



#endif //UI_H
