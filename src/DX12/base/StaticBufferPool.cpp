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

#include "stdafx.h"
#include "Device.h"
#include "StaticBufferPool.h"
#include "Misc/Misc.h"

namespace CAULDRON_DX12
{
    void StaticBufferPool::OnCreate(Device *pDevice, uint32_t totalMemSize, bool bUseVidMem, const char* name)
    {
        m_pDevice = pDevice;
        m_totalMemSize = totalMemSize;
        m_memOffset = 0;
        m_memInit = 0;
        m_pData = nullptr;
        m_bUseVidMem = bUseVidMem;

        if (bUseVidMem)
		{
			auto properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
			auto buffer = CD3DX12_RESOURCE_DESC::Buffer(totalMemSize);

            ThrowIfFailed(
                m_pDevice->GetDevice()->CreateCommittedResource(
                    &properties,
                    D3D12_HEAP_FLAG_NONE,
					&buffer,
                    // ADJUSTMENT: Allow computer shader access to vertex buffers via UAV
                //  D3D12_RESOURCE_STATE_INDEX_BUFFER | D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
					D3D12_RESOURCE_STATE_COMMON,
					nullptr,
                    IID_PPV_ARGS(&m_pVidMemBuffer))
            );
            SetName(m_pVidMemBuffer, "StaticBufferPoolDX12::m_pVidMemBuffer");
        }

        {
            auto properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            auto buffer = CD3DX12_RESOURCE_DESC::Buffer(totalMemSize);

            ThrowIfFailed(
                m_pDevice->GetDevice()->CreateCommittedResource(
                    &properties,
                    D3D12_HEAP_FLAG_NONE,
					&buffer,
					// ADJUSTMENT: Allow computer shader access to vertex buffers via UAV
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    IID_PPV_ARGS(&m_pSysMemBuffer))
            );
            SetName(m_pSysMemBuffer, "StaticBufferPoolDX12::m_pSysMemBuffer");
        }

        m_pSysMemBuffer->Map(0, NULL, reinterpret_cast<void**>(&m_pData));
    }

    void StaticBufferPool::OnDestroy()
    {
        if (m_bUseVidMem)
        {
            if(m_pVidMemBuffer)
            {
                m_pVidMemBuffer->Release();
                m_pVidMemBuffer = nullptr;
            }
        }

        if(m_pSysMemBuffer)
        {
            m_pSysMemBuffer->Release();
            m_pSysMemBuffer = nullptr;
        }
    }

    static uint32_t gcd(uint32_t a, uint32_t b)
    {
        while (a != b)
        {
            if (a > b)
                a = a - b;
            else
                b = b - a;
        }

        return a;
    }

	static uint32_t lcm(uint32_t a, uint32_t b)
    {
		return (a * b) / gcd(a, b);
	}

    bool StaticBufferPool::AllocBuffer(uint32_t numbeOfElements, uint32_t strideInBytes, void **pData, D3D12_GPU_VIRTUAL_ADDRESS *pBufferLocation, uint32_t* pSize, uint32_t baseAlignment)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        // ADJUSTMENT: Make position locations divisible by 12 / sizeof(float3), 768/12=64, for global indexing
        D3D12_GPU_VIRTUAL_ADDRESS address = ((m_bUseVidMem) ? m_pVidMemBuffer->GetGPUVirtualAddress() : m_pSysMemBuffer->GetGPUVirtualAddress());
		uint32_t safe_size = AlignUpSafe(numbeOfElements * strideInBytes, baseAlignment);
		uint32_t safe_offset = AlignUpSafe(m_memOffset, lcm(strideInBytes, baseAlignment)) - m_memOffset;

		m_memOffset += safe_offset;

		assert(m_memOffset % strideInBytes == 0);
		assert(m_memOffset % baseAlignment == 0);
		assert(m_memOffset + safe_size < m_totalMemSize);

        *pData = (void *)(m_pData + m_memOffset);
        *pBufferLocation = address + m_memOffset;
		*pSize = safe_size;

		m_memOffset += safe_size;

