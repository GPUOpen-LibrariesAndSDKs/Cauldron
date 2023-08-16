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
#include "Misc/ColorConversion.h"
#include "Base/FreesyncHDR.h"
#include "Base/DynamicBufferRing.h"
#include "Base/ShaderCompiler.h"
#include "Base/UploadHeap.h"
#include "ToneMappingCS.h"

namespace CAULDRON_DX12
{
    void ToneMappingCS::OnCreate(Device* pDevice, ResourceViewHeaps *pResourceViewHeaps, DynamicBufferRing *pDynamicBufferRing)
    {
        m_pDynamicBufferRing = pDynamicBufferRing;
        m_toneMapping.OnCreate(pDevice, pResourceViewHeaps, "TonemappingCS.hlsl", "main", 1, 0, 8, 8, 1);
    }

    void ToneMappingCS::OnDestroy()
    {
        m_toneMapping.OnDestroy();
    }

    void ToneMappingCS::Draw(ID3D12GraphicsCommandList* pCommandList, CBV_SRV_UAV *pHDRSRV, float exposure, int toneMapper, int width, int height)
    {
        UserMarker marker(pCommandList, "Tonemapping");

        D3D12_GPU_VIRTUAL_ADDRESS cbTonemappingHandle;
        ToneMappingConsts *pToneMapping;
        m_pDynamicBufferRing->AllocConstantBuffer(sizeof(ToneMappingConsts), (void **)&pToneMapping, &cbTonemappingHandle);
        pToneMapping->exposure = exposure;
        pToneMapping->toneMapper = toneMapper;

        const LPMOutputParams lpmOutputParams = GetLPMParameters();

        pToneMapping->lpmConsts.shoulder = lpmOutputParams.shoulder;
        pToneMapping->lpmConsts.con      = lpmOutputParams.lpmConfig.con;
        pToneMapping->lpmConsts.soft     = lpmOutputParams.lpmConfig.soft;
        pToneMapping->lpmConsts.con2     = lpmOutputParams.lpmConfig.con2;
        pToneMapping->lpmConsts.clip     = lpmOutputParams.lpmConfig.clip;
        pToneMapping->lpmConsts.scaleOnly = lpmOutputParams.lpmConfig.scaleOnly;
        pToneMapping->lpmConsts.displayMode = lpmOutputParams.displayMode;
        pToneMapping->lpmConsts.inputToOutputMatrix = lpmOutputParams.inputToOutputMatrix;
        memcpy(pToneMapping->lpmConsts.ctl, lpmOutputParams.ctl, sizeof(lpmOutputParams.ctl));

        m_toneMapping.Draw(pCommandList, cbTonemappingHandle, pHDRSRV, NULL, (width + 7) / 8, (height + 7) / 8, 1);
    }
}
