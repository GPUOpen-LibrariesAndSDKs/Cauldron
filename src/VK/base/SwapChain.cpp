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
#include "SwapChain.h"
#include "ExtFreeSyncHDR.h"
#include "ExtDebugUtils.h"
#include <vulkan\vulkan_win32.h>

namespace CAULDRON_VK
{

    //--------------------------------------------------------------------------------------
    //
    // OnCreate
    //
    //--------------------------------------------------------------------------------------
    void SwapChain::OnCreate(Device *pDevice, uint32_t numberBackBuffers, HWND hWnd)
    {
        VkResult res;

        m_hWnd = hWnd;
        m_pDevice = pDevice;
        m_backBufferCount = numberBackBuffers;
        m_semaphoreIndex = 0;
        m_prevSemaphoreIndex = 0;

        m_presentQueue = pDevice->GetPresentQueue();

        // Init FSHDR
        fsHdrInit(pDevice->GetDevice(), pDevice->GetSurface(), pDevice->GetPhysicalDevice(), hWnd);

        // set some safe format to start with
        m_displayMode = DISPLAYMODE_SDR;
        VkSurfaceFormatKHR surfaceFormat;
        surfaceFormat.format = VK_FORMAT_R8G8B8A8_UNORM;
        surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        m_swapChainFormat = surfaceFormat;

        VkDevice device = m_pDevice->GetDevice();

        // create fences and semaphores
        //
        m_cmdBufExecutedFences.resize(m_backBufferCount);
        m_ImageAvailableSemaphores.resize(m_backBufferCount);
        m_RenderFinishedSemaphores.resize(m_backBufferCount);
        for (uint32_t i = 0; i < m_backBufferCount; i++)
        {
            VkFenceCreateInfo fence_ci = {};
            fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

            // In each frame in WaitForSwapChain we wait for the executed fences to become signalled, before we can reuse the respective command buffers.
            // Hence all of them must start in the signalled bit initially (meaning "can be reused").
            fence_ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            res = vkCreateFence(device, &fence_ci, NULL, &m_cmdBufExecutedFences[i]);
            assert(res == VK_SUCCESS);

            VkSemaphoreCreateInfo imageAcquiredSemaphoreCreateInfo = {};
            imageAcquiredSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            res = vkCreateSemaphore(device, &imageAcquiredSemaphoreCreateInfo, NULL, &m_ImageAvailableSemaphores[i]);
            assert(res == VK_SUCCESS);
            res = vkCreateSemaphore(device, &imageAcquiredSemaphoreCreateInfo, NULL, &m_RenderFinishedSemaphores[i]);
            assert(res == VK_SUCCESS);
        }

        // if SDR then use a gamma corrected swapchain so the blending is correct
        if (m_displayMode == DISPLAYMODE_SDR)
        {
            assert(m_swapChainFormat.format == VK_FORMAT_R8G8B8A8_UNORM);
            m_swapChainFormat.format = VK_FORMAT_R8G8B8A8_SRGB;
        }

        CreateRenderPass();
    }

