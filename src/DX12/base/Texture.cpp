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
#include "dxgi.h"
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include "..\..\common\Misc\Misc.h"
#include "Helper.h"
#include "Texture.h"

namespace CAULDRON_DX12
{
    DXGI_FORMAT Translate(DXGI_FORMAT format, bool useSRGB);

    //--------------------------------------------------------------------------------------
    // Constructor of the Texture class
    // initializes all members
    //--------------------------------------------------------------------------------------
    Texture::Texture() {}

    //--------------------------------------------------------------------------------------
    // Destructor of the Texture class
    //--------------------------------------------------------------------------------------
    Texture::~Texture() {}


    void Texture::OnDestroy()
    {
        if (m_pResource)
        {
            m_pResource->Release();
            m_pResource = nullptr;
        }
    }

    bool Texture::isDXT(DXGI_FORMAT format) const
    {
        return (format >= DXGI_FORMAT_BC1_TYPELESS) && (format <= DXGI_FORMAT_BC5_SNORM);
    }

    //--------------------------------------------------------------------------------------
    // return the byte size of a pixel (or block if block compressed)
    //--------------------------------------------------------------------------------------
    UINT32 Texture::GetPixelSize(DXGI_FORMAT fmt) const
    {
        switch (fmt)
        {
        case(DXGI_FORMAT_BC1_TYPELESS):
        case(DXGI_FORMAT_BC1_UNORM):
        case(DXGI_FORMAT_BC1_UNORM_SRGB):
        case(DXGI_FORMAT_BC4_TYPELESS):
        case(DXGI_FORMAT_BC4_UNORM):
        case(DXGI_FORMAT_BC4_SNORM):
        case(DXGI_FORMAT_R16G16B16A16_FLOAT):
        case(DXGI_FORMAT_R16G16B16A16_TYPELESS):
            return 8;

        case(DXGI_FORMAT_BC2_TYPELESS):
        case(DXGI_FORMAT_BC2_UNORM):
        case(DXGI_FORMAT_BC2_UNORM_SRGB):
        case(DXGI_FORMAT_BC3_TYPELESS):
        case(DXGI_FORMAT_BC3_UNORM):
        case(DXGI_FORMAT_BC3_UNORM_SRGB):
        case(DXGI_FORMAT_BC5_TYPELESS):
        case(DXGI_FORMAT_BC5_UNORM):
        case(DXGI_FORMAT_BC5_SNORM):
        case(DXGI_FORMAT_BC6H_TYPELESS):
        case(DXGI_FORMAT_BC6H_UF16):
        case(DXGI_FORMAT_BC6H_SF16):
        case(DXGI_FORMAT_BC7_TYPELESS):
        case(DXGI_FORMAT_BC7_UNORM):
        case(DXGI_FORMAT_BC7_UNORM_SRGB):
        case(DXGI_FORMAT_R32G32B32A32_FLOAT):
        case(DXGI_FORMAT_R32G32B32A32_TYPELESS):
            return 16;

        case(DXGI_FORMAT_R10G10B10A2_TYPELESS):
        case(DXGI_FORMAT_R10G10B10A2_UNORM):
        case(DXGI_FORMAT_R10G10B10A2_UINT):
        case(DXGI_FORMAT_R11G11B10_FLOAT):
        case(DXGI_FORMAT_R8G8B8A8_TYPELESS):
        case(DXGI_FORMAT_R8G8B8A8_UNORM):
        case(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB):
        case(DXGI_FORMAT_R8G8B8A8_UINT):
        case(DXGI_FORMAT_R8G8B8A8_SNORM):
        case(DXGI_FORMAT_R8G8B8A8_SINT):
        case(DXGI_FORMAT_B8G8R8A8_UNORM):
        case(DXGI_FORMAT_B8G8R8X8_UNORM):
        case(DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM):
        case(DXGI_FORMAT_B8G8R8A8_TYPELESS):
        case(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB):
        case(DXGI_FORMAT_B8G8R8X8_TYPELESS):
        case(DXGI_FORMAT_B8G8R8X8_UNORM_SRGB):
        case(DXGI_FORMAT_R16G16_TYPELESS):
        case(DXGI_FORMAT_R16G16_FLOAT):
        case(DXGI_FORMAT_R16G16_UNORM):
        case(DXGI_FORMAT_R16G16_UINT):
        case(DXGI_FORMAT_R16G16_SNORM):
        case(DXGI_FORMAT_R16G16_SINT):
        case(DXGI_FORMAT_R32_TYPELESS):
        case(DXGI_FORMAT_D32_FLOAT):
        case(DXGI_FORMAT_R32_FLOAT):
        case(DXGI_FORMAT_R32_UINT):
        case(DXGI_FORMAT_R32_SINT):
            return 4;

        default:
            assert(0);
            break;
        }
        return 0;
    }

