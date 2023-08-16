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

#include "Misc/ColorConversion.h"
#include "PostProcCS.h"
#include "Base/ResourceViewHeaps.h"

namespace CAULDRON_VK
{
    class ToneMappingCS
    {
    public:
        void OnCreate(Device* pDevice, ResourceViewHeaps *pResourceViewHeaps, DynamicBufferRing *pDynamicBufferRing);
        void OnDestroy();

        void Draw(VkCommandBuffer cmd_buf, VkImageView HDRSRV, float exposure, int toneMapper, int width, int height);

    private:
        Device* m_pDevice;
        ResourceViewHeaps *m_pResourceViewHeaps;

        PostProcCS m_toneMapping;
        DynamicBufferRing *m_pDynamicBufferRing = NULL;

        uint32_t              m_descriptorIndex;
        static const uint32_t s_descriptorBuffers = 10;

        VkDescriptorSet       m_descriptorSet[s_descriptorBuffers];
        VkDescriptorSetLayout m_descriptorSetLayout;

        struct ToneMappingConsts {
            float exposure; int toneMapper; float pad0; float pad1;
            LPMConsts lpmConsts;
        };
    };
}
