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

#include "stdafx.h"
#include "Device.h"
#include "Misc/Misc.h"
#include "Base/Helper.h"

#include <dxgi1_6.h>
#pragma comment(lib, "dxguid.lib")
#include <DXGIDebug.h>

namespace CAULDRON_DX12
{
    void Device::OnCreate(const char *pAppName, const char *pEngine, bool bValidationEnabled, bool bGpuValidationEnabled, HWND hWnd, bool bInitializeWithAGS)
    {
        // Enable the D3D12 debug layer
        //
        // Note that it turns out the validation and debug layer are known to cause
        // deadlocks in certain circumstances, for example when the vsync interval
        // is 0 and full screen is used
        if (bValidationEnabled || bGpuValidationEnabled)
        {
            ID3D12Debug1 *pDebugController;
            if (D3D12GetDebugInterface(IID_PPV_ARGS(&pDebugController)) == S_OK)
            {
                // Enabling GPU Validation without enabling the debug layer does nothing
                if (bValidationEnabled || bGpuValidationEnabled)
                {
                    pDebugController->EnableDebugLayer();
                    pDebugController->SetEnableGPUBasedValidation(bGpuValidationEnabled);
                }
                pDebugController->Release();
            }
        }

        // Initialize Adapter
        {
            UINT factoryFlags = 0;
            if (bValidationEnabled || bGpuValidationEnabled)
                factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

            IDXGIFactory* pFactory;
            IDXGIFactory6* pFactory6;

            ThrowIfFailed(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&pFactory)));

