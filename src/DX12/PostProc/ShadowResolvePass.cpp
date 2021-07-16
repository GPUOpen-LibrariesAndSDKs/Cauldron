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
#include "base/Device.h"
#include "base/DynamicBufferRing.h"
#include "base/ShaderCompilerHelper.h"
#include "GLTF/GLTFTexturesAndBuffers.h"
#include "GLTF/GltfHelpers.h"

#include "ShadowResolvePass.h"

namespace CAULDRON_DX12
{
    //--------------------------------------------------------------------------------------
    //
    // OnCreate
    //
    //--------------------------------------------------------------------------------------
    void ShadowResolvePass::OnCreate(
        Device* pDevice,
        ResourceViewHeaps *pResourceViewHeaps,
        DynamicBufferRing *pDynamicBufferRing)
    {
        m_pResourceViewHeaps = pResourceViewHeaps;
        m_pDynamicBufferRing = pDynamicBufferRing;

        // Compile shaders
        //
        DefineList defines;
        defines["ID_shadowMap"] = "0";
        defines["TILE_SIZE"] = std::to_string(s_TileSize);
        D3D12_SHADER_BYTECODE shaderByteCode = {};
        CompileShaderFromFile("ShadowResolve.hlsl", &defines, "mainCS", "-T cs_6_0", &shaderByteCode);

        // Create root signature
        //
        {
            D3D12_STATIC_SAMPLER_DESC shadowSamplerDesc;
            CreateSamplerForShadowMap(0, &shadowSamplerDesc);
            shadowSamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

            CD3DX12_DESCRIPTOR_RANGE desc_ranges[3];
            desc_ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
            desc_ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
            desc_ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

            CD3DX12_ROOT_PARAMETER root_params[ARRAYSIZE(desc_ranges) + 1];
            root_params[0].InitAsConstantBufferView(0);
            for (uint32_t i = 0; i < ARRAYSIZE(desc_ranges); ++i)
                root_params[i + 1].InitAsDescriptorTable(1, &desc_ranges[i]);

            CD3DX12_ROOT_SIGNATURE_DESC root_signature_desc;
            root_signature_desc.NumParameters = ARRAYSIZE(root_params);
            root_signature_desc.pParameters = root_params;
            root_signature_desc.NumStaticSamplers = 1;
            root_signature_desc.pStaticSamplers = &shadowSamplerDesc;
            root_signature_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

            ID3DBlob *pOutBlob, *pErrorBlob = NULL;
            ThrowIfFailed(D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &pOutBlob, &pErrorBlob));
            ThrowIfFailed(
                pDevice->GetDevice()->CreateRootSignature(0, pOutBlob->GetBufferPointer(), pOutBlob->GetBufferSize(), IID_PPV_ARGS(&m_pRootSignature))
            );
            SetName(m_pRootSignature, "ShadowResolvePass::m_pRootSignature");

            if (pErrorBlob)
                pErrorBlob->Release();
            pOutBlob->Release();
        }

        // Create the pipeline state object
        //
        {
            D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline_state_desc = {};
            pipeline_state_desc.CS = shaderByteCode;
            pipeline_state_desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
            pipeline_state_desc.pRootSignature = m_pRootSignature;
            pipeline_state_desc.NodeMask = 0;

            pDevice->GetDevice()->CreateComputePipelineState(&pipeline_state_desc, IID_PPV_ARGS(&m_pPipelineState));
            SetName(m_pPipelineState, "ShadowResolvePass::m_pPipelineState");
        }
    }

    //--------------------------------------------------------------------------------------
    //
    // OnDestroy
    //
    //--------------------------------------------------------------------------------------
    void ShadowResolvePass::OnDestroy()
    {
        if (m_pRootSignature)
        {
            m_pRootSignature->Release();
            m_pRootSignature = NULL;
        }

        if (m_pPipelineState)
        {
            m_pPipelineState->Release();
            m_pPipelineState = NULL;
        }
    }

    //--------------------------------------------------------------------------------------
    //
    // Draw
    //
    //--------------------------------------------------------------------------------------
    void ShadowResolvePass::Draw(ID3D12GraphicsCommandList *pCommandList, GLTFTexturesAndBuffers *pGLTFTexturesAndBuffers, ShadowResolveFrame *pShadowResolveFrame)
    {
        UserMarker marker(pCommandList, "ShadowResolvePass");

        // Bind the descriptor heaps and root signature
        //
        ID3D12DescriptorHeap *pDescriptorHeaps[] = { m_pResourceViewHeaps->GetCBV_SRV_UAVHeap() };
        pCommandList->SetDescriptorHeaps(ARRAYSIZE(pDescriptorHeaps), pDescriptorHeaps);
        pCommandList->SetComputeRootSignature(m_pRootSignature);

        // Bind the descriptors
        //
        pCommandList->SetComputeRootConstantBufferView(0, pGLTFTexturesAndBuffers->GetPerFrameConstants());
        pCommandList->SetComputeRootDescriptorTable(1, pShadowResolveFrame->m_ShadowMapSRV.GetGPU());
        pCommandList->SetComputeRootDescriptorTable(2, pShadowResolveFrame->m_DepthBufferSRV.GetGPU());
        pCommandList->SetComputeRootDescriptorTable(3, pShadowResolveFrame->m_ShadowBufferUAV.GetGPU());

        // Bind the PSO
        //
        pCommandList->SetPipelineState(m_pPipelineState);

        // Dispatch
        //
        uint32_t threadX = (pShadowResolveFrame->m_Width + (s_TileSize - 1)) / s_TileSize;
        uint32_t threadY = (pShadowResolveFrame->m_Height + (s_TileSize - 1)) / s_TileSize;

        pCommandList->Dispatch(threadX, threadY, 1);
    }
}
