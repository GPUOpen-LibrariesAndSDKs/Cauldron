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
#pragma once

#include "ResourceViewHeaps.h"
#include "UploadHeap.h"
#include "Misc\ImgLoader.h"

namespace CAULDRON_DX12
{
    // This class provides functionality to create a 2D-texture from a DDS or any texture format from WIC file.
    class Texture
    {
    public:
        Texture();
        virtual                 ~Texture();
        virtual void            OnDestroy();

        // load file into heap
        virtual bool InitFromFile(Device* pDevice, UploadHeap* pUploadHeap, const char *szFilename, bool useSRGB = false, float cutOff = 1.0f);
        INT32 InitRenderTarget(Device* pDevice, const char *pDebugName, const CD3DX12_RESOURCE_DESC *pDesc, D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_RENDER_TARGET);
        INT32 InitDepthStencil(Device* pDevice, const char *pDebugName, const CD3DX12_RESOURCE_DESC *pDesc);
        bool InitBuffer(Device* pDevice, const char *pDebugName, const CD3DX12_RESOURCE_DESC *pDesc, uint32_t structureSize, D3D12_RESOURCE_STATES state);     // structureSize needs to be 0 if using a valid DXGI_FORMAT
        bool InitCounter(Device* pDevice, const char *pDebugName, const CD3DX12_RESOURCE_DESC *pCounterDesc, uint32_t counterSize, D3D12_RESOURCE_STATES state);
        bool InitFromData(Device* pDevice, const char *pDebugName, UploadHeap& uploadHeap, const IMG_INFO& header, const void* data);

        ID3D12Resource* GetResource() { return m_pResource; }

        void CreateRTV(uint32_t index, RTV *pRV, int mipLevel = -1);
        void CreateSRV(uint32_t index, CBV_SRV_UAV *pRV, int mipLevel = -1);
        void CreateDSV(uint32_t index, DSV *pRV, int arraySlice = 1);
        void CreateUAV(uint32_t index, CBV_SRV_UAV *pRV);
        void CreateBufferUAV(uint32_t index, Texture *pCounterTex, CBV_SRV_UAV *pRV);  // if counter=true, then UAV is in index, counter is in index+1
        void CreateCubeSRV(uint32_t index, CBV_SRV_UAV *pRV);

        uint32_t GetWidth() { return m_header.width; }
        uint32_t GetHeight() { return m_header.height; }
        uint32_t GetMipCount() { return m_header.mipMapCount; }

        static std::map<std::string, Texture *> m_cache;

    protected:
        void CreateTextureCommitted(Device* pDevice, const char *pDebugName, bool useSRGB = false);
        void LoadAndUpload(Device* pDevice, UploadHeap* pUploadHeap, ImgLoader *pDds, ID3D12Resource *pRes);

        ID3D12Resource*         m_pResource = nullptr;

        IMG_INFO                m_header = {};
        uint32_t                m_structuredBufferStride = 0;

        void                    PatchFmt24To32Bit(unsigned char *pDst, unsigned char *pSrc, UINT32 pixelCount);
        bool                    isDXT(DXGI_FORMAT format)const;
        UINT32                  GetPixelSize(DXGI_FORMAT fmt) const;
        bool                    isCubemap()const;
    };
}