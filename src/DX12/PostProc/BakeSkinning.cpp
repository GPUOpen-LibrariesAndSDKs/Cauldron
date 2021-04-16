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
#include "../Base/Helper.h"
#include "../Base/DynamicBufferRing.h"
#include "../Base/ShaderCompilerHelper.h"
#include "BakeSkinning.h"


namespace CAULDRON_DX12
{
    BakeSkinning::BakeSkinning()
    {
    }

    BakeSkinning::~BakeSkinning()
    {
    }

    void BakeSkinning::OnCreate(Device *pDevice, ResourceViewHeaps *pResourceViewHeaps, int weightsJointsSets)
    {
        m_pDevice = pDevice;

        m_pResourceViewHeaps = pResourceViewHeaps;

        // Compile shaders
        //
        D3D12_SHADER_BYTECODE shaderByteCode = {};
        DefineList defines;
        int inputTableCount = 0;
        defines["HAS_POSITION"] = std::to_string(inputTableCount++);
        if (weightsJointsSets >= 1)
        {
            defines["HAS_WEIGHTS_0"] = std::to_string(inputTableCount++);
            defines["HAS_JOINTS_0"] = std::to_string(inputTableCount++);
        }
        if (weightsJointsSets >= 2)
        {
            defines["HAS_WEIGHTS_1"] = std::to_string(inputTableCount++);
            defines["HAS_JOINTS_1"] = std::to_string(inputTableCount++);
        }
        defines["ID_SKINNING_MATRICES"] = "5";
        defines["ID_OUTPUT"] = "6";
        CompileShaderFromFile("BakeSkinning.hlsl", &defines, "main", "-T cs_6_0", &shaderByteCode);

        // Create root signature
        //
        {
            CD3DX12_DESCRIPTOR_RANGE DescRange;
            CD3DX12_ROOT_PARAMETER RTSlot[3];

            // cb input table: position, weights0, weights1, joints0, joints1
            int parameterCount = 0;
            DescRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, inputTableCount, 0);
            RTSlot[parameterCount++].InitAsDescriptorTable(1, &DescRange);

            // cb matrices
            RTSlot[parameterCount++].InitAsConstantBufferView(5);

            // output
            RTSlot[parameterCount++].InitAsUnorderedAccessView(6);

            // the root signature contains 3 slots to be used
            CD3DX12_ROOT_SIGNATURE_DESC descRootSignature = CD3DX12_ROOT_SIGNATURE_DESC();
            descRootSignature.NumParameters = parameterCount;
            descRootSignature.pParameters = RTSlot;
            descRootSignature.NumStaticSamplers = 0;
            descRootSignature.pStaticSamplers = NULL;

            // deny uneccessary access to certain pipeline stages   
            descRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

            ID3DBlob *pOutBlob, *pErrorBlob = NULL;
            ThrowIfFailed(D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &pOutBlob, &pErrorBlob));
            ThrowIfFailed(
                pDevice->GetDevice()->CreateRootSignature(0, pOutBlob->GetBufferPointer(), pOutBlob->GetBufferSize(), IID_PPV_ARGS(&m_pRootSignature))
            );
            SetName(m_pRootSignature, "BakeSkinning::m_pRootSignature");

            pOutBlob->Release();
            if (pErrorBlob)
                pErrorBlob->Release();
        }

        {
            D3D12_COMPUTE_PIPELINE_STATE_DESC descPso = {};
            descPso.CS = shaderByteCode;
            descPso.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
            descPso.pRootSignature = m_pRootSignature;
            descPso.NodeMask = 0;

            pDevice->GetDevice()->CreateComputePipelineState(&descPso, IID_PPV_ARGS(&m_pPipeline));
            SetName(m_pPipeline, "BakeSkinning::m_pPipeline");
        }
    }

    void BakeSkinning::OnDestroy()
    {
        m_pPipeline->Release();
        m_pRootSignature->Release();
    }

    void BakeSkinning::Draw(ID3D12GraphicsCommandList* pCommandList, CBV_SRV_UAV *pUAVInputs, D3D12_GPU_VIRTUAL_ADDRESS skinningMatrices, D3D12_GPU_VIRTUAL_ADDRESS outputUAV, int numVertices)
    {
        if (m_pPipeline == NULL)
            return;

        // Bind Descriptor heaps and the root signature
        //                
        ID3D12DescriptorHeap *pDescriptorHeaps[] = { m_pResourceViewHeaps->GetCBV_SRV_UAVHeap(), m_pResourceViewHeaps->GetSamplerHeap() };
        pCommandList->SetDescriptorHeaps(2, pDescriptorHeaps);
        pCommandList->SetComputeRootSignature(m_pRootSignature);

        // Bind Descriptor the descriptor sets
        //                
        int params = 0;
        pCommandList->SetComputeRootDescriptorTable(params++, pUAVInputs->GetGPU());
        pCommandList->SetComputeRootConstantBufferView(params++, skinningMatrices);
        pCommandList->SetComputeRootUnorderedAccessView(params++, outputUAV);

        // Bind Pipeline
        //
        pCommandList->SetPipelineState(m_pPipeline);

        // Dispatch
        //
        pCommandList->Dispatch((numVertices + 63) / 64, 1, 1);
    }
}