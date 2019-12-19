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
#pragma once

#include "DeviceProperties.h"
#include "InstanceProperties.h"

namespace CAULDRON_VK
{
    enum DisplayModes
    {
        DISPLAYMODE_SDR,
        DISPLAYMODE_FS2_Gamma22,
        DISPLAYMODE_FS2_SCRGB,
        DISPLAYMODE_HDR10_2084,
        DISPLAYMODE_HDR10_SCRGB
    };

    // only the swapchain should be using these functions

    bool fs2Init(VkDevice device, VkSurfaceKHR surface, VkPhysicalDevice physicalDevice, HWND hWnd);
    bool fs2EnumerateDisplayModes(std::vector<DisplayModes> *pModes);
    VkSurfaceFormatKHR fs2GetFormat(DisplayModes displayMode);
    bool fs2SetDisplayMode(DisplayModes displayMode, VkSwapchainKHR swapChain);
    const char *fs2GetDisplayModeString(DisplayModes displayMode);
    const VkHdrMetadataEXT* fs2GetDisplayInfo();

    void fs2SetLocalDimmingMode(VkSwapchainKHR swapchain, VkBool32 localDimmingEnable);
    void fs2SetFullscreenState(bool fullscreen, VkSwapchainKHR swapchain);

    void fs2GetPhysicalDeviceSurfaceCapabilities2KHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR *pSurfCapabilities);
    VkSwapchainDisplayNativeHdrCreateInfoAMD* GetVkSwapchainDisplayNativeHdrCreateInfoAMD();
}
