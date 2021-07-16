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

#include <d3d12.h>

namespace CAULDRON_DX12
{
    class SaveTexture
    {
        UINT64 UplHeapSize = 0;
        D3D12_RESOURCE_DESC bufferFromDesc = { };
        ID3D12Resource* pResourceReadBack = nullptr;
    public:
        //
        // Call this function first so the resource gets copied into the staging one
        //
        void CopyRenderTargetIntoStagingTexture(ID3D12Device *pDevice, ID3D12GraphicsCommandList* pCmdLst2, ID3D12Resource* pResourceFrom, D3D12_RESOURCE_STATES state);

        //
        //  Then, only after the commandlist of the above call has been submitted call this one to actually save the image to disk.
        //
        void SaveStagingTextureAsJpeg(ID3D12Device *pDevice, ID3D12CommandQueue *pDirectQueue, const char *pFilename);
    };
}