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
#include "GLTF/GltfHelpers.h"
#include "Base/ShaderCompilerHelper.h"
#include "Base/ResourceViewHeaps.h"
#include "Base/ExtDebugUtils.h"
#include "Base/Helper.h"
#include "Misc/Async.h"
#include "Misc/Misc.h"

#include "GLTF/GltfPbrMaterial.h"
#include "GltfMotionVectorsPass.h"

namespace CAULDRON_VK
{
    //--------------------------------------------------------------------------------------
    //
    // OnCreate
    //
    //--------------------------------------------------------------------------------------
    void GltfMotionVectorsPass::OnCreate(
        Device *pDevice,
        VkRenderPass renderPass,
        UploadHeap *pUploadHeap,
        ResourceViewHeaps *pHeaps,
        DynamicBufferRing *pDynamicBufferRing,
        StaticBufferPool *pStaticBufferPool,
        GLTFTexturesAndBuffers *pGLTFTexturesAndBuffers,
        VkFormat motionVectorsBufferFormat,
        VkFormat normalBufferFormat,
        AsyncPool *pAsyncPool)
    {
        m_pDevice = pDevice;
        m_pResourceViewHeaps = pHeaps;
        m_pStaticBufferPool = pStaticBufferPool;
        m_pDynamicBufferRing = pDynamicBufferRing;
        m_pGLTFTexturesAndBuffers = pGLTFTexturesAndBuffers;

        m_normalBufferFormat = normalBufferFormat;
        m_motionVectorsBufferFormat = motionVectorsBufferFormat;

        const json &j3 = pGLTFTexturesAndBuffers->m_pGLTFCommon->j3;

        /////////////////////////////////////////////
        // Create default material

        m_defaultMaterial.m_textureCount = 0;
        m_defaultMaterial.m_descriptorSet = VK_NULL_HANDLE;
        m_defaultMaterial.m_descriptorSetLayout = VK_NULL_HANDLE;
        m_defaultMaterial.m_doubleSided = false;

        // Create static sampler 
        //
        {
            VkSamplerCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            info.magFilter = VK_FILTER_LINEAR;
            info.minFilter = VK_FILTER_LINEAR;
            info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            info.minLod = -1000;
            info.maxLod = 1000;
            info.maxAnisotropy = 1.0f;
            VkResult res = vkCreateSampler(pDevice->GetDevice(), &info, NULL, &m_sampler);
            assert(res == VK_SUCCESS);
        }

        // Create materials (in a depth pass materials are still needed to handle non opaque textures
        //
        if (j3.find("materials") != j3.end())
        {
            const json &materials = j3["materials"];

            m_materialsData.resize(materials.size());
            for (uint32_t i = 0; i < materials.size(); i++)
            {
                const json &material = materials[i];

                MotionVectorMaterial *tfmat = &m_materialsData[i];

                // Load material constants. This is a depth pass and we are only interested in the mask texture
                //               
                tfmat->m_doubleSided = GetElementBoolean(material, "doubleSided", false);
                std::string alphaMode = GetElementString(material, "alphaMode", "OPAQUE");
                tfmat->m_defines["DEF_alphaMode_" + alphaMode] = std::to_string(1);

                std::vector<VkImageView> textures;

                if (normalBufferFormat != VK_FORMAT_UNDEFINED)
                {
                    int index, texCoord;
                    if (ProcessGetTextureIndexAndTextCoord(material, "normalTexture", &index, &texCoord))
                    {
                        tfmat->m_defines["ID_normalTexture"] = std::to_string(textures.size());
                        tfmat->m_defines["ID_normalTexCoord"] = std::to_string(texCoord);
                        textures.push_back(pGLTFTexturesAndBuffers->GetTextureViewByID(index));
                    }
                }

                // If transparent use the baseColorTexture for alpha
                //
                if (alphaMode != "OPAQUE")
                {
                    tfmat->m_defines["DEF_alphaCutoff"] = std::to_string(GetElementFloat(material, "alphaCutoff", 0.5));

                    auto pbrMetallicRoughnessIt = material.find("pbrMetallicRoughness");
                    if (pbrMetallicRoughnessIt != material.end())
                    {
                        tfmat->m_defines["MATERIAL_METALLICROUGHNESS"] = "1";

                        int index, texCoord;
                        if (ProcessGetTextureIndexAndTextCoord(*pbrMetallicRoughnessIt, "baseColorTexture", &index, &texCoord))
                        {
                            tfmat->m_defines["ID_baseColorTexture"] = std::to_string(textures.size());
                            tfmat->m_defines["ID_baseTexCoord"] = std::to_string(texCoord);
                            textures.push_back(pGLTFTexturesAndBuffers->GetTextureViewByID(index));
                        }
                    }
                    else
                    {
                        auto extensions = material.find("extensions");
                        if (extensions != material.end())
                        {
                            auto KHR_materials_pbrSpecularGlossinessIt = extensions->find("KHR_materials_pbrSpecularGlossiness");
                            if (KHR_materials_pbrSpecularGlossinessIt != extensions->end())
                            {
                                tfmat->m_defines["MATERIAL_SPECULARGLOSSINESS"] = "1";

                                int index, texCoord;
                                if (ProcessGetTextureIndexAndTextCoord(*KHR_materials_pbrSpecularGlossinessIt, "diffuseTexture", &index, &texCoord))
                                {
                                    tfmat->m_defines["ID_diffuseTexture"] = std::to_string(textures.size());
                                    tfmat->m_defines["ID_diffuseTexCoord"] = std::to_string(texCoord);
                                    textures.push_back(pGLTFTexturesAndBuffers->GetTextureViewByID(index));
                                }
                            }
                        }
                    }
                }
                
                // allocate descriptor table for the texture
                tfmat->m_textureCount = static_cast<int>(textures.size());
                m_pResourceViewHeaps->AllocDescriptor(tfmat->m_textureCount, NULL, &tfmat->m_descriptorSetLayout, &tfmat->m_descriptorSet);
                int cnt = 0;
                for (auto const &it : textures)
                {
                    SetDescriptorSet(m_pDevice->GetDevice(), cnt++, it, &m_sampler, tfmat->m_descriptorSet);                    
                }
            }
        }

        // Configure rendertargets
        //
        DefineList definesRenderTargets;
        int formatsCount=0;
        if (m_motionVectorsBufferFormat != VK_FORMAT_UNDEFINED)
        {
            definesRenderTargets["HAS_MOTION_VECTORS"] = std::to_string(formatsCount++);
        }
        if (m_normalBufferFormat != VK_FORMAT_UNDEFINED)
        {
            definesRenderTargets["HAS_NORMALS"] = std::to_string(formatsCount++);
        }

        // Load Meshes
        //
        if (j3.find("meshes") != j3.end())
        {
            const json &meshes = j3["meshes"];
            const json &accessors = j3["accessors"];
            m_meshes.resize(meshes.size());
            for (uint32_t i = 0; i < meshes.size(); i++)
            {
                MotionVectorMesh *tfmesh = &m_meshes[i];

                const json &primitives = meshes[i]["primitives"];
                tfmesh->m_pPrimitives.resize(primitives.size());
                for (uint32_t p = 0; p < primitives.size(); p++)
                {
                    const json &primitive = primitives[p];
                    MotionVectorPrimitives *pPrimitive = &tfmesh->m_pPrimitives[p];

                    ExecAsyncIfThereIsAPool(pAsyncPool, [this, i, formatsCount, normalBufferFormat, renderPass, definesRenderTargets, &primitive, pPrimitive]()
                    {
                        // Set Material
                        //
                        auto mat = primitive.find("material");
                        pPrimitive->m_pMaterial = (mat != primitive.end()) ? &m_materialsData[mat.value()] : &m_defaultMaterial;

                        // specify attributes needed to render the material
                        //
                        std::vector<std::string> requiredAttributes;
                        for (auto const & it : primitive["attributes"].items())
                        {
                            const std::string semanticName = it.key();
                            if (
                                (semanticName == "POSITION") ||
                                (semanticName.substr(0, 7) == "WEIGHTS") || // for skinning
                                (semanticName.substr(0, 6) == "JOINTS") || // for skinning
                                (DoesMaterialUseSemantic(pPrimitive->m_pMaterial->m_defines, semanticName) == true) ||
                                ((normalBufferFormat != DXGI_FORMAT_UNKNOWN) && ((semanticName == "NORMAL") || (semanticName == "TANGENT"))) // for obvious reasons
                                )
                            {
                                requiredAttributes.push_back(semanticName);
                            }
                        }

                        // holds all the #defines from materials, geometry and texture IDs, the VS & PS shaders need this to get the bindings and code paths
                        //
                        DefineList defines = definesRenderTargets + pPrimitive->m_pMaterial->m_defines;

                        // Get input layout from glTF attributes
                        //
                        std::vector<VkVertexInputAttributeDescription> layout;
                        m_pGLTFTexturesAndBuffers->CreateGeometry(primitive, requiredAttributes, layout, defines, &pPrimitive->m_geometry);

                        // Create Pipeline
                        //
                        int skinId = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->FindMeshSkinId(i);
                        int inverseMatrixBufferSize = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->GetInverseBindMatricesBufferSizeByID(skinId);
                        CreatePipeline(renderPass, formatsCount, inverseMatrixBufferSize, layout, defines, pPrimitive);
                    });
                }
            }
        }
    }

