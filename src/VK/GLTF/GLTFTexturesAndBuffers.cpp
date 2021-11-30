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
#include "Misc/Misc.h"
#include "GltfHelpers.h"
#include "Base/UploadHeap.h"
#include "Misc/ThreadPool.h"
#include "GLTFTexturesAndBuffers.h"
#include "../common/GLTF/GltfPbrMaterial.h"

namespace CAULDRON_VK
{
    bool GLTFTexturesAndBuffers::OnCreate(Device* pDevice, GLTFCommon *pGLTFCommon, UploadHeap* pUploadHeap, StaticBufferPool *pStaticBufferPool, DynamicBufferRing *pDynamicBufferRing)
    {
        m_pDevice = pDevice;
        m_pGLTFCommon = pGLTFCommon;
        m_pUploadHeap = pUploadHeap;
        m_pStaticBufferPool = pStaticBufferPool;
        m_pDynamicBufferRing = pDynamicBufferRing;

        return true;
    }

    void GLTFTexturesAndBuffers::LoadTextures(AsyncPool *pAsyncPool)
    {
        // load textures and create views
        //
        if (m_pGLTFCommon->j3.find("images") != m_pGLTFCommon->j3.end())
        {
            m_pTextureNodes = &m_pGLTFCommon->j3["textures"];
            const json &images = m_pGLTFCommon->j3["images"];
            const json &materials = m_pGLTFCommon->j3["materials"];

            std::vector<Async *> taskQueue(images.size());
            for (int textureIndex = 0; textureIndex < m_pTextureNodes->size(); textureIndex++) {
                m_textureToImage[textureIndex] =   (*m_pTextureNodes)[textureIndex]["source"].get<int>();
            }
            m_textures.resize(images.size());
            m_textureViews.resize(images.size());
            for (int imageIndex = 0; imageIndex < images.size(); imageIndex++)
            {
                Texture *pTex = &m_textures[imageIndex];
                std::string filename = m_pGLTFCommon->m_path + images[imageIndex]["uri"].get<std::string>();

                ExecAsyncIfThereIsAPool(pAsyncPool, [imageIndex, pTex, this, filename, materials]()
                {
                    bool useSRGB;
                    float cutOff;
                    GetSrgbAndCutOffOfImageGivenItsUse(imageIndex, materials, m_textureToImage, &useSRGB, &cutOff);

                    bool result = pTex->InitFromFile(m_pDevice, m_pUploadHeap, filename.c_str(), useSRGB, 0 /*VkImageUsageFlags*/, cutOff);
                    assert(result != false);

                    m_textures[imageIndex].CreateSRV(&m_textureViews[imageIndex]);
                });
            }

            LoadGeometry();

            if (pAsyncPool)
                pAsyncPool->Flush();

            // copy textures and apply barriers, then flush the GPU
            m_pUploadHeap->FlushAndFinish();
        }        
    }

    void GLTFTexturesAndBuffers::LoadGeometry()
    {
        if (m_pGLTFCommon->j3.find("meshes") != m_pGLTFCommon->j3.end())
        {
            for (const json &mesh : m_pGLTFCommon->j3["meshes"])
            {
                for (const json &primitive : mesh["primitives"])
                {
                    //
                    //  Load vertex buffers
                    //
                    for (const json &attributeId : primitive["attributes"])
                    {
                        tfAccessor vertexBufferAcc;
                        m_pGLTFCommon->GetBufferDetails(attributeId, &vertexBufferAcc);

                        VkDescriptorBufferInfo vbv;
                        m_pStaticBufferPool->AllocBuffer(vertexBufferAcc.m_count, vertexBufferAcc.m_stride, vertexBufferAcc.m_data, &vbv);

                        m_vertexBufferMap[attributeId] = vbv;
                    }

                    //
                    //  Load index buffers
                    //
                    int indexAcc = primitive.value("indices", -1);
                    if (indexAcc >= 0)
                    {
                        tfAccessor indexBufferAcc;
                        m_pGLTFCommon->GetBufferDetails(indexAcc, &indexBufferAcc);

                        VkDescriptorBufferInfo ibv;

                        // Some exporters use 1-byte indices, need to convert them to shorts since the GPU doesn't support 1-byte indices
                        if (indexBufferAcc.m_stride == 1)
                        {
                            unsigned short *pIndices = (unsigned short *)malloc(indexBufferAcc.m_count * (2 * indexBufferAcc.m_stride));
                            for (int i = 0; i < indexBufferAcc.m_count; i++)
                                pIndices[i] = ((unsigned char *)indexBufferAcc.m_data)[i];
                            m_pStaticBufferPool->AllocBuffer(indexBufferAcc.m_count, 2 * indexBufferAcc.m_stride, pIndices, &ibv);
                            free(pIndices);
                        }
                        else
                        {
                            m_pStaticBufferPool->AllocBuffer(indexBufferAcc.m_count, indexBufferAcc.m_stride, indexBufferAcc.m_data, &ibv);
                        }

                        m_IndexBufferMap[indexAcc] = ibv;
                    }
                }
            }
        }
    }

