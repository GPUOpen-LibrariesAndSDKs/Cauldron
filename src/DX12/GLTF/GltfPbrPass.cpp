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

#include "stdafx.h"
#include "GltfPbrPass.h"
#include "Misc/ThreadPool.h"
#include "GltfHelpers.h"
#include "Base/GBuffer.h"
#include "Base/ShaderCompilerHelper.h"

namespace CAULDRON_DX12
{
    //--------------------------------------------------------------------------------------
    //
    // OnCreate
    //
    //--------------------------------------------------------------------------------------
    void GltfPbrPass::OnCreate(
        Device *pDevice,
        UploadHeap* pUploadHeap,
        ResourceViewHeaps *pHeaps,
        DynamicBufferRing *pDynamicBufferRing,
        GLTFTexturesAndBuffers *pGLTFTexturesAndBuffers,
        SkyDome *pSkyDome,
        bool bUseSSAOMask,
        bool bUseShadowMask,
        GBufferRenderPass *pGBufferRenderPass,
        AsyncPool *pAsyncPool,
        bool bInvertedDepth)
    {
        m_pDevice = pDevice;
        m_pGBufferRenderPass = pGBufferRenderPass;
        m_sampleCount = 1;
        m_pResourceViewHeaps = pHeaps;
        m_pDynamicBufferRing = pDynamicBufferRing;
        m_pGLTFTexturesAndBuffers = pGLTFTexturesAndBuffers;
        m_bInvertedDepth = bInvertedDepth;

        m_doLighting = true;

        DefineList rtDefines;
        m_pGBufferRenderPass->GetCompilerDefinesAndGBufferFormats(rtDefines, m_outFormats, m_depthFormat);

        // Load BRDF look up table for the PBR shader
        //
        m_BrdfLut.InitFromFile(pDevice, pUploadHeap, "BrdfLut.dds", false); // LUT images are stored as linear

        // Create default material, this material will be used if none is assigned
        //
        {
            SetDefaultMaterialParamters(&m_defaultMaterial.m_pbrMaterialParameters);
            
            std::map<std::string, Texture *> texturesBase;
            CreateDescriptorTableForMaterialTextures(&m_defaultMaterial, texturesBase, pSkyDome, bUseShadowMask, bUseSSAOMask);
        }

        const json &j3 = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->j3;

        // Load PBR 2.0 Materials
        //
        const json &materials = j3["materials"];
        m_materialsData.resize(materials.size());
        for (uint32_t i = 0; i < materials.size(); i++)
        {
            PBRMaterial *tfmat = &m_materialsData[i];

            // Get PBR material parameters and texture IDs
            //
            std::map<std::string, int> textureIds;
            ProcessMaterials(materials[i], &tfmat->m_pbrMaterialParameters, textureIds);

            // translate texture IDs into textureViews
            //
            std::map<std::string, Texture *> texturesBase;
            for (auto const& value : textureIds)
                texturesBase[value.first] = m_pGLTFTexturesAndBuffers->GetTextureViewByID(value.second);

            CreateDescriptorTableForMaterialTextures(tfmat, texturesBase, pSkyDome, bUseShadowMask, bUseSSAOMask);
        }

        // Load Meshes
        //
        if (j3.find("meshes") != j3.end())
        {
            const json &meshes = j3["meshes"];
            m_meshes.resize(meshes.size());
            for (uint32_t i = 0; i < meshes.size(); i++)
            {
                const json &primitives = meshes[i]["primitives"];

                // Loop through all the primitives (sets of triangles with a same material) and 
                // 1) create an input layout for the geometry
                // 2) then take its material and create a Root descriptor
                // 3) With all the above, create a pipeline
                //
                PBRMesh *tfmesh = &m_meshes[i];
                tfmesh->m_pPrimitives.resize(primitives.size());
                for (uint32_t p = 0; p < primitives.size(); p++)
                {
                    const json &primitive = primitives[p];
                    PBRPrimitives *pPrimitive = &tfmesh->m_pPrimitives[p];

                    ExecAsyncIfThereIsAPool(pAsyncPool, [this, i, &primitive, rtDefines, pPrimitive, bUseSSAOMask]()
                    {
                        // Sets primitive's material, or set a default material if none was specified in the GLTF
                        //
                        auto mat = primitive.find("material");
                        pPrimitive->m_pMaterial = (mat != primitive.end()) ? &m_materialsData[mat.value()] : &m_defaultMaterial;

                        // holds all the #defines from materials, geometry and texture IDs, the VS & PS shaders need this to get the bindings and code paths
                        //
                        DefineList defines = pPrimitive->m_pMaterial->m_pbrMaterialParameters.m_defines + rtDefines;

                        // make a list of all the attribute names our pass requires, in the case of PBR we need them all
                        //
                        std::vector<std::string> requiredAttributes;
                        for (auto const & it : primitive["attributes"].items())
                            requiredAttributes.push_back(it.key());

                        // create an input layout from the required attributes
                        // shader's can tell the slots from the #defines
                        //
                        std::vector<std::string> semanticNames;
                        std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;
                        m_pGLTFTexturesAndBuffers->CreateGeometry(primitive, requiredAttributes, semanticNames, inputLayout, defines, &pPrimitive->m_geometry);

                        // Create the descriptors, the root signature and the pipeline
                        //
                        bool bUsingSkinning = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->FindMeshSkinId(i) != -1;
                        CreateRootSignature(bUsingSkinning, defines, pPrimitive, bUseSSAOMask);
                        CreatePipeline(inputLayout, defines, pPrimitive);
                        semanticNames;
                    });
                }
            }
        }
    }

