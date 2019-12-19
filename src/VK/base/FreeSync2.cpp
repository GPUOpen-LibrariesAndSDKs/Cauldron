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
#include "stdafx.h"

#include "ExtFreeSync2.h"
#include "FreeSync2.h"
#include "Misc/Misc.h"
#include "Misc/Error.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

namespace CAULDRON_VK
{
    static VkSurfaceFullScreenExclusiveWin32InfoEXT s_SurfaceFullScreenExclusiveWin32InfoEXT;
    static VkSurfaceFullScreenExclusiveInfoEXT s_SurfaceFullScreenExclusiveInfoEXT;
    static VkPhysicalDeviceSurfaceInfo2KHR s_PhysicalDeviceSurfaceInfo2KHR;
    static VkDisplayNativeHdrSurfaceCapabilitiesAMD s_DisplayNativeHdrSurfaceCapabilitiesAMD;

    static VkHdrMetadataEXT s_HdrMetadataEXT;
    static VkSurfaceCapabilities2KHR s_SurfaceCapabilities2KHR;
    static VkSwapchainDisplayNativeHdrCreateInfoAMD s_SwapchainDisplayNativeHdrCreateInfoAMD;

    static VkDevice s_device;
    static VkSurfaceKHR s_surface;
    static VkPhysicalDevice s_physicalDevice;
    static bool s_isfullScreen;
    static HWND s_hWnd = NULL;

    void SetFreesync2Structures(HWND hWnd, VkSurfaceKHR surface)
    {
        s_SurfaceFullScreenExclusiveWin32InfoEXT.sType = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_WIN32_INFO_EXT;
        s_SurfaceFullScreenExclusiveWin32InfoEXT.pNext = nullptr;
        s_SurfaceFullScreenExclusiveWin32InfoEXT.hmonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY);

        s_SurfaceFullScreenExclusiveInfoEXT.sType = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT;
        s_SurfaceFullScreenExclusiveInfoEXT.pNext = &s_SurfaceFullScreenExclusiveWin32InfoEXT;
        s_SurfaceFullScreenExclusiveInfoEXT.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT;

