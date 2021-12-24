#pragma once

/*
 * 1.1.0
 * Added IPC feature to prevent plugins from initializing D3D individually;
 *
 * 1.0.0
 * Basic ImGui implementation;
 * Callback hook setup;
 * RAW_D3D definition allowed for direct control without ImGui;
 *
 */


// AdditionalInclude
#include <algorithm>
#include <d3d11.h>
#include <type_traits>
#include <vector>

#ifndef RAW_D3D
// imgui
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#endif

// DKUtil
#include "Hook.hpp"
#include "Logger.hpp"
#include "Utility.hpp"

#pragma comment ( lib, "d3d11.lib" )


#define VMT_PRESENT_INDEX   0x8
#define WM_DKU_G_ADD        WM_USER + 103
#define WM_DKU_G_REMOVE     WM_USER + 108
#define WM_DKU_G_QUERY      WM_USER + 109
#define LR_DKU_G_SUCCESS    1
#define LR_DKU_G_FAILURE    0
#define LR_DKU_G_DISABLE    -1


#define DKU_G(GLOBAL)       DKUtil::GUI::detail::global::GLOBAL
#define DKU_G_DEVICE        DKU_G(D3D11.Device)
#define DKU_G_SWAPCHAIN     DKU_G(D3D11.SwapChain)
#define DKU_G_CONTEXT       DKU_G(D3D11.DeviceContext)
#define DKU_G_TARGETVIEW    DKU_G(D3D11.RenderView)
#define DKU_G_TARGETHWND    DKU_G(D3D11.TargetWindow)


namespace DKUtil::GUI
{
    extern std::int64_t AddCallback(Hook::FuncInfo) noexcept;
    extern std::int64_t RemoveCallback(Hook::FuncInfo) noexcept;


    namespace detail
    {
        using namespace DKUtil::Alias;

        using WndProcFunc = std::add_pointer_t<LRESULT((__stdcall)(HWND, UINT, WPARAM, LPARAM))>;
        using PresentFunc = std::add_pointer_t<HRESULT(IDXGISwapChain*, UINT, UINT)>;
#ifndef RAW_D3D
        using CallbackFunc = std::add_pointer_t<void()>; // receiver unknown of d3d state
#else
        using CallbackFunc = std::add_pointer_t<void(IDXGISwapChain*, UINT, UINT)>;
#endif

        namespace global
        {
            // d3d
            struct D3D
            {
                ID3D11Device*           Device = nullptr;
                IDXGISwapChain*         SwapChain = nullptr;
                ID3D11DeviceContext*    DeviceContext = nullptr;
                ID3D11RenderTargetView* RenderView = nullptr;
                HWND				    TargetWindow = nullptr;
#ifndef RAW_D3D
                ImGuiContext*           ImGuiCtx = nullptr;
                ImGuiMemAllocFunc       AllocFunc;
                ImGuiMemFreeFunc        FreeFunc;
                void*                   BackEnd = nullptr;
#endif
            };
            D3D                         D3D11;

            // ipc
            HWND                        IPCHost = nullptr;
            bool                        SelfHost = false;
            std::string                 HostName = Version::PROJECT.data();
            std::wstring                WndClass = L"DKU_G_base";
            std::wstring                WndTitle = L"DKU_G_host";
            const char*                 LastSender = Version::PROJECT.data();

            // gui
            WndProcFunc                 OldWndProc;
            PresentFunc                 OldPresent;
            std::vector<CallbackFunc>   CallbackQueue;

            // hook
            HookHandle                  _Hook_Present;
        } // namespace Global


        LRESULT __stdcall IPCWndProc([[maybe_unused]] HWND a_hWnd, UINT a_uMsg, WPARAM a_wParam, LPARAM a_lParam) noexcept
        {
            switch (a_uMsg) {
            case WM_DKU_G_ADD:
            {
                global::LastSender = reinterpret_cast<const char*>(a_lParam);
                auto func = reinterpret_cast<Hook::FuncInfo*>(a_wParam);

                if (!func) {
                    return LR_DKU_G_FAILURE;
                }

                return AddCallback(*func);
            }
            case WM_DKU_G_REMOVE:
            {
                global::LastSender = reinterpret_cast<const char*>(a_lParam);
                auto func = reinterpret_cast<Hook::FuncInfo*>(a_wParam);

                if (!func) {
                    return LR_DKU_G_FAILURE;
                }

                return RemoveCallback(*func);
            }
            case WM_DKU_G_QUERY:
            {
                *reinterpret_cast<std::string*>(a_wParam) = global::HostName;
                *reinterpret_cast<global::D3D*>(a_lParam) = global::D3D11;
                return global::HostName.c_str() ? LR_DKU_G_SUCCESS : LR_DKU_G_FAILURE;
            }
            default:
                return LR_DKU_G_FAILURE;
            }
        }


