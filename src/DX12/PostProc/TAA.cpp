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
    void TAA::OnCreate(Device *pDevice, ResourceViewHeaps *pResourceViewHeaps, StaticBufferPool *pStaticBufferPool, bool sharpening)
    {
        m_bSharpening = sharpening;
        m_pDevice = pDevice;

        m_pResourceViewHeaps = pResourceViewHeaps;

        // TAA
        //
        {
            // Alloc descriptors
            pResourceViewHeaps->AllocCBV_SRV_UAVDescriptor(5, &m_TaaTable);

            // Compile shader
            D3D12_SHADER_BYTECODE shaderByteCode = {}, shaderByteCodeFirst = {};
            CompileShaderFromFile("TAA.hlsl", NULL, "main", "-T cs_6_0", &shaderByteCode);
            CompileShaderFromFile("TAA.hlsl", NULL, "first", "-T cs_6_0", &shaderByteCodeFirst);

            // Create root signature
            //
            {
                CD3DX12_DESCRIPTOR_RANGE descriptorRanges[2];
                descriptorRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4u, 0u);
                descriptorRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1u, 0u);

                CD3DX12_ROOT_PARAMETER rootParameters;
                rootParameters.InitAsDescriptorTable(2, descriptorRanges);

                CD3DX12_STATIC_SAMPLER_DESC staticSamplerDescs[4];
                staticSamplerDescs[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
                staticSamplerDescs[1].Init(1, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
                staticSamplerDescs[2].Init(2, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
                staticSamplerDescs[3].Init(3, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

                CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
                rootSignatureDesc.Init(1, &rootParameters, ARRAYSIZE(staticSamplerDescs), staticSamplerDescs);

                ID3DBlob *pOutBlob, *pErrorBlob = NULL;
                ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pOutBlob, &pErrorBlob));
                ThrowIfFailed(
                    pDevice->GetDevice()->CreateRootSignature(0, pOutBlob->GetBufferPointer(), pOutBlob->GetBufferSize(), IID_PPV_ARGS(&m_pTaaRootSignature))
                );
                SetName(m_pTaaRootSignature, "m_pTaaRootSignature");

                pOutBlob->Release();
                if (pErrorBlob)
                    pErrorBlob->Release();
            }

            // Create pipeline state
            //
            {
                D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineStateDesc = {};
                pipelineStateDesc.pRootSignature = m_pTaaRootSignature;
                pipelineStateDesc.CS = shaderByteCode;
                pDevice->GetDevice()->CreateComputePipelineState(&pipelineStateDesc, IID_PPV_ARGS(&m_pTaaPipelineState));
                SetName(m_pTaaPipelineState, "m_pTaaPipelineState");
                pipelineStateDesc.CS = shaderByteCodeFirst;
                pDevice->GetDevice()->CreateComputePipelineState(&pipelineStateDesc, IID_PPV_ARGS(&m_pFirstPipelineState));
                SetName(m_pTaaPipelineState, "FirstTaaPipelineState");
            }
        }

        // Sharpener
        //
        {
            // Alloc descriptors
            pResourceViewHeaps->AllocCBV_SRV_UAVDescriptor(3, &m_SharpenerTable);

            // Create root signature
            //
            {
                CD3DX12_DESCRIPTOR_RANGE descriptorRanges[2];
                descriptorRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, 0u);
                descriptorRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 2u, 0u);

                CD3DX12_ROOT_PARAMETER rootParameters;
                rootParameters.InitAsDescriptorTable(2, descriptorRanges);

                CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
                rootSignatureDesc.Init(1, &rootParameters, 0, NULL);

                ID3DBlob *pOutBlob, *pErrorBlob = NULL;
                ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pOutBlob, &pErrorBlob));
                ThrowIfFailed(
                    pDevice->GetDevice()->CreateRootSignature(0, pOutBlob->GetBufferPointer(), pOutBlob->GetBufferSize(), IID_PPV_ARGS(&m_pSharpenerRootSignature))
                );
                SetName(m_pSharpenerRootSignature, "m_pSharpenerRootSignature");

                pOutBlob->Release();
                if (pErrorBlob)
                    pErrorBlob->Release();
            }

            // Create pipeline state
            //
            {
                // Compile shader
                D3D12_SHADER_BYTECODE shaderByteCode = {};
                CompileShaderFromFile("TAASharpenerCS.hlsl", NULL, "mainCS", "-T cs_6_0", &shaderByteCode);

                D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineStateDesc = {};
                pipelineStateDesc.pRootSignature = m_pSharpenerRootSignature;
                pipelineStateDesc.CS = shaderByteCode;

                pDevice->GetDevice()->CreateComputePipelineState(&pipelineStateDesc, IID_PPV_ARGS(&m_pSharpenerPipelineState));
                SetName(m_pSharpenerPipelineState, "m_pSharpenerPipelineState");
            }
            {
                // Compile shader
                D3D12_SHADER_BYTECODE shaderByteCode = {};
                CompileShaderFromFile("TAASharpenerCS.hlsl", NULL, "postCS", "-T cs_6_0", &shaderByteCode);

                D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineStateDesc = {};
                pipelineStateDesc.pRootSignature = m_pSharpenerRootSignature;
                pipelineStateDesc.CS = shaderByteCode;

                pDevice->GetDevice()->CreateComputePipelineState(&pipelineStateDesc, IID_PPV_ARGS(&m_pPostPipelineState));
                SetName(m_pPostPipelineState, "TAA Post PipelineState");
            }
        }

    }

    //--------------------------------------------------------------------------------------
    //
    // OnDestroy
    //
    //--------------------------------------------------------------------------------------
    void TAA::OnDestroy()
    {
        {
            if (m_pTaaRootSignature)
            {
                m_pTaaRootSignature->Release();
                m_pTaaRootSignature = NULL;
            }

            if (m_pTaaPipelineState)
            {
                m_pFirstPipelineState->Release();
                m_pTaaPipelineState->Release();
                m_pTaaPipelineState = NULL;
            }
        }

        {
            if (m_pSharpenerRootSignature)
            {
                m_pSharpenerRootSignature->Release();
                m_pSharpenerRootSignature = NULL;
            }

            if (m_pSharpenerPipelineState)
            {
                m_pSharpenerPipelineState->Release();
                m_pPostPipelineState->Release();
                m_pSharpenerPipelineState = NULL;
                m_pPostPipelineState = 0;
            }
        }
    }

    //--------------------------------------------------------------------------------------
    //
    // OnCreateWindowSizeDependentResources
    //
    //--------------------------------------------------------------------------------------
    void TAA::OnCreateWindowSizeDependentResources(uint32_t Width, uint32_t Height, GBuffer *pGBuffer)
    {
        m_pGBuffer = pGBuffer;

        m_Width = Width;
        m_Height = Height;

        // TAA buffers
        //
        CD3DX12_RESOURCE_DESC TAADesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16B16A16_FLOAT, Width, Height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        m_TAABuffer.Init(m_pDevice, "m_TAABuffer", &TAADesc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, NULL);

        CD3DX12_RESOURCE_DESC HistoryDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16B16A16_FLOAT, Width, Height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        m_HistoryBuffer.Init(m_pDevice, "m_HistoryBuffer", &HistoryDesc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, NULL);

        // set TAA descriptor
        m_pGBuffer->m_HDR.CreateSRV(0, &m_TaaTable);
        m_pGBuffer->m_DepthBuffer.CreateSRV(1, &m_TaaTable);
        m_HistoryBuffer.CreateSRV(2, &m_TaaTable);
        m_pGBuffer->m_MotionVectors.CreateSRV(3, &m_TaaTable);
        m_TAABuffer.CreateUAV(4, &m_TaaTable);

        // set Sharpener descriptor
        m_TAABuffer.CreateSRV(0, &m_SharpenerTable);
        m_pGBuffer->m_HDR.CreateUAV(1, &m_SharpenerTable);
        m_HistoryBuffer.CreateUAV(2, &m_SharpenerTable);

        m_bFirst = true;
    }
    
    //--------------------------------------------------------------------------------------
    //
    // OnDestroyWindowSizeDependentResources
    //
    //--------------------------------------------------------------------------------------
    void TAA::OnDestroyWindowSizeDependentResources()
    {
        m_HistoryBuffer.OnDestroy();
        m_TAABuffer.OnDestroy();
    }

    //--------------------------------------------------------------------------------------
    //
    // Draw
    //
    //--------------------------------------------------------------------------------------
    void TAA::Draw(ID3D12GraphicsCommandList *pCommandList, D3D12_RESOURCE_STATES hdrBefore, D3D12_RESOURCE_STATES hdrAfter)
    {

        {
            D3D12_RESOURCE_BARRIER preTAA[] = {
                CD3DX12_RESOURCE_BARRIER::Transition(m_pGBuffer->m_HDR.GetResource(), hdrBefore, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
                CD3DX12_RESOURCE_BARRIER::Transition(m_pGBuffer->m_DepthBuffer.GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
                CD3DX12_RESOURCE_BARRIER::Transition(m_pGBuffer->m_MotionVectors.GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
                CD3DX12_RESOURCE_BARRIER::Transition(m_TAABuffer.GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
            };
            pCommandList->ResourceBarrier(ARRAYSIZE(preTAA), preTAA);

            {
                UserMarker marker(pCommandList, "TAA");

                // Bind the descriptor heaps and root signature
                //
                ID3D12DescriptorHeap *pDescriptorHeaps[] = { m_pResourceViewHeaps->GetCBV_SRV_UAVHeap() };
                pCommandList->SetDescriptorHeaps(ARRAYSIZE(pDescriptorHeaps), pDescriptorHeaps);
                pCommandList->SetComputeRootSignature(m_pTaaRootSignature);

                // Bind the descriptor set
                //
                pCommandList->SetComputeRootDescriptorTable(0, m_TaaTable.GetGPU());

                // Bind the pipeline state
                //
                if (m_bFirst)
                {
                    m_bFirst = false;
                    pCommandList->SetPipelineState(m_pFirstPipelineState);
                } else
                    pCommandList->SetPipelineState(m_pTaaPipelineState);

                // Dispatch
                //
                uint32_t ThreadGroupCountX = (m_Width + 15) / 16;
                uint32_t ThreadGroupCountY = (m_Height + 15) / 16;
                pCommandList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
            }
        }

        // sharpen pass
        //
        {
            D3D12_RESOURCE_BARRIER postTAA[] = {
                CD3DX12_RESOURCE_BARRIER::Transition(m_pGBuffer->m_HDR.GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
                CD3DX12_RESOURCE_BARRIER::Transition(m_HistoryBuffer.GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
                CD3DX12_RESOURCE_BARRIER::Transition(m_TAABuffer.GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            };
            pCommandList->ResourceBarrier(ARRAYSIZE(postTAA), postTAA);

            {
                UserMarker marker(pCommandList, "TAA");

                ID3D12DescriptorHeap *pDescriptorHeaps[] = { m_pResourceViewHeaps->GetCBV_SRV_UAVHeap() };
                pCommandList->SetDescriptorHeaps(ARRAYSIZE(pDescriptorHeaps), pDescriptorHeaps);
                pCommandList->SetComputeRootSignature(m_pSharpenerRootSignature);
                pCommandList->SetComputeRootDescriptorTable(0, m_SharpenerTable.GetGPU());
                if( m_bSharpening )
                    pCommandList->SetPipelineState(m_pSharpenerPipelineState);
                else
                    pCommandList->SetPipelineState(m_pPostPipelineState);

                uint32_t ThreadGroupCountX = (m_Width + 7) / 8;
                uint32_t ThreadGroupCountY = (m_Height + 7) / 8;
                pCommandList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
            }

            D3D12_RESOURCE_BARRIER postSharpen[] = {
                CD3DX12_RESOURCE_BARRIER::Transition(m_pGBuffer->m_HDR.GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, hdrAfter),
                CD3DX12_RESOURCE_BARRIER::Transition(m_pGBuffer->m_DepthBuffer.GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE),
                CD3DX12_RESOURCE_BARRIER::Transition(m_pGBuffer->m_MotionVectors.GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
                CD3DX12_RESOURCE_BARRIER::Transition(m_HistoryBuffer.GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
            };
            pCommandList->ResourceBarrier(ARRAYSIZE(postSharpen), postSharpen);
        }

    }
}
