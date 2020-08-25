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
#include "base/Device.h"
#include "base/ResourceViewHeaps.h"
#include "base/ShaderCompilerHelper.h"
#include "TAA.h"

namespace CAULDRON_DX12
{
    //--------------------------------------------------------------------------------------
    //
    // OnCreate
    //
    //--------------------------------------------------------------------------------------
    void TAA::OnCreate(Device *pDevice, ResourceViewHeaps *pResourceViewHeaps)
    {
        m_pResourceViewHeaps = pResourceViewHeaps;

        // Compile shader
        //
        D3D12_SHADER_BYTECODE shaderByteCode = {};
        CompileShaderFromFile("taa.hlsl", NULL, "main", "-T cs_6_0", &shaderByteCode);

        // Create root signature
        //
        {
            CD3DX12_DESCRIPTOR_RANGE descriptorRanges[2];
            descriptorRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1u, 0u);
            descriptorRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4u, 0u);

            CD3DX12_ROOT_PARAMETER rootParameters[ARRAYSIZE(descriptorRanges)];
            for (uint32_t i = 0; i < ARRAYSIZE(rootParameters); ++i)
                rootParameters[i].InitAsDescriptorTable(1u, &descriptorRanges[i]);

            CD3DX12_STATIC_SAMPLER_DESC staticSamplerDescs[4];
            staticSamplerDescs[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
            staticSamplerDescs[1].Init(1, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
            staticSamplerDescs[2].Init(2, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
            staticSamplerDescs[3].Init(3, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

            CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
            rootSignatureDesc.Init(ARRAYSIZE(rootParameters), rootParameters, ARRAYSIZE(staticSamplerDescs), staticSamplerDescs);

            ID3DBlob *pOutBlob, *pErrorBlob = NULL;
            ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pOutBlob, &pErrorBlob));
            ThrowIfFailed(
                pDevice->GetDevice()->CreateRootSignature(0, pOutBlob->GetBufferPointer(), pOutBlob->GetBufferSize(), IID_PPV_ARGS(&m_pRootSignature))
            );
            SetName(m_pRootSignature, "TAA");

            pOutBlob->Release();
            if (pErrorBlob)
                pErrorBlob->Release();
        }

        // Create pipeline state
        //
        {
            D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineStateDesc = {};
            pipelineStateDesc.pRootSignature = m_pRootSignature;
            pipelineStateDesc.CS = shaderByteCode;

            pDevice->GetDevice()->CreateComputePipelineState(&pipelineStateDesc, IID_PPV_ARGS(&m_pPipelineState));
            SetName(m_pPipelineState, "TAA");
        }
    }

    //--------------------------------------------------------------------------------------
    //
    // OnDestroy
    //
    //--------------------------------------------------------------------------------------
    void TAA::OnDestroy()
    {
        if (m_pRootSignature)
            m_pRootSignature->Release();
        m_pRootSignature = NULL;

        if (m_pPipelineState)
            m_pPipelineState->Release();
        m_pPipelineState = NULL;
    }

    //--------------------------------------------------------------------------------------
    //
    // Draw
    //
    //--------------------------------------------------------------------------------------
    void TAA::Draw(ID3D12GraphicsCommandList *pCommandList, CBV_SRV_UAV *pUAVTable, CBV_SRV_UAV *pSRVTable, uint32_t width, uint32_t height)
    {
        if (m_pPipelineState == NULL)
            return;
        UserMarker marker(pCommandList, "TAA");

        // Bind the descriptor heaps and root signature
        //
        ID3D12DescriptorHeap *pDescriptorHeaps[] = { m_pResourceViewHeaps->GetCBV_SRV_UAVHeap() };
        pCommandList->SetDescriptorHeaps(ARRAYSIZE(pDescriptorHeaps), pDescriptorHeaps);
        pCommandList->SetComputeRootSignature(m_pRootSignature);

        // Bind the descriptor sets
        //
        pCommandList->SetComputeRootDescriptorTable(0, pUAVTable->GetGPU());
        pCommandList->SetComputeRootDescriptorTable(1, pSRVTable->GetGPU());

        // Bind the pipeline state
        //
        pCommandList->SetPipelineState(m_pPipelineState);

        // Dispatch
        //
        uint32_t threadX = (width + 15) / 16;
        uint32_t threadY = (height + 15) / 16;

        pCommandList->Dispatch(threadX, threadY, 1);
    }
}
