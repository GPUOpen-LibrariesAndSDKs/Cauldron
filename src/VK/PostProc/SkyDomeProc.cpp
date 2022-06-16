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
#include "SkyDomeProc.h"

namespace CAULDRON_VK
{
    void SkyDomeProc::OnCreate(Device* pDevice, VkRenderPass renderPass, UploadHeap* pUploadHeap, VkFormat outFormat, ResourceViewHeaps *pResourceViewHeaps, DynamicBufferRing *pDynamicBufferRing, StaticBufferPool  *pStaticBufferPool, VkSampleCountFlagBits sampleDescCount, bool invertedDepth)
    {
        m_pDevice = pDevice;
        m_pDynamicBufferRing = pDynamicBufferRing;
        m_pResourceViewHeaps = pResourceViewHeaps;
        m_bInvertedDepth = invertedDepth;

        // Create Descriptor Set Layout, all we need is a uniform dynamic buffer to pass some parameters to the shader. All is procedural and we need no textures.
        //
        std::vector<VkDescriptorSetLayoutBinding> layoutBindings(1);
        layoutBindings[0].binding = 0;
        layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        layoutBindings[0].descriptorCount = 1;
        layoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        layoutBindings[0].pImmutableSamplers = NULL;

        m_pResourceViewHeaps->CreateDescriptorSetLayoutAndAllocDescriptorSet(&layoutBindings, &m_descriptorLayout, &m_descriptorSet);
        pDynamicBufferRing->SetDescriptorSet(0, sizeof(SkyDomeProc::Constants), m_descriptorSet);

        m_skydome.OnCreate(pDevice, renderPass, "SkyDomeProc.glsl", "main", "", pStaticBufferPool, pDynamicBufferRing, m_descriptorLayout, NULL, sampleDescCount, m_bInvertedDepth);
    }

    void SkyDomeProc::OnDestroy()
    {
        m_skydome.OnDestroy();

        m_pResourceViewHeaps->FreeDescriptor(m_descriptorSet);

        vkDestroyDescriptorSetLayout(m_pDevice->GetDevice(), m_descriptorLayout, NULL);
    }

    void SkyDomeProc::Draw(VkCommandBuffer cmd_buf, SkyDomeProc::Constants constants)
    {
        SetPerfMarkerBegin(cmd_buf, "Skydome Proc");

        SkyDomeProc::Constants *cbPerDraw;
        VkDescriptorBufferInfo constantBuffer;
        m_pDynamicBufferRing->AllocConstantBuffer(sizeof(SkyDomeProc::Constants), (void **)&cbPerDraw, &constantBuffer);
        *cbPerDraw = constants;

        m_skydome.Draw(cmd_buf, &constantBuffer, m_descriptorSet);

        SetPerfMarkerEnd(cmd_buf);
    }
}