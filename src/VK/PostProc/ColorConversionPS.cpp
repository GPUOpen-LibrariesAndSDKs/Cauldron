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
#include "Base/FreesyncHDR.h"
#include "Base/Texture.h"
#include "Base/Helper.h"
#include "Misc/ColorConversion.h"
#include "ColorConversionPS.h"

namespace CAULDRON_VK
{
    void ColorConversionPS::OnCreate(Device* pDevice, VkRenderPass renderPass, ResourceViewHeaps *pResourceViewHeaps, StaticBufferPool  *pStaticBufferPool, DynamicBufferRing *pDynamicBufferRing)
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

        m_pResourceViewHeaps->CreateDescriptorSetLayout(&layoutBindings, &m_descriptorSetLayout);

        m_ColorConversion.OnCreate(m_pDevice, renderPass, "ColorConversionPS.glsl", "main", "", pStaticBufferPool, pDynamicBufferRing, m_descriptorSetLayout, NULL, VK_SAMPLE_COUNT_1_BIT);

        m_descriptorIndex = 0;
        for (int i = 0; i < s_descriptorBuffers; i++)
            m_pResourceViewHeaps->AllocDescriptor(m_descriptorSetLayout, &m_descriptorSet[i]);
    }

    void ColorConversionPS::OnDestroy()
    {
        m_ColorConversion.OnDestroy();

        for (int i = 0; i < s_descriptorBuffers; i++)
            m_pResourceViewHeaps->FreeDescriptor(m_descriptorSet[i]);

        vkDestroySampler(m_pDevice->GetDevice(), m_sampler, nullptr);

        vkDestroyDescriptorSetLayout(m_pDevice->GetDevice(), m_descriptorSetLayout, NULL);
    }

    void ColorConversionPS::UpdatePipelines(VkRenderPass renderPass, DisplayMode displayMode)
    {
        m_ColorConversion.UpdatePipeline(renderPass, NULL, VK_SAMPLE_COUNT_1_BIT);

        m_colorConversionConsts.m_displayMode = displayMode;

        if (displayMode != DISPLAYMODE_SDR)
        {
            const VkHdrMetadataEXT *pHDRMetatData = fsHdrGetDisplayInfo();

            m_colorConversionConsts.m_displayMinLuminancePerNits = (float)pHDRMetatData->minLuminance / 80.0f; // RGB(1, 1, 1) maps to 80 nits in scRGB;
            m_colorConversionConsts.m_displayMaxLuminancePerNits = (float)pHDRMetatData->maxLuminance / 80.0f; // This means peak white equals RGB(m_maxLuminanace/80.0f, m_maxLuminanace/80.0f, m_maxLuminanace/80.0f) in scRGB;

            FillDisplaySpecificPrimaries(
                pHDRMetatData->whitePoint.x, pHDRMetatData->whitePoint.y,
                pHDRMetatData->displayPrimaryRed.x, pHDRMetatData->displayPrimaryRed.y,
                pHDRMetatData->displayPrimaryGreen.x, pHDRMetatData->displayPrimaryGreen.y,
                pHDRMetatData->displayPrimaryBlue.x, pHDRMetatData->displayPrimaryBlue.y
            );

            SetupGamutMapperMatrices(
                ColorSpace_REC709,
                ColorSpace_Display,
                &m_colorConversionConsts.m_contentToMonitorRecMatrix);
        }
    }

    void ColorConversionPS::Draw(VkCommandBuffer cmd_buf, VkImageView HDRSRV, uint32_t isLPMToneMapperSelected)
    {
        SetPerfMarkerBegin(cmd_buf, "ColorConversion");

        VkDescriptorBufferInfo cbColorConversionHandle;
        ColorConversionConsts *pColorConversionConsts;
        m_pDynamicBufferRing->AllocConstantBuffer(sizeof(ColorConversionConsts), (void **)&pColorConversionConsts, &cbColorConversionHandle);
        *pColorConversionConsts = m_colorConversionConsts;
        pColorConversionConsts->m_isLPMToneMapperSelected = isLPMToneMapperSelected;

        // We'll be modifying the descriptor set(DS), to prevent writing on a DS that is in use we 
        // need to do some basic buffering. Just to keep it safe and simple we'll have 10 buffers.
        VkDescriptorSet descriptorSet = m_descriptorSet[m_descriptorIndex];
        m_descriptorIndex = (m_descriptorIndex + 1) % s_descriptorBuffers;

        // modify Descriptor set
        SetDescriptorSet(m_pDevice->GetDevice(), 1, HDRSRV, &m_sampler, descriptorSet);
        m_pDynamicBufferRing->SetDescriptorSet(0, sizeof(ColorConversionConsts), descriptorSet);

        // Draw!
        m_ColorConversion.Draw(cmd_buf, &cbColorConversionHandle, descriptorSet);

        SetPerfMarkerEnd(cmd_buf);
    }
}
