//
// Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#include "stdafx.h"
#include "UserMarkers.h"
#include "../../libs/AGS/amd_ags.h"
#include <pix3.h>

namespace CAULDRON_DX12
{
     AGSContext* UserMarker::s_agsContext = nullptr;

     UserMarker::UserMarker(ID3D12GraphicsCommandList* commandBuffer, const char* name) 
     {
         m_commandBuffer = commandBuffer;

#if TODO_AGS
         if (s_agsContext)
             commandBuffer->SetMarker(s_agsContext, m_commandBuffer, name);
#endif // TODO_AGS

         PIXBeginEvent(m_commandBuffer, 0, name);
     }

#if 0
     UserMarker::UserMarker(ID3D12CommandQueue* queue, const char* name)
     {
         if (s_agsContext)
         {
             ID3D12Device* dev = nullptr;
             if (FAILED(queue->GetDevice(IID_PPV_ARGS(&dev))))
             {
                 return;
             }

             auto queueDesc = queue->GetDesc();

             ID3D12CommandAllocator* cmdAlloc = nullptr;
             dev->CreateCommandAllocator(queueDesc.Type, IID_PPV_ARGS(&cmdAlloc));

             ID3D12CommandList* cmdList = nullptr;
             dev->CreateCommandList(queueDesc.NodeMask, queueDesc.Type, 0, 0, IID_PPV_ARGS(cmdList));

             cmdAlloc->Reset();
             cmdList->
         }
     }
#endif


     UserMarker::~UserMarker()
     {
#if TODO_AGS
         if (s_agsContext)
             agsDriverExtensionsDX12_PopMarker(s_agsContext, m_commandBuffer);
#endif // TODO_AGS

         PIXEndEvent(m_commandBuffer);
     }
}