    //--------------------------------------------------------------------------------------
    //
    // OnDestroy
    //
    //--------------------------------------------------------------------------------------
    void GltfMotionVectorsPass::OnDestroy()
    {
        for (uint32_t m = 0; m < m_meshes.size(); m++)
        {
            MotionVectorMesh *pMesh = &m_meshes[m];
            for (uint32_t p = 0; p < pMesh->m_pPrimitives.size(); p++)
            {
                MotionVectorPrimitives *pPrimitive = &pMesh->m_pPrimitives[p];
                vkDestroyPipeline(m_pDevice->GetDevice(), pPrimitive->m_pipeline, nullptr);
                pPrimitive->m_pipeline = VK_NULL_HANDLE;
                vkDestroyPipelineLayout(m_pDevice->GetDevice(), pPrimitive->m_pipelineLayout, nullptr);
                vkDestroyDescriptorSetLayout(m_pDevice->GetDevice(), pPrimitive->m_descriptorSetLayout, NULL);
                m_pResourceViewHeaps->FreeDescriptor(pPrimitive->m_descriptorSet);
            }
        }

        for (int i = 0; i < m_materialsData.size(); i++)
        {
            vkDestroyDescriptorSetLayout(m_pDevice->GetDevice(), m_materialsData[i].m_descriptorSetLayout, NULL);
            m_pResourceViewHeaps->FreeDescriptor(m_materialsData[i].m_descriptorSet);
        }

        vkDestroySampler(m_pDevice->GetDevice(), m_sampler, nullptr);
    }

