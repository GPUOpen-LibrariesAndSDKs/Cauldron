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

#include "GLTFTexturesAndBuffers.h"

namespace CAULDRON_VK
{
    // Material, primitive and mesh structs specific for the depth pass (you might want to compare these structs with the ones used for the depth pass in GltfPbrPass.h)

    struct DepthMaterial
    {
        int m_textureCount = 0;
        VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;

        DefineList m_defines;
        bool m_doubleSided = false;
    };

    struct DepthPrimitives
    {
        Geometry m_geometry;

        DepthMaterial *m_pMaterial = NULL;

        VkPipeline m_pipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

        VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    };

    struct DepthMesh
    {
        std::vector<DepthPrimitives> m_pPrimitives;
    };

    class GltfDepthPass
    {
    public:
        struct per_object
        {
            math::Matrix4 mWorld;
        };

        void OnCreate(
            Device* pDevice,
            VkRenderPass renderPass,
            UploadHeap* pUploadHeap,
            ResourceViewHeaps *pHeaps,
            DynamicBufferRing *pDynamicBufferRing,
            StaticBufferPool *pStaticBufferPool,
            GLTFTexturesAndBuffers *pGLTFTexturesAndBuffers,
            AsyncPool *pAsyncPool = NULL,
            bool invertedDepth = false);

        void OnDestroy();
        per_frame *SetPerFrameConstants();
        void Draw(VkCommandBuffer cmd_buf);
    private:
        ResourceViewHeaps *m_pResourceViewHeaps;
        DynamicBufferRing *m_pDynamicBufferRing;
        StaticBufferPool *m_pStaticBufferPool;

        std::vector<DepthMesh> m_meshes;
        std::vector<DepthMaterial> m_materialsData;

        DepthMaterial m_defaultMaterial;

        GLTFTexturesAndBuffers *m_pGLTFTexturesAndBuffers;
        Device* m_pDevice;
        VkRenderPass m_renderPass = VK_NULL_HANDLE;
        VkSampler m_sampler = VK_NULL_HANDLE;
        VkDescriptorBufferInfo m_perFrameDesc;

        bool                     m_bInvertedDepth;

        void CreateDescriptors(int inverseMatrixBufferSize, DefineList *pAttributeDefines, DepthPrimitives *pPrimitive);
        void CreatePipeline(std::vector<VkVertexInputAttributeDescription> layout, const DefineList &defines, DepthPrimitives *pPrimitive);
    };
}