    void GLTFTexturesAndBuffers::OnDestroy()
    {
        for (int i = 0; i < m_textures.size(); i++)
        {
            vkDestroyImageView(m_pDevice->GetDevice(), m_textureViews[i], NULL);
            m_textures[i].OnDestroy();
        }
    }

    VkImageView GLTFTexturesAndBuffers::GetTextureViewByID(int id)
    {
        int tex = m_pTextureNodes->at(id)["source"];
        return m_textureViews[tex];
    }

    // Creates a Index Buffer from the accessor
    //
    //
    void GLTFTexturesAndBuffers::CreateIndexBuffer(int indexBufferId, uint32_t *pNumIndices, VkIndexType *pIndexType, VkDescriptorBufferInfo *pIBV)
    {
        tfAccessor indexBuffer;
        m_pGLTFCommon->GetBufferDetails(indexBufferId, &indexBuffer);

        *pNumIndices = indexBuffer.m_count;
        *pIndexType = (indexBuffer.m_stride == 4) ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;

        *pIBV = m_IndexBufferMap[indexBufferId];
    }

    // Creates Vertex Buffers from accessors and sets them in the Primitive struct.
    //
    //
    void GLTFTexturesAndBuffers::CreateGeometry(int indexBufferId, std::vector<int> &vertexBufferIds, Geometry *pGeometry)
    {
        CreateIndexBuffer(indexBufferId, &pGeometry->m_NumIndices, &pGeometry->m_indexType, &pGeometry->m_IBV);

        // load the rest of the buffers onto the GPU
        pGeometry->m_VBV.resize(vertexBufferIds.size());
        for (int i = 0; i < vertexBufferIds.size(); i++)
        {
            pGeometry->m_VBV[i] = m_vertexBufferMap[vertexBufferIds[i]];
        }
    }

    // Creates buffers and the input assemby at the same time. It needs a list of attributes to use.
    //
    void GLTFTexturesAndBuffers::CreateGeometry(const json &primitive, const std::vector<std::string> requiredAttributes, std::vector<VkVertexInputAttributeDescription> &layout, DefineList &defines, Geometry *pGeometry)
    {
        // Get Index buffer view
        //
        tfAccessor indexBuffer;
        int indexBufferId = primitive.value("indices", -1);
        CreateIndexBuffer(indexBufferId, &pGeometry->m_NumIndices, &pGeometry->m_indexType, &pGeometry->m_IBV);

        // Create vertex buffers and input layout
        //
        int cnt = 0;
        layout.resize(requiredAttributes.size());
        pGeometry->m_VBV.resize(requiredAttributes.size());
        const json &attributes = primitive.at("attributes");
        for (auto attrName : requiredAttributes)
        {
            // get vertex buffer view
            // 
            const int attr = attributes.find(attrName).value();
            pGeometry->m_VBV[cnt] = m_vertexBufferMap[attr];

            // let the compiler know we have this stream
            defines[std::string("ID_") + attrName] = std::to_string(cnt);

            const json &inAccessor = m_pGLTFCommon->m_pAccessors->at(attr);

            // Create Input Layout
            //
            VkVertexInputAttributeDescription l = {};
            l.location = (uint32_t)cnt;
            l.format = GetFormat(inAccessor["type"], inAccessor["componentType"]);
            l.offset = 0;
            l.binding = cnt;
            layout[cnt]=l;

            cnt++;
        }
    }

    void GLTFTexturesAndBuffers::SetPerFrameConstants()
    {
        per_frame *cbPerFrame;
        m_pDynamicBufferRing->AllocConstantBuffer(sizeof(per_frame), (void **)&cbPerFrame, &m_perFrameConstants);
        *cbPerFrame = m_pGLTFCommon->m_perFrameData;
    }

    void GLTFTexturesAndBuffers::SetSkinningMatricesForSkeletons()
    {
        for (auto &t : m_pGLTFCommon->m_worldSpaceSkeletonMats)
        {
            std::vector<Matrix2> *matrices = &t.second;

            VkDescriptorBufferInfo perSkeleton = {};
            Matrix2 *cbPerSkeleton;
            uint32_t size = (uint32_t)(matrices->size() * sizeof(Matrix2));
            m_pDynamicBufferRing->AllocConstantBuffer(size, (void **)&cbPerSkeleton, &perSkeleton);
            memcpy(cbPerSkeleton, matrices->data(), size);
            m_skeletonMatricesBuffer[t.first] = perSkeleton;
        }
    }

    VkDescriptorBufferInfo *GLTFTexturesAndBuffers::GetSkinningMatricesBuffer(int skinIndex)
    {
        auto it = m_skeletonMatricesBuffer.find(skinIndex);

        if (it == m_skeletonMatricesBuffer.end())
            return NULL;

        return &it->second;
    }
}