    void Texture::PatchFmt24To32Bit(unsigned char *pDst, unsigned char *pSrc, UINT32 pixelCount)
    {
        // copy pixel data, interleave with A
        for (unsigned int i = 0; i < pixelCount; ++i)
        {
            pDst[0] = pSrc[0];
            pDst[1] = pSrc[1];
            pDst[2] = pSrc[2];
            pDst[3] = 0xFF;
            pDst += 4;
            pSrc += 3;
        }
    }

    bool Texture::isCubemap() const
    {
        return m_header.arraySize == 6;
    }

    INT32 Texture::InitRenderTarget(Device* pDevice, const char *pDebugName, const CD3DX12_RESOURCE_DESC *pDesc, D3D12_RESOURCE_STATES state)
    {
        // Performance tip: Tell the runtime at resource creation the desired clear value.
        D3D12_CLEAR_VALUE clearValue;
        clearValue.Format = pDesc->Format;
        clearValue.Color[0] = 0.0f;
        clearValue.Color[1] = 0.0f;
        clearValue.Color[2] = 0.0f;
        clearValue.Color[3] = 0.0f;

        bool isRenderTarget = pDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET ? 1 : 0;

        pDevice->GetDevice()->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            pDesc,
            state,
            isRenderTarget ? &clearValue : 0,
            IID_PPV_ARGS(&m_pResource));

        m_header.width = (UINT32)pDesc->Width;
        m_header.height = pDesc->Height;
        m_header.mipMapCount = pDesc->MipLevels;

        SetName(m_pResource, pDebugName);

