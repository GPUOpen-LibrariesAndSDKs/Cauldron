// AMD Cauldron code
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
#include "Base/DynamicBufferRing.h"
#include "Base/StaticBufferPool.h"
#include "Base/ExtDebugUtils.h"
#include "Base/UploadHeap.h"
#include "Base/Texture.h"
#include "Base/Helper.h"
#include "TAA.h"

namespace CAULDRON_VK
{
    void TAA::OnCreate(Device* pDevice, ResourceViewHeaps *pResourceViewHeaps, StaticBufferPool  *pStaticBufferPool, DynamicBufferRing *pDynamicBufferRing, bool sharpening)
    {
        m_bSharpening = sharpening;
        m_pDevice = pDevice;
        m_pResourceViewHeaps = pResourceViewHeaps;
        VkResult res;

        // TAA
        //
        {
            // Create samplers
            VkSamplerCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            info.magFilter = VK_FILTER_NEAREST;
            info.minFilter = VK_FILTER_NEAREST;
            info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            info.minLod = -1000;
            info.maxLod = 1000;
            info.maxAnisotropy = 1.0f;

            res = vkCreateSampler(pDevice->GetDevice(), &info, NULL, &m_samplers[0]);
            assert(res == VK_SUCCESS);

            res = vkCreateSampler(pDevice->GetDevice(), &info, NULL, &m_samplers[1]);
            assert(res == VK_SUCCESS);

            res = vkCreateSampler(pDevice->GetDevice(), &info, NULL, &m_samplers[3]);
            (res == VK_SUCCESS);

            info.magFilter = VK_FILTER_LINEAR;
            info.minFilter = VK_FILTER_LINEAR;
            info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            res = vkCreateSampler(pDevice->GetDevice(), &info, NULL, &m_samplers[2]);
            assert(res == VK_SUCCESS);

            // Create VkDescriptor Set Layout Bindings
            //

            std::vector<VkDescriptorSetLayoutBinding> m_TAAInputs(4+1+4);
            for (int i = 0; i < 4; i++)
            {
                m_TAAInputs[i].binding = i;
                m_TAAInputs[i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                m_TAAInputs[i].descriptorCount = 1;
                m_TAAInputs[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                m_TAAInputs[i].pImmutableSamplers = NULL;
            }

            m_TAAInputs[4].binding = 4;
            m_TAAInputs[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            m_TAAInputs[4].descriptorCount = 1;
            m_TAAInputs[4].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            m_TAAInputs[4].pImmutableSamplers = NULL;

            for (int i = 5; i < 4 + 5; i++)
            {
                m_TAAInputs[i].binding = i;
                m_TAAInputs[i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
                m_TAAInputs[i].descriptorCount = 1;
                m_TAAInputs[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                m_TAAInputs[i].pImmutableSamplers = NULL;// &m_samplers[i];
            }

            m_pResourceViewHeaps->CreateDescriptorSetLayout(&m_TAAInputs, &m_TaaDescriptorSetLayout);
            m_pResourceViewHeaps->AllocDescriptor(m_TaaDescriptorSetLayout, &m_TaaDescriptorSet);

            m_TAA.OnCreate(m_pDevice, "TAA.hlsl", "main", "-T cs_6_0", m_TaaDescriptorSetLayout, 16, 16, 1, NULL);
            m_TAAFirst.OnCreate(m_pDevice, "TAA.hlsl", "first", "-T cs_6_0", m_TaaDescriptorSetLayout, 16, 16, 1, NULL);
        }

        // Sharpener
        //
        {
            std::vector<VkDescriptorSetLayoutBinding> m_SharpenInputs(3);
            m_SharpenInputs[0].binding = 0;
            m_SharpenInputs[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            m_SharpenInputs[0].descriptorCount = 1;
            m_SharpenInputs[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            m_SharpenInputs[0].pImmutableSamplers = NULL;

            m_SharpenInputs[1].binding = 1;
            m_SharpenInputs[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            m_SharpenInputs[1].descriptorCount = 1;
            m_SharpenInputs[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            m_SharpenInputs[1].pImmutableSamplers = NULL;

            m_SharpenInputs[2].binding = 2;
            m_SharpenInputs[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            m_SharpenInputs[2].descriptorCount = 1;
            m_SharpenInputs[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            m_SharpenInputs[2].pImmutableSamplers = NULL;

            m_pResourceViewHeaps->CreateDescriptorSetLayout(&m_SharpenInputs, &m_SharpenDescriptorSetLayout);
            m_pResourceViewHeaps->AllocDescriptor(m_SharpenDescriptorSetLayout, &m_SharpenDescriptorSet);

            m_Sharpen.OnCreate(m_pDevice, "TAASharpenerCS.hlsl", "mainCS", "-T cs_6_0", m_SharpenDescriptorSetLayout, 8, 8, 1, NULL);            
            m_Post.OnCreate(m_pDevice, "TAASharpenerCS.hlsl", "postCS", "-T cs_6_0", m_SharpenDescriptorSetLayout, 8, 8, 1, NULL);
        }
    }

    void TAA::OnDestroy()
    {
        m_TAA.OnDestroy();
        m_TAAFirst.OnDestroy();
        vkDestroyDescriptorSetLayout(m_pDevice->GetDevice(), m_TaaDescriptorSetLayout, NULL);

        m_Sharpen.OnDestroy();
        m_Post.OnDestroy();
        vkDestroyDescriptorSetLayout(m_pDevice->GetDevice(), m_SharpenDescriptorSetLayout, NULL);

        for (int i = 0; i < 4; i++)
            vkDestroySampler(m_pDevice->GetDevice(), m_samplers[i] , nullptr);

        m_pResourceViewHeaps->FreeDescriptor(m_SharpenDescriptorSet);
        m_pResourceViewHeaps->FreeDescriptor(m_TaaDescriptorSet);
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
        m_TAABuffer.InitRenderTarget(m_pDevice, Width, Height, VK_FORMAT_R16G16B16A16_SFLOAT, VK_SAMPLE_COUNT_1_BIT, (VkImageUsageFlags)(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT), false, "m_TAABuffer");
        m_TAABuffer.CreateSRV(&m_TAABufferSRV);
        m_TAABuffer.CreateSRV(&m_TAABufferUAV);

        m_HistoryBuffer.InitRenderTarget(m_pDevice, Width, Height, VK_FORMAT_R16G16B16A16_SFLOAT, VK_SAMPLE_COUNT_1_BIT, (VkImageUsageFlags)(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT), false, "m_HistoryBuffer");
        m_HistoryBuffer.CreateSRV(&m_HistoryBufferSRV);
        m_HistoryBuffer.CreateSRV(&m_HistoryBufferUAV);
        
        m_TexturesInUndefinedLayout = true;

        // update the TAA descriptor
        //
        SetDescriptorSet(m_pDevice->GetDevice(), 0, m_pGBuffer->m_HDRSRV, NULL, m_TaaDescriptorSet);
        SetDescriptorSet(m_pDevice->GetDevice(), 1, m_pGBuffer->m_DepthBufferSRV, NULL, m_TaaDescriptorSet);
        SetDescriptorSet(m_pDevice->GetDevice(), 2, m_HistoryBufferSRV, NULL, m_TaaDescriptorSet);
        SetDescriptorSet(m_pDevice->GetDevice(), 3, m_pGBuffer->m_MotionVectorsSRV, NULL, m_TaaDescriptorSet);
        SetDescriptorSet(m_pDevice->GetDevice(), 4, m_TAABufferUAV, m_TaaDescriptorSet);

        for (int i = 0; i < 4; i++)
        {
            VkDescriptorImageInfo samplerInfo = {};
            samplerInfo.sampler = m_samplers[i];

            VkWriteDescriptorSet write;
            write = {};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.pNext = NULL;
            write.dstSet = m_TaaDescriptorSet;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            write.pImageInfo = &samplerInfo;
            write.dstBinding = i+5;
            write.dstArrayElement = 0;

            vkUpdateDescriptorSets(m_pDevice->GetDevice(), 1, &write, 0, NULL);
        }

        // update the Sharpen descriptor
        //
        SetDescriptorSet(m_pDevice->GetDevice(), 0, m_TAABufferSRV, NULL, m_SharpenDescriptorSet);
        SetDescriptorSet(m_pDevice->GetDevice(), 1, m_pGBuffer->m_HDRSRV, m_SharpenDescriptorSet);
        SetDescriptorSet(m_pDevice->GetDevice(), 2, m_HistoryBufferSRV, m_SharpenDescriptorSet);

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

        vkDestroyImageView(m_pDevice->GetDevice(), m_TAABufferSRV, nullptr);
        vkDestroyImageView(m_pDevice->GetDevice(), m_TAABufferUAV, nullptr);
        vkDestroyImageView(m_pDevice->GetDevice(), m_HistoryBufferSRV, nullptr);
        vkDestroyImageView(m_pDevice->GetDevice(), m_HistoryBufferUAV, nullptr);
    }

    void TAA::Draw(VkCommandBuffer cmd_buf)
    {
        {
            SetPerfMarkerBegin(cmd_buf, "TAA");

            {
                VkImageMemoryBarrier barrier = {};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.pNext = NULL;
                barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = 1;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;

                VkImageMemoryBarrier barriers[2];
                barriers[0] = barrier;
                barriers[0].dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                barriers[0].oldLayout = m_TexturesInUndefinedLayout ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barriers[0].newLayout = VK_IMAGE_LAYOUT_GENERAL;
                barriers[0].image = m_TAABuffer.Resource();

                barriers[1] = barrier;
                barriers[1].oldLayout = m_TexturesInUndefinedLayout ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barriers[1].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barriers[1].image = m_HistoryBuffer.Resource();

                m_TexturesInUndefinedLayout = false;

                vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, NULL, 0, NULL, 2, barriers);
            }
            if (m_bFirst)
            {
                m_bFirst = false;
                m_TAAFirst.Draw(cmd_buf, NULL, m_TaaDescriptorSet, (m_Width + 15) / 16, (m_Height + 15) / 16, 1);
            } else
                m_TAA.Draw(cmd_buf, NULL, m_TaaDescriptorSet, (m_Width + 15) / 16, (m_Height + 15) / 16, 1);
        }

        {
            SetPerfMarkerBegin(cmd_buf, "TAASharpener");

            {
                // default is color texture from SRV to UAV
                VkImageMemoryBarrier barrier = {};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.pNext = NULL;
                barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = 1;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;

                VkImageMemoryBarrier barriers[3];
                barriers[0] = barrier;
                barriers[0].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                barriers[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                barriers[0].oldLayout = VK_IMAGE_LAYOUT_GENERAL;
                barriers[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barriers[0].image = m_TAABuffer.Resource();

                barriers[1] = barrier;
                barriers[1].image = m_HistoryBuffer.Resource();

                barriers[2] = barrier;
                barriers[2].image = m_pGBuffer->m_HDR.Resource();

                vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, NULL, 0, NULL, 3, barriers);
            }
            if( m_bSharpening )
                m_Sharpen.Draw(cmd_buf, NULL, m_SharpenDescriptorSet, (m_Width + 7) / 8, (m_Height + 7) / 8, 1);
            else
                m_Post.Draw(cmd_buf, NULL, m_SharpenDescriptorSet, (m_Width + 7) / 8, (m_Height + 7) / 8, 1);
            {
                VkImageMemoryBarrier barrier = {};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.pNext = NULL;
                barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = 1;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;

                VkImageMemoryBarrier barriers[2];
                barriers[0] = barrier;
                barriers[0].oldLayout = VK_IMAGE_LAYOUT_GENERAL;
                barriers[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barriers[0].image = m_HistoryBuffer.Resource();

                barriers[1] = barrier;
                barriers[1].oldLayout = VK_IMAGE_LAYOUT_GENERAL;
                barriers[1].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barriers[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barriers[1].image = m_pGBuffer->m_HDR.Resource();

                vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, NULL, 0, NULL, 2, barriers);
            }
        }

        SetPerfMarkerEnd(cmd_buf);
    }
}
