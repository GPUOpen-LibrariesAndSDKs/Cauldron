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
#include "Base/Texture.h"
#include "Base/StaticBufferPool.h"
#include "Base/GBuffer.h"

namespace CAULDRON_DX12
{
    class TAA
    {
    public:
        void OnCreate(Device *pDevice, ResourceViewHeaps *pResourceViewHeaps, StaticBufferPool *pStaticBufferPool, bool sharpening = true);
        void OnDestroy();
        void OnCreateWindowSizeDependentResources(uint32_t Width, uint32_t Height, GBuffer *pGBuffer);
        void OnDestroyWindowSizeDependentResources();
        
        void Draw(ID3D12GraphicsCommandList *pCommandList, D3D12_RESOURCE_STATES hdrBefore, D3D12_RESOURCE_STATES hdrAfter);

    private:
        Device                         *m_pDevice;
        ResourceViewHeaps              *m_pResourceViewHeaps;

        uint32_t                        m_Width, m_Height;

        GBuffer                        *m_pGBuffer;

        // TAA buffer
        Texture                         m_TAABuffer;
        Texture                         m_HistoryBuffer;

        CBV_SRV_UAV                     m_TaaTable;
        CBV_SRV_UAV                     m_SharpenerTable;

        ID3D12RootSignature            *m_pTaaRootSignature;
        ID3D12PipelineState            *m_pTaaPipelineState;
        ID3D12PipelineState            *m_pFirstPipelineState;
        
        ID3D12RootSignature            *m_pSharpenerRootSignature;
        ID3D12PipelineState            *m_pSharpenerPipelineState;
        ID3D12PipelineState            *m_pPostPipelineState;
        bool                           m_bSharpening = true;
        bool                           m_bFirst = true;
    };
}

