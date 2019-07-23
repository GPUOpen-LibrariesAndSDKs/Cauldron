// AMD AMDUtils code
// 
// Copyright(c) 2018 Advanced Micro Devices, Inc.All rights reserved.
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

#include "stdafx.h"
#include "Device.h"
#include "Misc\Misc.h"
#include "base\Helper.h"

#include <dxgi1_4.h>

#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#ifdef _DEBUG
#pragma comment(lib, "dxguid.lib")
#include <DXGIDebug.h>
#endif 

//#define USE_AGS

namespace CAULDRON_DX12
{
    Device::Device()
    {
    }

    Device::~Device()
    {
    }

    void Device::OnCreate(const char *pAppName, const char *pEngine, bool bValidationEnabled, HWND hWnd)
    {
#ifdef USE_AGS
        agsInit(&m_agsContext, nullptr, &m_agsGPUInfo);
        UserMarker::SetAgsContext(m_agsContext);
#endif

#ifdef _DEBUG
        // Enable the D3D12 debug layer.
        {
            ID3D12Debug1 *pDebugController;
            ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&pDebugController)));
            pDebugController->EnableDebugLayer();
            //pDebugController->SetEnableGPUBasedValidation(TRUE);
            pDebugController->SetEnableSynchronizedCommandQueueValidation(TRUE);
            pDebugController->Release();
        }
#else 
        Trace("Validation is only available in debug mode");
#endif

        // Use AGS
        //
#ifdef USE_AGS
        UINT factoryFlags = 0;
#ifdef _DEBUG
        factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
        IDXGIFactory *pFactory;
        CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&pFactory));

        IDXGIAdapter *pAdapter;
        pFactory->EnumAdapters(0, &pAdapter);
        pFactory->Release();

        if (m_agsGPUInfo.devices[0].vendorId == 0x1002)
        {
            AGSDX12DeviceCreationParams creationParams = {};
            creationParams.pAdapter = pAdapter;
            creationParams.iid = __uuidof(m_pDevice);
            creationParams.FeatureLevel = D3D_FEATURE_LEVEL_12_0;

            AGSDX12ExtensionParams extensionParams = {};
            AGSDX12ReturnedParams returnedParams = {};

            AGSReturnCode rc = agsDriverExtensionsDX12_CreateDevice(m_agsContext, &creationParams, &extensionParams, &returnedParams);
            if (rc == AGS_SUCCESS)
            {
                m_pDevice = returnedParams.pDevice;
                m_agsCreatedDevice = true;
            }
        }
        pAdapter->Release();
#endif

        // Create Device
        //
        ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_pDevice)));

        // Check for FP16 support
        //
        D3D12_FEATURE_DATA_D3D12_OPTIONS featureDataOptions = {};
        m_pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &featureDataOptions, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS));
        m_fp16Supported = (featureDataOptions.MinPrecisionSupport & D3D12_SHADER_MIN_PRECISION_SUPPORT_16_BIT) != 0;

        // create queues
        //
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queueDesc.NodeMask = 0;
        m_pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_pDirectQueue));
        SetName(m_pDirectQueue, "DirectQueue");

        // Init debug marker extension if found
        //

    }

    void Device::CreatePipelineCache()
    {
    }

    void Device::DestroyPipelineCache()
    {
    }

    void Device::OnDestroy()
    {
        m_pDirectQueue->Release();

#ifdef USE_AGS
        if (m_agsCreatedDevice)
        {
            agsDriverExtensionsDX12_DestroyDevice(m_agsContext, m_pDevice, nullptr);
            m_agsCreatedDevice = false;
        }
        else
        {
            m_pDevice->Release();
        }

        if (m_agsContext)
        {
            agsDeInit(m_agsContext);
            m_agsContext = nullptr;
        }
#else
        m_pDevice->Release();
#endif

#ifdef _DEBUG
        // Report live objects
        {
            IDXGIDebug1 *pDxgiDebug;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDxgiDebug))))
            {
                pDxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
                pDxgiDebug->Release();
            }
        }
#endif
    }

    AGSContext* Device::GetAgsContext()
    {
        return NULL;
    }

    void Device::GPUFlush()
    {
        ID3D12Fence *pFence;
        ThrowIfFailed(m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence)));

        ThrowIfFailed(m_pDirectQueue->Signal(pFence, 1));

        HANDLE mHandleFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        pFence->SetEventOnCompletion(1, mHandleFenceEvent);
        WaitForSingleObject(mHandleFenceEvent, INFINITE);
        CloseHandle(mHandleFenceEvent);

        pFence->Release();
    }
}
