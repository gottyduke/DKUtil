#pragma once

/*
 * 1.2.0
 * Complete IPC feature;
 * Naming convention formatting;
 * 
 * 1.1.0
 * Added IPC feature to prevent plugins from initializing D3D individually;
 * Separate ImGui and D3D implementation;
 *
 * 1.0.0
 * Basic ImGui implementation;
 * Callback hook setup;
 * RAW_D3D definition allowed for direct control without ImGui;
 *
 */


#define DKU_G_VERSION_MAJOR     1
#define DKU_G_VERSION_MINOR     2
#define DKU_G_VERSION_REVISION  0


// AdditionalInclude
#include <algorithm>
#include <bit>
#include <d3d11.h>
#include <type_traits>
#include <queue>
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

#ifdef DKU_G_NDEBUG
#define DKU_U_NDEBUG
#define DEBUG(...)	void(0)
#endif

#include "Utility.hpp"

#pragma comment ( lib, "d3d11.lib" )


#define VMT_PRESENT_INDEX   0x8
#define WM_DKU_G_ADD        WM_USER + 103
#define WM_DKU_G_REMOVE     WM_USER + 108
#define WM_DKU_G_QUERY_D    WM_USER + 109
#ifndef RAW_D3D
#define WM_DKU_G_QUERY_I    WM_USER + 110
#endif

#define LR_DKU_G_SUCCESS    static_cast<LRESULT>(1)
#define LR_DKU_G_FAILURE    static_cast<LRESULT>(0)
#define LR_DKU_G_DISABLE    static_cast<LRESULT>(-1)


#define DKU_G(GLOBAL)       DKUtil::GUI::detail::global::GLOBAL
#define DKU_G_DEVICE        DKU_G(D3D11.Device)
#define DKU_G_SWAPCHAIN     DKU_G(D3D11.SwapChain)
#define DKU_G_CONTEXT       DKU_G(D3D11.DeviceContext)
#define DKU_G_TARGETVIEW    DKU_G(D3D11.RenderView)
#define DKU_G_TARGETHWND    DKU_G(D3D11.TargetWindow)


namespace DKUtil::GUI
{
    constexpr auto DKU_G_VERSION = DKU_G_VERSION_MAJOR * 10000 + DKU_G_VERSION_MINOR * 100 + DKU_G_VERSION_REVISION;


    extern LRESULT AddCallback(Hook::FuncInfo, const bool = false) noexcept;
    extern LRESULT RemoveCallback(Hook::FuncInfo) noexcept;
    extern void InitImGui() noexcept;


    namespace detail
    {
        using namespace DKUtil::Alias;


        using PresentFunc = std::add_pointer_t<HRESULT(IDXGISwapChain*, UINT, UINT)>;
        using CallbackFunc = std::add_pointer_t<void(IDXGISwapChain*, UINT, UINT)>;
        using CallbackList = std::vector<std::uintptr_t>;
#ifndef RAW_D3D
        using ContextFunc = std::add_pointer_t<void(ImGuiContext*)>;
        using AllocFunc = std::add_pointer_t<void(ImGuiMemAllocFunc, ImGuiMemFreeFunc, void*)>;
        using ImGuiCallback = std::pair<std::uintptr_t, std::uintptr_t>;
#endif

        namespace global
        {
            // d3d
            struct D3DInfo
            {
                ID3D11Device*           Device = nullptr;
                IDXGISwapChain*         SwapChain = nullptr;
                ID3D11DeviceContext*    DeviceContext = nullptr;
                ID3D11RenderTargetView* RenderView = nullptr;
                HWND				    TargetWindow = nullptr;
            };
            D3DInfo                     D3D11;

            // gui
            PresentFunc                 _Present;
            CallbackList               HostQueue;
            CallbackList               LocalQueue;

            // hook
            HookHandle                  _Hook_Present;

#ifndef RAW_D3D
            struct ImGuiInfo
            {
                ImGuiContext*           Context = nullptr;
                ImGuiMemAllocFunc       AllocFunc = nullptr;
                ImGuiMemFreeFunc        FreeFunc = nullptr;
                void*                   Backend = nullptr;

                const std::uintptr_t    CtxCallback = reinterpret_cast<std::uintptr_t>(ImGui::SetCurrentContext);
                const std::uintptr_t    AllocCallback = reinterpret_cast<std::uintptr_t>(ImGui::SetAllocatorFunctions);
            };
            ImGuiInfo                   ImGui;
            std::queue<ImGuiCallback>   ImGuiQueue;
#endif

            // ipc
            HostInfo                    D3DHost{ L"DKU_G_d3d_base", L"DKU_d3d_host" };
            HostInfo                    ImGuiHost{ L"DKU_G_imgui_base", L"DKU_G_imgui_host" }; // required even if not using imgui, relay to second query
            const char*                 LastSender = PROJECT_NAME;
        } // namespace Global


