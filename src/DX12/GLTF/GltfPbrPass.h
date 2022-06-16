// AMD Cauldron code
// 
// Copyright(c) 2020 Advanced Micro Devices, Inc.All rights reserved.
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
#include "../Common/GLTF/GltfPbrMaterial.h"
#include "base/GBuffer.h"

namespace CAULDRON_DX12
{
    struct PBRMaterial
    {
        int m_textureCount = 0;
        CBV_SRV_UAV m_texturesTable;

        D3D12_STATIC_SAMPLER_DESC m_samplers[10];

        PBRMaterialParameters m_pbrMaterialParameters;
    };

    struct PBRPrimitives
    {
        Geometry m_geometry;

        PBRMaterial *m_pMaterial = NULL;

        ID3D12RootSignature	*m_RootSignature;
        ID3D12PipelineState	*m_PipelineRender;
        ID3D12PipelineState *m_PipelineWireframeRender;

        void DrawPrimitive(ID3D12GraphicsCommandList *pCommandList, CBV_SRV_UAV *pShadowBufferSRV, D3D12_GPU_VIRTUAL_ADDRESS perSceneDesc, D3D12_GPU_VIRTUAL_ADDRESS perObjectDesc, D3D12_GPU_VIRTUAL_ADDRESS pPerSkeleton, bool bWireframe);
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
            D3D12_GPU_VIRTUAL_ADDRESS m_perFrameDesc;
            D3D12_GPU_VIRTUAL_ADDRESS m_perObjectDesc;
            D3D12_GPU_VIRTUAL_ADDRESS m_pPerSkeleton;
            operator float() { return -m_depth; }
        };

        void OnCreate(
            Device* pDevice,
            UploadHeap* pUploadHeap,
            ResourceViewHeaps* pHeaps,
            DynamicBufferRing* pDynamicBufferRing,
            GLTFTexturesAndBuffers* pGLTFTexturesAndBuffers,
            SkyDome* pSkyDome,
            bool bUseSSAOMask,
            bool bUseShadowMask,
            GBufferRenderPass* pGBufferRenderPass,
            AsyncPool* pAsyncPool = NULL,
            bool bInvertedDepth = false);

        void OnDestroy();
        void OnUpdateWindowSizeDependentResources(Texture *pSSAO);
        void BuildBatchLists(std::vector<BatchList> *pSolid, std::vector<BatchList> *pTransparent, bool bWireframe = false);
        void DrawBatchList(ID3D12GraphicsCommandList *pCommandList, CBV_SRV_UAV *pShadowBufferSRV, std::vector<BatchList> *pBatchList, bool bWireframe = false);
    private:
        Device                  *m_pDevice;
        GBufferRenderPass       *m_pGBufferRenderPass;

        GLTFTexturesAndBuffers  *m_pGLTFTexturesAndBuffers;

        ResourceViewHeaps       *m_pResourceViewHeaps;
        DynamicBufferRing       *m_pDynamicBufferRing;

        std::vector<PBRMesh>     m_meshes;
        std::vector<PBRMaterial> m_materialsData;

        GltfPbrPass::per_frame   m_cbPerFrame;

        PBRMaterial              m_defaultMaterial;

        Texture                  m_BrdfLut;

        bool                     m_doLighting;

        DXGI_FORMAT              m_depthFormat;
        std::vector<DXGI_FORMAT> m_outFormats;
        uint32_t                 m_sampleCount;

        bool                     m_bInvertedDepth;

        void CreateDescriptorTableForMaterialTextures(PBRMaterial *tfmat, std::map<std::string, Texture *> &texturesBase, SkyDome *pSkyDome, bool bUseShadowMask, bool bUseSSAOMask);
        void CreateRootSignature(bool bUsingSkinning, DefineList &defines, PBRPrimitives *pPrimitive, bool bUseSSAOMask);
        void CreatePipeline(std::vector<D3D12_INPUT_ELEMENT_DESC> layout, const DefineList &defines, PBRPrimitives *pPrimitive);
    };
}