        s_PhysicalDeviceSurfaceInfo2KHR.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR;
        s_PhysicalDeviceSurfaceInfo2KHR.pNext = &s_SurfaceFullScreenExclusiveInfoEXT;
        s_PhysicalDeviceSurfaceInfo2KHR.surface = surface;
    }

    void SetCapabilitiesStructs()
    {
        s_HdrMetadataEXT.sType = VK_STRUCTURE_TYPE_HDR_METADATA_EXT;
        s_HdrMetadataEXT.pNext = nullptr;

        s_DisplayNativeHdrSurfaceCapabilitiesAMD.sType = VK_STRUCTURE_TYPE_DISPLAY_NATIVE_HDR_SURFACE_CAPABILITIES_AMD;
        s_DisplayNativeHdrSurfaceCapabilitiesAMD.pNext = &s_HdrMetadataEXT;

        s_SurfaceCapabilities2KHR.sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR;
        s_SurfaceCapabilities2KHR.pNext = &s_DisplayNativeHdrSurfaceCapabilitiesAMD;
    }

    void SetFreesync2SwapchainStructure()
    {
        s_SwapchainDisplayNativeHdrCreateInfoAMD.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_DISPLAY_NATIVE_HDR_CREATE_INFO_AMD;
        s_SwapchainDisplayNativeHdrCreateInfoAMD.pNext = &s_SurfaceFullScreenExclusiveInfoEXT;
        s_SwapchainDisplayNativeHdrCreateInfoAMD.localDimmingEnable = s_DisplayNativeHdrSurfaceCapabilitiesAMD.localDimmingSupport;
    }

    VkSwapchainDisplayNativeHdrCreateInfoAMD* GetVkSwapchainDisplayNativeHdrCreateInfoAMD()
    {
        return &s_SwapchainDisplayNativeHdrCreateInfoAMD;
    }

    //--------------------------------------------------------------------------------------
    //
    // fs2Init, if it returns false the fs2 extensions are not present
    //
    //--------------------------------------------------------------------------------------
    bool fs2Init(VkDevice device, VkSurfaceKHR surface, VkPhysicalDevice physicalDevice, HWND hWnd)
    {
        s_hWnd = hWnd;
        s_device = device;
        s_surface = surface;
        s_physicalDevice = physicalDevice;
        s_isfullScreen = false;

        return ExtFreeSync2AreAllExtensionsPresent();       
    }

    //--------------------------------------------------------------------------------------
    //
    // fs2EnumerateDisplayModes, enumerates availabe modes
    //
    //--------------------------------------------------------------------------------------
    bool fs2EnumerateDisplayModes(std::vector<DisplayModes> *pModes)
    {
        pModes->clear();
        pModes->push_back(DISPLAYMODE_SDR);

        if (ExtFreeSync2AreAllExtensionsPresent() == false)
            return true;

        // Note we are getting the caps of the fullscreen modes, that is where FS2 really works.
        SetFreesync2Structures(s_hWnd, s_surface);
        SetCapabilitiesStructs();
        fs2GetPhysicalDeviceSurfaceCapabilities2KHR(s_physicalDevice, s_surface, NULL);
        SetFreesync2SwapchainStructure();

        // Get the list of formats
        //
        uint32_t formatCount;
        VkResult res = g_vkGetPhysicalDeviceSurfaceFormats2KHR(s_physicalDevice, &s_PhysicalDeviceSurfaceInfo2KHR, &formatCount, NULL);
        assert(res == VK_SUCCESS);
        if (res != VK_SUCCESS)
            return false;

        std::vector<VkSurfaceFormat2KHR> surfFormats(formatCount);
        for (UINT i = 0; i < formatCount; ++i)
        {
            surfFormats[i].sType = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR;
        }
        res = g_vkGetPhysicalDeviceSurfaceFormats2KHR(s_physicalDevice, &s_PhysicalDeviceSurfaceInfo2KHR, &formatCount, surfFormats.data());
        assert(res == VK_SUCCESS);
        if (res != VK_SUCCESS)
            return false;

        for (uint32_t i = 0; i < formatCount; i++)
        {
            if (surfFormats[i].surfaceFormat.format == VK_FORMAT_A2R10G10B10_UNORM_PACK32 &&
                surfFormats[i].surfaceFormat.colorSpace == VK_COLOR_SPACE_DISPLAY_NATIVE_AMD)
            {
                pModes->push_back(DISPLAYMODE_FS2_Gamma22);
                pModes->push_back(DISPLAYMODE_FS2_SCRGB);
                break;
            }
        }

        for (uint32_t i = 0; i < formatCount; i++)
        {
            if (surfFormats[i].surfaceFormat.format == VK_FORMAT_A2R10G10B10_UNORM_PACK32 &&
                surfFormats[i].surfaceFormat.colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT)
            {
                pModes->push_back(DISPLAYMODE_HDR10_2084);
                pModes->push_back(DISPLAYMODE_HDR10_SCRGB);
                break;
            }
        }

        return true;
    }

    //--------------------------------------------------------------------------------------
    //
    // fs2GetDisplayModeString
    //
    //--------------------------------------------------------------------------------------
    const char *fs2GetDisplayModeString(DisplayModes displayMode)
    {
        // note that these string must match the order of the DisplayModes enum
        const char *DisplayModesStrings[]
        {
            "SDR",
            "FS2_Gamma22",
            "FS2_SCRGB",
            "HDR10_2084",
            "HDR10_SCRGB"
        };

        return DisplayModesStrings[displayMode];
    }

    //--------------------------------------------------------------------------------------
    //
    // fs2GetFormat
    //
    //--------------------------------------------------------------------------------------
    VkSurfaceFormatKHR fs2GetFormat(DisplayModes displayMode)
    {
        VkSurfaceFormatKHR surfaceFormat;

        switch (displayMode)
        {
        case DISPLAYMODE_SDR:
            surfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
            surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
            break;

        case DISPLAYMODE_FS2_Gamma22:
            surfaceFormat.format = VK_FORMAT_A2R10G10B10_UNORM_PACK32;
            surfaceFormat.colorSpace = VK_COLOR_SPACE_DISPLAY_NATIVE_AMD;
            break;

        case DISPLAYMODE_FS2_SCRGB:
            surfaceFormat.format = VK_FORMAT_R16G16B16A16_SFLOAT;
            surfaceFormat.colorSpace = VK_COLOR_SPACE_DISPLAY_NATIVE_AMD;
            break;

        case DISPLAYMODE_HDR10_2084:
            surfaceFormat.format = VK_FORMAT_A2R10G10B10_UNORM_PACK32;
            surfaceFormat.colorSpace = VK_COLOR_SPACE_HDR10_ST2084_EXT;
            break;

        case DISPLAYMODE_HDR10_SCRGB:
            surfaceFormat.format = VK_FORMAT_R16G16B16A16_SFLOAT;
            surfaceFormat.colorSpace = VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT;
            break;
        }

        return surfaceFormat;
    }

    //--------------------------------------------------------------------------------------
    //
    // fs2SetDisplayMode
    //
    //--------------------------------------------------------------------------------------
    bool fs2SetDisplayMode(DisplayModes displayMode, VkSwapchainKHR swapChain)
    {
        if (ExtFreeSync2AreAllExtensionsPresent() == false)
            return false;

        switch (displayMode)
        {
        case DISPLAYMODE_SDR:
            s_HdrMetadataEXT.displayPrimaryRed.x = 0.64f;
            s_HdrMetadataEXT.displayPrimaryRed.y = 0.33f;
            s_HdrMetadataEXT.displayPrimaryGreen.x = 0.30f;
            s_HdrMetadataEXT.displayPrimaryGreen.y = 0.60f;
            s_HdrMetadataEXT.displayPrimaryBlue.x = 0.15f;
            s_HdrMetadataEXT.displayPrimaryBlue.y = 0.06f;
            s_HdrMetadataEXT.minLuminance = 0.0f;
            s_HdrMetadataEXT.maxLuminance = 300.0f;
            break;
        case DISPLAYMODE_FS2_Gamma22:
            // use the values that we queried using VkGetPhysicalDeviceSurfaceCapabilities2KHR
            break;
        case DISPLAYMODE_FS2_SCRGB:
            // use the values that we queried using VkGetPhysicalDeviceSurfaceCapabilities2KHR
            break;
        case DISPLAYMODE_HDR10_2084:
            s_HdrMetadataEXT.displayPrimaryRed.x = 0.708f;
            s_HdrMetadataEXT.displayPrimaryRed.y = 0.292f;
            s_HdrMetadataEXT.displayPrimaryGreen.x = 0.170f;
            s_HdrMetadataEXT.displayPrimaryGreen.y = 0.797f;
            s_HdrMetadataEXT.displayPrimaryBlue.x = 0.131f;
            s_HdrMetadataEXT.displayPrimaryBlue.y = 0.046f;
            s_HdrMetadataEXT.minLuminance = 0.0f;
            s_HdrMetadataEXT.maxLuminance = 1000.0f; // This will cause tonemapping to happen on display end as long as it's greater than display's actual queried max luminance. The look will change and it will be display dependent!
            s_HdrMetadataEXT.maxContentLightLevel = 1000.0f;
            s_HdrMetadataEXT.maxFrameAverageLightLevel = 400.0f; // max and average content light level data will be used to do tonemapping on display
            break;
        case DISPLAYMODE_HDR10_SCRGB:
            s_HdrMetadataEXT.displayPrimaryRed.x = 0.708f;
            s_HdrMetadataEXT.displayPrimaryRed.y = 0.292f;
            s_HdrMetadataEXT.displayPrimaryGreen.x = 0.170f;
            s_HdrMetadataEXT.displayPrimaryGreen.y = 0.797f;
            s_HdrMetadataEXT.displayPrimaryBlue.x = 0.131f;
            s_HdrMetadataEXT.displayPrimaryBlue.y = 0.046f;
            s_HdrMetadataEXT.minLuminance = 0.0f;
            s_HdrMetadataEXT.maxLuminance = 1000.0f; // This will cause tonemapping to happen on display end as long as it's greater than display's actual queried max luminance. The look will change and it will be display dependent!
            s_HdrMetadataEXT.maxContentLightLevel = 1000.0f;
            s_HdrMetadataEXT.maxFrameAverageLightLevel = 400.0f; // max and average content light level data will be used to do tonemapping on display
            break;
        }

        g_vkSetHdrMetadataEXT(s_device, 1, &swapChain, &s_HdrMetadataEXT);

        return true;
    }

    //--------------------------------------------------------------------------------------
    //
    // fs2SetLocalDimmingMode
    //
    //--------------------------------------------------------------------------------------
    void fs2SetLocalDimmingMode(VkSwapchainKHR swapchain, VkBool32 localDimmingEnable)
    {
        g_vkSetLocalDimmingAMD(s_device, swapchain, localDimmingEnable);
        VkResult res = g_vkGetPhysicalDeviceSurfaceCapabilities2KHR(s_physicalDevice, &s_PhysicalDeviceSurfaceInfo2KHR, &s_SurfaceCapabilities2KHR);
        assert(res == VK_SUCCESS);
        g_vkSetHdrMetadataEXT(s_device, 1, &swapchain, &s_HdrMetadataEXT);
    }

    //--------------------------------------------------------------------------------------
    //
    // fs2SetFullscreenState
    //
    //--------------------------------------------------------------------------------------
    void fs2SetFullscreenState(bool fullscreen, VkSwapchainKHR swapchain)
    {
        if (fullscreen)
        {
            VkResult res = g_vkAcquireFullScreenExclusiveModeEXT(s_device, swapchain);
            assert(res == VK_SUCCESS);
        }
        else
        {
            VkResult res = g_vkReleaseFullScreenExclusiveModeEXT(s_device, swapchain);
            assert(res == VK_SUCCESS);
        }
        s_isfullScreen = fullscreen;
    }

    //--------------------------------------------------------------------------------------
    //
    // fs2GetDisplayInfo
    //
    //--------------------------------------------------------------------------------------
    const VkHdrMetadataEXT* fs2GetDisplayInfo()
    {
        return &s_HdrMetadataEXT;
    }

    void fs2GetPhysicalDeviceSurfaceCapabilities2KHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR *pSurfCapabilities)
    {
        assert(surface == s_surface);
        assert(physicalDevice == s_physicalDevice);

        VkResult res = g_vkGetPhysicalDeviceSurfaceCapabilities2KHR(s_physicalDevice, &s_PhysicalDeviceSurfaceInfo2KHR, &s_SurfaceCapabilities2KHR);
        assert(res == VK_SUCCESS);

        if (pSurfCapabilities)
            *pSurfCapabilities = s_SurfaceCapabilities2KHR.surfaceCapabilities;
    }
}
