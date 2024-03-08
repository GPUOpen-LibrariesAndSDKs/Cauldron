// AMD Cauldron code
// 
// Copyright(c) 2024 Advanced Micro Devices, Inc.All rights reserved.
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
#include "Misc/ImgLoader.h"

namespace CAULDRON_DX12
{
    class Buffer
    {
    public:
		void InitFromMem(Device* pDevice, const char* pDebugName, UploadHeap* pUploadHeap, const void* pData, int numElements, int elementSize, uint32_t minimumSize = 0);
		void UpdateFromMem(UploadHeap* pUploadHeap, const void* pData, int numElements, int elementSize);
        void Release();
		void CreateSRV(uint32_t index, CBV_SRV_UAV* pRV, uint32_t beginning = 0);
		void CreateUAV(uint32_t index, CBV_SRV_UAV* pRV, uint32_t beginning = 0);

		ID3D12Resource* GetResource() const { return m_pBuffer; }

    private:
        Device                    *m_pDevice;
        ID3D12Resource            *m_pBuffer;

        int m_numElements;
        int m_elementSize;
    };
}