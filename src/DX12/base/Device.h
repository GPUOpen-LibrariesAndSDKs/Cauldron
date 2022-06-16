// AMD Cauldron code
// 
// Copyright(c) 2020 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
#pragma once

#include <d3d12.h>
#include "../../libs/d3d12x/d3dx12.h"
#include "../AGS/amd_ags.h"

namespace CAULDRON_DX12
{
    //
    // Just a device class with many helper functions
    //

    class Device
    {
    public:
        void OnCreate(const char *pAppName, const char *pEngine, bool bValidationEnabled, bool bGpuValidationEnabled, HWND hWnd, bool bInitializeWithAGS);
        void OnDestroy();

        ID3D12Device *GetDevice() { return m_pDevice; }
        IDXGIAdapter *GetAdapter() { return m_pAdapter; }
        ID3D12CommandQueue *GetGraphicsQueue() { return m_pDirectQueue; }
        ID3D12CommandQueue *GetComputeQueue() { return m_pComputeQueue; }
        void GetDeviceInfo(std::string *deviceName, std::string *driverVersion);

        AGSContext *GetAGSContext() { return m_agsContext; }
        AGSGPUInfo *GetAGSGPUInfo() { return &m_agsGPUInfo; }

        bool IsFp16Supported() { return m_fp16Supported; }
        bool IsRT10Supported() { return m_rt10Supported; }
        bool IsRT11Supported() { return m_rt11Supported; }
        bool IsVRSTier1Supported() { return m_vrs1Supported; }
        bool IsVRSTier2Supported() { return m_vrs2Supported; }
        bool IsBarycentricsSupported() { return m_barycentricsSupported; }

        void GPUFlush(D3D12_COMMAND_LIST_TYPE queueType);
        void GPUFlush();  // flushes all queues

    private:
        ID3D12Device         *m_pDevice = nullptr;
        IDXGIAdapter         *m_pAdapter = nullptr;
        ID3D12CommandQueue   *m_pDirectQueue = nullptr;
        ID3D12CommandQueue   *m_pComputeQueue = nullptr;

        AGSContext           *m_agsContext = nullptr;
        AGSGPUInfo            m_agsGPUInfo = {};
        bool                  m_fp16Supported = false;
        bool                  m_rt10Supported = false;
        bool                  m_rt11Supported = false;
        bool                  m_vrs1Supported = false;
        bool                  m_vrs2Supported = false;
        bool                  m_barycentricsSupported = false;
    };
}

#define CAULDRON_APP_USE_DX12_AGILITY_SDK(Version, Path)    \
    extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = Version; }   \
    extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = Path; }
