// AMD Cauldron code
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
#include "Device.h"
#include "CommandListRing.h"
#include "ExtDebugUtils.h"
#include "Misc/Misc.h"

namespace CAULDRON_VK
{
    //--------------------------------------------------------------------------------------
    //
    // OnCreate
    //
    //--------------------------------------------------------------------------------------
    void CommandListRing::OnCreate(Device *pDevice, uint32_t numberOfBackBuffers, uint32_t commandListsPerBackBuffer, bool compute /* = false */)
    {
        m_pDevice = pDevice;
        m_numberOfAllocators = numberOfBackBuffers;
        m_commandListsPerBackBuffer = commandListsPerBackBuffer;

        m_pCommandBuffers = new CommandBuffersPerFrame[m_numberOfAllocators]();

        // Create command allocators, for each frame in flight we wannt to have a single Command Allocator, and <commandListsPerBackBuffer> command buffers
        //
        for (uint32_t a = 0; a < m_numberOfAllocators; a++)
        {
            CommandBuffersPerFrame *pCBPF = &m_pCommandBuffers[a];

            // Create allocator
            //
            VkCommandPoolCreateInfo cmd_pool_info = {};
            cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            cmd_pool_info.pNext = NULL;
            if (compute == false)
            {
                cmd_pool_info.queueFamilyIndex = pDevice->GetGraphicsQueueFamilyIndex();
            }
            else
            {
                cmd_pool_info.queueFamilyIndex = pDevice->GetComputeQueueFamilyIndex();
            }
            cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
            VkResult res = vkCreateCommandPool(pDevice->GetDevice(), &cmd_pool_info, NULL, &pCBPF->m_commandPool);
            assert(res == VK_SUCCESS);

            // Create command buffers
            //
            pCBPF->m_pCommandBuffer = new VkCommandBuffer[m_commandListsPerBackBuffer];
            VkCommandBufferAllocateInfo cmd = {};
            cmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            cmd.pNext = NULL;
            cmd.commandPool = pCBPF->m_commandPool;
            cmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            cmd.commandBufferCount = commandListsPerBackBuffer;
            res = vkAllocateCommandBuffers(pDevice->GetDevice(), &cmd, pCBPF->m_pCommandBuffer);
            assert(res == VK_SUCCESS);

            pCBPF->m_UsedCls = 0;
        }

        m_frameIndex = 0;
        m_pCurrentFrame = &m_pCommandBuffers[m_frameIndex % m_numberOfAllocators];
        m_frameIndex++;
        m_pCurrentFrame->m_UsedCls = 0;
    }


    //--------------------------------------------------------------------------------------
    //
    // OnDestroy
    //
    //--------------------------------------------------------------------------------------
    void CommandListRing::OnDestroy()
    {
        //release and delete command allocators
        for (uint32_t a = 0; a < m_numberOfAllocators; a++)
        {
            vkFreeCommandBuffers(m_pDevice->GetDevice(), m_pCommandBuffers[a].m_commandPool, m_commandListsPerBackBuffer, m_pCommandBuffers[a].m_pCommandBuffer);
            delete[] m_pCommandBuffers[a].m_pCommandBuffer;
            vkDestroyCommandPool(m_pDevice->GetDevice(), m_pCommandBuffers[a].m_commandPool, NULL);
        }

        delete[] m_pCommandBuffers;
    }

    //--------------------------------------------------------------------------------------
    //
    // GetNewCommandList
    //
    //--------------------------------------------------------------------------------------
    VkCommandBuffer CommandListRing::GetNewCommandList()
    {
        VkCommandBuffer commandBuffer = m_pCurrentFrame->m_pCommandBuffer[m_pCurrentFrame->m_UsedCls++];

        assert(m_pCurrentFrame->m_UsedCls < m_commandListsPerBackBuffer); //if hit increase commandListsPerBackBuffer

        return commandBuffer;
    }

    //--------------------------------------------------------------------------------------
    //
    // OnBeginFrame
    //
    //--------------------------------------------------------------------------------------
    void CommandListRing::OnBeginFrame()
    {
        m_pCurrentFrame = &m_pCommandBuffers[m_frameIndex % m_numberOfAllocators];

        // Wait for any work that is not finished for current frame

        // NOTE: That code seems to not be working properly.
        // DEFENSIVE HACK: Since we are moving to Cauldron2 we are adding a hack to fix samples by waiting on the device to be idle
#if 0
        if(m_pCurrentFrame->m_cmdBufExecutedFences)
        {
            vkWaitForFences(m_pDevice->GetDevice(), 1, &m_pCurrentFrame->m_cmdBufExecutedFences, false, UINT64_MAX);
            vkResetFences(m_pDevice->GetDevice(), 1, &m_pCurrentFrame->m_cmdBufExecutedFences);
        }
        else
        {
            VkFenceCreateInfo fence_ci = {};
            fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fence_ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            auto res = vkCreateFence(m_pDevice->GetDevice(), &fence_ci, NULL, &m_pCurrentFrame->m_cmdBufExecutedFences);
        }
#else
        vkDeviceWaitIdle(m_pDevice->GetDevice());
#endif
        // NOTE: vkResetCommandPool - resets entire pool causes following:
        // Validation Error: [ VUID-vkResetCommandBuffer-commandBuffer-00045 ] Object 0: handle = 0x29ac6454070, type = VK_OBJECT_TYPE_COMMAND_BUFFER; | MessageID = 0x1e7883ea | Attempt to reset VkCommandBuffer 0x29ac6454070[] which is in use. The Vulkan spec states: commandBuffer must not be in the pending state (https://vulkan.lunarg.com/doc/view/1.3.204.1/windows/1.3-extensions/vkspec.html#VUID-vkResetCommandBuffer-commandBuffer-00045)
        vkResetCommandPool(m_pDevice->GetDevice(), m_pCurrentFrame->m_commandPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);

        m_pCurrentFrame->m_UsedCls = 0;

        m_frameIndex++;
    }

}