        template <typename invocable_t, typename func_t>
        inline constexpr invocable_t Invocable(func_t a_func) noexcept
        {
            return std::bit_cast<invocable_t>(a_func);
        }


        /* WndProc */

        LRESULT __stdcall HostWndProc([[maybe_unused]] HWND a_window, UINT a_msg, WPARAM a_payload1, LPARAM a_payload2) noexcept
        {
            switch (a_msg) {
            case WM_DKU_G_ADD:
            {
                global::LastSender = std::bit_cast<const char*>(a_payload2);
                auto func = std::bit_cast<Hook::FuncInfo*>(a_payload1);

                if (!func) {
                    return LR_DKU_G_FAILURE;
                }

                return AddCallback(*func);
            }
            case WM_DKU_G_REMOVE:
            {
                global::LastSender = std::bit_cast<const char*>(a_payload2);
                auto* func = std::bit_cast<Hook::FuncInfo*>(a_payload1);

                if (!func) {
                    return LR_DKU_G_FAILURE;
                }

                return RemoveCallback(*func);
            }
            case WM_DKU_G_QUERY_D:
            {
                *std::bit_cast<std::string*>(a_payload1) = global::D3DHost.Name;
                auto* targetInfo = std::bit_cast<global::D3DInfo*>(a_payload2);

                if (!targetInfo) {
                    return LR_DKU_G_FAILURE;
                }

                *targetInfo = global::D3D11;

                return LR_DKU_G_SUCCESS;
            }
#ifndef RAW_D3D
            case WM_DKU_G_QUERY_I:
            {
                *std::bit_cast<std::string*>(a_payload1) = global::D3DHost.Name;
                auto* targetInfo = std::bit_cast<global::ImGuiInfo*>(a_payload2);

                if (!targetInfo) {
                    return LR_DKU_G_FAILURE;
                }

                global::ImGuiQueue.push(std::make_pair(targetInfo->CtxCallback, targetInfo->AllocCallback));

                return LR_DKU_G_SUCCESS;
            }
#endif
            default:
                return LR_DKU_G_DISABLE;
            }
        }


        /* Present */

        HRESULT __cdecl Hook_Present(IDXGISwapChain* a_this, UINT a_syncInterval, UINT a_flags) noexcept
        {
            using namespace global;

            static std::once_flag D3D11Init;
            std::call_once(D3D11Init, [&]()
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

                        DEBUG("DKU_G: Acquired D3D runtime info"sv);
#ifndef RAW_D3D
                        ImGui_ImplWin32_Init(D3D11.TargetWindow);
                        ImGui_ImplDX11_Init(D3D11.Device, D3D11.DeviceContext);

                        ImGui::GetAllocatorFunctions(&ImGui.AllocFunc, &ImGui.FreeFunc, &ImGui.Backend);

                        if (!ImGui.AllocFunc || !ImGui.FreeFunc) {
                            ERROR("DKU_G: Failed acquiring ImGui runtime info"sv);
                        }

                        DEBUG("DKU_G: Acquired ImGui runtime info"sv);
#endif
                    }
                }
            );

            // host info shouldn't change within one frame
            const auto isImGuiHost = ImGuiHost.Type == HostType::kSelf;

            if (isImGuiHost) {
#ifndef RAW_D3D
                while (!global::ImGuiQueue.empty()) {
                    auto& [context, alloc] = global::ImGuiQueue.front();

                    Invocable<ContextFunc>(context)(global::ImGui.Context);
                    Invocable<AllocFunc>(alloc)(global::ImGui.AllocFunc, global::ImGui.FreeFunc, global::ImGui.Backend);

                    global::ImGuiQueue.pop();
                }

                ImGui_ImplDX11_NewFrame();
                ImGui_ImplWin32_NewFrame();
                ImGui::NewFrame();
#endif
            }

            for (auto& func : HostQueue) {
                Invocable<CallbackFunc>(func)(a_this, a_syncInterval, a_flags);
            }

            if (isImGuiHost) {
#ifndef RAW_D3D
                ImGui::EndFrame();
                ImGui::Render();
                ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
#endif
            }

            D3D11.DeviceContext->OMSetRenderTargets(1, &D3D11.RenderView, nullptr);

