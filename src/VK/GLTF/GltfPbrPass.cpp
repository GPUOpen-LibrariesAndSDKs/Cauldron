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
#include "Misc/Async.h"
#include "GltfHelpers.h"
#include "Base/Helper.h"
#include "Base/ShaderCompilerHelper.h"
#include "Base/ExtDebugUtils.h"
#include "PostProc/Skydome.h"

#include "GltfPbrPass.h"

namespace CAULDRON_VK
{
    //--------------------------------------------------------------------------------------
    //
    // OnCreate
    //
    //--------------------------------------------------------------------------------------
    void GltfPbrPass::OnCreate(
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
        AsyncPool *pAsyncPool,
        bool invertedDepth
    )
    {
        m_pDevice = pDevice;
        m_pRenderPass = pRenderPass;
        m_pResourceViewHeaps = pHeaps;
        m_pStaticBufferPool = pStaticBufferPool;
        m_pDynamicBufferRing = pDynamicBufferRing;
        m_pGLTFTexturesAndBuffers = pGLTFTexturesAndBuffers;
        m_bInvertedDepth = invertedDepth;

        //set bindings for the render targets
        //
        DefineList rtDefines;
        m_pRenderPass->GetCompilerDefines(rtDefines);

        // Load BRDF look up table for the PBR shader
        //
        m_brdfLutTexture.InitFromFile(pDevice, pUploadHeap, "BrdfLut.dds", false); // LUT images are stored as linear
        m_brdfLutTexture.CreateSRV(&m_brdfLutView);

        /////////////////////////////////////////////
        // Create Samplers

        //for pbr materials
        {
            VkSamplerCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            info.magFilter = VK_FILTER_LINEAR;
            info.minFilter = VK_FILTER_LINEAR;
            info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            info.minLod = 0;
            info.maxLod = 10000;
            info.maxAnisotropy = 1.0f;
            VkResult res = vkCreateSampler(pDevice->GetDevice(), &info, NULL, &m_samplerPbr);
            assert(res == VK_SUCCESS);
        }

        // specular BRDF lut sampler
        {
            VkSamplerCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            info.magFilter = VK_FILTER_LINEAR;
            info.minFilter = VK_FILTER_LINEAR;
            info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            info.minLod = -1000;
            info.maxLod = 1000;
            info.maxAnisotropy = 1.0f;
            VkResult res = vkCreateSampler(pDevice->GetDevice(), &info, NULL, &m_brdfLutSampler);
            assert(res == VK_SUCCESS);
        }

        // shadowmap sampler
        {
            VkSamplerCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            info.magFilter = VK_FILTER_LINEAR;
            info.minFilter = VK_FILTER_LINEAR;
            info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            info.compareEnable = VK_TRUE;
            info.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
            info.minLod = -1000;
            info.maxLod = 1000;
            info.maxAnisotropy = 1.0f;
            VkResult res = vkCreateSampler(pDevice->GetDevice(), &info, NULL, &m_samplerShadow);
            assert(res == VK_SUCCESS);
        }

        // Create default material, this material will be used if none is assigned
        //
        {
            SetDefaultMaterialParamters(&m_defaultMaterial.m_pbrMaterialParameters);

            std::map<std::string, VkImageView> texturesBase;
            CreateDescriptorTableForMaterialTextures(&m_defaultMaterial, texturesBase, pSkyDome, ShadowMapViewPool, bUseSSAOMask);
        }

        // Load PBR 2.0 Materials
        //
        const json &j3 = pGLTFTexturesAndBuffers->m_pGLTFCommon->j3;

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
            std::map<std::string, VkImageView> texturesBase;
            for (auto const& value : textureIds)
                texturesBase[value.first] = m_pGLTFTexturesAndBuffers->GetTextureViewByID(value.second);

            CreateDescriptorTableForMaterialTextures(tfmat, texturesBase, pSkyDome, ShadowMapViewPool, bUseSSAOMask);
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

                    ExecAsyncIfThereIsAPool(pAsyncPool, [this, i, rtDefines, &primitive, pPrimitive, bUseSSAOMask]()
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
                        std::vector<VkVertexInputAttributeDescription> inputLayout;
                        m_pGLTFTexturesAndBuffers->CreateGeometry(primitive, requiredAttributes, inputLayout, defines, &pPrimitive->m_geometry);

                        // Create descriptors and pipelines
                        //
                        int skinId = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->FindMeshSkinId(i);
                        int inverseMatrixBufferSize = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->GetInverseBindMatricesBufferSizeByID(skinId);
                        CreateDescriptors(inverseMatrixBufferSize, &defines, pPrimitive, bUseSSAOMask);
                        CreatePipeline(inputLayout, defines, pPrimitive);
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
    void GltfPbrPass::CreateDescriptorTableForMaterialTextures(PBRMaterial *tfmat, std::map<std::string, VkImageView> &texturesBase, SkyDome *pSkyDome, std::vector<VkImageView>& ShadowMapViewPool, bool bUseSSAOMask)
    {
        std::vector<uint32_t> descriptorCounts;
        // count the number of textures to init bindings and descriptor
        {
            tfmat->m_textureCount = (int)texturesBase.size();
            for (int i = 0; i < texturesBase.size(); ++i)
            {
                descriptorCounts.push_back(1);
            }

            if (pSkyDome)
            {
                tfmat->m_textureCount += 3;   // +3 because the skydome has a specular, diffusse and a BDRF LUT map
                descriptorCounts.push_back(1);
                descriptorCounts.push_back(1);
                descriptorCounts.push_back(1);
            }

            if (bUseSSAOMask)
            {
                tfmat->m_textureCount += 1;
                descriptorCounts.push_back(1);
            }

            //if (ShadowMapView != VK_NULL_HANDLE)
            if (!ShadowMapViewPool.empty())
            {
                assert(ShadowMapViewPool.size() <= MaxShadowInstances);
                tfmat->m_textureCount += (int)ShadowMapViewPool.size();//1;
                // this is an array of samplers/textures
                // We should set the exact number of descriptors to avoid validation errors
                descriptorCounts.push_back(MaxShadowInstances);
            }
        }

        // Alloc a descriptor layout and init the descriptor set for the following textures 
        // 1) all the textures of the PBR material (if any)
        // 2) the 3 textures used for IBL: 
        //         - 1 BRDF LUT 
        //         - 2 cubemaps for the specular, difusse
        // 3) SSAO texture
        // 4) the shadowmaps (array of MaxShadowInstances entries -- maximum)
        // for each entry we create a #define with that texture name that hold the id of the texture. That way the PS knows in what slot is each texture.      
        {
            // allocate descriptor table for the textures
            m_pResourceViewHeaps->AllocDescriptor(descriptorCounts, NULL, &tfmat->m_texturesDescriptorSetLayout, &tfmat->m_texturesDescriptorSet);

            uint32_t cnt = 0;

            // 1) create SRV for the PBR materials
            for (auto const &it : texturesBase)
            {
                tfmat->m_pbrMaterialParameters.m_defines[std::string("ID_") + it.first] = std::to_string(cnt);
                SetDescriptorSet(m_pDevice->GetDevice(), cnt, it.second, &m_samplerPbr, tfmat->m_texturesDescriptorSet);
                cnt++;
            }

            // 2) 3 SRVs for the IBL probe
            if (pSkyDome)
            {
                tfmat->m_pbrMaterialParameters.m_defines["ID_brdfTexture"] = std::to_string(cnt);
                SetDescriptorSet(m_pDevice->GetDevice(), cnt, m_brdfLutView, &m_brdfLutSampler, tfmat->m_texturesDescriptorSet);
                cnt++;

                tfmat->m_pbrMaterialParameters.m_defines["ID_diffuseCube"] = std::to_string(cnt);
                pSkyDome->SetDescriptorDiff(cnt, tfmat->m_texturesDescriptorSet);
                cnt++;

                tfmat->m_pbrMaterialParameters.m_defines["ID_specularCube"] = std::to_string(cnt);
                pSkyDome->SetDescriptorSpec(cnt, tfmat->m_texturesDescriptorSet);
                cnt++;

                tfmat->m_pbrMaterialParameters.m_defines["USE_IBL"] = "1";
            }

            // 3) SSAO mask
            //
            if (bUseSSAOMask)
            {
                tfmat->m_pbrMaterialParameters.m_defines["ID_SSAO"] = std::to_string(cnt);
                cnt++;
            }

            // 4) Up to MaxShadowInstances SRVs for the shadowmaps
            if (!ShadowMapViewPool.empty())
            {
                tfmat->m_pbrMaterialParameters.m_defines["ID_shadowMap"] = std::to_string(cnt);
                
                SetDescriptorSet(m_pDevice->GetDevice(), cnt, descriptorCounts[cnt], ShadowMapViewPool, &m_samplerShadow, tfmat->m_texturesDescriptorSet);
                cnt++;
            }
        }
    }

    //--------------------------------------------------------------------------------------
    //
    // OnUpdateWindowSizeDependentResources
    //
    //--------------------------------------------------------------------------------------
    void GltfPbrPass::OnUpdateWindowSizeDependentResources(VkImageView SSAO)
    {
        for (uint32_t i = 0; i < m_materialsData.size(); i++)
        {
            PBRMaterial *tfmat = &m_materialsData[i];

            DefineList def = tfmat->m_pbrMaterialParameters.m_defines;

            auto id = def.find("ID_SSAO");
            if (id != def.end())
            {
                int index = std::stoi(id->second);
                SetDescriptorSet(m_pDevice->GetDevice(), index, SSAO, &m_samplerPbr, tfmat->m_texturesDescriptorSet);                
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
                vkDestroyPipeline(m_pDevice->GetDevice(), pPrimitive->m_pipeline, nullptr);
                pPrimitive->m_pipeline = VK_NULL_HANDLE;
                vkDestroyPipeline(m_pDevice->GetDevice(), pPrimitive->m_pipelineWireframe, nullptr);
                pPrimitive->m_pipelineWireframe = VK_NULL_HANDLE;
                vkDestroyPipelineLayout(m_pDevice->GetDevice(), pPrimitive->m_pipelineLayout, nullptr);
                vkDestroyDescriptorSetLayout(m_pDevice->GetDevice(), pPrimitive->m_uniformsDescriptorSetLayout, NULL);
                m_pResourceViewHeaps->FreeDescriptor(pPrimitive->m_uniformsDescriptorSet);
            }
        }

        for (int i = 0; i < m_materialsData.size(); i++)
        {
            vkDestroyDescriptorSetLayout(m_pDevice->GetDevice(), m_materialsData[i].m_texturesDescriptorSetLayout, NULL);
            m_pResourceViewHeaps->FreeDescriptor(m_materialsData[i].m_texturesDescriptorSet);
        }

        //destroy default material
        vkDestroyDescriptorSetLayout(m_pDevice->GetDevice(), m_defaultMaterial.m_texturesDescriptorSetLayout, NULL);
        m_pResourceViewHeaps->FreeDescriptor(m_defaultMaterial.m_texturesDescriptorSet);

        vkDestroySampler(m_pDevice->GetDevice(), m_samplerPbr, nullptr);
        vkDestroySampler(m_pDevice->GetDevice(), m_samplerShadow, nullptr);

        vkDestroyImageView(m_pDevice->GetDevice(), m_brdfLutView, NULL);
        vkDestroySampler(m_pDevice->GetDevice(), m_brdfLutSampler, nullptr);
        m_brdfLutTexture.OnDestroy();

    }

    //--------------------------------------------------------------------------------------
    //
    // CreateDescriptors for a combination of material and geometry
    //
    //--------------------------------------------------------------------------------------
    void GltfPbrPass::CreateDescriptors(int inverseMatrixBufferSize, DefineList *pAttributeDefines, PBRPrimitives *pPrimitive, bool bUseSSAOMask)
    {
        // Creates descriptor set layout binding for the constant buffers
        //
        std::vector<VkDescriptorSetLayoutBinding> layout_bindings(2);

        // Constant buffer 'per frame'
        layout_bindings[0].binding = 0;
        layout_bindings[0].descriptorCount = 1;
        layout_bindings[0].pImmutableSamplers = NULL;
        layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        (*pAttributeDefines)["ID_PER_FRAME"] = std::to_string(layout_bindings[0].binding);

        // Constant buffer 'per object'
        layout_bindings[1].binding = 1;
        layout_bindings[1].descriptorCount = 1;
        layout_bindings[1].pImmutableSamplers = NULL;
        layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        layout_bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        (*pAttributeDefines)["ID_PER_OBJECT"] = std::to_string(layout_bindings[1].binding);

        // Constant buffer holding the skinning matrices
        if (inverseMatrixBufferSize >= 0)
        {
            VkDescriptorSetLayoutBinding b;

            // skinning matrices
            b.binding = 2;
            b.descriptorCount = 1;
            b.pImmutableSamplers = NULL;
            b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            (*pAttributeDefines)["ID_SKINNING_MATRICES"] = std::to_string(b.binding);

            layout_bindings.push_back(b);
        }

        m_pResourceViewHeaps->CreateDescriptorSetLayoutAndAllocDescriptorSet(&layout_bindings, &pPrimitive->m_uniformsDescriptorSetLayout, &pPrimitive->m_uniformsDescriptorSet);

        // Init descriptors sets for the constant buffers
        //
        m_pDynamicBufferRing->SetDescriptorSet(0, sizeof(per_frame), pPrimitive->m_uniformsDescriptorSet);
        m_pDynamicBufferRing->SetDescriptorSet(1, sizeof(per_object), pPrimitive->m_uniformsDescriptorSet);

        if (inverseMatrixBufferSize >= 0)
        {
            m_pDynamicBufferRing->SetDescriptorSet(2, (uint32_t)inverseMatrixBufferSize, pPrimitive->m_uniformsDescriptorSet);
        }

        // Create the pipeline layout
        //
        std::vector<VkDescriptorSetLayout> descriptorSetLayout = { pPrimitive->m_uniformsDescriptorSetLayout };
        if (pPrimitive->m_pMaterial->m_texturesDescriptorSetLayout != VK_NULL_HANDLE)
            descriptorSetLayout.push_back(pPrimitive->m_pMaterial->m_texturesDescriptorSetLayout);

        VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
        pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pPipelineLayoutCreateInfo.pNext = NULL;
        pPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        pPipelineLayoutCreateInfo.pPushConstantRanges = NULL;
        pPipelineLayoutCreateInfo.setLayoutCount = (uint32_t)descriptorSetLayout.size();
        pPipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayout.data();

        VkResult res = vkCreatePipelineLayout(m_pDevice->GetDevice(), &pPipelineLayoutCreateInfo, NULL, &pPrimitive->m_pipelineLayout);
        assert(res == VK_SUCCESS);
        SetResourceName(m_pDevice->GetDevice(), VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t)pPrimitive->m_pipelineLayout, "GltfPbrPass PL");
    }

    //--------------------------------------------------------------------------------------
    //
    // CreatePipeline
    //
    //--------------------------------------------------------------------------------------
    void GltfPbrPass::CreatePipeline(std::vector<VkVertexInputAttributeDescription> layout, const DefineList &defines, PBRPrimitives *pPrimitive)
    {
        // Compile and create shaders
        //
        VkPipelineShaderStageCreateInfo vertexShader = {}, fragmentShader = {};
        VKCompileFromFile(m_pDevice->GetDevice(), VK_SHADER_STAGE_VERTEX_BIT, "GLTFPbrPass-vert.glsl", "main", "", &defines, &vertexShader);
        VKCompileFromFile(m_pDevice->GetDevice(), VK_SHADER_STAGE_FRAGMENT_BIT, "GLTFPbrPass-frag.glsl", "main", "", &defines, &fragmentShader);

        std::vector<VkPipelineShaderStageCreateInfo> shaderStages = { vertexShader, fragmentShader };

        // Create pipeline
        //

        // vertex input state

        std::vector<VkVertexInputBindingDescription> vi_binding(layout.size());
        for (int i = 0; i < layout.size(); i++)
        {
            vi_binding[i].binding = layout[i].binding;
            vi_binding[i].stride = SizeOfFormat(layout[i].format);
            vi_binding[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        }

        VkPipelineVertexInputStateCreateInfo vi = {};
        vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vi.pNext = NULL;
        vi.flags = 0;
        vi.vertexBindingDescriptionCount = (uint32_t)vi_binding.size();
        vi.pVertexBindingDescriptions = vi_binding.data();
        vi.vertexAttributeDescriptionCount = (uint32_t)layout.size();
        vi.pVertexAttributeDescriptions = layout.data();

        // input assembly state

        VkPipelineInputAssemblyStateCreateInfo ia;
        ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        ia.pNext = NULL;
        ia.flags = 0;
        ia.primitiveRestartEnable = VK_FALSE;
        ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        // rasterizer state

        VkPipelineRasterizationStateCreateInfo rs;
        rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rs.pNext = NULL;
        rs.flags = 0;
        rs.polygonMode = VK_POLYGON_MODE_FILL;
        rs.cullMode = pPrimitive->m_pMaterial->m_pbrMaterialParameters.m_doubleSided ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;
        rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rs.depthClampEnable = VK_FALSE;
        rs.rasterizerDiscardEnable = VK_FALSE;
        rs.depthBiasEnable = VK_FALSE;
        rs.depthBiasConstantFactor = 0;
        rs.depthBiasClamp = 0;
        rs.depthBiasSlopeFactor = 0;
        rs.lineWidth = 1.0f;


        std::vector<VkPipelineColorBlendAttachmentState> att_states;
        if (defines.Has("HAS_FORWARD_RT"))
        {
            VkPipelineColorBlendAttachmentState att_state = {};
            att_state.colorWriteMask = 0xf;
            att_state.blendEnable = (defines.Has("DEF_alphaMode_BLEND"));
            att_state.colorBlendOp = VK_BLEND_OP_ADD;
            att_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            att_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            att_state.alphaBlendOp = VK_BLEND_OP_ADD;
            att_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            att_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            att_states.push_back(att_state);
        }
        if (defines.Has("HAS_SPECULAR_ROUGHNESS_RT"))
        {
            VkPipelineColorBlendAttachmentState att_state = {};
            att_state.colorWriteMask = 0xf;
            att_state.blendEnable = defines.Has("DEF_alphaMode_BLEND");
            att_state.alphaBlendOp = VK_BLEND_OP_ADD;
            att_state.colorBlendOp = VK_BLEND_OP_ADD;
            att_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            att_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            att_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            att_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            att_states.push_back(att_state);
        }
        if (defines.Has("HAS_DIFFUSE_RT"))
        {
            VkPipelineColorBlendAttachmentState att_state = {};
            att_state.colorWriteMask = 0xf;
            att_state.blendEnable = defines.Has("DEF_alphaMode_BLEND");
            att_state.alphaBlendOp = VK_BLEND_OP_ADD;
            att_state.colorBlendOp = VK_BLEND_OP_ADD;
            att_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            att_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            att_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            att_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            att_states.push_back(att_state);
        }
        if (defines.Has("HAS_NORMALS_RT"))
        {
            VkPipelineColorBlendAttachmentState att_state = {};
            att_state.colorWriteMask = 0xf;
            att_state.blendEnable = defines.Has("DEF_alphaMode_BLEND");
            att_state.alphaBlendOp = VK_BLEND_OP_ADD;
            att_state.colorBlendOp = VK_BLEND_OP_ADD;
            att_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            att_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            att_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            att_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            att_states.push_back(att_state);
        }
        if (defines.Has("HAS_MOTION_VECTORS_RT"))
        {
            VkPipelineColorBlendAttachmentState att_state = {};
            att_state.colorWriteMask = 0xf;
            att_state.blendEnable = defines.Has("DEF_alphaMode_BLEND");
            att_state.alphaBlendOp = VK_BLEND_OP_ADD;
            att_state.colorBlendOp = VK_BLEND_OP_ADD;
            att_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            att_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            att_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            att_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            att_states.push_back(att_state);
        }
        {
            VkPipelineColorBlendAttachmentState att_state = {};
            att_state.colorWriteMask = defines.Has("DEF_alphaMode_BLEND") ? VK_COLOR_COMPONENT_R_BIT : VK_COLOR_COMPONENT_A_BIT;
            att_state.blendEnable = defines.Has("DEF_alphaMode_BLEND");
            att_state.alphaBlendOp = VK_BLEND_OP_ADD;
            att_state.colorBlendOp = VK_BLEND_OP_ADD;
            att_state.srcColorBlendFactor = defines.Has("DEF_alphaMode_BLEND") ? VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR : VK_BLEND_FACTOR_ONE;
            att_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
            att_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            att_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

            bool bHasAnimatedTexture = false;
            bHasAnimatedTexture |= defines.Has("HAS_NORMAL_UV_TRANSFORM");
            bHasAnimatedTexture |= defines.Has("HAS_EMISSIVE_UV_TRANSFORM");
            bHasAnimatedTexture |= defines.Has("HAS_OCCLSION_UV_TRANSFORM");
            bHasAnimatedTexture |= defines.Has("HAS_BASECOLOR_UV_TRANSFORM");
            bHasAnimatedTexture |= defines.Has("HAS_METALLICROUGHNESS_UV_TRANSFORM");
            bHasAnimatedTexture |= defines.Has("HAS_SPECULARGLOSSINESS_UV_TRANSFORM");
            bHasAnimatedTexture |= defines.Has("HAS_DIFFUSE_UV_TRANSFORM");

            if (bHasAnimatedTexture)
                att_state.colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;

            if (defines.Has("HAS_UPSCALE_REACTIVE_RT"))
                att_states.push_back(att_state);

            if (defines.Has("HAS_UPSCALE_TRANSPARENCY_AND_COMPOSITION_RT"))
                att_states.push_back(att_state);
        }

        // Color blend state

        VkPipelineColorBlendStateCreateInfo cb;
        cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        cb.flags = 0;
        cb.pNext = NULL;
        cb.attachmentCount = static_cast<uint32_t>(att_states.size());
        cb.pAttachments = att_states.data();
        cb.logicOpEnable = VK_FALSE;
        cb.logicOp = VK_LOGIC_OP_NO_OP;
        cb.blendConstants[0] = 1.0f;
        cb.blendConstants[1] = 1.0f;
        cb.blendConstants[2] = 1.0f;
        cb.blendConstants[3] = 1.0f;

        std::vector<VkDynamicState> dynamicStateEnables = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_FRAGMENT_SHADING_RATE_KHR,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState = {};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.pNext = NULL;
        dynamicState.pDynamicStates = dynamicStateEnables.data();
        dynamicState.dynamicStateCount = (uint32_t)dynamicStateEnables.size();

        // view port state

        VkPipelineViewportStateCreateInfo vp = {};
        vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        vp.pNext = NULL;
        vp.flags = 0;
        vp.viewportCount = 1;
        vp.scissorCount = 1;
        vp.pScissors = NULL;
        vp.pViewports = NULL;

        // depth stencil state

        VkPipelineDepthStencilStateCreateInfo ds;
        ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        ds.pNext = NULL;
        ds.flags = 0;
        ds.depthTestEnable = true;
        if (defines.Has("DEF_alphaMode_BLEND")) {
            ds.depthWriteEnable = false;
        }
        else {
            ds.depthWriteEnable = true;
        }
        ds.depthCompareOp = m_bInvertedDepth ? VK_COMPARE_OP_GREATER_OR_EQUAL : VK_COMPARE_OP_LESS_OR_EQUAL;
        ds.back.failOp = VK_STENCIL_OP_KEEP;
        ds.back.passOp = VK_STENCIL_OP_KEEP;
        ds.back.compareOp = VK_COMPARE_OP_ALWAYS;
        ds.back.compareMask = 0;
        ds.back.reference = 0;
        ds.back.depthFailOp = VK_STENCIL_OP_KEEP;
        ds.back.writeMask = 0;
        ds.depthBoundsTestEnable = VK_FALSE;
        ds.minDepthBounds = 0;
        ds.maxDepthBounds = 0;
        ds.stencilTestEnable = VK_FALSE;
        ds.front = ds.back;

        // multi sample state

        VkPipelineMultisampleStateCreateInfo ms;
        ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        ms.pNext = NULL;
        ms.flags = 0;
        ms.pSampleMask = NULL;
        ms.rasterizationSamples = m_pRenderPass->GetSampleCount();
        ms.sampleShadingEnable = VK_FALSE;
        ms.alphaToCoverageEnable = VK_FALSE;
        ms.alphaToOneEnable = VK_FALSE;
        ms.minSampleShading = 0.0;

        // create pipeline 
        //
        VkGraphicsPipelineCreateInfo pipeline = {};
        pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline.pNext = NULL;
        pipeline.layout = pPrimitive->m_pipelineLayout;
        pipeline.basePipelineHandle = VK_NULL_HANDLE;
        pipeline.basePipelineIndex = 0;
        pipeline.flags = 0;
        pipeline.pVertexInputState = &vi;
        pipeline.pInputAssemblyState = &ia;
        pipeline.pRasterizationState = &rs;
        pipeline.pColorBlendState = &cb;
        pipeline.pTessellationState = NULL;
        pipeline.pMultisampleState = &ms;
        pipeline.pDynamicState = &dynamicState;
        pipeline.pViewportState = &vp;
        pipeline.pDepthStencilState = &ds;
        pipeline.pStages = shaderStages.data();
        pipeline.stageCount = (uint32_t)shaderStages.size();
        pipeline.renderPass = m_pRenderPass->GetRenderPass();
        pipeline.subpass = 0;

        VkResult res = vkCreateGraphicsPipelines(m_pDevice->GetDevice(), m_pDevice->GetPipelineCache(), 1, &pipeline, NULL, &pPrimitive->m_pipeline);
        assert(res == VK_SUCCESS);
        SetResourceName(m_pDevice->GetDevice(), VK_OBJECT_TYPE_PIPELINE, (uint64_t)pPrimitive->m_pipeline, "GltfPbrPass P");

        // create wireframe pipeline
        rs.polygonMode = VK_POLYGON_MODE_LINE;
        rs.cullMode = VK_CULL_MODE_NONE;
        res = vkCreateGraphicsPipelines(m_pDevice->GetDevice(), m_pDevice->GetPipelineCache(), 1, &pipeline, NULL, &pPrimitive->m_pipelineWireframe);
        assert(res == VK_SUCCESS);
        SetResourceName(m_pDevice->GetDevice(), VK_OBJECT_TYPE_PIPELINE, (uint64_t)pPrimitive->m_pipelineWireframe, "GltfPbrPass Wireframe P");
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
            VkDescriptorBufferInfo *pPerSkeleton = m_pGLTFTexturesAndBuffers->GetSkinningMatricesBuffer(pNode->skinIndex);

            math::Matrix4 mModelViewProj =  m_pGLTFTexturesAndBuffers->m_pGLTFCommon->m_perFrameData.mCameraCurrViewProj * pNodesMatrices[i].GetCurrent();

            // loop through primitives
            //
            PBRMesh *pMesh = &m_meshes[pNode->meshIndex];
            for (uint32_t p = 0; p < pMesh->m_pPrimitives.size(); p++)
            {
                PBRPrimitives *pPrimitive = &pMesh->m_pPrimitives[p];

                if ((bWireframe && pPrimitive->m_pipelineWireframe == VK_NULL_HANDLE)
                    || (!bWireframe && pPrimitive->m_pipeline == VK_NULL_HANDLE))
                    continue;

                // do frustrum culling
                //
                tfPrimitives boundingBox = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->m_meshes[pNode->meshIndex].m_pPrimitives[p];
                if (CameraFrustumToBoxCollision(mModelViewProj, boundingBox.m_center, boundingBox.m_radius))
                    continue;

                PBRMaterialParameters *pPbrParams = &pPrimitive->m_pMaterial->m_pbrMaterialParameters;

                // Set per Object constants from material
                //
                per_object *cbPerObject;
                VkDescriptorBufferInfo perObjectDesc;
                m_pDynamicBufferRing->AllocConstantBuffer(sizeof(per_object), (void **)&cbPerObject, &perObjectDesc);
                cbPerObject->mCurrentWorld = pNodesMatrices[i].GetCurrent();
                cbPerObject->mPreviousWorld = pNodesMatrices[i].GetPrevious();
                cbPerObject->m_pbrParams = pPbrParams->m_params;

                // compute depth for sorting
                //
                math::Vector4 v = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->m_meshes[pNode->meshIndex].m_pPrimitives[p].m_center;
                float depth = (mModelViewProj * v).getW();

                BatchList t;
                t.m_depth = depth;
                t.m_pPrimitive = pPrimitive;
                t.m_perFrameDesc = m_pGLTFTexturesAndBuffers->m_perFrameConstants;
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

    void GltfPbrPass::DrawBatchList(VkCommandBuffer commandBuffer, std::vector<BatchList> *pBatchList, bool bWireframe/*=false*/)
    {
        SetPerfMarkerBegin(commandBuffer, "gltfPBR");
        
        for (auto &t : *pBatchList)
        {
            t.m_pPrimitive->DrawPrimitive(commandBuffer, t.m_perFrameDesc, t.m_perObjectDesc, t.m_pPerSkeleton, bWireframe);
        }

        SetPerfMarkerEnd(commandBuffer);
    }

    void PBRPrimitives::DrawPrimitive(VkCommandBuffer cmd_buf, VkDescriptorBufferInfo perFrameDesc, VkDescriptorBufferInfo perObjectDesc, VkDescriptorBufferInfo *pPerSkeleton, bool bWireframe)
    {
        // Bind indices and vertices using the right offsets into the buffer
        //
        for (uint32_t i = 0; i < m_geometry.m_VBV.size(); i++)
        {
            vkCmdBindVertexBuffers(cmd_buf, i, 1, &m_geometry.m_VBV[i].buffer, &m_geometry.m_VBV[i].offset);
        }

        vkCmdBindIndexBuffer(cmd_buf, m_geometry.m_IBV.buffer, m_geometry.m_IBV.offset, m_geometry.m_indexType);

        // Bind Descriptor sets
        //
        VkDescriptorSet descritorSets[2] = { m_uniformsDescriptorSet, m_pMaterial->m_texturesDescriptorSet };
        uint32_t descritorSetsCount = (m_pMaterial->m_textureCount == 0) ? 1 : 2;

        uint32_t uniformOffsets[3] = { (uint32_t)perFrameDesc.offset,  (uint32_t)perObjectDesc.offset, (pPerSkeleton) ? (uint32_t)pPerSkeleton->offset : 0 };
        uint32_t uniformOffsetsCount = (pPerSkeleton) ? 3 : 2;

        vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, descritorSetsCount, descritorSets, uniformOffsetsCount, uniformOffsets);

        // Bind Pipeline
        //
        if (bWireframe)
            vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineWireframe);
        else
            vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

        // Draw
        //
        vkCmdDrawIndexed(cmd_buf, m_geometry.m_NumIndices, 1, 0, 0, 0);
    }
}