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

#include "Wireframe.h"
#include "Misc/WirePrimitives.h"

namespace CAULDRON_VK
{
    class WireframeBox : public Wireframe
    {
        // all bounding boxes of all the meshes use the same geometry, shaders and pipelines.
        uint32_t m_NumIndices;
        VkDescriptorBufferInfo m_IBV;
        VkDescriptorBufferInfo m_VBV;
    public:
        void OnCreate(
            Device* pDevice,
            ResourceViewHeaps *pResourceViewHeaps,
            DynamicBufferRing *pDynamicBufferRing,
            StaticBufferPool *pStaticBufferPool)
        {
            std::vector<unsigned short> indices;
            std::vector<float> vertices;

            GenerateBox(indices, vertices);

            // set indices
            m_NumIndices = (uint32_t)indices.size();
            pStaticBufferPool->AllocBuffer(m_NumIndices, sizeof(short), indices.data(), &m_IBV);
            pStaticBufferPool->AllocBuffer((uint32_t)(vertices.size() / 3), (uint32_t)(3 * sizeof(float)), vertices.data(), &m_VBV);
        }

        void OnDestroy() {}

        void Draw(VkCommandBuffer cmd_buf, Wireframe *pWireframe, const math::Matrix4& worldMatrix, const math::Vector4& vCenter, const math::Vector4& vRadius, const math::Vector4& vColor)
        {
            pWireframe->Draw(cmd_buf, m_NumIndices, m_IBV, m_VBV, worldMatrix, vCenter, vRadius, vColor);
        }
    };
}

