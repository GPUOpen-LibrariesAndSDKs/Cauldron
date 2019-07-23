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
#include "Base\DynamicBufferRing.h"
#include "Base\StaticBufferPool.h"
#include "Base\DebugMarkersExt.h"
#include "Base\UploadHeap.h"
#include "Base\Texture.h"
#include "Base\Helper.h"
#include "ToneMapping.h"

namespace CAULDRON_VK
{
    void ToneMapping::OnCreate(Device* pDevice, VkRenderPass renderPass, ResourceViewHeaps *pResourceViewHeaps, StaticBufferPool  *pStaticBufferPool, DynamicBufferRing *pDynamicBufferRing)
    {
        m_pDevice = pDevice;
        m_pDynamicBufferRing = pDynamicBufferRing;
        m_pResourceViewHeaps = pResourceViewHeaps;

        {
            VkSamplerCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            info.magFilter = VK_FILTER_LINEAR;
            info.minFilter = VK_FILTER_LINEAR;
            info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            info.minLod = -1000;
            info.maxLod = 1000;
            info.maxAnisotropy = 1.0f;
            VkResult res = vkCreateSampler(m_pDevice->GetDevice(), &info, NULL, &m_sampler);
            assert(res == VK_SUCCESS);
        }

        std::vector<VkDescriptorSetLayoutBinding> layoutBindings(2);
        layoutBindings[0].binding = 0;
        layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        layoutBindings[0].descriptorCount = 1;
        layoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        layoutBindings[0].pImmutableSamplers = NULL;

        layoutBindings[1].binding = 1;
        layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layoutBindings[1].descriptorCount = 1;
        layoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        layoutBindings[1].pImmutableSamplers = NULL;

        m_pResourceViewHeaps->CreateDescriptorSetLayoutAndAllocDescriptorSet(&layoutBindings, &m_descriptorSetLayout, &m_descriptorSet);
        m_pDynamicBufferRing->SetDescriptorSet(0, sizeof(ToneMappingConsts), m_descriptorSet);

        m_toneMapping.OnCreate(m_pDevice, renderPass, "Tonemapping.glsl", pStaticBufferPool, pDynamicBufferRing, m_descriptorSetLayout, NULL, VK_SAMPLE_COUNT_1_BIT);
    }

    void ToneMapping::OnDestroy()
    {
        m_toneMapping.OnDestroy();

        m_pResourceViewHeaps->FreeDescriptor(m_descriptorSet);

        vkDestroySampler(m_pDevice->GetDevice(), m_sampler, nullptr);

        vkDestroyDescriptorSetLayout(m_pDevice->GetDevice(), m_descriptorSetLayout, NULL);
    }

    void ToneMapping::OnCreateWindowSizeDependentResources(VkImageView HDRSRV)
    {
        SetDescriptorSet(m_pDevice->GetDevice(), 1, HDRSRV, &m_sampler, m_descriptorSet);
    }

    void ToneMapping::OnDestroyWindowSizeDependentResources()
    {

    }

    void ToneMapping::Draw(VkCommandBuffer cmd_buf, float exposure, int toneMapper)
    {
        SetPerfMarkerBegin(cmd_buf, "tonemapping");

        VkDescriptorBufferInfo cbTonemappingHandle;
        ToneMappingConsts *pToneMapping;
        m_pDynamicBufferRing->AllocConstantBuffer(sizeof(ToneMappingConsts), (void **)&pToneMapping, &cbTonemappingHandle);
        pToneMapping->exposure = exposure;
        pToneMapping->toneMapper = toneMapper;

        m_toneMapping.Draw(cmd_buf, cbTonemappingHandle, m_descriptorSet);

        SetPerfMarkerEnd(cmd_buf);
    }
}