    //--------------------------------------------------------------------------------------
    //
    // OnDestroy
    //
    //--------------------------------------------------------------------------------------
    void SwapChain::OnDestroy()
    {
        DestroyRenderPass();

        for (int i = 0; i < m_cmdBufExecutedFences.size(); i++)
        {
            vkDestroyFence(m_pDevice->GetDevice(), m_cmdBufExecutedFences[i], nullptr);
            vkDestroySemaphore(m_pDevice->GetDevice(), m_ImageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(m_pDevice->GetDevice(), m_RenderFinishedSemaphores[i], nullptr);
        }
    }

    //--------------------------------------------------------------------------------------
    //
    // EnumerateDisplayModes
    //
    //--------------------------------------------------------------------------------------
    void SwapChain::EnumerateDisplayModes(std::vector<DisplayMode> *pModes, std::vector<const char *> *pNames, bool includeFreesyncHDR, PresentationMode fullscreenMode, bool enableLocalDimming)
    {
        fsHdrEnumerateDisplayModes(pModes, includeFreesyncHDR, fullscreenMode, enableLocalDimming);

        if (pNames != NULL)
        {
            pNames->clear();
            for (DisplayMode mode : *pModes)
                pNames->push_back(fsHdrGetDisplayModeString(mode));
        }
    }

    //--------------------------------------------------------------------------------------
    //
    // EnumerateDisplayModes
    //
    //--------------------------------------------------------------------------------------
    bool SwapChain::IsModeSupported(DisplayMode displayMode, PresentationMode fullscreenMode, bool enableLocalDimming)
    {
        std::vector<DisplayMode> displayModesAvailable;
        EnumerateDisplayModes(&displayModesAvailable, nullptr, displayMode != DISPLAYMODE_SDR, fullscreenMode, enableLocalDimming);
        return  std::find(displayModesAvailable.begin(), displayModesAvailable.end(), displayMode) != displayModesAvailable.end();
    }

    VkImage SwapChain::GetCurrentBackBuffer()
    {
        return m_images[m_imageIndex];
    }

    VkImageView SwapChain::GetCurrentBackBufferRTV()
    {
        return m_imageViews[m_imageIndex];
    }

    uint32_t SwapChain::WaitForSwapChain()
    {
        vkAcquireNextImageKHR(m_pDevice->GetDevice(), m_swapChain, UINT64_MAX, m_ImageAvailableSemaphores[m_semaphoreIndex], VK_NULL_HANDLE, &m_imageIndex);

        m_prevSemaphoreIndex = m_semaphoreIndex;
        m_semaphoreIndex++;
        if (m_semaphoreIndex >= m_backBufferCount)
            m_semaphoreIndex = 0;

        vkWaitForFences(m_pDevice->GetDevice(), 1, &m_cmdBufExecutedFences[m_semaphoreIndex], VK_TRUE, UINT64_MAX);
        vkResetFences(m_pDevice->GetDevice(), 1, &m_cmdBufExecutedFences[m_semaphoreIndex]);

        return m_imageIndex;
    }

    void SwapChain::GetSemaphores(VkSemaphore *pImageAvailableSemaphore, VkSemaphore *pRenderFinishedSemaphores, VkFence *pCmdBufExecutedFences)
    {
        *pImageAvailableSemaphore = m_ImageAvailableSemaphores[m_prevSemaphoreIndex];
        *pRenderFinishedSemaphores = m_RenderFinishedSemaphores[m_semaphoreIndex];
        *pCmdBufExecutedFences = m_cmdBufExecutedFences[m_semaphoreIndex];
    }

    VkResult SwapChain::Present()
    {
        VkPresentInfoKHR present = {};
        present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present.pNext = NULL;
        present.waitSemaphoreCount = 1;
        present.pWaitSemaphores = &(m_RenderFinishedSemaphores[m_semaphoreIndex]);
        present.swapchainCount = 1;
        present.pSwapchains = &m_swapChain;
        present.pImageIndices = &m_imageIndex;
        present.pResults = NULL;

        VkResult res = vkQueuePresentKHR(m_presentQueue, &present);
        return res;
    }

    void SwapChain::SetFullScreen(bool fullscreen)
    {
        if (ExtAreFSEExtensionsPresent())
        {
            fsHdrSetFullscreenState(fullscreen, m_swapChain);
        }
    }

    void SwapChain::OnCreateWindowSizeDependentResources(uint32_t dwWidth, uint32_t dwHeight, bool bVSyncOn, DisplayMode displayMode, PresentationMode fullscreenMode, bool enableLocalDimming)
    {
        // check whether the requested mode is supported and fall back to SDR if not supported
        bool bIsModeSupported = IsModeSupported(displayMode, fullscreenMode, enableLocalDimming);
        if (bIsModeSupported == false)
        {
            assert(!"FSHDR display mode not supported");
            displayMode = DISPLAYMODE_SDR;
        }

        m_displayMode = displayMode;
        m_swapChainFormat = fsHdrGetFormat(displayMode);
        m_bVSyncOn = bVSyncOn;

        // if SDR then use a gamma corrected swapchain so the blending is correct
        if (m_displayMode == DISPLAYMODE_SDR)
        {
            // Query support for VK_FORMAT_R8G8B8A8_SRGB or VK_FORMAT_B8G8R8A8_SRGB
            uint32_t formatCount = 0;
            std::vector<VkSurfaceFormatKHR> surfaceFormats;
            vkGetPhysicalDeviceSurfaceFormatsKHR(m_pDevice->GetPhysicalDevice(), m_pDevice->GetSurface(), &formatCount, nullptr);

            if (formatCount != 0)
            {
                surfaceFormats.resize(formatCount);
                vkGetPhysicalDeviceSurfaceFormatsKHR(m_pDevice->GetPhysicalDevice(), m_pDevice->GetSurface(), &formatCount, &surfaceFormats[0]);
            }

            for (int i = 0; i < surfaceFormats.size(); i++)
            {
                if ((surfaceFormats[i].format == VK_FORMAT_R8G8B8A8_SRGB || surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB) && surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                {
                    m_swapChainFormat = surfaceFormats[i];
                    break;
                }
            }
        }

        VkResult res;
        VkDevice device = m_pDevice->GetDevice();
        VkPhysicalDevice physicaldevice = m_pDevice->GetPhysicalDevice();
        VkSurfaceKHR surface = m_pDevice->GetSurface();

        // redo the render passes for rendering using the new format
        DestroyRenderPass();
        CreateRenderPass();

        VkSurfaceCapabilitiesKHR surfCapabilities;
        res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicaldevice, surface, &surfCapabilities);
        assert(res == VK_SUCCESS);

        VkExtent2D swapchainExtent;
        swapchainExtent.width = dwWidth;
        swapchainExtent.height = dwHeight;

        // width and height are either both 0xFFFFFFFF, or both not 0xFFFFFFFF.
        if (surfCapabilities.currentExtent.width == 0xFFFFFFFF) {
            // If the surface size is undefined, the size is set to
            // the size of the images requested.
            if (swapchainExtent.width < surfCapabilities.minImageExtent.width)
            {
                swapchainExtent.width = surfCapabilities.minImageExtent.width;
            }
            else if (swapchainExtent.width > surfCapabilities.maxImageExtent.width)
            {
                swapchainExtent.width = surfCapabilities.maxImageExtent.width;
            }

            if (swapchainExtent.height < surfCapabilities.minImageExtent.height)
            {
                swapchainExtent.height = surfCapabilities.minImageExtent.height;
            }
            else if (swapchainExtent.height > surfCapabilities.maxImageExtent.height)
            {
                swapchainExtent.height = surfCapabilities.maxImageExtent.height;
            }
        }
        else
        {
            // If the surface size is defined, the swap chain size must match
            //swapchainExtent = surfCapabilities.currentExtent;
        }

        // Set identity transform
        VkSurfaceTransformFlagBitsKHR preTransform = (surfCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR : surfCapabilities.currentTransform;

        // Find a supported composite alpha mode - one of these is guaranteed to be set
        VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        VkCompositeAlphaFlagBitsKHR compositeAlphaFlags[4] =
        {
            VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
        };

        for (uint32_t i = 0; i < _countof(compositeAlphaFlags); i++)
        {
            if (surfCapabilities.supportedCompositeAlpha & compositeAlphaFlags[i])
            {
                compositeAlpha = compositeAlphaFlags[i];
                break;
            }
        }

        // Get present modes
        //
        uint32_t presentModeCount;
        res = vkGetPhysicalDeviceSurfacePresentModesKHR(physicaldevice, surface, &presentModeCount, NULL);
        assert(res == VK_SUCCESS);
        std::vector<VkPresentModeKHR> present_modes(presentModeCount);
        res = vkGetPhysicalDeviceSurfacePresentModesKHR(physicaldevice, surface, &presentModeCount, &present_modes[0]);
        assert(res == VK_SUCCESS);

        VkSwapchainCreateInfoKHR swapchain_ci = {};
        swapchain_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchain_ci.pNext = nullptr;
        if (ExtAreFSEExtensionsPresent())
        {
            // NOTE: what GetVkSurfaceFullScreenExclusiveInfoEXT returns is set up by the previous IsModeSupported call
            swapchain_ci.pNext = GetVkSurfaceFullScreenExclusiveInfoEXT();
        }
        swapchain_ci.surface = surface;
        swapchain_ci.imageFormat = m_swapChainFormat.format;
        swapchain_ci.minImageCount = m_backBufferCount;
        swapchain_ci.imageColorSpace = m_swapChainFormat.colorSpace;
        swapchain_ci.imageExtent.width = swapchainExtent.width;
        swapchain_ci.imageExtent.height = swapchainExtent.height;
        swapchain_ci.preTransform = preTransform;
        swapchain_ci.compositeAlpha = compositeAlpha;
        swapchain_ci.imageArrayLayers = 1;
        swapchain_ci.presentMode = m_bVSyncOn ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;  // The FIFO present mode is guaranteed by the spec to be supported
        swapchain_ci.oldSwapchain = VK_NULL_HANDLE;
        swapchain_ci.clipped = true;
        swapchain_ci.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchain_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_ci.queueFamilyIndexCount = 0;
        swapchain_ci.pQueueFamilyIndices = NULL;
        uint32_t queueFamilyIndices[2] = { m_pDevice->GetGraphicsQueueFamilyIndex(), m_pDevice->GetPresentQueueFamilyIndex() };
        if (queueFamilyIndices[0] != queueFamilyIndices[1])
        {
            // If the graphics and present queues are from different queue families,
            // we either have to explicitly transfer ownership of images between the
            // queues, or we have to create the swapchain with imageSharingMode
            // as VK_SHARING_MODE_CONCURRENT
            swapchain_ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swapchain_ci.queueFamilyIndexCount = 2;
            swapchain_ci.pQueueFamilyIndices = queueFamilyIndices;
        }

        res = vkCreateSwapchainKHR(device, &swapchain_ci, NULL, &m_swapChain);
        assert(res == VK_SUCCESS);

        // Get capabilities is only for FS HDR
        //
        if (displayMode == DISPLAYMODE_FSHDR_Gamma22 || displayMode == DISPLAYMODE_FSHDR_SCRGB)
        {
            fsHdrGetPhysicalDeviceSurfaceCapabilities2KHR(physicaldevice, surface, nullptr);
        }
        fsHdrSetDisplayMode(displayMode, m_swapChain);

        // we are querying the swapchain count so the next call doesn't generate a validation warning
        uint32_t backBufferCount;
        res = vkGetSwapchainImagesKHR(device, m_swapChain, &backBufferCount, NULL);
        assert(res == VK_SUCCESS);

        assert(backBufferCount == m_backBufferCount);

        m_images.resize(m_backBufferCount);
        res = vkGetSwapchainImagesKHR(device, m_swapChain, &m_backBufferCount, m_images.data());
        assert(res == VK_SUCCESS);

        CreateRTV();
        CreateFramebuffers(dwWidth, dwHeight);

        m_imageIndex = 0;
    }

    void SwapChain::OnDestroyWindowSizeDependentResources()
    {
        DestroyRenderPass();
        DestroyFramebuffers();
        DestroyRTV();
        vkDestroySwapchainKHR(m_pDevice->GetDevice(), m_swapChain, nullptr);
    }

    void SwapChain::CreateRenderPass()
    {
        // color RT
        VkAttachmentDescription attachments[1];
        attachments[0].format = m_swapChainFormat.format;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        attachments[0].flags = 0;

        VkAttachmentReference color_reference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.flags = 0;
        subpass.inputAttachmentCount = 0;
        subpass.pInputAttachments = NULL;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_reference;
        subpass.pResolveAttachments = NULL;
        subpass.pDepthStencilAttachment = NULL;
        subpass.preserveAttachmentCount = 0;
        subpass.pPreserveAttachments = NULL;

        VkSubpassDependency dep = {};
        dep.dependencyFlags = 0;
        dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dep.dstSubpass = 0;
        dep.srcAccessMask = 0;
        dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dep.srcSubpass = VK_SUBPASS_EXTERNAL;

        VkRenderPassCreateInfo rp_info = {};
        rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rp_info.pNext = NULL;
        rp_info.attachmentCount = 1;
        rp_info.pAttachments = attachments;
        rp_info.subpassCount = 1;
        rp_info.pSubpasses = &subpass;
        rp_info.dependencyCount = 1;
        rp_info.pDependencies = &dep;

        VkResult res = vkCreateRenderPass(m_pDevice->GetDevice(), &rp_info, NULL, &m_render_pass_swap_chain);
        assert(res == VK_SUCCESS);
    }

    void SwapChain::DestroyRenderPass()
    {
        if (m_render_pass_swap_chain != VK_NULL_HANDLE)
        {
            vkDestroyRenderPass(m_pDevice->GetDevice(), m_render_pass_swap_chain, nullptr);
            m_render_pass_swap_chain = VK_NULL_HANDLE;
        }
    }

    void SwapChain::CreateRTV()
    {
        m_imageViews.resize(m_images.size());
        for (uint32_t i = 0; i < m_images.size(); i++)
        {
            VkImageViewCreateInfo color_image_view = {};
            color_image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            color_image_view.pNext = NULL;
            color_image_view.format = m_swapChainFormat.format;
            color_image_view.components.r = VK_COMPONENT_SWIZZLE_R;
            color_image_view.components.g = VK_COMPONENT_SWIZZLE_G;
            color_image_view.components.b = VK_COMPONENT_SWIZZLE_B;
            color_image_view.components.a = VK_COMPONENT_SWIZZLE_A;
            color_image_view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            color_image_view.subresourceRange.baseMipLevel = 0;
            color_image_view.subresourceRange.levelCount = 1;
            color_image_view.subresourceRange.baseArrayLayer = 0;
            color_image_view.subresourceRange.layerCount = 1;
            color_image_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
            color_image_view.flags = 0;
            color_image_view.image = m_images[i];

            VkResult res = vkCreateImageView(m_pDevice->GetDevice(), &color_image_view, NULL, &m_imageViews[i]);
            assert(res == VK_SUCCESS);

            SetResourceName(m_pDevice->GetDevice(), VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)m_imageViews[i], "Swapchain");
        }
    }

    void SwapChain::DestroyRTV()
    {
        for (uint32_t i = 0; i < m_imageViews.size(); i++)
        {
            vkDestroyImageView(m_pDevice->GetDevice(), m_imageViews[i], nullptr);
        }
    }

    void SwapChain::CreateFramebuffers(uint32_t dwWidth, uint32_t dwHeight)
    {
        m_framebuffers.resize(m_imageViews.size());
        for (uint32_t i = 0; i < m_imageViews.size(); i++)
        {
            VkImageView attachments[] = { m_imageViews[i] };

            VkFramebufferCreateInfo fb_info = {};
            fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fb_info.pNext = NULL;
            fb_info.renderPass = m_render_pass_swap_chain;
            fb_info.attachmentCount = 1;
            fb_info.pAttachments = attachments;
            fb_info.width = dwWidth;
            fb_info.height = dwHeight;
            fb_info.layers = 1;

            VkResult res = vkCreateFramebuffer(m_pDevice->GetDevice(), &fb_info, NULL, &m_framebuffers[i]);
            assert(res == VK_SUCCESS);

            SetResourceName(m_pDevice->GetDevice(), VK_OBJECT_TYPE_FRAMEBUFFER, (uint64_t)m_framebuffers[i], "Swapchain");
        }
    }

    void SwapChain::DestroyFramebuffers()
    {
        for (uint32_t i = 0; i < m_framebuffers.size(); i++)
        {
            vkDestroyFramebuffer(m_pDevice->GetDevice(), m_framebuffers[i], nullptr);
        }
    }
}
