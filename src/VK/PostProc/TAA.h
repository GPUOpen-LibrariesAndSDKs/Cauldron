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
#pragma once

#include "PostProcCS.h"
#include "Base/ResourceViewHeaps.h"
#include "Base/GBuffer.h"

namespace CAULDRON_VK
{
    class TAA
    {
    public:
        void OnCreate(Device* pDevice, ResourceViewHeaps *pResourceViewHeaps, StaticBufferPool  *pStaticBufferPool, DynamicBufferRing *pDynamicBufferRing, bool sharpening = true);
        void OnDestroy();

        void OnCreateWindowSizeDependentResources(uint32_t Width, uint32_t Height, GBuffer *pGBuffer);
        void OnDestroyWindowSizeDependentResources();

        void Draw(VkCommandBuffer cmd_buf);

    private:
        Device               *m_pDevice;
        ResourceViewHeaps    *m_pResourceViewHeaps;

        uint32_t              m_Width, m_Height;

        GBuffer              *m_pGBuffer;

        bool                  m_TexturesInUndefinedLayout;

        Texture               m_TAABuffer;
        VkImageView           m_TAABufferSRV;
        VkImageView           m_TAABufferUAV;
        
        Texture               m_HistoryBuffer;
        VkImageView           m_HistoryBufferSRV;
        VkImageView           m_HistoryBufferUAV;

        VkSampler             m_samplers[4];

        VkDescriptorSet       m_TaaDescriptorSet;
        VkDescriptorSetLayout m_TaaDescriptorSetLayout;
        PostProcCS            m_TAA;
        PostProcCS            m_TAAFirst;

        //
        VkDescriptorSet       m_SharpenDescriptorSet;
        VkDescriptorSetLayout m_SharpenDescriptorSetLayout;
        PostProcCS            m_Sharpen;
        PostProcCS            m_Post;
        bool                  m_bSharpening = true;
        bool                  m_bFirst = true;
    };
}
