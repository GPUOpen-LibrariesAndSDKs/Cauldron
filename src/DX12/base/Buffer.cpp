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
#include "Buffer.h"
#include "Base/Helper.h"

namespace CAULDRON_DX12
{
    void Buffer::Release()
    {
        m_pBuffer->Release();
    }

    void Buffer::InitFromMem(Device* pDevice, const char* pDebugName, UploadHeap* pUploadHeap, const void* pData, int numElements, int elementSize, uint32_t minimumSize)
	{
		uint32_t size = numElements * elementSize;
		uint32_t buffersize = std::max<uint32_t>(size, minimumSize);

        m_pDevice = pDevice;
        m_numElements = buffersize / elementSize;
        m_elementSize = elementSize;

        D3D12_HEAP_PROPERTIES defaultHeapProp = {};
        defaultHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC bufDesc = {};
        bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufDesc.Width = buffersize;
        bufDesc.Height = 1;
        bufDesc.DepthOrArraySize = 1;
        bufDesc.MipLevels = 1;
        bufDesc.Format = DXGI_FORMAT_UNKNOWN;
        bufDesc.SampleDesc.Count = 1;
		bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		bufDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS; // ADJUSTMENT: allow UAVs

        m_pDevice->GetDevice()->CreateCommittedResource(&defaultHeapProp, D3D12_HEAP_FLAG_NONE, &bufDesc, D3D12_RESOURCE_STATE_COMMON, NULL, IID_PPV_ARGS(&m_pBuffer));
        SetName(m_pBuffer, pDebugName);

        if (pData)
        {
			pUploadHeap->AddBufferCopy(pData, size, m_pBuffer);
		}
    }

    void Buffer::UpdateFromMem(UploadHeap* pUploadHeap, const void* pData, int numElements, int elementSize)
	{
		uint32_t size = numElements * elementSize;

		if (pData)
		{
			pUploadHeap->AddBufferCopy(pData, size, m_pBuffer);
		}
    }

    void Buffer::CreateSRV(uint32_t index, CBV_SRV_UAV *pRV, uint32_t beginning)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Buffer.FirstElement = beginning;
        srvDesc.Buffer.NumElements = m_numElements - beginning;
        srvDesc.Buffer.StructureByteStride = m_elementSize;
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;// D3D12_BUFFER_SRV_FLAG_RAW;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        m_pDevice->GetDevice()->CreateShaderResourceView(m_pBuffer, &srvDesc, pRV->GetCPU(index));
	}

	void Buffer::CreateUAV(uint32_t index, CBV_SRV_UAV* pRV, uint32_t beginning)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = beginning;
		uavDesc.Buffer.NumElements = m_numElements - beginning;
		uavDesc.Buffer.StructureByteStride = m_elementSize;
		uavDesc.Buffer.CounterOffsetInBytes = 0;
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;// D3D12_BUFFER_SRV_FLAG_RAW;
		m_pDevice->GetDevice()->CreateUnorderedAccessView(m_pBuffer, nullptr, &uavDesc, pRV->GetCPU(index));
	}
}