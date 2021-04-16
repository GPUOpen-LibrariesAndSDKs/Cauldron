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

#include "GBuffer.h"
#include "Helper.h"

namespace CAULDRON_DX12
{
    void GBufferRenderPass::OnCreate(GBuffer *pGBuffer, GBufferFlags flags)
    {
        m_GBufferFlags = flags;
        m_pGBuffer = pGBuffer;

    }

    void GBufferRenderPass::OnDestroy()
    {
    }

    void GBufferRenderPass::OnCreateWindowSizeDependentResources(uint32_t Width, uint32_t Height)
    {
        m_RenderTargetViewList.clear();

        if (m_GBufferFlags & GBUFFER_DEPTH)
        {
            m_DepthBuffer = m_pGBuffer->m_DepthBufferDSV.GetCPU();
        }

        // Create Texture + RTV, to hold the resolved scene 
        //
        if (m_GBufferFlags & GBUFFER_FORWARD)
        {
            m_RenderTargetViewList.push_back(m_pGBuffer->m_HDRRTV.GetCPU());
        }

        // Motion Vectors
        //
        if (m_GBufferFlags & GBUFFER_MOTION_VECTORS)
        {
            m_RenderTargetViewList.push_back(m_pGBuffer->m_MotionVectorsRTV.GetCPU());
        }

        // Normal Buffer
        //
        if (m_GBufferFlags & GBUFFER_NORMAL_BUFFER)
        {
            m_RenderTargetViewList.push_back(m_pGBuffer->m_NormalBufferRTV.GetCPU());
        }

        // Diffuse
        //
        if (m_GBufferFlags & GBUFFER_DIFFUSE)
        {
            m_RenderTargetViewList.push_back(m_pGBuffer->m_DiffuseRTV.GetCPU());
        }

        // Specular Roughness
        //
        if (m_GBufferFlags & GBUFFER_SPECULAR_ROUGHNESS)
        {
            m_RenderTargetViewList.push_back(m_pGBuffer->m_SpecularRoughnessRTV.GetCPU());
        }
    }

    void GBufferRenderPass::OnDestroyWindowSizeDependentResources()
    {
    }

