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
#pragma once

#include <string>

#include "base/StaticBufferPool.h"
#include "base/DynamicBufferRing.h"
#include "base/ResourceViewHeaps.h"

namespace CAULDRON_VK
{
    class PostProcPS
    {
    public:
        void OnCreate(
            Device* pDevice,
            VkRenderPass renderPass,
            const std::string &string,
            StaticBufferPool *pStaticBufferPool,
            DynamicBufferRing *pDynamicBufferRing,
            VkDescriptorSetLayout descriptorSetLayout,
            VkPipelineColorBlendStateCreateInfo *pBlendDesc = nullptr,
            VkSampleCountFlagBits sampleDescCount = VK_SAMPLE_COUNT_1_BIT
        );
        void OnDestroy();
        void Draw(VkCommandBuffer cmd_buf, VkDescriptorBufferInfo constantBuffer, VkDescriptorSet descriptorSet = nullptr);

    private:
        Device* m_pDevice;
        VkRenderPass m_renderPass;

        // all bounding boxes of all the meshes use the same geometry, shaders and pipelines.
        VkIndexType m_indexType;
        uint32_t m_NumIndices;
        VkDescriptorBufferInfo m_IBV;
        VkDescriptorBufferInfo m_verticesView;

        VkPipeline m_pipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_pipelineLayout;
    };
}
