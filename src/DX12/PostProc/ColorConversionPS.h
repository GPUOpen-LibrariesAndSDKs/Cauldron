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

#include "PostProcPS.h"

namespace CAULDRON_DX12
{
    class ColorConversionPS
    {
    public:
        void OnCreate(Device* pDevice, ResourceViewHeaps *pResourceViewHeaps, DynamicBufferRing *pDynamicBufferRing, StaticBufferPool  *pStaticBufferPool, DXGI_FORMAT outFormat);
        void OnDestroy();
        void UpdatePipelines(DXGI_FORMAT outFormat, DisplayMode displayMode);
        void Draw(ID3D12GraphicsCommandList* pCommandList, CBV_SRV_UAV *pHDRSRV, uint32_t isLPMToneMapperSelected = 0);

    private:
        PostProcPS m_ColorConversion;

        DynamicBufferRing *m_pDynamicBufferRing = NULL;

        struct ColorConversionConsts
        {
            math::Matrix4 m_contentToMonitorRecMatrix;
            DisplayMode m_displayMode;
            float m_displayMinLuminancePerNits;
            float m_displayMaxLuminancePerNits;
            uint32_t m_isLPMToneMapperSelected;
        };

        ColorConversionConsts m_colorConversionConsts;
    };
}