    //--------------------------------------------------------------------------------------
    //
    // CreateDescriptorTableForMaterialTextures
    //
    //--------------------------------------------------------------------------------------
    void GltfPbrPass::CreateDescriptorTableForMaterialTextures(PBRMaterial *tfmat, std::map<std::string, Texture *> &texturesBase, SkyDome *pSkyDome, bool bUseShadowMask, bool bUseSSAOMask)
    {
        uint32_t cnt = 0;

        {
            // count the number of textures to init bindings and descriptor
            {
                tfmat->m_textureCount = (int)texturesBase.size();

                if (pSkyDome)
                {
                    tfmat->m_textureCount += 3;   // +3 because the skydome has a specular, diffusse and a BDRF LUT map
                }

                if (bUseSSAOMask)
                {
                    tfmat->m_textureCount += 1;
                }

                // allocate descriptor table for the textures
                m_pResourceViewHeaps->AllocCBV_SRV_UAVDescriptor(tfmat->m_textureCount, &tfmat->m_texturesTable);
            }


            // Create SRV for the PBR materials
            //
            for (auto const &it : texturesBase)
            {
                tfmat->m_pbrMaterialParameters.m_defines[std::string("ID_") + it.first] = std::to_string(cnt);
                it.second->CreateSRV(cnt, &tfmat->m_texturesTable);
                CreateSamplerForPBR(cnt, &tfmat->m_samplers[cnt]);
                cnt++;
            }

            if (m_doLighting)
            {
                // 3 SRVs for the IBL probe
                //
                if (pSkyDome && m_doLighting)
                {
                    tfmat->m_pbrMaterialParameters.m_defines["ID_brdfTexture"] = std::to_string(cnt);
                    CreateSamplerForBrdfLut(cnt, &tfmat->m_samplers[cnt]);
                    m_BrdfLut.CreateSRV(cnt, &tfmat->m_texturesTable);
                    cnt++;

                    tfmat->m_pbrMaterialParameters.m_defines["ID_diffuseCube"] = std::to_string(cnt);
                    pSkyDome->SetDescriptorDiff(cnt, &tfmat->m_texturesTable, cnt, &tfmat->m_samplers[cnt]);
                    cnt++;

                    tfmat->m_pbrMaterialParameters.m_defines["ID_specularCube"] = std::to_string(cnt);
                    pSkyDome->SetDescriptorSpec(cnt, &tfmat->m_texturesTable, cnt, &tfmat->m_samplers[cnt]);
                    cnt++;

                    tfmat->m_pbrMaterialParameters.m_defines["USE_IBL"] = "1";
                }
            }

            // SSAO mask
            //
            if (bUseSSAOMask)
            {
                tfmat->m_pbrMaterialParameters.m_defines["ID_SSAO"] = std::to_string(cnt);
                CreateSamplerForPBR(cnt, &tfmat->m_samplers[cnt]);
                cnt++;
            }

        }
            
        // the SRVs for the shadows is provided externally, here we just create the #defines for the shader bindings
        //
        if (bUseShadowMask)
        {
            assert(cnt <= 9);   // 10th slot is reserved for shadow buffer
            tfmat->m_pbrMaterialParameters.m_defines["ID_shadowBuffer"] = std::to_string(9);
            CreateSamplerForShadowBuffer(9, &tfmat->m_samplers[cnt]);
        }
        else
        {
            assert(cnt <= 9);   // 10th slot is reserved for shadow buffer
            tfmat->m_pbrMaterialParameters.m_defines["ID_shadowMap"] = std::to_string(9);
            CreateSamplerForShadowMap(9, &tfmat->m_samplers[cnt]);
        }        
    }

