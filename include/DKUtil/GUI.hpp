#pragma once


// AdditionalInclude
#include <algorithm>
#include <d3d11.h>
#include <type_traits>
#include <vector>

// imgui
#include <imgui/imgui.h>
#include <imgui/imgui_impl_dx11.h>
#include <imgui/imgui_impl_win32.h>

// DKUtil
#include "Hook.hpp"
#include "Logger.hpp"
#include "Utility.hpp"

#pragma comment ( lib, "d3d11.lib" )


#define VMT_PRESENT_INDEX   0x8


extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


namespace DKUtil::GUI
{

	namespace Impl
	{
        using namespace DKUtil::Alias;

        using WndProcFunc = std::add_pointer_t<LRESULT((__stdcall)(HWND, UINT, WPARAM, LPARAM))>;
        using PresentFunc = std::add_pointer_t<HRESULT(IDXGISwapChain*, UINT, UINT)>;
        using CallbackFunc = std::add_pointer_t<void()>;


		ID3D11Device*			    D3D11Device = nullptr;
		IDXGISwapChain*			    DXGISwapChain = nullptr;
		ID3D11DeviceContext*	    D3D11DeviceContext = nullptr;
		ID3D11RenderTargetView*     D3D11RenderView = nullptr;

		HWND					    TargetWindow = nullptr;

        WndProcFunc                 OldWndProc;
        PresentFunc                 OldPresent;
        std::vector<CallbackFunc>   CallbackQueue;

		bool					    ShowMenu = true;
        HookHandle                  _Hook_Present;


        LRESULT __stdcall WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
        {
            ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
            return CallWindowProc(OldWndProc, hWnd, uMsg, wParam, lParam);
        }


        HRESULT __cdecl Hook_Present(IDXGISwapChain* a_this, UINT a_syncInterval, UINT a_flags) noexcept
        {
            static std::once_flag ImguiInit;
            std::call_once(ImguiInit, [&]()
                {
                    DXGISwapChain = a_this;
                    if (SUCCEEDED(DXGISwapChain->GetDevice(IID_PPV_ARGS(&D3D11Device)))) {
                        D3D11Device->GetImmediateContext(&D3D11DeviceContext);
                        DXGI_SWAP_CHAIN_DESC sd;
                        DXGISwapChain->GetDesc(&sd);
                        TargetWindow = sd.OutputWindow;
                        ID3D11Texture2D* backBuffer;
                        DXGISwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
                        D3D11Device->CreateRenderTargetView(backBuffer, nullptr, &D3D11RenderView);
                        backBuffer->Release();
                        OldWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(TargetWindow, GWLP_WNDPROC, (LONG_PTR)WndProc));

                        ImGui::CreateContext();
                        ImGuiIO& io = ImGui::GetIO();
                        io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
                        ImGui_ImplWin32_Init(TargetWindow);
                        ImGui_ImplDX11_Init(D3D11Device, D3D11DeviceContext);
                    }
                }
            );

            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            // dispatch all callbacks
            for (auto index = 0; index < CallbackQueue.size(); ++index) {
                CallbackQueue[index]();
            }

            ImGui::EndFrame();
            ImGui::Render();

            D3D11DeviceContext->OMSetRenderTargets(1, &D3D11RenderView, nullptr);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

            return OldPresent(a_this, a_syncInterval, a_flags);
        }


        bool TryAcquireD3DData() noexcept
        {
            WNDCLASSEX wc{ };
            wc.cbSize = sizeof(wc);
            wc.lpfnWndProc = DefWindowProc;
            wc.lpszClassName = TEXT("base");

            if (!RegisterClassEx(&wc)) {
                return false;
            }

            auto* const hWnd = CreateWindow(wc.lpszClassName, TEXT(""), WS_DISABLED, 0, 0, 0, 0, nullptr, nullptr, nullptr, nullptr);

            DXGI_SWAP_CHAIN_DESC swapChainDesc{ };
            swapChainDesc.BufferCount = 1;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            swapChainDesc.OutputWindow = hWnd;
            swapChainDesc.SampleDesc.Count = 1;
            swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
            swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
            swapChainDesc.Windowed = TRUE;

            D3D_FEATURE_LEVEL featureLevel{ };

            const auto hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &swapChainDesc, &Impl::DXGISwapChain, &Impl::D3D11Device, &featureLevel, nullptr);

            if (FAILED(hr)) {
                DestroyWindow(swapChainDesc.OutputWindow);
                UnregisterClass(wc.lpszClassName, GetModuleHandle(nullptr));

                return false;
            }

            DestroyWindow(swapChainDesc.OutputWindow);
            UnregisterClass(wc.lpszClassName, GetModuleHandle(nullptr));

            return true;
        }


        CallbackFunc callback_cast(std::uintptr_t a_func) noexcept
        {
            return reinterpret_cast<CallbackFunc>(a_func);
        }
	} // namespace Impl


    /* API */

    void InitD3D() noexcept
    {
        using namespace Impl;

        static std::once_flag D3DInit;
        std::call_once(D3DInit, [&]()
            {
                if (TryAcquireD3DData()) {
                    _Hook_Present = Hook::AddVMTHook(DXGISwapChain, VMT_PRESENT_INDEX, FUNC_INFO(Hook_Present));
                    OldPresent = reinterpret_cast<PresentFunc>(dynamic_cast<Hook::VMTHookHandle&>(*_Hook_Present).OldAddress);
                } else {
                    ERROR("DKU_G: Error initializing D3D11"sv);
                }
            }
        );

        _Hook_Present->Enable();
    }


    void AddCallback(Hook::FuncInfo a_func) noexcept
    {
        using namespace Impl;

        if (!a_func.Address) {
            ERROR("DKU_G: Unable to init D3D with invalid function pointer"sv);
        }

        CallbackQueue.push_back(callback_cast(a_func.Address));
        DEBUG("DKU_G: Present callback registered -> {}@{}.{:x}", a_func.Name.data(), Version::PROJECT.data(), a_func.Address);
    }


    void RemoveCallback(Hook::FuncInfo a_func) noexcept
    {
        using namespace Impl;

        const auto iter = std::find(CallbackQueue.begin(), CallbackQueue.end(), callback_cast(a_func.Address));
        if (iter != CallbackQueue.end()) {
            CallbackQueue.erase(iter);
            DEBUG("DKU_G: Present callback removed -> {}@{}.{:x}", a_func.Name.data(), Version::PROJECT.data(), a_func.Address);
        }
    }


    void StopAll() noexcept
    {
        using namespace Impl;
        
        _Hook_Present->Disable();
    }
} // namespace DKUtil::GUI