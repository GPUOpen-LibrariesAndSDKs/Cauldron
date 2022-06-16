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
#include "PostProc/SkyDome.h"
#include "Base/GBuffer.h"
#include "../common/GLTF/GltfPbrMaterial.h"

namespace CAULDRON_VK
{
    // Material, primitive and mesh structs specific for the PBR pass (you might want to compare these structs with the ones used for the depth pass in GltfDepthPass.h)

    struct PBRMaterial
    {
        int m_textureCount = 0;
        VkDescriptorSet m_texturesDescriptorSet = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_texturesDescriptorSetLayout = VK_NULL_HANDLE;

        PBRMaterialParameters m_pbrMaterialParameters;
    };

    struct PBRPrimitives
    {
        Geometry m_geometry;

        PBRMaterial *m_pMaterial = NULL;

        VkPipeline m_pipeline = VK_NULL_HANDLE;
        VkPipeline m_pipelineWireframe = VK_NULL_HANDLE;
        VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

        VkDescriptorSet m_uniformsDescriptorSet = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_uniformsDescriptorSetLayout = VK_NULL_HANDLE;

        void DrawPrimitive(VkCommandBuffer cmd_buf, VkDescriptorBufferInfo perSceneDesc, VkDescriptorBufferInfo perObjectDesc, VkDescriptorBufferInfo *pPerSkeleton, bool bWireframe);
    };

    struct PBRMesh
    {
        std::vector<PBRPrimitives> m_pPrimitives;
    };

    class GltfPbrPass
    {
    public:
        struct per_object
        {
            math::Matrix4 mCurrentWorld;
            math::Matrix4 mPreviousWorld;

            PBRMaterialParametersConstantBuffer m_pbrParams;
        };

        struct BatchList
        {
            float m_depth;
            PBRPrimitives *m_pPrimitive;
            VkDescriptorBufferInfo m_perFrameDesc;
            VkDescriptorBufferInfo m_perObjectDesc;
            VkDescriptorBufferInfo *m_pPerSkeleton;
            operator float() { return -m_depth; }
        };

        void OnCreate(
            Device* pDevice,
            UploadHeap* pUploadHeap,
            ResourceViewHeaps *pHeaps,
            DynamicBufferRing *pDynamicBufferRing,
            StaticBufferPool *pStaticBufferPool,
            GLTFTexturesAndBuffers *pGLTFTexturesAndBuffers,
            SkyDome *pSkyDome,
            bool bUseSSAOMask,
            std::vector<VkImageView>& ShadowMapViewPool,
            GBufferRenderPass *pRenderPass,
            AsyncPool *pAsyncPool = NULL,
            bool invertedDepth = false
        );

        void OnDestroy();
        void BuildBatchLists(std::vector<BatchList> *pSolid, std::vector<BatchList> *pTransparent, bool bWireframe=false);
        void DrawBatchList(VkCommandBuffer commandBuffer, std::vector<BatchList> *pBatchList, bool bWireframe=false);
        void OnUpdateWindowSizeDependentResources(VkImageView SSAO);
    private:
        GLTFTexturesAndBuffers *m_pGLTFTexturesAndBuffers;

        ResourceViewHeaps *m_pResourceViewHeaps;
        DynamicBufferRing *m_pDynamicBufferRing;
        StaticBufferPool *m_pStaticBufferPool;

        std::vector<PBRMesh> m_meshes;
        std::vector<PBRMaterial> m_materialsData;

        GltfPbrPass::per_frame m_cbPerFrame;

        PBRMaterial m_defaultMaterial;

        Device   *m_pDevice;
        GBufferRenderPass *m_pRenderPass;
        VkSampler m_samplerPbr = VK_NULL_HANDLE, m_samplerShadow = VK_NULL_HANDLE;

        // PBR Brdf
        Texture m_brdfLutTexture;
        VkImageView m_brdfLutView = VK_NULL_HANDLE;
        VkSampler m_brdfLutSampler = VK_NULL_HANDLE;

        bool                     m_bInvertedDepth;

        void CreateDescriptorTableForMaterialTextures(PBRMaterial *tfmat, std::map<std::string, VkImageView> &texturesBase, SkyDome *pSkyDome, std::vector<VkImageView>& ShadowMapViewPool, bool bUseSSAOMask);
        void CreateDescriptors(int inverseMatrixBufferSize, DefineList *pAttributeDefines, PBRPrimitives *pPrimitive, bool bUseSSAOMask);
        void CreatePipeline(std::vector<VkVertexInputAttributeDescription> layout, const DefineList &defines, PBRPrimitives *pPrimitive);
    };
}