    //--------------------------------------------------------------------------------------
    //
    // OnUpdateWindowSizeDependentResources
    //
    //--------------------------------------------------------------------------------------
    void GltfPbrPass::OnUpdateWindowSizeDependentResources(Texture *pSSAO)
    {
        for (uint32_t i = 0; i < m_materialsData.size(); i++)
        {
            PBRMaterial *tfmat = &m_materialsData[i];

            DefineList def = tfmat->m_pbrMaterialParameters.m_defines;

            auto id = def.find("ID_SSAO");
            if (id != def.end())
            {
                int index = std::stoi(id->second);
                pSSAO->CreateSRV(index, &tfmat->m_texturesTable);
            }
        }
    }

    //--------------------------------------------------------------------------------------
    //
    // OnDestroy
    //
    //--------------------------------------------------------------------------------------
    void GltfPbrPass::OnDestroy()
    {
        for (uint32_t m = 0; m < m_meshes.size(); m++)
        {
            PBRMesh *pMesh = &m_meshes[m];
            for (uint32_t p = 0; p < pMesh->m_pPrimitives.size(); p++)
            {
                PBRPrimitives *pPrimitive = &pMesh->m_pPrimitives[p];
                if (pPrimitive->m_PipelineRender)
                    pPrimitive->m_PipelineRender->Release();
                if (pPrimitive->m_PipelineWireframeRender)
                    pPrimitive->m_PipelineWireframeRender->Release();
                if (pPrimitive->m_RootSignature)
                    pPrimitive->m_RootSignature->Release();
            }
        }

        m_BrdfLut.OnDestroy();
    }

    //--------------------------------------------------------------------------------------
    //
    // CreateDescriptors for a combination of material and geometry
    //
    //--------------------------------------------------------------------------------------
    void GltfPbrPass::CreateRootSignature(bool bUsingSkinning, DefineList &defines, PBRPrimitives *pPrimitive, bool bUseSSAOMask)
    {              
        int rootParamCnt = 0;
        CD3DX12_ROOT_PARAMETER rootParameter[6];
        int desccRangeCnt = 0;
        CD3DX12_DESCRIPTOR_RANGE descRange[3];

        // b0 <- Constant buffer 'per frame'
        {
            rootParameter[rootParamCnt].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
            rootParamCnt++;
        }

        // textures table
        if (pPrimitive->m_pMaterial->m_textureCount > 0)
        {
            descRange[desccRangeCnt].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, pPrimitive->m_pMaterial->m_textureCount, 0); // texture table
            rootParameter[rootParamCnt].InitAsDescriptorTable(1, &descRange[desccRangeCnt], D3D12_SHADER_VISIBILITY_PIXEL);
            desccRangeCnt++;
            rootParamCnt++;
        }

