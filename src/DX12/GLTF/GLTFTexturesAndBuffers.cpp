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

#include "stdafx.h"
#include "Misc/Misc.h"
#include "GltfHelpers.h"
#include "Base/ShaderCompiler.h"
#include "Misc/ThreadPool.h"
#include "GLTFTexturesAndBuffers.h"
#include "../common/GLTF/GltfPbrMaterial.h"

class DefineList;

namespace CAULDRON_DX12
{
    bool GLTFTexturesAndBuffers::OnCreate(Device *pDevice, GLTFCommon *pGLTFCommon, UploadHeap* pUploadHeap, StaticBufferPool *pStaticBufferPool, DynamicBufferRing *pDynamicBufferRing)
    {
        m_pDevice = pDevice;
        m_pGLTFCommon = pGLTFCommon;
        m_pUploadHeap = pUploadHeap;
        m_pStaticBufferPool = pStaticBufferPool;
        m_pDynamicBufferRing = pDynamicBufferRing;

        return true;
    }

    void GLTFTexturesAndBuffers::LoadTextures()
    {
        // load textures 
        //
        if (m_pGLTFCommon->j3.find("images") != m_pGLTFCommon->j3.end())
        {
            m_pTextureNodes = &m_pGLTFCommon->j3["textures"];
            const json &images = m_pGLTFCommon->j3["images"];
            const json &materials = m_pGLTFCommon->j3["materials"];

            m_textures.resize(images.size());
            for (int imageIndex = 0; imageIndex < images.size(); imageIndex++)
            {
                bool useSRGB;
                float cutOff;
                GetSrgbAndCutOffOfImageGivenItsUse(imageIndex, materials, &useSRGB, &cutOff);

                std::string filename = images[imageIndex]["uri"];
                bool result = m_textures[imageIndex].InitFromFile(m_pDevice, m_pUploadHeap, (m_pGLTFCommon->m_path + filename).c_str(), useSRGB, cutOff);
                assert(result != false);
            }
            m_pUploadHeap->FlushAndFinish();
        }
    }

    void GLTFTexturesAndBuffers::OnDestroy()
    {
        for (int i = 0; i < m_textures.size(); i++)
        {
            m_textures[i].OnDestroy();
        }
    }

    Texture *GLTFTexturesAndBuffers::GetTextureViewByID(int id)
    {
        int tex = m_pTextureNodes->at(id)["source"];
        return &m_textures[tex];
    }

    // Creates a Index Buffer from the accessor
    //
    //
    void GLTFTexturesAndBuffers::CreateIndexBuffer(tfAccessor indexBuffer, uint32_t *pNumIndices, DXGI_FORMAT *pIndexType, D3D12_INDEX_BUFFER_VIEW *pIBV)
    {
        *pNumIndices = indexBuffer.m_count;
        *pIndexType = (indexBuffer.m_stride == 4) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;

        // Some exporters use 1-byte indices, need to convert them to shorts
        if (indexBuffer.m_stride == 1)
        {
            unsigned short *pIndices = (unsigned short *)malloc(indexBuffer.m_count * (2 * indexBuffer.m_stride));
            for (int i = 0; i < indexBuffer.m_count; i++)
                pIndices[i] = ((unsigned char *)indexBuffer.m_data)[i];
            m_pStaticBufferPool->AllocIndexBuffer(indexBuffer.m_count, 2 * indexBuffer.m_stride, pIndices, pIBV);
            free(pIndices);
        }
        else
        {
            m_pStaticBufferPool->AllocIndexBuffer(indexBuffer.m_count, indexBuffer.m_stride, indexBuffer.m_data, pIBV);
        }
    }

