#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <algorithm>
#include <string>
#include <map>
#include <time.h>
#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")
#include <D3DX11tex.h>
#pragma comment(lib, "D3DX11.lib")
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "../../SDK/SDK/SDK/_Includes.hpp"
#include "3rdParty/MinHook.h"

__inline void PatchBytes(void* m_Address, size_t m_Size, uint8_t* m_Bytes)
{
    DWORD m_OldProtect = 0x0;
    if (VirtualProtect(m_Address, m_Size, PAGE_EXECUTE_READWRITE, &m_OldProtect))
        memcpy(m_Address, m_Bytes, m_Size);
}

namespace UIHKScreenCamera
{
    void __fastcall Snapshot(Illusion::CTarget* m_Target)
    {
        ID3D11RenderTargetView* m_TargetView = m_Target->mTargetPlat->mRenderTargetView[0][0];
        if (!m_TargetView)
            return;

        ID3D11Resource* m_TextureResource = nullptr;
        m_TargetView->GetResource(&m_TextureResource);
        if (!m_TextureResource)
            return;

        time_t m_RawTime;
        time(&m_RawTime);
        tm* m_TimeInfo = localtime(&m_RawTime);

        char m_FileFormat[128];
        strftime(m_FileFormat, sizeof(m_FileFormat), "Camera\\%F_%H%M%S.jpg", m_TimeInfo);

        D3DX11SaveTextureToFileA(*reinterpret_cast<ID3D11DeviceContext**>(UFG::Global::D3D11DeviceContext), m_TextureResource, D3DX11_IFF_JPG, m_FileFormat);
    }

    typedef void(__fastcall* m_tUpdate)(void*);
    m_tUpdate m_oUpdate;

    void __fastcall Update(void* rcx)
    {
        m_oUpdate(rcx);

        struct ScreenCamera_t
        {
            UFG_PAD(0xA0);

            float mTriggerCooldown;
            bool mValidPhoto;
            bool mUploadPhoto;
        };
        ScreenCamera_t* m_Camera = reinterpret_cast<ScreenCamera_t*>(rcx);
        if (m_Camera->mTriggerCooldown == 0.f && !m_Camera->mValidPhoto)
            UFG::RenderWorld::RequestScreenShot(Snapshot, 1.f);
    }
}

int __stdcall DllMain(HMODULE m_hModule, DWORD m_dReason, void* m_pReserved)
{
    if (m_dReason == DLL_PROCESS_ATTACH)
    {
        uint8_t m_PDA_RootMenu_Patch[] = { 0xC6, 0x45, 0xE8, 0x01 };
        PatchBytes(reinterpret_cast<void*>(UFG_RVA(0x5E00C7)), sizeof(m_PDA_RootMenu_Patch), m_PDA_RootMenu_Patch);

        uint8_t m_PDA_CanUseCamera_Patch[] = { 0xB0, 0x01, 0xC3 };
        PatchBytes(reinterpret_cast<void*>(UFG_RVA(0x5D48F0)), sizeof(m_PDA_CanUseCamera_Patch), m_PDA_CanUseCamera_Patch);

        MH_Initialize();
        auto MH_AddHook = [](uintptr_t m_uFunction, void* m_pHook, void** m_pOriginal = nullptr)
        {
            void* m_pFunction = reinterpret_cast<void*>(m_uFunction);
            MH_CreateHook(m_pFunction, m_pHook, m_pOriginal);
            MH_EnableHook(m_pFunction);
        };
        MH_AddHook(UFG_RVA(0x641CC0), UIHKScreenCamera::Update, (void**)&UIHKScreenCamera::m_oUpdate);

        CreateDirectoryA("Camera", 0);
    }

    return 1;
}