        return 0;
    }

    bool Texture::InitBuffer(Device* pDevice, const char *pDebugName, const CD3DX12_RESOURCE_DESC *pDesc, uint32_t structureSize, D3D12_RESOURCE_STATES state)
    {
        assert(pDevice && pDesc);
        assert(pDesc->Dimension == D3D12_RESOURCE_DIMENSION_BUFFER && pDesc->Height == 1 && pDesc->MipLevels == 1);

        D3D12_RESOURCE_DESC desc = *pDesc;

        if (pDesc->Format != DXGI_FORMAT_UNKNOWN)
        {
            // Formatted buffer
            assert(structureSize == 0);
            m_structuredBufferStride = 0;
            m_header.width = (UINT32)pDesc->Width;
            m_header.format = pDesc->Format;

            // Fix up the D3D12_RESOURCE_DESC to be a typeless buffer.  The type/format will be associated with the UAV/SRV
            desc.Format = DXGI_FORMAT_UNKNOWN;
            desc.Width = GetPixelSize(m_header.format) * pDesc->Width;
        }
        else
        {
            // Structured buffer
            m_structuredBufferStride = structureSize;
            m_header.width = (UINT32)pDesc->Width / m_structuredBufferStride;
            m_header.format = DXGI_FORMAT_UNKNOWN;
        }

        m_header.height = 1;
        m_header.mipMapCount = 1;

        HRESULT hr = pDevice->GetDevice()->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &desc,
            state,
            nullptr,
            IID_PPV_ARGS(&m_pResource));

        SetName(m_pResource, pDebugName);

        return hr == S_OK;
    }

    bool Texture::InitCounter(Device* pDevice, const char *pDebugName, const CD3DX12_RESOURCE_DESC *pCounterDesc, uint32_t counterSize, D3D12_RESOURCE_STATES state)
    {
        return InitBuffer(pDevice, pDebugName, pCounterDesc, counterSize, state);
    }

    void Texture::CreateRTV(uint32_t index, RTV *pRV, int mipLevel)
    {
        ID3D12Device* pDevice;
        m_pResource->GetDevice(__uuidof(*pDevice), reinterpret_cast<void**>(&pDevice));
        D3D12_RESOURCE_DESC texDesc = m_pResource->GetDesc();

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = texDesc.Format;
        if (texDesc.SampleDesc.Count == 1)
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        else
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;

        if (mipLevel == -1)
        {
            rtvDesc.Texture2D.MipSlice = 0;
        }
        else
        {
            rtvDesc.Texture2D.MipSlice = mipLevel;
        }

        pDevice->CreateRenderTargetView(m_pResource, &rtvDesc, pRV->GetCPU(index));

        pDevice->Release();
    }


    bool Texture::InitFromData(Device* pDevice, const char *pDebugName, UploadHeap& uploadHeap, const IMG_INFO& header, const void* data)
    {
        assert(!m_pResource);
        assert(header.arraySize == 1 && header.mipMapCount == 1);

        m_header = header;

        CreateTextureCommitted(pDevice, pDebugName, false);

        // Get mip footprints (if it is an array we reuse the mip footprints for all the elements of the array)
        //
        UINT64 UplHeapSize;
        uint32_t num_rows = {};
        UINT64 row_sizes_in_bytes = {};
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT placedTex2D = {};
        CD3DX12_RESOURCE_DESC RDescs = CD3DX12_RESOURCE_DESC::Tex2D((DXGI_FORMAT)m_header.format, m_header.width, m_header.height, 1, 1);
        pDevice->GetDevice()->GetCopyableFootprints(&RDescs, 0, 1, 0, &placedTex2D, &num_rows, &row_sizes_in_bytes, &UplHeapSize);

        //compute pixel size
        //
        //UINT32 bytePP = m_header.bitCount / 8;

        // allocate memory for mip chain from upload heap
        //
        UINT8 *pixels = uploadHeap.Suballocate(SIZE_T(UplHeapSize), D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
        if (pixels == NULL)
        {
            // oh! We ran out of mem in the upload heap, flush it and try allocating mem from it again
            uploadHeap.FlushAndFinish();
            pixels = uploadHeap.Suballocate(SIZE_T(UplHeapSize), D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
            assert(pixels);
        }

        placedTex2D.Offset += UINT64(pixels - uploadHeap.BasePtr());

        // copy all the mip slices into the offsets specified by the footprint structure
        //
        for (uint32_t y = 0; y < num_rows; y++)
        {
            memcpy(pixels + y*placedTex2D.Footprint.RowPitch, (UINT8*)data + y*placedTex2D.Footprint.RowPitch, row_sizes_in_bytes);
        }

        CD3DX12_TEXTURE_COPY_LOCATION Dst(m_pResource, 0);
        CD3DX12_TEXTURE_COPY_LOCATION Src(uploadHeap.GetResource(), placedTex2D);
        uploadHeap.GetCommandList()->CopyTextureRegion(&Dst, 0, 0, 0, &Src, NULL);


        // prepare to shader read
        //
        D3D12_RESOURCE_BARRIER RBDesc = {};
        RBDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        RBDesc.Transition.pResource = m_pResource;
        RBDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        RBDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        RBDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

        uploadHeap.GetCommandList()->ResourceBarrier(1, &RBDesc);

        return true;
    }

    void Texture::CreateUAV(uint32_t index, CBV_SRV_UAV *pRV)
    {
        ID3D12Device* pDevice;
        m_pResource->GetDevice(__uuidof(*pDevice), reinterpret_cast<void**>(&pDevice));
        D3D12_RESOURCE_DESC texDesc = m_pResource->GetDesc();

        D3D12_UNORDERED_ACCESS_VIEW_DESC UAViewDesc = {};
        UAViewDesc.Format = texDesc.Format;
        UAViewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        pDevice->CreateUnorderedAccessView(m_pResource, NULL, &UAViewDesc, pRV->GetCPU(index));

        pDevice->Release();
    }

    void Texture::CreateBufferUAV(uint32_t index, Texture *pCounterTex, CBV_SRV_UAV *pRV)
    {
        ID3D12Device* pDevice;
        m_pResource->GetDevice(__uuidof(*pDevice), reinterpret_cast<void**>(&pDevice));
        D3D12_RESOURCE_DESC resourceDesc = m_pResource->GetDesc();

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = m_header.format;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.FirstElement = 0;
        uavDesc.Buffer.NumElements = m_header.width;
        uavDesc.Buffer.StructureByteStride = m_structuredBufferStride;
        uavDesc.Buffer.CounterOffsetInBytes = 0;
        uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
        pDevice->CreateUnorderedAccessView(m_pResource, pCounterTex ? pCounterTex->GetResource() : nullptr, &uavDesc, pRV->GetCPU(index));

        pDevice->Release();
    }

    void Texture::CreateSRV(uint32_t index, CBV_SRV_UAV *pRV, int mipLevel)
    {
        ID3D12Device* pDevice;
        m_pResource->GetDevice(__uuidof(*pDevice), reinterpret_cast<void**>(&pDevice));
        D3D12_RESOURCE_DESC resourceDesc = m_pResource->GetDesc();

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

        if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
        {
            srvDesc.Format = m_header.format;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            srvDesc.Buffer.FirstElement = 0;
            srvDesc.Buffer.NumElements = m_header.width;
            srvDesc.Buffer.StructureByteStride = m_structuredBufferStride;
            srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
        }
        else
        {
            if (resourceDesc.Format == DXGI_FORMAT_R32_TYPELESS)
            {
                srvDesc.Format = DXGI_FORMAT_R32_FLOAT; //special case for the depth buffer
            }
            else
            {
                D3D12_RESOURCE_DESC desc = m_pResource->GetDesc();
                srvDesc.Format = desc.Format;
            }

            if (resourceDesc.SampleDesc.Count == 1)
            {
                if (resourceDesc.DepthOrArraySize == 1)
                {
                    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                    if (mipLevel == -1)
                    {
                        srvDesc.Texture2D.MostDetailedMip = 0;
                        srvDesc.Texture2D.MipLevels = m_header.mipMapCount;
                    }
                    else
                    {
                        srvDesc.Texture2D.MostDetailedMip = mipLevel;
                        srvDesc.Texture2D.MipLevels = 1;
                    }
                }
                else
                {
                    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                    if (mipLevel == -1)
                    {
                        srvDesc.Texture2DArray.MostDetailedMip = 0;
                        srvDesc.Texture2DArray.MipLevels = m_header.mipMapCount;
                        srvDesc.Texture2DArray.ArraySize = resourceDesc.DepthOrArraySize;
                    }
                    else
                    {
                        srvDesc.Texture2DArray.MostDetailedMip = mipLevel;
                        srvDesc.Texture2DArray.MipLevels = 1;
                        srvDesc.Texture2DArray.ArraySize = resourceDesc.DepthOrArraySize;
                    }
                }
            }
            else
            {
                if (resourceDesc.DepthOrArraySize == 1)
                {
                    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
                }
                else
                {
                    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
                    if (mipLevel == -1)
                    {
                        srvDesc.Texture2DMSArray.FirstArraySlice = 0;
                        srvDesc.Texture2DMSArray.ArraySize = resourceDesc.DepthOrArraySize;
                    }
                    else
                    {
                        srvDesc.Texture2DMSArray.FirstArraySlice = 0;
                        srvDesc.Texture2DMSArray.ArraySize = resourceDesc.DepthOrArraySize;
                    }
                }
            }
        }

        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        pDevice->CreateShaderResourceView(m_pResource, &srvDesc, pRV->GetCPU(index));

        pDevice->Release();
    }

    void Texture::CreateCubeSRV(uint32_t index, CBV_SRV_UAV *pRV)
    {
        ID3D12Device* pDevice;
        m_pResource->GetDevice(__uuidof(*pDevice), reinterpret_cast<void**>(&pDevice));

        D3D12_RESOURCE_DESC texDesc = m_pResource->GetDesc();
        D3D12_RESOURCE_DESC desc = m_pResource->GetDesc();
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = desc.Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MostDetailedMip = 0;
        srvDesc.TextureCube.ResourceMinLODClamp = 0;
        srvDesc.TextureCube.MipLevels = m_header.mipMapCount;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        pDevice->CreateShaderResourceView(m_pResource, &srvDesc, pRV->GetCPU(index));

        pDevice->Release();
    }

    void Texture::CreateDSV(uint32_t index, DSV *pRV, int arraySlice)
    {
        ID3D12Device* pDevice;
        m_pResource->GetDevice(__uuidof(*pDevice), reinterpret_cast<void**>(&pDevice));
        D3D12_RESOURCE_DESC texDesc = m_pResource->GetDesc();

        D3D12_DEPTH_STENCIL_VIEW_DESC DSViewDesc = {};
        DSViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
        if (texDesc.SampleDesc.Count == 1)
        {
            if (texDesc.DepthOrArraySize == 1)
            {
                DSViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
                DSViewDesc.Texture2D.MipSlice = 0;
            }
            else
            {
                DSViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                DSViewDesc.Texture2DArray.MipSlice = 0;
                DSViewDesc.Texture2DArray.FirstArraySlice = arraySlice;
                DSViewDesc.Texture2DArray.ArraySize = 1;// texDesc.DepthOrArraySize;
            }
        }
        else
        {
            DSViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
        }


        pDevice->CreateDepthStencilView(m_pResource, &DSViewDesc, pRV->GetCPU(index));

        pDevice->Release();
    }

    INT32 Texture::InitDepthStencil(Device* pDevice, const char *pDebugName, const CD3DX12_RESOURCE_DESC *pDesc)
    {
        // Performance tip: Tell the runtime at resource creation the desired clear value.
        D3D12_CLEAR_VALUE clearValue;
        clearValue.Format = DXGI_FORMAT_D32_FLOAT;
        clearValue.DepthStencil.Depth = 1.0f;
        clearValue.DepthStencil.Stencil = 0;

        D3D12_RESOURCE_STATES states = D3D12_RESOURCE_STATE_COMMON;
        //if (pDesc->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE)
        //    states |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        states |= D3D12_RESOURCE_STATE_DEPTH_WRITE;

        pDevice->GetDevice()->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            pDesc,
            states,
            &clearValue,
            IID_PPV_ARGS(&m_pResource));

        m_header.width = (UINT32)pDesc->Width;
        m_header.height = pDesc->Height;
        m_header.depth = pDesc->Depth();
        m_header.mipMapCount = pDesc->MipLevels;

        SetName(m_pResource, pDebugName);

        return 0;
    }

    //--------------------------------------------------------------------------------------
    // create a comitted resource using m_header
    //--------------------------------------------------------------------------------------
    void Texture::CreateTextureCommitted(Device *pDevice, const char *pDebugName, bool useSRGB)
    {
        m_header.format = Translate((DXGI_FORMAT)m_header.format, useSRGB);

        CD3DX12_RESOURCE_DESC RDescs;
        RDescs = CD3DX12_RESOURCE_DESC::Tex2D((DXGI_FORMAT)m_header.format, m_header.width, m_header.height, m_header.arraySize, m_header.mipMapCount);

        pDevice->GetDevice()->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &RDescs,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&m_pResource)
        );

        SetName(m_pResource, pDebugName);
    }

    void Texture::LoadAndUpload(Device *pDevice, UploadHeap* pUploadHeap, ImgLoader *pDds, ID3D12Resource *pRes)
    {
        // Get mip footprints (if it is an array we reuse the mip footprints for all the elements of the array)
        //
        UINT64 UplHeapSize;
        uint32_t num_rows[D3D12_REQ_MIP_LEVELS] = { 0 };
        UINT64 row_sizes_in_bytes[D3D12_REQ_MIP_LEVELS] = { 0 };
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT placedTex2D[D3D12_REQ_MIP_LEVELS];
        CD3DX12_RESOURCE_DESC RDescs = CD3DX12_RESOURCE_DESC::Tex2D((DXGI_FORMAT)m_header.format, m_header.width, m_header.height, 1, m_header.mipMapCount);
        pDevice->GetDevice()->GetCopyableFootprints(&RDescs, 0, m_header.mipMapCount, 0, placedTex2D, num_rows, row_sizes_in_bytes, &UplHeapSize);

        //compute pixel size
        //
        UINT32 bytePP = m_header.bitCount / 8;
        if ((m_header.format >= DXGI_FORMAT_BC1_TYPELESS) && (m_header.format <= DXGI_FORMAT_BC5_SNORM))
        {
            bytePP = GetPixelSize((DXGI_FORMAT)m_header.format);
        }

        for (uint32_t a = 0; a < m_header.arraySize; a++)
        {
            // allocate memory for mip chain from upload heap
            //
            UINT8 *pixels = pUploadHeap->Suballocate(SIZE_T(UplHeapSize), D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
            if (pixels == NULL)
            {
                // oh! We ran out of mem in the upload heap, flush it and try allocating mem from it again
                pUploadHeap->FlushAndFinish();
                pixels = pUploadHeap->Suballocate(SIZE_T(UplHeapSize), D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
                assert(pixels != NULL);
            }

            // copy all the mip slices into the offsets specified by the footprint structure
            //
            for (uint32_t mip = 0; mip < m_header.mipMapCount; mip++)
            {
                pDds->CopyPixels(pixels + placedTex2D[mip].Offset, placedTex2D[mip].Footprint.RowPitch, placedTex2D[mip].Footprint.Width * bytePP, num_rows[mip]);

                D3D12_PLACED_SUBRESOURCE_FOOTPRINT slice = placedTex2D[mip];
                slice.Offset += (pixels - pUploadHeap->BasePtr());

                CD3DX12_TEXTURE_COPY_LOCATION Dst(m_pResource, a*m_header.mipMapCount + mip);
                CD3DX12_TEXTURE_COPY_LOCATION Src(pUploadHeap->GetResource(), slice);
                pUploadHeap->GetCommandList()->CopyTextureRegion(&Dst, 0, 0, 0, &Src, NULL);
            }
        }

        // prepare to shader read
        //
        D3D12_RESOURCE_BARRIER RBDesc = {};
        RBDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        RBDesc.Transition.pResource = m_pResource;
        RBDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        RBDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        RBDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

        pUploadHeap->GetCommandList()->ResourceBarrier(1, &RBDesc);
    }

    //--------------------------------------------------------------------------------------
    // entry function to initialize an image from a .DDS texture
    //--------------------------------------------------------------------------------------
    bool Texture::InitFromFile(Device *pDevice, UploadHeap *pUploadHeap, const char *pFilename, bool useSRGB, float cutOff)
    {
        assert(m_pResource == NULL);

        ImgLoader *img = GetImageLoader(pFilename);
        bool result = img->Load(pFilename, cutOff, &m_header);
        if (result)
        {
            CreateTextureCommitted(pDevice, pFilename, useSRGB);
            LoadAndUpload(pDevice, pUploadHeap, img, m_pResource);
        }

        delete(img);

        return result;
    }

    DXGI_FORMAT Translate(DXGI_FORMAT format, bool useSRGB)
    {
        if (useSRGB)
        {
            switch (format)
            {
            case DXGI_FORMAT_B8G8R8A8_UNORM: return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
            case DXGI_FORMAT_R8G8B8A8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            case DXGI_FORMAT_BC1_UNORM: return DXGI_FORMAT_BC1_UNORM_SRGB;
            case DXGI_FORMAT_BC2_UNORM: return DXGI_FORMAT_BC2_UNORM_SRGB;
            case DXGI_FORMAT_BC3_UNORM: return DXGI_FORMAT_BC3_UNORM_SRGB;
            case DXGI_FORMAT_BC7_UNORM: return DXGI_FORMAT_BC7_UNORM_SRGB;
            default: assert(false);  return DXGI_FORMAT_UNKNOWN;
            }
        }

        return format;
    }
}