    void GBufferRenderPass::BeginPass(ID3D12GraphicsCommandList* pCommandList, bool bClear)
    {
        if (bClear)
        {
            float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
            for (D3D12_CPU_DESCRIPTOR_HANDLE handle : m_RenderTargetViewList)
            {
                pCommandList->ClearRenderTargetView(handle, clearColor, 0, nullptr);
            }

            if (m_GBufferFlags & GBUFFER_DEPTH)
            {
                pCommandList->ClearDepthStencilView(m_DepthBuffer, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
            }
        }

        pCommandList->OMSetRenderTargets((UINT)m_RenderTargetViewList.size(), m_RenderTargetViewList.data(), false, &m_DepthBuffer);
    }

    void GBufferRenderPass::EndPass()
    {        
    }

    void GBufferRenderPass::GetCompilerDefinesAndGBufferFormats(DefineList &defines, std::vector<DXGI_FORMAT> &outFormats, DXGI_FORMAT &depthFormat)
    {
        int rtIndex = 0;

        depthFormat = (m_GBufferFlags & GBUFFER_DEPTH) ? m_pGBuffer->m_DepthBuffer.GetFormat() : DXGI_FORMAT_UNKNOWN;

        // GDR (Forward pass)
        //
        if (m_GBufferFlags & GBUFFER_FORWARD)
        {
            defines["HAS_FORWARD_RT"] = std::to_string(rtIndex++);
            outFormats.push_back(m_pGBuffer->m_HDR.GetFormat());
        }

        // Motion Vectors
        //
        if (m_GBufferFlags & GBUFFER_MOTION_VECTORS)
        {
            defines["HAS_MOTION_VECTORS"] = std::to_string(1);
            defines["HAS_MOTION_VECTORS_RT"] = std::to_string(rtIndex++);
            outFormats.push_back(m_pGBuffer->m_MotionVectors.GetFormat());
        }

        // Normal Buffer
        //
        if (m_GBufferFlags & GBUFFER_NORMAL_BUFFER)
        {
            defines["HAS_NORMALS_RT"] = std::to_string(rtIndex++);
            outFormats.push_back(m_pGBuffer->m_NormalBuffer.GetFormat());
        }

        // Diffuse
        //
        if (m_GBufferFlags & GBUFFER_DIFFUSE)
        {
            defines["HAS_DIFFUSE_RT"] = std::to_string(rtIndex++);
            outFormats.push_back(m_pGBuffer->m_Diffuse.GetFormat());
        }

        // Specular roughness
        //
        if (m_GBufferFlags & GBUFFER_SPECULAR_ROUGHNESS)
        {
            defines["HAS_SPECULAR_ROUGHNESS_RT"] = std::to_string(rtIndex++);
            outFormats.push_back(m_pGBuffer->m_SpecularRoughness.GetFormat());
        }
    }

    int32_t GBufferRenderPass::GetSampleCount()
    { 
        return m_pGBuffer->GetSampleCount(); 
    }

    void GBuffer::OnCreate(Device *pDevice, ResourceViewHeaps *pHeaps, const std::map<GBufferFlags, DXGI_FORMAT> &formats, uint32_t sampleCount)
    {
        m_GBufferFlags = GBUFFER_NONE;
        for (auto a : formats)
            m_GBufferFlags = m_GBufferFlags | a.first;

        m_pDevice = pDevice;
        m_sampleCount = sampleCount;
        m_formats = formats;

        // just allocate RTVs for everything (despite that we might only just use a few)
        
        pHeaps->AllocDSVDescriptor(1, &m_DepthBufferDSV);
        pHeaps->AllocCBV_SRV_UAVDescriptor(1, &m_DepthBufferSRV);
        
        pHeaps->AllocRTVDescriptor(1, &m_HDRRTV);
        pHeaps->AllocCBV_SRV_UAVDescriptor(1, &m_HDRSRV);
        pHeaps->AllocCBV_SRV_UAVDescriptor(1, &m_HDRUAV);

        pHeaps->AllocRTVDescriptor(1, &m_MotionVectorsRTV);
        pHeaps->AllocCBV_SRV_UAVDescriptor(1, &m_MotionVectorsSRV);

        pHeaps->AllocRTVDescriptor(1, &m_NormalBufferRTV);
        pHeaps->AllocCBV_SRV_UAVDescriptor(1, &m_NormalBufferSRV);

        pHeaps->AllocRTVDescriptor(1, &m_DiffuseRTV);
        pHeaps->AllocCBV_SRV_UAVDescriptor(1, &m_DiffuseSRV);
        
        pHeaps->AllocRTVDescriptor(1, &m_SpecularRoughnessRTV);
        pHeaps->AllocCBV_SRV_UAVDescriptor(1, &m_SpecularRoughnessSRV);        
    }

    void GBuffer::OnDestroy()
    {

    }

    void GBuffer::OnCreateWindowSizeDependentResources(SwapChain *pSwapChain, uint32_t Width, uint32_t Height)
    {
        if (m_GBufferFlags & GBUFFER_DEPTH)
        {
            m_DepthBuffer.InitDepthStencil(m_pDevice, "m_depthbuffer", &CD3DX12_RESOURCE_DESC::Tex2D(m_formats[GBUFFER_DEPTH], Width, Height, 1, 1, m_sampleCount, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL));
            m_DepthBuffer.CreateDSV(0, &m_DepthBufferDSV);
            m_DepthBuffer.CreateSRV(0, &m_DepthBufferSRV);
        }

        // Create Texture + RTV, to hold the resolved scene 
        //
        if (m_GBufferFlags & GBUFFER_FORWARD)
        {
            m_HDR.InitRenderTarget(m_pDevice, "m_HDR", &CD3DX12_RESOURCE_DESC::Tex2D(m_formats[GBUFFER_FORWARD], Width, Height, 1, 1, m_sampleCount, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS), D3D12_RESOURCE_STATE_RENDER_TARGET);
            m_HDR.CreateSRV(0, &m_HDRSRV);
            m_HDR.CreateUAV(0, &m_HDRUAV);
            m_HDR.CreateRTV(0, &m_HDRRTV);
        }

        // Motion Vectors
        //
        if (m_GBufferFlags & GBUFFER_MOTION_VECTORS)
        {
            m_MotionVectors.InitRenderTarget(m_pDevice, "m_MotionVector", &CD3DX12_RESOURCE_DESC::Tex2D(m_formats[GBUFFER_MOTION_VECTORS], Width, Height, 1, 1, m_sampleCount, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET));
            m_MotionVectors.CreateRTV(0, &m_MotionVectorsRTV);
            m_MotionVectors.CreateSRV(0, &m_MotionVectorsSRV);
        }

        // Normal Buffer
        //
        if (m_GBufferFlags & GBUFFER_NORMAL_BUFFER)
        {
            m_NormalBuffer.InitRenderTarget(m_pDevice, "m_NormalBuffer", &CD3DX12_RESOURCE_DESC::Tex2D(m_formats[GBUFFER_NORMAL_BUFFER], Width, Height, 1, 1, m_sampleCount, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET));
            m_NormalBuffer.CreateRTV(0, &m_NormalBufferRTV);
            m_NormalBuffer.CreateSRV(0, &m_NormalBufferSRV);
        }

        // Diffuse
        //
        if (m_GBufferFlags & GBUFFER_DIFFUSE)
        {
            m_Diffuse.InitRenderTarget(m_pDevice, "m_Diffuse", &CD3DX12_RESOURCE_DESC::Tex2D(m_formats[GBUFFER_DIFFUSE], Width, Height, 1, 1, m_sampleCount, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET));
            m_Diffuse.CreateRTV(0, &m_DiffuseRTV);
            m_Diffuse.CreateSRV(0, &m_DiffuseSRV);
        }

        // Specular Roughness
        //
        if (m_GBufferFlags & GBUFFER_SPECULAR_ROUGHNESS)
        {
            m_SpecularRoughness.InitRenderTarget(m_pDevice, "m_SpecularRoughness", &CD3DX12_RESOURCE_DESC::Tex2D(m_formats[GBUFFER_SPECULAR_ROUGHNESS], Width, Height, 1, 1, m_sampleCount, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET));
            m_SpecularRoughness.CreateRTV(0, &m_SpecularRoughnessRTV);
            m_SpecularRoughness.CreateSRV(0, &m_SpecularRoughnessSRV);
        }
    }

    void GBuffer::OnDestroyWindowSizeDependentResources()
    {
        if (m_GBufferFlags & GBUFFER_SPECULAR_ROUGHNESS)
            m_SpecularRoughness.OnDestroy();
        
        if (m_GBufferFlags & GBUFFER_DIFFUSE)
            m_Diffuse.OnDestroy();        
        
        if (m_GBufferFlags & GBUFFER_NORMAL_BUFFER)
            m_NormalBuffer.OnDestroy();
        
        if (m_GBufferFlags & GBUFFER_MOTION_VECTORS)
            m_MotionVectors.OnDestroy();        
        
        if (m_GBufferFlags & GBUFFER_FORWARD)
            m_HDR.OnDestroy();
        
        if (m_GBufferFlags & GBUFFER_DEPTH)
            m_DepthBuffer.OnDestroy();
    }
}