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
#include "GBuffer.h"
#include "Helper.h"
#include <assert.h>

namespace CAULDRON_VK
{
    void GBufferRenderPass::OnCreate(GBuffer *pGBuffer, GBufferFlags flags, bool bClear, const std::string &name)
    {
        m_flags = flags;
        m_pGBuffer = pGBuffer;
        m_pDevice = pGBuffer->GetDevice();

        m_renderPass = pGBuffer->CreateRenderPass(flags, bClear);
        SetResourceName(m_pDevice->GetDevice(), VK_OBJECT_TYPE_RENDER_PASS, (uint64_t)m_renderPass, name.c_str());
    }

    void GBufferRenderPass::OnDestroy()
    {
        vkDestroyRenderPass(m_pGBuffer->GetDevice()->GetDevice(), m_renderPass, nullptr);
    }

    void GBufferRenderPass::OnCreateWindowSizeDependentResources(uint32_t Width, uint32_t Height)
    {
        std::vector<VkImageView> attachments;
        m_pGBuffer->GetAttachmentList(m_flags, &attachments, &m_clearValues);
        m_frameBuffer = CreateFrameBuffer(m_pGBuffer->GetDevice()->GetDevice(), m_renderPass, &attachments, Width, Height);
    }

    void GBufferRenderPass::OnDestroyWindowSizeDependentResources()
    {
        vkDestroyFramebuffer(m_pGBuffer->GetDevice()->GetDevice(), m_frameBuffer, nullptr);
    }