    // Creates Vertex Buffers from accessors and sets them in the Primitive struct.
    //
    //
    void GLTFTexturesAndBuffers::CreateGeometry(tfAccessor indexBuffer, std::vector<tfAccessor> &vertexBuffers, Geometry *pGeometry)
    {
        CreateIndexBuffer(indexBuffer, &pGeometry->m_NumIndices, &pGeometry->m_indexType, &pGeometry->m_IBV);

        // load the rest of the buffers onto the GPU
        pGeometry->m_VBV.resize(vertexBuffers.size());
        for (int i = 0; i < vertexBuffers.size(); i++)
        {
            tfAccessor *pVertexAccessor = &vertexBuffers[i];
            m_pStaticBufferPool->AllocVertexBuffer(pVertexAccessor->m_count, pVertexAccessor->m_stride, pVertexAccessor->m_data, &pGeometry->m_VBV[i]);
        }
    }

    // Creates buffers and the input assemby at the same time. It needs a list of attributes to use.
    //
    void GLTFTexturesAndBuffers::CreateGeometry(const json &primitive, const std::vector<std::string> requiredAttributes, std::vector<std::string> &semanticNames, std::vector<D3D12_INPUT_ELEMENT_DESC> &layout, DefineList &defines, Geometry *pGeometry)
    {
        // Get Index Buffer accessor
        //
        tfAccessor indexBuffer;
        int indexAcc = primitive.value("indices", -1);
        m_pGLTFCommon->GetBufferDetails(indexAcc, &indexBuffer);
        CreateIndexBuffer(indexBuffer, &pGeometry->m_NumIndices, &pGeometry->m_indexType, &pGeometry->m_IBV);

        // Create vertex buffers and input layout
        //
        int cnt = 0;
        layout.resize(requiredAttributes.size());
        semanticNames.resize(requiredAttributes.size());
        pGeometry->m_VBV.resize(requiredAttributes.size());
        const json &attributes = primitive.at("attributes");
        for (auto attrName : requiredAttributes)
        {
            // get vertex buffer into static pool
            // 
            tfAccessor acc;
            const int attr = attributes.find(attrName).value();
            m_pGLTFCommon->GetBufferDetails(attr, &acc);
            m_pStaticBufferPool->AllocVertexBuffer(acc.m_count, acc.m_stride, acc.m_data, &pGeometry->m_VBV[cnt]);

            // Set define so the shaders knows this stream is available
            //
            defines[std::string("HAS_") + attrName] = std::string("1");

            // split semantic name from index, DX doesnt like the trailing number
            uint32_t semanticIndex = 0;
            SplitGltfAttribute(attrName, &semanticNames[cnt], &semanticIndex);

            const json &inAccessor = m_pGLTFCommon->m_pAccessors->at(attr);

            // Create Input Layout
            //
            D3D12_INPUT_ELEMENT_DESC l = {};
            l.SemanticName = semanticNames[cnt].c_str(); // we need to set it in the pipeline function (because of multithreading)
            l.SemanticIndex = semanticIndex;
            l.Format = GetFormat(inAccessor["type"], inAccessor["componentType"]);
            l.InputSlot = (UINT)cnt;
            l.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            l.InstanceDataStepRate = 0;
            l.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
            layout[cnt] = l;

            cnt++;            
        }
    }

    void GLTFTexturesAndBuffers::SetPerFrameConstants()
    {
        m_perFrameConstants = m_pDynamicBufferRing->AllocConstantBuffer(sizeof(per_frame), &m_pGLTFCommon->m_perFrameData);
    }

    void GLTFTexturesAndBuffers::SetSkinningMatricesForSkeletons()
    {
        for (auto &t : m_pGLTFCommon->m_pCurrentFrameTransformedData->m_worldSpaceSkeletonMats)
        {
            std::vector<XMMATRIX> *matrices = &t.second;
            D3D12_GPU_VIRTUAL_ADDRESS perSkeleton = m_pDynamicBufferRing->AllocConstantBuffer((uint32_t)(matrices->size() * sizeof(XMMATRIX)), matrices->data());

            m_skeletonMatricesBuffer[t.first] = perSkeleton;
        }
    }

    D3D12_GPU_VIRTUAL_ADDRESS GLTFTexturesAndBuffers::GetSkinningMatricesBuffer(int skinIndex)
    {
        auto it = m_skeletonMatricesBuffer.find(skinIndex);

        if (it == m_skeletonMatricesBuffer.end())
            return NULL;

        return it->second;
    }
}