			// Try to get Factory6 in order to use EnumAdapterByGpuPreference method. If it fails, fall back to regular EnumAdapters.
			if (S_OK == pFactory->QueryInterface(IID_PPV_ARGS(&pFactory6)))
			{
                ThrowIfFailed(pFactory6->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&m_pAdapter)));
				pFactory6->Release();
			}
			else
			{
                ThrowIfFailed(pFactory->EnumAdapters(0, &m_pAdapter));
			}
            
            pFactory->Release();
        }

        // Create Device
        //
        if (bInitializeWithAGS)
        {
            // check if AMD GPU
            DXGI_ADAPTER_DESC AdapterDesc;
            m_pAdapter->GetDesc(&AdapterDesc);
            const bool bAMDGPU = (AdapterDesc.VendorId == 0x1002);

            if (bAMDGPU)
            {
                AGSReturnCode result = agsInitialize(AGS_MAKE_VERSION(AMD_AGS_VERSION_MAJOR, AMD_AGS_VERSION_MINOR, AMD_AGS_VERSION_PATCH), nullptr, &m_agsContext, &m_agsGPUInfo);
                if (result == AGS_SUCCESS)
                {
                    UserMarker::SetAgsContext(m_agsContext);

                    AGSDX12DeviceCreationParams creationParams = {};
                    creationParams.pAdapter = m_pAdapter;
                    creationParams.iid = __uuidof(m_pDevice);
                    creationParams.FeatureLevel = D3D_FEATURE_LEVEL_12_0;

                    AGSDX12ExtensionParams extensionParams = {};
                    AGSDX12ReturnedParams returnedParams = {};

                    // Create AGS Device
                    //
                    AGSReturnCode rc = agsDriverExtensionsDX12_CreateDevice(m_agsContext, &creationParams, &extensionParams, &returnedParams);
                    if (rc == AGS_SUCCESS)
                    {
                        m_pDevice = returnedParams.pDevice;
                    }
                    else
                    {
                        Trace("Warning: AGS CreateDevice() failed w/ code=%d", rc);
                    }
                }
                else
                {
                    Trace("Warning: agsInitialize() failed w/ code=%d", result);
                }
            }
        }
        if (m_pDevice==NULL) // If the AGS device wasn't created then try using a regular device
        {
            ThrowIfFailed(D3D12CreateDevice(m_pAdapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_pDevice)));

            if (bValidationEnabled || bGpuValidationEnabled)
            {
                ID3D12InfoQueue* pInfoQueue;
                if (m_pDevice->QueryInterface(IID_PPV_ARGS(&pInfoQueue)) == S_OK)
                {
                    pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
                    pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
                    pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
                    pInfoQueue->Release();
                }
            }
        }
        SetName(m_pDevice, "device");

        // Check for FP16 support
        D3D12_FEATURE_DATA_D3D12_OPTIONS featureDataOptions = {};
        ThrowIfFailed(m_pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &featureDataOptions, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS)));
        m_fp16Supported = (featureDataOptions.MinPrecisionSupport & D3D12_SHADER_MIN_PRECISION_SUPPORT_16_BIT) != 0;

		// Check for RT support
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureDataOptions5 = {};
		ThrowIfFailed(m_pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureDataOptions5, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5)));
		m_rt10Supported = (featureDataOptions5.RaytracingTier & D3D12_RAYTRACING_TIER_1_0) != 0;
        m_rt11Supported = (featureDataOptions5.RaytracingTier & D3D12_RAYTRACING_TIER_1_1) != 0;

		// Check for VRS support
		D3D12_FEATURE_DATA_D3D12_OPTIONS6 featureDataOptions6 = {};
		ThrowIfFailed(m_pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &featureDataOptions6, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS6)));
		m_vrs1Supported = (featureDataOptions6.VariableShadingRateTier & D3D12_VARIABLE_SHADING_RATE_TIER_1) != 0;
        m_vrs2Supported = (featureDataOptions6.VariableShadingRateTier & D3D12_VARIABLE_SHADING_RATE_TIER_2) != 0;

        // Check for Barycentrics support
        D3D12_FEATURE_DATA_D3D12_OPTIONS3 featureDataOptions3 = {};
        ThrowIfFailed(m_pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, &featureDataOptions3, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS3)));
        m_barycentricsSupported = featureDataOptions3.BarycentricsSupported;

        // create direct and compute queues
        //
        {
            D3D12_COMMAND_QUEUE_DESC queueDesc = {};
            queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            queueDesc.NodeMask = 0;
            ThrowIfFailed(m_pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_pDirectQueue)));
            SetName(m_pDirectQueue, "DirectQueue");
        }
        {
            D3D12_COMMAND_QUEUE_DESC queueDesc = {};
            queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
            queueDesc.NodeMask = 0;
            ThrowIfFailed(m_pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_pComputeQueue)));
            SetName(m_pComputeQueue, "ComputeQueue");
        }

    }

    void Device::GetDeviceInfo(std::string *deviceName, std::string *driverVersion)
    {
        DXGI_ADAPTER_DESC adapterDescription;
        m_pAdapter->GetDesc(&adapterDescription);

        *deviceName = format("%S", adapterDescription.Description);

        if (m_agsContext)
            *driverVersion = m_agsGPUInfo.driverVersion;
        else
            *driverVersion = "Enable AGS for Driver Version";
    }

    void Device::OnDestroy()
    {
        m_pComputeQueue->Release();
        m_pDirectQueue->Release();
        m_pAdapter->Release();

        if (m_agsContext)
        {
            agsDriverExtensionsDX12_DestroyDevice(m_agsContext, m_pDevice, nullptr);
            agsDeInitialize(m_agsContext);
        }
        else
        {
            m_pDevice->Release();
        }

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

    void Device::GPUFlush(D3D12_COMMAND_LIST_TYPE queueType)
    {
        ID3D12Fence *pFence;
        ThrowIfFailed(m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence)));

        ID3D12CommandQueue* queue = (queueType == D3D12_COMMAND_LIST_TYPE_COMPUTE) ? GetComputeQueue() : GetGraphicsQueue();

        ThrowIfFailed(queue->Signal(pFence, 1));

        HANDLE mHandleFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        pFence->SetEventOnCompletion(1, mHandleFenceEvent);
        WaitForSingleObject(mHandleFenceEvent, INFINITE);
        CloseHandle(mHandleFenceEvent);

        pFence->Release();
    }

    void Device::GPUFlush()
    {
        GPUFlush(D3D12_COMMAND_LIST_TYPE_COMPUTE);
        GPUFlush(D3D12_COMMAND_LIST_TYPE_DIRECT);
    }
}