    void GBufferRenderPass::BeginPass(VkCommandBuffer commandList, VkRect2D renderArea)
    {
        VkRenderPassBeginInfo rp_begin;
        rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rp_begin.pNext = NULL;
        rp_begin.renderPass = m_renderPass;
        rp_begin.framebuffer = m_frameBuffer;
        rp_begin.renderArea = renderArea;
        rp_begin.pClearValues = m_clearValues.data();
        rp_begin.clearValueCount = (uint32_t)m_clearValues.size();
        vkCmdBeginRenderPass(commandList, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

        SetViewportAndScissor(commandList, renderArea.offset.x, renderArea.offset.y, renderArea.extent.width, renderArea.extent.height);
    }
    
    void GBufferRenderPass::EndPass(VkCommandBuffer commandList)
    {
        vkCmdEndRenderPass(commandList);
    }

    void GBufferRenderPass::GetCompilerDefines(DefineList &defines)
    {
        int rtIndex = 0;

        // GDR (Forward pass)
        //
        if (m_flags & GBUFFER_FORWARD)
        {
            defines["HAS_FORWARD_RT"] = std::to_string(rtIndex++);
        }

        // Motion Vectors
        //
        if (m_flags & GBUFFER_MOTION_VECTORS)
        {
            defines["HAS_MOTION_VECTORS"] = std::to_string(1);
            defines["HAS_MOTION_VECTORS_RT"] = std::to_string(rtIndex++);
        }

        // Normal Buffer
        //
        if (m_flags & GBUFFER_NORMAL_BUFFER)
        {
            defines["HAS_NORMALS_RT"] = std::to_string(rtIndex++);
        }

        // Diffuse
        //
        if (m_flags & GBUFFER_DIFFUSE)
        {
            defines["HAS_DIFFUSE_RT"] = std::to_string(rtIndex++);
        }

        // Specular roughness
        //
        if (m_flags & GBUFFER_SPECULAR_ROUGHNESS)
        {
            defines["HAS_SPECULAR_ROUGHNESS_RT"] = std::to_string(rtIndex++);
        }
    }

    VkSampleCountFlagBits GBufferRenderPass::GetSampleCount()
    { 
        return m_pGBuffer->GetSampleCount(); 
    }

    //-------------------------------------------------------------------------------------

    void GBuffer::OnCreate(Device *pDevice, ResourceViewHeaps *pHeaps, const std::map<GBufferFlags, VkFormat> &formats, int sampleCount)
    {
        m_GBufferFlags = GBUFFER_NONE;
        for (auto a : formats)
            m_GBufferFlags = m_GBufferFlags | a.first;

        m_pDevice = pDevice;
        m_sampleCount = (VkSampleCountFlagBits)sampleCount;
        m_formats = formats;
    }

    void GBuffer::OnDestroy()
    {
    }

    //
    // create render pass based on usage flags
    //
    VkRenderPass GBuffer::CreateRenderPass(GBufferFlags flags, bool bClear)
    {
        VkAttachmentDescription depthAttachment;
        VkAttachmentDescription colorAttachments[10];
        uint32_t colorAttanchmentCount = 0;

        auto addAttachment = bClear ? AttachClearBeforeUse : AttachBlending;
        VkImageLayout previousColor = bClear ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkImageLayout previousDepth = bClear ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        if (flags & GBUFFER_FORWARD)
        {
            addAttachment(m_formats[GBUFFER_FORWARD], m_sampleCount, previousColor, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, &colorAttachments[colorAttanchmentCount++]);
            assert(m_GBufferFlags & GBUFFER_FORWARD); // asserts if there if the RT is not present in the GBuffer
        }

        if (flags & GBUFFER_MOTION_VECTORS)
        {
            addAttachment(m_formats[GBUFFER_MOTION_VECTORS], m_sampleCount, previousColor, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, &colorAttachments[colorAttanchmentCount++]);
            assert(m_GBufferFlags & GBUFFER_MOTION_VECTORS); // asserts if there if the RT is not present in the GBuffer
        }

        if (flags & GBUFFER_NORMAL_BUFFER)
        {
            addAttachment(m_formats[GBUFFER_NORMAL_BUFFER], m_sampleCount, previousColor, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, &colorAttachments[colorAttanchmentCount++]);
            assert(m_GBufferFlags & GBUFFER_NORMAL_BUFFER); // asserts if there if the RT is not present in the GBuffer
        }

        if (flags & GBUFFER_DIFFUSE)
        {
            addAttachment(m_formats[GBUFFER_DIFFUSE], m_sampleCount, previousColor, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, &colorAttachments[colorAttanchmentCount++]);
            assert(m_GBufferFlags & GBUFFER_DIFFUSE); // asserts if there if the RT is not present in the GBuffer
        }

        if (flags & GBUFFER_SPECULAR_ROUGHNESS)
        {
            addAttachment(m_formats[GBUFFER_SPECULAR_ROUGHNESS], m_sampleCount, previousColor, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, &colorAttachments[colorAttanchmentCount++]);
            assert(m_GBufferFlags & GBUFFER_SPECULAR_ROUGHNESS); // asserts if there if the RT is not present in the GBuffer
        }

        if (flags & GBUFFER_DEPTH)
        {
            addAttachment(m_formats[GBUFFER_DEPTH], m_sampleCount, previousDepth, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, &depthAttachment);
            assert(m_GBufferFlags & GBUFFER_DEPTH); // asserts if there if the RT is not present in the GBuffer
        }

        return CreateRenderPassOptimal(m_pDevice->GetDevice(), colorAttanchmentCount, colorAttachments, &depthAttachment);
    }

    void GBuffer::GetAttachmentList(GBufferFlags flags, std::vector<VkImageView> *pAttachments, std::vector<VkClearValue> *pClearValues)
    {
        pAttachments->clear();

        // Create Texture + RTV, to hold the resolved scene
        //
        if (flags & GBUFFER_FORWARD)
        {
            pAttachments->push_back(m_HDRSRV);

            if (pClearValues)
            {
                VkClearValue cv;
                cv.color = { 0.0f, 0.0f, 0.0f, 0.0f };
                pClearValues->push_back(cv);
            }
        }

        // Motion Vectors
        //
        if (flags & GBUFFER_MOTION_VECTORS)
        {
            pAttachments->push_back(m_MotionVectorsSRV);

            if (pClearValues)
            {
                VkClearValue cv;
                cv.color = { 0.0f, 0.0f, 0.0f, 0.0f };
                pClearValues->push_back(cv);
            }
        }

        // Normal Buffer
        //
        if (flags & GBUFFER_NORMAL_BUFFER)
        {
            pAttachments->push_back(m_NormalBufferSRV);

            if (pClearValues)
            {
                VkClearValue cv;
                cv.color = { 0.0f, 0.0f, 0.0f, 0.0f };
                pClearValues->push_back(cv);
            }
        }

        // Diffuse
        //
        if (flags & GBUFFER_DIFFUSE)
        {
            pAttachments->push_back(m_DiffuseSRV);

            if (pClearValues)
            {
                VkClearValue cv;
                cv.color = { 0.0f, 0.0f, 0.0f, 0.0f };
                pClearValues->push_back(cv);
            }
        }

        // Specular Roughness
        //
        if (flags & GBUFFER_SPECULAR_ROUGHNESS)
        {
            pAttachments->push_back(m_SpecularRoughnessSRV);

            if (pClearValues)
            {
                VkClearValue cv;
                cv.color = { 0.0f, 0.0f, 0.0f, 0.0f };
                pClearValues->push_back(cv);
            }
        }

        // Create depth buffer
        //
        if (flags & GBUFFER_DEPTH)
        {
            pAttachments->push_back(m_DepthBufferDSV);

            if (pClearValues)
            {
                VkClearValue cv;
                cv.depthStencil = { 1.0f, 0 };
                pClearValues->push_back(cv);
            }
        }
    }

    void GBuffer::OnCreateWindowSizeDependentResources(SwapChain *pSwapChain, uint32_t Width, uint32_t Height)
    {
        // Create Texture + RTV, to hold the resolved scene
        //
        if (m_GBufferFlags & GBUFFER_FORWARD)
        {
            m_HDR.InitRenderTarget(m_pDevice, Width, Height, m_formats[GBUFFER_FORWARD], m_sampleCount, (VkImageUsageFlags)(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT), false, "m_HDR");
            m_HDR.CreateSRV(&m_HDRSRV);
        }

        // Motion Vectors
        //
        if (m_GBufferFlags & GBUFFER_MOTION_VECTORS)
        {
            m_MotionVectors.InitRenderTarget(m_pDevice, Width, Height, m_formats[GBUFFER_MOTION_VECTORS], m_sampleCount, (VkImageUsageFlags)(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT), false, "m_MotionVector");
            m_MotionVectors.CreateSRV(&m_MotionVectorsSRV);
        }

        // Normal Buffer
        //
        if (m_GBufferFlags & GBUFFER_NORMAL_BUFFER)
        {
            m_NormalBuffer.InitRenderTarget(m_pDevice, Width, Height, m_formats[GBUFFER_NORMAL_BUFFER], m_sampleCount, (VkImageUsageFlags)(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT), false, "m_NormalBuffer");
            m_NormalBuffer.CreateSRV(&m_NormalBufferSRV);
        }

        // Diffuse
        //
        if (m_GBufferFlags & GBUFFER_DIFFUSE)
        {
            m_Diffuse.InitRenderTarget(m_pDevice, Width, Height, m_formats[GBUFFER_DIFFUSE], m_sampleCount, (VkImageUsageFlags)(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT), false, "m_Diffuse");
            m_Diffuse.CreateSRV(&m_DiffuseSRV);
        }

        // Specular Roughness
        //
        if (m_GBufferFlags & GBUFFER_SPECULAR_ROUGHNESS)
        {
            m_SpecularRoughness.InitRenderTarget(m_pDevice, Width, Height, m_formats[GBUFFER_SPECULAR_ROUGHNESS], m_sampleCount, (VkImageUsageFlags)(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT), false, "m_SpecularRoughness");
            m_SpecularRoughness.CreateSRV(&m_SpecularRoughnessSRV);
        }

        // Create depth buffer
        //
        if (m_GBufferFlags & GBUFFER_DEPTH)
        {
            m_DepthBuffer.InitDepthStencil(m_pDevice, Width, Height, m_formats[GBUFFER_DEPTH], m_sampleCount, "DepthBuffer");
            m_DepthBuffer.CreateDSV(&m_DepthBufferDSV);
            m_DepthBuffer.CreateRTV(&m_DepthBufferSRV);            
        }
    }

    void GBuffer::OnDestroyWindowSizeDependentResources()
    {
        if (m_GBufferFlags & GBUFFER_SPECULAR_ROUGHNESS)
        {
            vkDestroyImageView(m_pDevice->GetDevice(), m_SpecularRoughnessSRV, nullptr);
            m_SpecularRoughness.OnDestroy();
        }

        if (m_GBufferFlags & GBUFFER_DIFFUSE)
        {
            vkDestroyImageView(m_pDevice->GetDevice(), m_DiffuseSRV, nullptr);
            m_Diffuse.OnDestroy();
        }

        if (m_GBufferFlags & GBUFFER_NORMAL_BUFFER)
        {
            vkDestroyImageView(m_pDevice->GetDevice(), m_NormalBufferSRV, nullptr);
            m_NormalBuffer.OnDestroy();
        }

        if (m_GBufferFlags & GBUFFER_MOTION_VECTORS)
        {
            vkDestroyImageView(m_pDevice->GetDevice(), m_MotionVectorsSRV, nullptr);
            m_MotionVectors.OnDestroy();
        }

        if (m_GBufferFlags & GBUFFER_FORWARD)
        {
            vkDestroyImageView(m_pDevice->GetDevice(), m_HDRSRV, nullptr);
            m_HDR.OnDestroy();
        }

        if (m_GBufferFlags & GBUFFER_DEPTH)
        {
            vkDestroyImageView(m_pDevice->GetDevice(), m_DepthBufferDSV, nullptr);
            vkDestroyImageView(m_pDevice->GetDevice(), m_DepthBufferSRV, nullptr);
            m_DepthBuffer.OnDestroy();
        }
    }
}