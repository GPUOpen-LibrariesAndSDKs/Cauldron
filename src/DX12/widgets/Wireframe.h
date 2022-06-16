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

#include "../Base/Device.h"
#include "../Base/DynamicBufferRing.h"
#include "../Base/StaticBufferPool.h"
#include "../Base/ResourceViewHeaps.h"
#include "../Base/UploadHeap.h"
#include "../../common/Misc/Camera.h"

namespace CAULDRON_DX12
{
    class Wireframe
    {
    public:
        void OnCreate(
            Device* pDevice,
            ResourceViewHeaps *pHeaps,
            DynamicBufferRing *pDynamicBufferRing,
            StaticBufferPool *pStaticBufferPool,
            DXGI_FORMAT outFormat,
            uint32_t sampleDescCount,
            bool bInvertedDepth = false);

        void OnDestroy();
        void Draw(ID3D12GraphicsCommandList* pCommandList, int numIndices, D3D12_INDEX_BUFFER_VIEW IBV, D3D12_VERTEX_BUFFER_VIEW VBV, const math::Matrix4& WorldViewProj, const math::Vector4& vCenter, const math::Vector4& vRadius, const math::Vector4& vColor);

    private:
        DynamicBufferRing *m_pDynamicBufferRing;
        ResourceViewHeaps *m_pResourceViewHeaps;

        ID3D12PipelineState*	m_pPipeline;
        ID3D12RootSignature*	m_RootSignature;

        struct per_object
        {
            math::Matrix4 m_mWorldViewProj;
            math::Vector4 m_vCenter;
            math::Vector4 m_vRadius;
            math::Vector4 m_vColor;
        };
    };
}