        return true;
    }

    bool StaticBufferPool::AllocBuffer(uint32_t numbeOfElements, uint32_t strideInBytes, void *pInitData, D3D12_GPU_VIRTUAL_ADDRESS *pBufferLocation, uint32_t *pSize, uint32_t baseAlignment)
    {
        void *pData;
        if (AllocBuffer(numbeOfElements, strideInBytes, &pData, pBufferLocation, pSize, baseAlignment))
        {
            memcpy(pData, pInitData, numbeOfElements*strideInBytes);
            return true;
        }
        return false;
    }

    bool StaticBufferPool::AllocVertexBuffer(uint32_t numberOfVertices, uint32_t strideInBytes, void **pData, D3D12_VERTEX_BUFFER_VIEW *pView)
    {
        AllocBuffer(numberOfVertices, strideInBytes, pData, &pView->BufferLocation, &pView->SizeInBytes, 4u);
        pView->StrideInBytes = strideInBytes;

        return true;
    }

    bool StaticBufferPool::AllocVertexBuffer(uint32_t numbeOfVertices, uint32_t strideInBytes, const void *pInitData, D3D12_VERTEX_BUFFER_VIEW *pOut)
    {
        void *pData;
        if (AllocVertexBuffer(numbeOfVertices, strideInBytes, &pData, pOut))
        {
            memcpy(pData, pInitData, numbeOfVertices*strideInBytes);
            return true;
        }
        return false;
    }

    bool StaticBufferPool::AllocIndexBuffer(uint32_t numbeOfIndices, uint32_t strideInBytes, void **pData, D3D12_INDEX_BUFFER_VIEW *pView)
    {
        AllocBuffer(numbeOfIndices, strideInBytes, pData, &pView->BufferLocation, &pView->SizeInBytes, 4u);
        pView->Format = (strideInBytes == 4) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;

        return true;
    }

    bool StaticBufferPool::AllocIndexBuffer(uint32_t numbeOfIndices, uint32_t strideInBytes, const void *pInitData, D3D12_INDEX_BUFFER_VIEW *pOut)
    {
        void *pData;
        if (AllocIndexBuffer(numbeOfIndices, strideInBytes, &pData, pOut))
        {
            memcpy(pData, pInitData, numbeOfIndices*strideInBytes);
            return true;
        }
        return false;
    }

    bool StaticBufferPool::AllocConstantBuffer(uint32_t size, void **pData, D3D12_CONSTANT_BUFFER_VIEW_DESC  *pViewDesc)
    {
        AllocBuffer(size, 1, pData, &pViewDesc->BufferLocation, &pViewDesc->SizeInBytes, 256u);

        return true;
    }

    bool StaticBufferPool::AllocConstantBuffer(uint32_t size, void *pInitData, D3D12_CONSTANT_BUFFER_VIEW_DESC *pOut)
    {
        void *pData;
        if (AllocConstantBuffer(size, &pData, pOut))
        {
            memcpy(pData, pInitData, size);
            return true;
        }
        return false;
    }

    void StaticBufferPool::UploadData(ID3D12GraphicsCommandList *pCmdList)
    {
        if (m_bUseVidMem)
		{
			// ADJUSTMENT: Allow computer shader access to vertex buffers via UAV
            {
                auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_pVidMemBuffer,
                    D3D12_RESOURCE_STATE_INDEX_BUFFER | D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
                pCmdList->ResourceBarrier(1, &barrier);
            }

            pCmdList->CopyBufferRegion(m_pVidMemBuffer, m_memInit, m_pSysMemBuffer, m_memInit, m_memOffset - m_memInit);

            // With 'dynamic resources' we can use a same resource to hold Constant, Index and Vertex buffers.
            // That is because we dont need to use a transition.
            //
            // With static buffers though we need to transition them and we only have 2 options
            //      1) D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
            //      2) D3D12_RESOURCE_STATE_INDEX_BUFFER
            // Because we need to transition the whole buffer we cant have now Index buffers to share the 
            // same resource with the Vertex or Constant buffers. Hence is why we need separate classes.
            // For Index and Vertex buffers we *could* use the same resource, but index buffers need their own resource.
            // Please note that in the interest of clarity vertex buffers and constant buffers have been split into two different classes though
			{
				auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_pVidMemBuffer,
                    D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER | D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                pCmdList->ResourceBarrier(1, &barrier);
            }

            m_memInit = m_memOffset;
        }
	}

    void StaticBufferPool::FreeUploadHeap()
    {
        if (m_bUseVidMem)
        {
            assert(m_pSysMemBuffer != nullptr);
            m_pSysMemBuffer->Release();
            m_pSysMemBuffer = nullptr;
        }
    }

    ID3D12Resource *StaticBufferPool::GetResource() const
    {
        return (m_bUseVidMem) ? m_pVidMemBuffer : m_pSysMemBuffer;
	}

    // ADJUSTMENT: Get a descriptor for the whole mesh heap in the given format
	void StaticBufferPool::CreateSRV(uint32_t index, CBV_SRV_UAV* pRV, DXGI_FORMAT elementFormat, size_t elementSize)
	{
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = elementFormat;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = UINT(GetSize() / elementSize);
		srvDesc.Buffer.StructureByteStride = 0;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;// D3D12_BUFFER_SRV_FLAG_RAW;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		m_pDevice->GetDevice()->CreateShaderResourceView(GetResource(), &srvDesc, pRV->GetCPU(index));
	}

}