        // shadow buffer (only if we are doing lighting, for example in the forward pass)
        if (m_doLighting)
        {
            descRange[desccRangeCnt].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, MaxShadowInstances, 9);                    // shadow buffer
            rootParameter[rootParamCnt].InitAsDescriptorTable(1, &descRange[desccRangeCnt], D3D12_SHADER_VISIBILITY_PIXEL);
            desccRangeCnt++;
            rootParamCnt++;
        }

        // b1 <- Constant buffer 'per object', these are mainly the material data
        {
            rootParameter[rootParamCnt].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);
            rootParamCnt++;
        }

        // b2 <- Constant buffer holding the skinning matrices
        if (bUsingSkinning)
        {
            rootParameter[rootParamCnt].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_VERTEX);
            defines["ID_SKINNING_MATRICES"] = std::to_string(2);
            rootParamCnt++;
        }

        // the root signature contains up to 5 slots to be used
        CD3DX12_ROOT_SIGNATURE_DESC descRootSignature = CD3DX12_ROOT_SIGNATURE_DESC();
        descRootSignature.pParameters = rootParameter;
        descRootSignature.NumParameters = rootParamCnt;
        descRootSignature.pStaticSamplers = pPrimitive->m_pMaterial->m_samplers;
        descRootSignature.NumStaticSamplers = pPrimitive->m_pMaterial->m_textureCount;
        descRootSignature.NumStaticSamplers += 1;   // account for shadow sampler

        // deny uneccessary access to certain pipeline stages
        descRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE
            | D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
            | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
            | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
            | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        ID3DBlob *pOutBlob, *pErrorBlob = NULL;
        ThrowIfFailed(D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &pOutBlob, &pErrorBlob));
        ThrowIfFailed(m_pDevice->GetDevice()->CreateRootSignature(0, pOutBlob->GetBufferPointer(), pOutBlob->GetBufferSize(), IID_PPV_ARGS(&pPrimitive->m_RootSignature)));
        SetName(pPrimitive->m_RootSignature, "GltfPbr::m_RootSignature");

        pOutBlob->Release();
        if (pErrorBlob)
            pErrorBlob->Release();
    }

    //--------------------------------------------------------------------------------------
    //
    // CreatePipeline
    //
    //--------------------------------------------------------------------------------------
    void GltfPbrPass::CreatePipeline(std::vector<D3D12_INPUT_ELEMENT_DESC> layout, const DefineList &defines, PBRPrimitives *pPrimitive)
    {
        // Compile and create shaders
        //
        D3D12_SHADER_BYTECODE shaderVert, shaderPixel;
        CompileShaderFromFile("GLTFPbrPass-VS.hlsl", &defines, "mainVS", "-T vs_6_0 -Zi -Od", &shaderVert);
        CompileShaderFromFile("GLTFPbrPass-PS.hlsl", &defines, "mainPS", "-T ps_6_0 -Zi -Od", &shaderPixel);

        // Set blending
        //
        int upscaleReactiveRT = m_pGBufferRenderPass->GetRtIndex(GBUFFER_UPSCALEREACTIVE);
        int upscaleTransparencyAndCompositionRT = m_pGBufferRenderPass->GetRtIndex(GBUFFER_UPSCALE_TRANSPARENCY_AND_COMPOSITION);
        CD3DX12_BLEND_DESC blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        blendState.IndependentBlendEnable = (upscaleReactiveRT != -1);
        for (int i = 0; i < this->m_outFormats.size(); ++i)
        {
            blendState.RenderTarget[i] = D3D12_RENDER_TARGET_BLEND_DESC
            {
                (defines.Has("DEF_alphaMode_BLEND")),
                FALSE,
                D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD,
                D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
                D3D12_LOGIC_OP_NOOP,
                D3D12_COLOR_WRITE_ENABLE_ALL,
            };
            if (i == upscaleReactiveRT || i == upscaleTransparencyAndCompositionRT)
            {
                true,
                blendState.RenderTarget[i].SrcBlend = defines.Has("DEF_alphaMode_BLEND") ? D3D12_BLEND_INV_DEST_COLOR : D3D12_BLEND_ONE;
                blendState.RenderTarget[i].DestBlend = D3D12_BLEND_ONE;
                blendState.RenderTarget[i].RenderTargetWriteMask = defines.Has("DEF_alphaMode_BLEND") ? D3D12_COLOR_WRITE_ENABLE_RED : D3D12_COLOR_WRITE_ENABLE_ALPHA;

                bool bHasAnimatedTexture = false;
                bHasAnimatedTexture |= defines.Has("HAS_NORMAL_UV_TRANSFORM");
                bHasAnimatedTexture |= defines.Has("HAS_EMISSIVE_UV_TRANSFORM");
                bHasAnimatedTexture |= defines.Has("HAS_OCCLSION_UV_TRANSFORM");
                bHasAnimatedTexture |= defines.Has("HAS_BASECOLOR_UV_TRANSFORM");
                bHasAnimatedTexture |= defines.Has("HAS_METALLICROUGHNESS_UV_TRANSFORM");
                bHasAnimatedTexture |= defines.Has("HAS_SPECULARGLOSSINESS_UV_TRANSFORM");
                bHasAnimatedTexture |= defines.Has("HAS_DIFFUSE_UV_TRANSFORM");
                if (bHasAnimatedTexture)
                    blendState.RenderTarget[i].RenderTargetWriteMask |= D3D12_COLOR_WRITE_ENABLE_BLUE;
            }
        }
        // Create a PSO description
        //
        D3D12_GRAPHICS_PIPELINE_STATE_DESC descPso = {};
        descPso.InputLayout = { layout.data(), (UINT)layout.size() };
        descPso.pRootSignature = pPrimitive->m_RootSignature;
        descPso.VS = shaderVert;
        descPso.PS = shaderPixel;
        descPso.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        descPso.RasterizerState.CullMode = (pPrimitive->m_pMaterial->m_pbrMaterialParameters.m_doubleSided) ? D3D12_CULL_MODE_NONE : D3D12_CULL_MODE_FRONT;
        descPso.BlendState = blendState;
        descPso.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        if (defines.Has("DEF_alphaMode_BLEND")) {
            descPso.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
        } else {
            descPso.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        }
        descPso.DepthStencilState.DepthFunc = m_bInvertedDepth ? D3D12_COMPARISON_FUNC_GREATER_EQUAL : D3D12_COMPARISON_FUNC_LESS_EQUAL;
        descPso.SampleMask = UINT_MAX;
        descPso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        descPso.NumRenderTargets = (UINT)m_outFormats.size();
        for (size_t i = 0; i < m_outFormats.size(); i++)
        {
            descPso.RTVFormats[i] = m_outFormats[i];
        }
        descPso.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        descPso.SampleDesc.Count = m_sampleCount;
        descPso.NodeMask = 0;

        ThrowIfFailed(
            m_pDevice->GetDevice()->CreateGraphicsPipelineState(&descPso, IID_PPV_ARGS(&pPrimitive->m_PipelineRender))
        );
        SetName(pPrimitive->m_PipelineRender, "GltfPbrPass::m_PipelineRender");

        // create wireframe pipeline
        descPso.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
        descPso.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        ThrowIfFailed(
            m_pDevice->GetDevice()->CreateGraphicsPipelineState(&descPso, IID_PPV_ARGS(&pPrimitive->m_PipelineWireframeRender))
        );
        SetName(pPrimitive->m_PipelineWireframeRender, "GltfPbrPass::m_PipelineWireframeRender");
    }

    //--------------------------------------------------------------------------------------
    //
    // BuildLists
    //
    //--------------------------------------------------------------------------------------
    void GltfPbrPass::BuildBatchLists(std::vector<BatchList> *pSolid, std::vector<BatchList> *pTransparent, bool bWireframe/*=false*/)
    {
        // loop through nodes
        //
        std::vector<tfNode> *pNodes = &m_pGLTFTexturesAndBuffers->m_pGLTFCommon->m_nodes;
        Matrix2 *pNodesMatrices = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->m_worldSpaceMats.data();        

        for (uint32_t i = 0; i < pNodes->size(); i++)
        {
            tfNode *pNode = &pNodes->at(i);
            if ((pNode == NULL) || (pNode->meshIndex < 0))
                continue;

            // skinning matrices constant buffer
            D3D12_GPU_VIRTUAL_ADDRESS pPerSkeleton = m_pGLTFTexturesAndBuffers->GetSkinningMatricesBuffer(pNode->skinIndex);

            math::Matrix4 mModelViewProj = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->m_perFrameData.mCameraCurrViewProj * pNodesMatrices[i].GetCurrent();

            // loop through primitives
            //
            PBRMesh *pMesh = &m_meshes[pNode->meshIndex];
            for (uint32_t p = 0; p < pMesh->m_pPrimitives.size(); p++)
            {
                PBRPrimitives *pPrimitive = &pMesh->m_pPrimitives[p];

                if ((bWireframe && pPrimitive->m_PipelineWireframeRender == NULL)
                    ||(!bWireframe && pPrimitive->m_PipelineRender == NULL))
                    continue;

                // do frustum culling
                //
                tfPrimitives boundingBox = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->m_meshes[pNode->meshIndex].m_pPrimitives[p];
                if (CameraFrustumToBoxCollision(mModelViewProj, boundingBox.m_center, boundingBox.m_radius))
                    continue;

                PBRMaterialParameters *pPbrParams = &pPrimitive->m_pMaterial->m_pbrMaterialParameters;

                // Set per Object constants from material
                //
                per_object cbPerObject;
                cbPerObject.mCurrentWorld = pNodesMatrices[i].GetCurrent();
                cbPerObject.mPreviousWorld = pNodesMatrices[i].GetPrevious();
                cbPerObject.m_pbrParams = pPbrParams->m_params;
                D3D12_GPU_VIRTUAL_ADDRESS perObjectDesc = m_pDynamicBufferRing->AllocConstantBuffer(sizeof(per_object), &cbPerObject);

                // compute depth for sorting
                //                
                math::Vector4 v = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->m_meshes[pNode->meshIndex].m_pPrimitives[p].m_center;
                float depth = (mModelViewProj * v).getW();

                BatchList t;
                t.m_depth = depth;
                t.m_pPrimitive = pPrimitive;
                t.m_perFrameDesc = m_pGLTFTexturesAndBuffers->GetPerFrameConstants();
                t.m_perObjectDesc = perObjectDesc;
                t.m_pPerSkeleton = pPerSkeleton;

                // append primitive to list 
                //
                if (pPbrParams->m_blending == false)
                {
                    pSolid->push_back(t);
                }
                else
                {
                    pTransparent->push_back(t);
                }
            }
        }
    }

    void GltfPbrPass::DrawBatchList(ID3D12GraphicsCommandList *pCommandList, CBV_SRV_UAV *pShadowBufferSRV, std::vector<BatchList> *pBatchList, bool bWireframe/*=false*/)
    {
        UserMarker marker(pCommandList, "GltfPbrPass::DrawBatchList");

        // Set descriptor heaps
        pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ID3D12DescriptorHeap *pDescriptorHeaps[] = { m_pResourceViewHeaps->GetCBV_SRV_UAVHeap(), m_pResourceViewHeaps->GetSamplerHeap() };
        pCommandList->SetDescriptorHeaps(2, pDescriptorHeaps);

        for (auto &t : *pBatchList)
        {
            t.m_pPrimitive->DrawPrimitive(pCommandList, pShadowBufferSRV, t.m_perFrameDesc, t.m_perObjectDesc, t.m_pPerSkeleton, bWireframe);
        }
    }

    void PBRPrimitives::DrawPrimitive(ID3D12GraphicsCommandList *pCommandList, CBV_SRV_UAV *pShadowBufferSRV, D3D12_GPU_VIRTUAL_ADDRESS perFrameDesc, D3D12_GPU_VIRTUAL_ADDRESS perObjectDesc, D3D12_GPU_VIRTUAL_ADDRESS pPerSkeleton, bool bWireframe)
    {
        // Bind indices and vertices using the right offsets into the buffer
        //
        pCommandList->IASetIndexBuffer(&m_geometry.m_IBV);
        pCommandList->IASetVertexBuffers(0, (UINT)m_geometry.m_VBV.size(), m_geometry.m_VBV.data());

        // Bind Descriptor sets
        //
        pCommandList->SetGraphicsRootSignature(m_RootSignature);
        int paramIndex = 0;

        // bind the per scene constant buffer descriptor
        pCommandList->SetGraphicsRootConstantBufferView(paramIndex++, perFrameDesc);

        // bind the textures and samplers descriptors
        if (m_pMaterial->m_textureCount > 0)
        {
            pCommandList->SetGraphicsRootDescriptorTable(paramIndex++, m_pMaterial->m_texturesTable.GetGPU());
        }

        // bind the shadow buffer
        if (pShadowBufferSRV)
        {
            pCommandList->SetGraphicsRootDescriptorTable(paramIndex++, pShadowBufferSRV->GetGPU());
        }

        // bind the per object constant buffer descriptor
        pCommandList->SetGraphicsRootConstantBufferView(paramIndex++, perObjectDesc);

        // bind the skeleton bind matrices constant buffer descriptor
        if (pPerSkeleton != 0)
            pCommandList->SetGraphicsRootConstantBufferView(paramIndex++, pPerSkeleton);

        // Bind Pipeline
        //
        if (bWireframe)
            pCommandList->SetPipelineState(m_PipelineWireframeRender);
        else
            pCommandList->SetPipelineState(m_PipelineRender);

        // Draw
        //
        pCommandList->DrawIndexedInstanced(m_geometry.m_NumIndices, 1, 0, 0, 0);
    }
}