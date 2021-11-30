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
#include "UploadHeap.h"
#include "Misc/Misc.h"
#include "Base/Helper.h"

namespace CAULDRON_DX12
{
    //--------------------------------------------------------------------------------------
    //
    // OnCreate
    //
    //--------------------------------------------------------------------------------------
    void UploadHeap::OnCreate(Device *pDevice, SIZE_T uSize)
    {
        m_pDevice = pDevice;
        m_pCommandQueue = pDevice->GetGraphicsQueue();

        // Create command list and allocators 

        pDevice->GetDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_pCommandAllocator));
        SetName(m_pCommandAllocator, "UploadHeap::m_pCommandAllocator");
        pDevice->GetDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandAllocator, nullptr, IID_PPV_ARGS(&m_pCommandList));
        SetName(m_pCommandList, "UploadHeap::m_pCommandList");

        // Create buffer to suballocate

        ThrowIfFailed(
            pDevice->GetDevice()->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(uSize),
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_pUploadHeap)
            )
        );

        ThrowIfFailed(m_pUploadHeap->Map(0, NULL, (void**)&m_pDataBegin));

        m_pDataCur = m_pDataBegin;
        m_pDataEnd = m_pDataBegin + m_pUploadHeap->GetDesc().Width;

        m_fenceValue = 0;
    }

    //--------------------------------------------------------------------------------------
    //
    // OnDestroy
    //
    //--------------------------------------------------------------------------------------
    void UploadHeap::OnDestroy()
    {
        m_pUploadHeap->Release();

        m_pCommandList->Release();
        m_pCommandAllocator->Release();
    }

    //--------------------------------------------------------------------------------------
    //
    // SuballocateFromUploadHeap
    //
    //--------------------------------------------------------------------------------------
    UINT8* UploadHeap::Suballocate(SIZE_T uSize, UINT64 uAlign)
    {
        // wait until we are done flusing the heap
        flushing.Wait();

        UINT8* pRet = NULL;

        {
            std::unique_lock<std::mutex> lock(m_mutex);

            // make sure resource (and its mips) would fit the upload heap, if not please make the upload heap bigger
            assert(uSize < (size_t)(m_pDataEnd - m_pDataBegin));

            m_pDataCur = reinterpret_cast<UINT8*>(AlignUp(reinterpret_cast<SIZE_T>(m_pDataCur), SIZE_T(uAlign)));

            // return NULL if we ran out of space in the heap
            if (m_pDataCur >= m_pDataEnd || m_pDataCur + uSize >= m_pDataEnd)
            {
                return NULL;
            }

            pRet = m_pDataCur;
            m_pDataCur += uSize;

            //Trace("allocated: %i", m_pDataCur - m_pDataBegin);
        }

        return pRet;
    }

    UINT8* UploadHeap::BeginSuballocate(SIZE_T uSize, UINT64 uAlign)
    {
        UINT8* pRes = NULL;

        for (;;) {
            pRes = Suballocate(uSize, uAlign);
            if (pRes != NULL)
            {
                break;
            }

            FlushAndFinish();
        }

        allocating.Inc();

        return pRes;
    }

    void UploadHeap::EndSuballocate()
    {
        allocating.Dec();
    }

    void UploadHeap::AddBufferCopy(const void *pData, int size, ID3D12Resource *pBufferDst, D3D12_RESOURCE_STATES state)
    {
        UINT8 *pixels = BeginSuballocate(size, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
        memcpy(pixels, pData, size);
        EndSuballocate();

        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_bufferCopies.push_back({ pBufferDst, (UINT64)(pixels - BasePtr()), size, state });

            D3D12_RESOURCE_BARRIER RBDesc = {};
            RBDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            RBDesc.Transition.pResource = pBufferDst;
            RBDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            RBDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
            RBDesc.Transition.StateAfter = state;
            m_toBarrierIntoShaderResource.push_back(RBDesc);
        }
    }


    void UploadHeap::AddCopy(CD3DX12_TEXTURE_COPY_LOCATION Src, CD3DX12_TEXTURE_COPY_LOCATION Dst)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_textureCopies.push_back({ Src, Dst });
    }

    void UploadHeap::AddBarrier(ID3D12Resource *pRes)
    {
        std::unique_lock<std::mutex> lock(m_mutex);

        D3D12_RESOURCE_BARRIER RBDesc = {};
        RBDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        RBDesc.Transition.pResource = pRes;
        RBDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        RBDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        RBDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

        m_toBarrierIntoShaderResource.push_back(RBDesc);
    }

    //--------------------------------------------------------------------------------------
    //
    // FlushAndFinish
    //
    //--------------------------------------------------------------------------------------
    void UploadHeap::FlushAndFinish()
    {
        // make sure another thread is not already flushing
        flushing.Wait();

        // begins a critical section, and make sure no allocations happen while a thread is inside it
        flushing.Inc();

        // wait for pending allocations to finish
        allocating.Wait();

        std::unique_lock<std::mutex> lock(m_mutex);
        Trace("flushing %i, %i", m_textureCopies.size(), m_bufferCopies.size());

        //issue copies
        for (TextureCopy c : m_textureCopies)
        {
            m_pCommandList->CopyTextureRegion(&c.Dst, 0, 0, 0, &c.Src, NULL);
        }
        m_textureCopies.clear();

        for (BufferCopy c : m_bufferCopies)
        {
            {
                D3D12_RESOURCE_BARRIER RBDesc = {};
                RBDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                RBDesc.Transition.pResource = c.pBufferDst;
                RBDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                RBDesc.Transition.StateBefore = c.state;
                RBDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
                m_pCommandList->ResourceBarrier(1, &RBDesc);
            }
            m_pCommandList->CopyBufferRegion(c.pBufferDst, 0, GetResource(), c.offset, c.size);
        }
        m_bufferCopies.clear();

        //apply barriers in one go
        if (m_toBarrierIntoShaderResource.size() > 0)
        {
            m_pCommandList->ResourceBarrier((UINT)m_toBarrierIntoShaderResource.size(), m_toBarrierIntoShaderResource.data());
            m_toBarrierIntoShaderResource.clear();
        }

        // Close & submit
        ThrowIfFailed(m_pCommandList->Close());
        m_pCommandQueue->ExecuteCommandLists(1, CommandListCast(&m_pCommandList));

        // Make sure it's been processed by the GPU
        m_pDevice->GPUFlush();

        // Reset so it can be reused
        m_pCommandAllocator->Reset();
        m_pCommandList->Reset(m_pCommandAllocator, nullptr);

        m_pDataCur = m_pDataBegin;

        flushing.Dec();
    }
}