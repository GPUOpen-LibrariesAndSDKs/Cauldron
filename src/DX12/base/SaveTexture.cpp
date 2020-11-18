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
#include "SaveTexture.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../stb/stb_image_write.h"
#include "Helper.h"
#include "../common/Misc/DxgiFormatHelper.h"

namespace CAULDRON_DX12
{

    void SaveTexture::CopyRenderTargetIntoStagingTexture(ID3D12Device *pDevice, ID3D12GraphicsCommandList* pCmdLst2, ID3D12Resource* pResourceFrom, D3D12_RESOURCE_STATES resourceCurrentState)
    {
        bufferFromDesc = pResourceFrom->GetDesc();

        CD3DX12_HEAP_PROPERTIES readBackHeapProperties(D3D12_HEAP_TYPE_READBACK);

        D3D12_RESOURCE_DESC bufferDesc = {};
        bufferDesc.Alignment = 0;
        bufferDesc.DepthOrArraySize = 1;
        bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        bufferDesc.Height = 1;
        bufferDesc.Width = bufferFromDesc.Width * bufferFromDesc.Height * GetPixelByteSize(bufferFromDesc.Format);
        bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        bufferDesc.MipLevels = 1;
        bufferDesc.SampleDesc.Count = 1;
        bufferDesc.SampleDesc.Quality = 0;

        pDevice->CreateCommittedResource(
            &readBackHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&pResourceReadBack));

        SetName(pResourceReadBack, "CopyRenderTargetIntoStagingTexture::pResourceReadBack");

        pCmdLst2->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pResourceFrom, resourceCurrentState, D3D12_RESOURCE_STATE_COPY_SOURCE));

        D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout[1] = { 0 };
        uint32_t num_rows[1] = { 0 };
        UINT64 row_sizes_in_bytes[1] = { 0 };
        pDevice->GetCopyableFootprints(&bufferFromDesc, 0, 1, 0, layout, num_rows, row_sizes_in_bytes, &UplHeapSize);

        CD3DX12_TEXTURE_COPY_LOCATION copyDest(pResourceReadBack, layout[0]);
        CD3DX12_TEXTURE_COPY_LOCATION copySrc(pResourceFrom, 0);
        pCmdLst2->CopyTextureRegion(&copyDest, 0, 0, 0, &copySrc, nullptr);

        pCmdLst2->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pResourceFrom, D3D12_RESOURCE_STATE_COPY_SOURCE, resourceCurrentState));
    }

    void SaveTexture::SaveStagingTextureAsJpeg(ID3D12Device *pDevice, ID3D12CommandQueue *pDirectQueue, const char *pFilename)
    {
        ID3D12Fence *pFence;
        ThrowIfFailed(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence)));
        SetName(pFence, "CopyRenderTargetIntoStagingTexture::pFence");
        ThrowIfFailed(pDirectQueue->Signal(pFence, 1));

        //wait for fence
        HANDLE mHandleFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        pFence->SetEventOnCompletion(1, mHandleFenceEvent);
        WaitForSingleObject(mHandleFenceEvent, INFINITE);
        CloseHandle(mHandleFenceEvent);

        pFence->Release();

        UINT64 *pTimingsBuffer = NULL;
        D3D12_RANGE range;
        range.Begin = 0;
        range.End = UplHeapSize;
        pResourceReadBack->Map(0, &range, reinterpret_cast<void**>(&pTimingsBuffer));
        stbi_write_jpg(pFilename, (int)bufferFromDesc.Width, (int)bufferFromDesc.Height, 4, pTimingsBuffer, 100);
        pResourceReadBack->Unmap(0, NULL);

        pResourceReadBack->Release();
        pResourceReadBack = nullptr;
    }
}