            return Invocable<PresentFunc>(_Present)(a_this, a_syncInterval, a_flags);
        }


        bool TryAcquireD3DData() noexcept
        {
            DXGI_SWAP_CHAIN_DESC swapChainDesc{ };
            swapChainDesc.BufferCount = 1;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            swapChainDesc.OutputWindow = global::D3DHost.Window;
            swapChainDesc.SampleDesc.Count = 1;
            swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
            swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
            swapChainDesc.Windowed = TRUE;

            D3D_FEATURE_LEVEL featureLevel{ };

            const auto result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &swapChainDesc, &global::D3D11.SwapChain, &global::D3D11.Device, &featureLevel, nullptr);

            return SUCCEEDED(result);
        }


        inline void LogResult(const LRESULT a_result, const char* a_action, const char* a_payload1, const std::uintptr_t a_payload2) noexcept
        {
            switch (a_result) {
            case LR_DKU_G_SUCCESS:
            {
                DEBUG("DKU_G: {} D3D11 callback succeeded -> {} @ {}.{:x}", a_action, a_payload1, global::LastSender, a_payload2);
                break;
            }
            case LR_DKU_G_FAILURE:
            {
                ERROR("DKU_G: {} D3D11 callback failed\n\nHost -> {}\n\nFunction -> {}", a_action, global::LastSender, a_payload1);
                break;
            }
            case LR_DKU_G_DISABLE:
            {
                DEBUG("DKU_G: {} D3D11 callback disabled -> {} @ {}.{:x}", a_action, a_payload1, global::LastSender, a_payload2);
                break;
            }
            default:
                ERROR("DKU_G: Unknown log result"sv);
                break;
            }

            global::LastSender = Version::PROJECT.data();
        }
    } // namespace Impl


    /* API */

    // Init D3D11 hook or query a loaded D3D11 host
    void InitD3D() noexcept
    {
        using namespace detail;
        using namespace detail::global;

        if (Utility::IPC::TryInitHost<WM_DKU_G_QUERY_D, LR_DKU_G_SUCCESS>(D3DHost, "D3D11", HostWndProc, &D3D11, []()
            {
                if (TryAcquireD3DData()) {
                    _Hook_Present = Hook::AddVMTHook(D3D11.SwapChain, FUNC_INFO(Hook_Present), VMT_PRESENT_INDEX);
                    _Present = Invocable<PresentFunc>(dynamic_cast<Hook::VMTHookHandle&>(*_Hook_Present).OldAddress);
                    _Hook_Present->Enable();
                    return true;
                } else {
                    return false;
                }
            })) {
            DEBUG("DKU_G: D3D11 init"sv);
        } else {
            ERROR("DKU_G: Failed initializing D3D11"sv);
        }
    }


#ifndef RAW_D3D
    // Init ImGui module and set the context & allocators
    void InitImGui() noexcept
    {
        using namespace detail;
        using namespace detail::global;

        if (Utility::IPC::TryInitHost<WM_DKU_G_QUERY_I, LR_DKU_G_SUCCESS>(ImGuiHost, "ImGui", HostWndProc, &ImGui, []()
            {
                ImGui::CreateContext();
                ImGui.Context = ImGui::GetCurrentContext();

                return ImGui.Context;
            }
            )) {
            DEBUG("DKU_G: ImGui init"sv);
        } else {
            ERROR("DKU_G: Failed initializing ImGui"sv);
        }
    }
#endif


    LRESULT AddCallback(Hook::FuncInfo a_func, const bool a_remove) noexcept
    {
        using namespace detail;
        using namespace detail::global;

        if (!D3DHost.Window) {
            ERROR("DKU_G: Call InitD3D() before registering D3D callback"sv);
        }

        if (!a_func.Address) {
            ERROR("DKU_G: Invalid function {}", a_func.Name.empty() ? "INVALID_NAME"sv : a_func.Name);
        }

        if (std::strcmp(LastSender, PROJECT_NAME) != 0) {
            const auto& iter = std::find(LocalQueue.begin(), LocalQueue.end(), a_func.Address);

            if (iter != LocalQueue.end() && a_remove) {
                LocalQueue.erase(iter);
            }
            if (iter == LocalQueue.end()) {
                LocalQueue.push_back(a_func.Address);
            }
        }

        LRESULT result{ LR_DKU_G_SUCCESS };

        if (D3DHost.Type == HostType::kSelf) {
            const auto& iter = std::find(HostQueue.begin(), HostQueue.end(), a_func.Address);

            if (iter != HostQueue.end() && a_remove) {
                HostQueue.erase(iter);
            } else if (iter == HostQueue.end()) {
                HostQueue.push_back(a_func.Address);
            } else {
                result = LR_DKU_G_DISABLE;
            }
        } else {
            result = SendMessageA(D3DHost.Window, a_remove ? WM_DKU_G_REMOVE :WM_DKU_G_ADD, std::bit_cast<std::uintptr_t>(std::addressof(a_func)), std::bit_cast<std::uintptr_t>(PROJECT_NAME));
        }
        
        LogResult(result, a_remove ? "Remove" : "Register", a_func.Name.data(), a_func.Address);

        return result;
    }


    LRESULT RemoveCallback(Hook::FuncInfo a_func) noexcept
    {
        return AddCallback(a_func, true);
    }


    // no definitive termination if hosting
    void Terminate() noexcept
    {
        using namespace detail;

        for (auto func : global::LocalQueue) {
            RemoveCallback({ func, 0, "Termination"sv });
        }
    }
} // namespace DKUtil::GUI