    //--------------------------------------------------------------------------------------
    //
    // CreatePipeline
    //
    //--------------------------------------------------------------------------------------
    void GltfMotionVectorsPass::CreatePipeline(VkRenderPass renderPass, int attachmentCount, int inverseMatrixBufferSize, std::vector<VkVertexInputAttributeDescription> layout, DefineList &defines, MotionVectorPrimitives *pPrimitive)
    {
        /////////////////////////////////////////////
        // Create descriptors 

        {
            std::vector<VkDescriptorSetLayoutBinding> layout_bindings(2);
            layout_bindings[0].binding = 0;
            layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            layout_bindings[0].descriptorCount = 1;
            layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            layout_bindings[0].pImmutableSamplers = NULL;
            defines["ID_PER_FRAME"] = std::to_string(layout_bindings[0].binding);

            layout_bindings[1].binding = 1;
            layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            layout_bindings[1].descriptorCount = 1;
            layout_bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            layout_bindings[1].pImmutableSamplers = NULL;
            defines["ID_PER_OBJECT"] = std::to_string(layout_bindings[1].binding);

            if (inverseMatrixBufferSize > 0)
            {
                VkDescriptorSetLayoutBinding b;

                // skinning matrices
                b.binding = 2;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                b.pImmutableSamplers = NULL;
                defines["ID_SKINNING_MATRICES"] = std::to_string(b.binding);

                layout_bindings.push_back(b);
            }

            m_pResourceViewHeaps->CreateDescriptorSetLayoutAndAllocDescriptorSet(&layout_bindings, &pPrimitive->m_descriptorSetLayout, &pPrimitive->m_descriptorSet);

            // set descriptors entries

            m_pDynamicBufferRing->SetDescriptorSet(0, sizeof(per_frame), pPrimitive->m_descriptorSet);
            m_pDynamicBufferRing->SetDescriptorSet(1, sizeof(per_object), pPrimitive->m_descriptorSet);

            if (inverseMatrixBufferSize > 0)
            {
                m_pDynamicBufferRing->SetDescriptorSet(2, (uint32_t)inverseMatrixBufferSize, pPrimitive->m_descriptorSet);
            }

            std::vector<VkDescriptorSetLayout> descriptorSetLayout = { pPrimitive->m_descriptorSetLayout };
            if (pPrimitive->m_pMaterial->m_descriptorSetLayout != VK_NULL_HANDLE)
                descriptorSetLayout.push_back(pPrimitive->m_pMaterial->m_descriptorSetLayout);

            /////////////////////////////////////////////
            // Create a PSO description

            VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
            pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pPipelineLayoutCreateInfo.pNext = NULL;
            pPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
            pPipelineLayoutCreateInfo.pPushConstantRanges = NULL;
            pPipelineLayoutCreateInfo.setLayoutCount = (uint32_t)descriptorSetLayout.size();
            pPipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayout.data();

            VkResult res = vkCreatePipelineLayout(m_pDevice->GetDevice(), &pPipelineLayoutCreateInfo, NULL, &pPrimitive->m_pipelineLayout);
            assert(res == VK_SUCCESS);
            SetResourceName(m_pDevice->GetDevice(), VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t)pPrimitive->m_pipelineLayout, "GltfDepthPass PL");
        }

