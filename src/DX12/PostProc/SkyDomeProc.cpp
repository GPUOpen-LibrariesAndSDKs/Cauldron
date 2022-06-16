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
#include "../Base/DynamicBufferRing.h"
#include "../Base/StaticBufferPool.h"
#include "../Base/UploadHeap.h"
#include "../Base/Texture.h"
#include "../Base/Helper.h"
#include "PostProcPS.h"
#include "SkyDomeProc.h"

namespace CAULDRON_DX12
{
    void SkyDomeProc::OnCreate(
        Device* pDevice,
        ResourceViewHeaps *pResourceViewHeaps,
        DynamicBufferRing *pDynamicBufferRing,
        StaticBufferPool *pStaticBufferPool,
        DXGI_FORMAT outFormat,
        uint32_t sampleDescCount,
        bool bInvertedDepth)
    {
        m_pDevice = pDevice;
        m_pDynamicBufferRing = pDynamicBufferRing;
        m_pResourceViewHeaps = pResourceViewHeaps;

        D3D12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        D3D12_DEPTH_STENCIL_DESC depthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_EQUAL;
        m_skydome.OnCreate(pDevice, "SkyDomeProc.hlsl", pResourceViewHeaps, pStaticBufferPool, 0, 0, NULL, outFormat, sampleDescCount, &blendDesc, &depthStencilDesc, 1, "-T vs_6_0", "-T ps_6_0", bInvertedDepth);
    }

    void SkyDomeProc::OnDestroy()
    {
        m_skydome.OnDestroy();
    }

    void SkyDomeProc::Draw(ID3D12GraphicsCommandList* pCommandList, SkyDomeProc::Constants constants)
    {
        UserMarker marker(pCommandList, "Skydome Proc");

        D3D12_GPU_VIRTUAL_ADDRESS constantBuffer = m_pDynamicBufferRing->AllocConstantBuffer(sizeof(SkyDomeProc::Constants), &constants);

        m_skydome.Draw(pCommandList, 0, NULL, constantBuffer);
    }
}