        HRESULT __cdecl Hook_Present(IDXGISwapChain* a_this, UINT a_syncInterval, UINT a_flags) noexcept
        {
            using namespace global;

            static std::once_flag WndProcInit;
            std::call_once(WndProcInit, [&]()
                {
                    D3D11.SwapChain = a_this;
                    if (SUCCEEDED(D3D11.SwapChain->GetDevice(IID_PPV_ARGS(&D3D11.Device)))) {
                        D3D11.Device->GetImmediateContext(&D3D11.DeviceContext);
                        DXGI_SWAP_CHAIN_DESC sd;
                        D3D11.SwapChain->GetDesc(&sd);
                        D3D11.TargetWindow = sd.OutputWindow;
                        ID3D11Texture2D* backBuffer;
                        D3D11.SwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
                        D3D11.Device->CreateRenderTargetView(backBuffer, nullptr, &D3D11.RenderView);
                        backBuffer->Release();

#ifndef RAW_D3D
                        ImGui::CreateContext();
                        ImGui_ImplWin32_Init(global::D3D11.TargetWindow);
                        ImGui_ImplDX11_Init(global::D3D11.Device, global::D3D11.DeviceContext);

                        D3D11.ImGuiCtx = ImGui::GetCurrentContext();
                        ImGui::GetAllocatorFunctions(&D3D11.AllocFunc, &D3D11.FreeFunc, &D3D11.BackEnd);
                    }
                }
            );

            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            for (auto& callback : CallbackQueue) {
                callback();
            }

            ImGui::EndFrame();
            ImGui::Render();

            D3D11.DeviceContext->OMSetRenderTargets(1, &D3D11.RenderView, nullptr);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
#else
                    }
                }
            );

            D3D11.DeviceContext->OMSetRenderTargets(1, &D3D11.RenderView, nullptr);

            for (auto& callback : CallbackQueue) {
                callback(a_this, a_syncInterval, a_flags);
            }