        /////////////////////////////////////////////
        // Compile and create shaders

        VkPipelineShaderStageCreateInfo vertexShader, fragmentShader = {};
        {
            VKCompileFromFile(m_pDevice->GetDevice(), VK_SHADER_STAGE_VERTEX_BIT, "GLTFMotionVectorsPass-vert.glsl", "main", "", &defines, &vertexShader);
            VKCompileFromFile(m_pDevice->GetDevice(), VK_SHADER_STAGE_FRAGMENT_BIT, "GLTFMotionVectorsPass-frag.glsl", "main", "", &defines, &fragmentShader);
        }
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages = { vertexShader, fragmentShader };


        /////////////////////////////////////////////
        // Create a Pipeline 
        {
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
            rs.cullMode = pPrimitive->m_pMaterial->m_doubleSided ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;;
            rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
            rs.depthClampEnable = VK_FALSE;
            rs.rasterizerDiscardEnable = VK_FALSE;
            rs.depthBiasEnable = VK_FALSE;
            rs.depthBiasConstantFactor = 0;
            rs.depthBiasClamp = 0;
            rs.depthBiasSlopeFactor = 0;
            rs.lineWidth = 1.0f;

            assert(attachmentCount <= 2); // Only support up to 2 additional color attachments (motion vectors & normals)
            VkPipelineColorBlendAttachmentState att_state[2];
            for (int i = 0; i < attachmentCount; ++i)
            {
                att_state[i].colorWriteMask = 0xf;
                att_state[i].blendEnable = VK_FALSE;
                att_state[i].alphaBlendOp = VK_BLEND_OP_ADD;
                att_state[i].colorBlendOp = VK_BLEND_OP_ADD;
                att_state[i].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
                att_state[i].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                att_state[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                att_state[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            }

            // Color blend state

            VkPipelineColorBlendStateCreateInfo cb;
            cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            cb.flags = 0;
            cb.pNext = NULL;
            cb.attachmentCount = attachmentCount; //set to 1 when transparency
            cb.pAttachments = att_state;
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
            ds.depthWriteEnable = true;
            ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
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
            ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
            ms.sampleShadingEnable = VK_FALSE;
            ms.alphaToCoverageEnable = VK_FALSE;
            ms.alphaToOneEnable = VK_FALSE;
            ms.minSampleShading = 0.0;

            // create pipeline 

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
            pipeline.renderPass = renderPass;
            pipeline.subpass = 0;

            VkResult res = vkCreateGraphicsPipelines(m_pDevice->GetDevice(), m_pDevice->GetPipelineCache(), 1, &pipeline, NULL, &pPrimitive->m_pipeline);
            assert(res == VK_SUCCESS);
            SetResourceName(m_pDevice->GetDevice(), VK_OBJECT_TYPE_PIPELINE, (uint64_t)pPrimitive->m_pipeline, "GltfDepthPass P");

        }
    }

    //--------------------------------------------------------------------------------------
    //
    // SetPerFrameConstants
    //
    //--------------------------------------------------------------------------------------
    GltfMotionVectorsPass::per_frame *GltfMotionVectorsPass::SetPerFrameConstants()
    {
        GltfMotionVectorsPass::per_frame *cbPerFrame;
        m_pDynamicBufferRing->AllocConstantBuffer(sizeof(GltfMotionVectorsPass::per_frame), (void **)&cbPerFrame, &m_perFrameDesc);

        return cbPerFrame;
    }

    //--------------------------------------------------------------------------------------
    //
    // Draw
    //
    //--------------------------------------------------------------------------------------
    void GltfMotionVectorsPass::Draw(VkCommandBuffer cmd_buf)
    {
        SetPerfMarkerBegin(cmd_buf, "MotionVectorPass");

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

            MotionVectorMesh *pMesh = &m_meshes[pNode->meshIndex];
            for (int p = 0; p < pMesh->m_pPrimitives.size(); p++)
            {
                MotionVectorPrimitives *pPrimitive = &pMesh->m_pPrimitives[p];

                if (pPrimitive->m_pipeline == VK_NULL_HANDLE)
                    continue;

                // Set per Object constants
                //
                per_object cbPerObject;
                cbPerObject.mCurrentWorld = pNodesMatrices[i].GetCurrent();
                cbPerObject.mPreviousWorld = pNodesMatrices[i].GetPrevious();

                VkDescriptorBufferInfo perObjectDesc = m_pDynamicBufferRing->AllocConstantBuffer(sizeof(per_object), &cbPerObject);

                // Bind indices and vertices using the right offsets into the buffer
                //
                Geometry *pGeometry = &pPrimitive->m_geometry;
                for (uint32_t i = 0; i < pGeometry->m_VBV.size(); i++)
                {
                    vkCmdBindVertexBuffers(cmd_buf, i, 1, &pGeometry->m_VBV[i].buffer, &pGeometry->m_VBV[i].offset);
                }

                vkCmdBindIndexBuffer(cmd_buf, pGeometry->m_IBV.buffer, pGeometry->m_IBV.offset, pGeometry->m_indexType);

                // Bind Descriptor sets
                //
                VkDescriptorSet descriptorSets[2] = { pPrimitive->m_descriptorSet, pPrimitive->m_pMaterial->m_descriptorSet };
                uint32_t descritorSetCount = 1 + (pPrimitive->m_pMaterial->m_textureCount > 0 ? 1 : 0);

                uint32_t uniformOffsets[3] = { (uint32_t)m_perFrameDesc.offset,  (uint32_t)perObjectDesc.offset, (pPerSkeleton) ? (uint32_t)pPerSkeleton->offset : 0 };
                uint32_t uniformOffsetsCount = (pPerSkeleton) ? 3 : 2;

                vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pPrimitive->m_pipelineLayout, 0, descritorSetCount, descriptorSets, uniformOffsetsCount, uniformOffsets);

                // Bind Pipeline
                //
                vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pPrimitive->m_pipeline);

                // Draw
                //
                vkCmdDrawIndexed(cmd_buf, pGeometry->m_NumIndices, 1, 0, 0, 0);
            }
        }

        SetPerfMarkerEnd(cmd_buf);
    }
}
