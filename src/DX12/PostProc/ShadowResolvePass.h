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

namespace CAULDRON_DX12
{
    class PostProcCS;
    class GLTFTexturesAndBuffers;

    class ShadowResolveFrame
    {
    public:
        uint32_t m_Width;
        uint32_t m_Height;
        CBV_SRV_UAV m_ShadowMapSRV;
        CBV_SRV_UAV m_DepthBufferSRV;
        CBV_SRV_UAV m_ShadowBufferUAV;
    };

    class ShadowResolvePass
    {
        const int s_TileSize = 16;

    public:
        void OnCreate(
            Device *pDevice,
            ResourceViewHeaps *pResourceViewHeaps,
            DynamicBufferRing *pDynamicBufferRing);
        void OnDestroy();

        void Draw(ID3D12GraphicsCommandList *pCommandList, GLTFTexturesAndBuffers *pGLTFTexturesAndBuffers, ShadowResolveFrame *pShadowResolveFrame);

    protected:
        ResourceViewHeaps *m_pResourceViewHeaps;
        DynamicBufferRing *m_pDynamicBufferRing;

        ID3D12RootSignature *m_pRootSignature;
        ID3D12PipelineState *m_pPipelineState;
    };
}
