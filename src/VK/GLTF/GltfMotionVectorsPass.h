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
    // This class takes a GltfCommon class (that holds all the non-GPU specific data) as an input and loads all the GPU specific data
    //
    struct MotionVectorMaterial
    {
        int m_textureCount = 0;
        VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;

        DefineList m_defines;
        bool m_doubleSided;
    };

    struct MotionVectorPrimitives
    {
        Geometry m_geometry;

        MotionVectorMaterial *m_pMaterial = NULL;

        VkPipeline m_pipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

        VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    };

    struct MotionVectorMesh
    {
        std::vector<MotionVectorPrimitives> m_pPrimitives;
    };

    class GltfMotionVectorsPass
    {
    public:
        struct per_frame
        {
            math::Matrix4 mCurrViewProj;
            math::Matrix4 mPrevViewProj;
        };

        struct per_object
        {
            math::Matrix4 mCurrentWorld;
            math::Matrix4 mPreviousWorld;
        };

        void OnCreate(
            Device *pDevice,
            VkRenderPass renderPass,
            UploadHeap *pUploadHeap,
            ResourceViewHeaps *pHeaps,
            DynamicBufferRing *pDynamicBufferRing,
            StaticBufferPool *pStaticBufferPool,
            GLTFTexturesAndBuffers *pGLTFTexturesAndBuffers,
            VkFormat motionVectorsBufferFormat,
            VkFormat normalBufferFormat = VK_FORMAT_UNDEFINED,
            AsyncPool *pAsyncPool = NULL);

        void OnDestroy();
        GltfMotionVectorsPass::per_frame *SetPerFrameConstants();
        void Draw(VkCommandBuffer cmd_buf);
    private:
        Device *m_pDevice;
        ResourceViewHeaps *m_pResourceViewHeaps;
        DynamicBufferRing *m_pDynamicBufferRing;
        StaticBufferPool *m_pStaticBufferPool;

        std::vector<MotionVectorMesh> m_meshes;
        std::vector<MotionVectorMaterial> m_materialsData;

        MotionVectorMaterial m_defaultMaterial;

        GLTFTexturesAndBuffers *m_pGLTFTexturesAndBuffers;
        VkSampler m_sampler = VK_NULL_HANDLE;
        VkDescriptorBufferInfo m_perFrameDesc;

        VkFormat m_normalBufferFormat;
        VkFormat m_motionVectorsBufferFormat;

        void CreatePipeline(VkRenderPass renderPass, int attachmentCount, int inverseMatrixBufferSize, std::vector<VkVertexInputAttributeDescription> layout, DefineList &defines, MotionVectorPrimitives *pPrimitive);
    };
}