#endif

            return OldPresent(a_this, a_syncInterval, a_flags);
        }


        bool TryAcquireD3DData() noexcept
        {
            using namespace global;

            WNDCLASSEX windowClass{ };
            windowClass.cbSize = sizeof(windowClass);
            windowClass.lpfnWndProc = DefWindowProc;
            windowClass.lpszClassName = global::WndClass.c_str();

            if (!RegisterClassEx(&windowClass)) {
                return false;
            }

            IPCHost = CreateWindow(windowClass.lpszClassName, global::WndTitle.c_str(), WS_CAPTION | WS_VISIBLE, 0, 0, 0, 0, D3D11.TargetWindow, nullptr, nullptr, nullptr);
            OldWndProc = reinterpret_cast<WndProcFunc>(SetWindowLongPtr(IPCHost, GWLP_WNDPROC, reinterpret_cast<std::int64_t>(IPCWndProc)));

            DXGI_SWAP_CHAIN_DESC swapChainDesc{ };
            swapChainDesc.BufferCount = 1;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            swapChainDesc.OutputWindow = IPCHost;
            swapChainDesc.SampleDesc.Count = 1;
            swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
            swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
            swapChainDesc.Windowed = TRUE;

            D3D_FEATURE_LEVEL featureLevel{ };

            const auto result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &swapChainDesc, &D3D11.SwapChain, &D3D11.Device, &featureLevel, nullptr);

            if (FAILED(result)) {
                DestroyWindow(swapChainDesc.OutputWindow);
                UnregisterClass(windowClass.lpszClassName, GetModuleHandle(nullptr));

                return false;
            }

            return true;
        }


        inline void LogResult(const std::int64_t a_result, const char* a_action, const char* a_payload1, const std::uintptr_t a_payload2) noexcept
        {
            switch (a_result) {
            case LR_DKU_G_SUCCESS:
            {
                DEBUG("DKU_G: {} D3D callback with host succeeded -> {} @ {}.{:x}", a_action, a_payload1, global::LastSender, a_payload2);
                break;
            }
            case LR_DKU_G_FAILURE:
            {
                ERROR("DKU_G: {} D3D callback with host failed\n\nHost -> {}\n\nFunction -> {}", a_action, global::LastSender, a_payload1);
                break;
            }
            case LR_DKU_G_DISABLE:
            {
                DEBUG("DKU_G: {} D3D callback operation disabled -> {} @ {}.{:x}", a_action, a_payload1, global::LastSender, a_payload2);
                break;
            }
            default:
                ERROR("DKU_G: Unknown log result"sv);
                break;
            }

            global::LastSender = Version::PROJECT.data();
        }


        inline CallbackFunc callback_cast(std::uintptr_t a_func) noexcept
        {
            return reinterpret_cast<CallbackFunc>(a_func);
        }
    } // namespace Impl


    /* API */

    // Init DKU_G D3D hook or query a loaded DKU_G D3D host
    void InitD3D() noexcept
    {
        using namespace detail;

        global::IPCHost = FindWindow(global::WndClass.c_str(), global::WndTitle.c_str());

        if (global::SelfHost) {
            global::_Hook_Present->Enable();
        } else if (!global::IPCHost) {
            DEBUG("DKU_G: Self hosting..."sv);

            static std::once_flag D3DInit;
            std::call_once(D3DInit, []()
                {
                    if (TryAcquireD3DData()) {
                        global::_Hook_Present = Hook::AddVMTHook(global::D3D11.SwapChain, FUNC_INFO(Hook_Present), VMT_PRESENT_INDEX);
                        global::OldPresent = reinterpret_cast<PresentFunc>(dynamic_cast<Hook::VMTHookHandle&>(*global::_Hook_Present).OldAddress);
                    } else {
                        ERROR("DKU_G: Error initializing D3D11"sv);
                    }

                    global::HostName = Version::PROJECT.data();
                    global::SelfHost = true;
                }
            );

            return InitD3D();
        } else {
            const auto result = SendMessageA(global::IPCHost, WM_DKU_G_QUERY, reinterpret_cast<std::uintptr_t>(std::addressof(global::HostName)), reinterpret_cast<std::uintptr_t>(std::addressof(global::D3D11)));

            if (result == LR_DKU_G_SUCCESS) {
                DEBUG("DKU_G: Host available -> {}", global::HostName.c_str());

                ImGui::SetCurrentContext(global::D3D11.ImGuiCtx);
                ImGui::SetAllocatorFunctions(global::D3D11.AllocFunc, global::D3D11.FreeFunc, global::D3D11.BackEnd);

            } else {
                DEBUG("DKU_G: Host unavailable, fallback..."sv);

                global::WndClass = Utility::String2WString(Version::PROJECT.data());
                global::WndTitle = Utility::String2WString(Version::PROJECT.data());

                return InitD3D();
            }
        }
    }


    std::int64_t AddCallback(Hook::FuncInfo a_func) noexcept
    {
        using namespace detail;

        if (!global::IPCHost) {
            ERROR("DKU_G: Call InitGUI() before registering D3D callback"sv);
        }

        if (!a_func.Address) {
            ERROR("DKU_G: Unable to register D3D callback with invalid function"sv);
        }

        std::int64_t result{ 0 };

        if (global::SelfHost) {
            const auto& iter = std::find(global::CallbackQueue.begin(), global::CallbackQueue.end(), callback_cast(a_func.Address));

            if (iter != global::CallbackQueue.end()) {
                result = LR_DKU_G_DISABLE;
            } else {
                global::CallbackQueue.push_back(callback_cast(a_func.Address));
                result = LR_DKU_G_SUCCESS;
            }
        } else {
            result = SendMessageA(global::IPCHost, WM_DKU_G_ADD, reinterpret_cast<std::uintptr_t>(std::addressof(a_func)), reinterpret_cast<std::uintptr_t>(Version::PROJECT.data()));
        }
        
        LogResult(result, "Register", a_func.Name.data(), a_func.Address);

        return result;
    }


    std::int64_t RemoveCallback(Hook::FuncInfo a_func) noexcept
    {
        using namespace detail;

        if (!global::IPCHost) {
            ERROR("DKU_G: Call InitGUI() before removing D3D callback"sv);
        }

        if (!a_func.Address) {
            ERROR("DKU_G: Unable to remove D3D callback with invalid function"sv);
        }

        std::int64_t result{ 0 };

        if (global::SelfHost) {
            const auto& iter = std::find(global::CallbackQueue.begin(), global::CallbackQueue.end(), callback_cast(a_func.Address));

            if (iter != global::CallbackQueue.end()) {
                global::CallbackQueue.erase(iter);
                result = LR_DKU_G_SUCCESS;
            } else {
                result = LR_DKU_G_DISABLE;
            }
        } else {
            result = SendMessageA(global::IPCHost, WM_DKU_G_REMOVE, reinterpret_cast<std::uintptr_t>(std::addressof(a_func)), reinterpret_cast<std::uintptr_t>(Version::PROJECT.data()));
        }

        LogResult(result, "Remove", a_func.Name.data(), a_func.Address);

        return result;
    }


    // Unable to re-continue from this point
    void StopAll() noexcept
    {
        using namespace detail;

        global::_Hook_Present->Disable();
    }
} // namespace DKUtil::GUI
