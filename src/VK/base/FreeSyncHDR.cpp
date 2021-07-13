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

#include "ExtFreeSyncHDR.h"
#include "FreeSyncHDR.h"
#include "Misc/Misc.h"
#include "Misc/Error.h"
#include <vulkan/vulkan.h>

#include <unordered_map>

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
    static HWND s_hWnd = NULL;

    static std::unordered_map<DisplayMode, VkSurfaceFormatKHR> availableDisplayModeSurfaceformats;

    bool s_windowsHdrtoggle = false;

    void SetHDRStructs(VkSurfaceKHR surface)
    {
        s_PhysicalDeviceSurfaceInfo2KHR.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR;
        s_PhysicalDeviceSurfaceInfo2KHR.pNext = nullptr;
        s_PhysicalDeviceSurfaceInfo2KHR.surface = surface;

        s_HdrMetadataEXT.sType = VK_STRUCTURE_TYPE_HDR_METADATA_EXT;
        s_HdrMetadataEXT.pNext = nullptr;
    }

    void SetFSEStructures(HWND hWnd, PresentationMode fullscreenMode)
    {
        // Required final chaining order
        // VkPhysicalDeviceSurfaceInfo2KHR -> VkSurfaceFullScreenExclusiveInfoEXT -> VkSurfaceFullScreenExclusiveWin32InfoEXT

        s_PhysicalDeviceSurfaceInfo2KHR.pNext = &s_SurfaceFullScreenExclusiveInfoEXT;
        s_SurfaceFullScreenExclusiveInfoEXT.pNext = &s_SurfaceFullScreenExclusiveWin32InfoEXT;

        s_SurfaceFullScreenExclusiveInfoEXT.sType = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT;
        if (fullscreenMode == PRESENTATIONMODE_WINDOWED)
            s_SurfaceFullScreenExclusiveInfoEXT.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_DISALLOWED_EXT;
        else if (fullscreenMode == PRESENTATIONMODE_BORDERLESS_FULLSCREEN)
            s_SurfaceFullScreenExclusiveInfoEXT.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_ALLOWED_EXT;
        else if (fullscreenMode == PRESENTATIONMODE_EXCLUSIVE_FULLSCREEN)
            s_SurfaceFullScreenExclusiveInfoEXT.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT;

        s_SurfaceFullScreenExclusiveWin32InfoEXT.sType = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_WIN32_INFO_EXT;
        s_SurfaceFullScreenExclusiveWin32InfoEXT.pNext = nullptr;
        s_SurfaceFullScreenExclusiveWin32InfoEXT.hmonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
    }

    void SetFreesyncHDRStructures()
    {
        // Required final chaning order
        // VkSurfaceCapabilities2KHR -> VkDisplayNativeHdrSurfaceCapabilitiesAMD -> VkHdrMetadataEXT

        s_SurfaceCapabilities2KHR.sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR;
        s_SurfaceCapabilities2KHR.pNext = &s_DisplayNativeHdrSurfaceCapabilitiesAMD;

        // This will cause validation error and complain as invalid structure attached.
        // But we are hijacking hdr metadata struct and attaching it here to fill it with monitor primaries
        // When getting surface capabilities
        s_DisplayNativeHdrSurfaceCapabilitiesAMD.sType = VK_STRUCTURE_TYPE_DISPLAY_NATIVE_HDR_SURFACE_CAPABILITIES_AMD;
        s_DisplayNativeHdrSurfaceCapabilitiesAMD.pNext = &s_HdrMetadataEXT;
    }

    void SetSwapchainFreesyncHDRStructures(bool enableLocalDimming)
    {
        // Required final chaning order
        // VkSurfaceFullScreenExclusiveInfoEXT -> VkSurfaceFullScreenExclusiveWin32InfoEXT -> VkSwapchainDisplayNativeHdrCreateInfoAMD
        s_SurfaceFullScreenExclusiveWin32InfoEXT.pNext = &s_SwapchainDisplayNativeHdrCreateInfoAMD;

        s_SwapchainDisplayNativeHdrCreateInfoAMD.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_DISPLAY_NATIVE_HDR_CREATE_INFO_AMD;
        s_SwapchainDisplayNativeHdrCreateInfoAMD.pNext = nullptr;
        // Enable local dimming if supported
        // Must requry FS HDR display capabilities
        // After value is set through swapchain creation when attached to swapchain pnext.
        s_SwapchainDisplayNativeHdrCreateInfoAMD.localDimmingEnable = s_DisplayNativeHdrSurfaceCapabilitiesAMD.localDimmingSupport && enableLocalDimming;
    }

    VkSurfaceFullScreenExclusiveInfoEXT* GetVkSurfaceFullScreenExclusiveInfoEXT()
    {
        return &s_SurfaceFullScreenExclusiveInfoEXT;
    }

    void GetSurfaceFormats(uint32_t *pFormatCount, std::vector<VkSurfaceFormat2KHR> *surfFormats)
    {
        // Get the list of formats
        //
        VkResult res = g_vkGetPhysicalDeviceSurfaceFormats2KHR(s_physicalDevice, &s_PhysicalDeviceSurfaceInfo2KHR, pFormatCount, NULL);
        assert(res == VK_SUCCESS);

        uint32_t formatCount = *pFormatCount;
        surfFormats->resize(formatCount);
        for (UINT i = 0; i < formatCount; ++i)
        {
            (*surfFormats)[i].sType = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR;
        }

        res = g_vkGetPhysicalDeviceSurfaceFormats2KHR(s_physicalDevice, &s_PhysicalDeviceSurfaceInfo2KHR, &formatCount, (*surfFormats).data());
        assert(res == VK_SUCCESS);
    }

    //--------------------------------------------------------------------------------------
    //
    // fsHdrInit, if it returns false the fsHdr extensions are not present
    //
    //--------------------------------------------------------------------------------------
    bool fsHdrInit(VkDevice device, VkSurfaceKHR surface, VkPhysicalDevice physicalDevice, HWND hWnd)
    {
        s_hWnd = hWnd;
        s_device = device;
        s_surface = surface;
        s_physicalDevice = physicalDevice;

        return ExtAreFreeSyncHDRExtensionsPresent();
    }

    //--------------------------------------------------------------------------------------
    //
    // fsHdrEnumerateDisplayModes, enumerates availabe modes
    //
    //--------------------------------------------------------------------------------------
    bool fsHdrEnumerateDisplayModes(std::vector<DisplayMode>* pModes, bool includeFreesyncHDR, PresentationMode fullscreenMode, bool enableLocalDimming)
    {
        pModes->clear();
        availableDisplayModeSurfaceformats.clear();

        VkSurfaceFormatKHR surfaceFormat;
        surfaceFormat.format = VK_FORMAT_R8G8B8A8_UNORM;
        surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

        pModes->push_back(DISPLAYMODE_SDR);
        availableDisplayModeSurfaceformats[DISPLAYMODE_SDR] = surfaceFormat;

        if (ExtAreHDRExtensionsPresent())
        {
            SetHDRStructs(s_surface);

            uint32_t formatCount;
            std::vector<VkSurfaceFormat2KHR> surfFormats;
            GetSurfaceFormats(&formatCount, &surfFormats);

            for (uint32_t i = 0; i < formatCount; i++)
            {
                if ((surfFormats[i].surfaceFormat.format == VK_FORMAT_A2R10G10B10_UNORM_PACK32 &&
                    surfFormats[i].surfaceFormat.colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT)
                    ||
                    (surfFormats[i].surfaceFormat.format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 &&
                    surfFormats[i].surfaceFormat.colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT))
                {
                    // If surface formats have HDR10 format even before fullscreen surface is attached, it can only mean windows hdr toggle is on
                    s_windowsHdrtoggle = true;
                    break;
                }
            }
        } else
        {
            s_PhysicalDeviceSurfaceInfo2KHR.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR;
            s_PhysicalDeviceSurfaceInfo2KHR.pNext = nullptr;
            s_PhysicalDeviceSurfaceInfo2KHR.surface = s_surface;
        }

        if (ExtAreFSEExtensionsPresent())
        {
            SetFSEStructures(s_hWnd, fullscreenMode);
        }

        if (includeFreesyncHDR && ExtAreFreeSyncHDRExtensionsPresent())
        {
            SetFreesyncHDRStructures();

            // Calling get capabilities here to query for local dimming support
            fsHdrGetPhysicalDeviceSurfaceCapabilities2KHR(s_physicalDevice, s_surface, NULL);

            SetSwapchainFreesyncHDRStructures(enableLocalDimming);
        }

        uint32_t formatCount;
        std::vector<VkSurfaceFormat2KHR> surfFormats;
        GetSurfaceFormats(&formatCount, &surfFormats);

        for (uint32_t i = 0; i < formatCount; i++)
        {
            if (surfFormats[i].surfaceFormat.format == VK_FORMAT_A2R10G10B10_UNORM_PACK32 &&
                surfFormats[i].surfaceFormat.colorSpace == VK_COLOR_SPACE_DISPLAY_NATIVE_AMD)
            {
                pModes->push_back(DISPLAYMODE_FSHDR_Gamma22);
                availableDisplayModeSurfaceformats[DISPLAYMODE_FSHDR_Gamma22] = surfFormats[i].surfaceFormat;
            }
            else if (surfFormats[i].surfaceFormat.format == VK_FORMAT_R16G16B16A16_SFLOAT &&
                     surfFormats[i].surfaceFormat.colorSpace == VK_COLOR_SPACE_DISPLAY_NATIVE_AMD)
            {
                pModes->push_back(DISPLAYMODE_FSHDR_SCRGB);
                availableDisplayModeSurfaceformats[DISPLAYMODE_FSHDR_SCRGB] = surfFormats[i].surfaceFormat;
            }
        }

        for (uint32_t i = 0; i < formatCount; i++)
        {
            if ((surfFormats[i].surfaceFormat.format == VK_FORMAT_A2R10G10B10_UNORM_PACK32 &&
                 surfFormats[i].surfaceFormat.colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT)
                 ||
                (surfFormats[i].surfaceFormat.format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 &&
                 surfFormats[i].surfaceFormat.colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT))
            {
                if (availableDisplayModeSurfaceformats.find(DISPLAYMODE_HDR10_2084) == availableDisplayModeSurfaceformats.end())
                {
                    pModes->push_back(DISPLAYMODE_HDR10_2084);
                    availableDisplayModeSurfaceformats[DISPLAYMODE_HDR10_2084] = surfFormats[i].surfaceFormat;
                }
            }
            else if (surfFormats[i].surfaceFormat.format == VK_FORMAT_R16G16B16A16_SFLOAT &&
                     surfFormats[i].surfaceFormat.colorSpace == VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT)
            {
                pModes->push_back(DISPLAYMODE_HDR10_SCRGB);
                availableDisplayModeSurfaceformats[DISPLAYMODE_HDR10_SCRGB] = surfFormats[i].surfaceFormat;
            }
        }

        return true;
    }

    //--------------------------------------------------------------------------------------
    //
    // fsHdrGetDisplayModeString
    //
    //--------------------------------------------------------------------------------------
    const char *fsHdrGetDisplayModeString(DisplayMode displayMode)
    {
        // note that these string must match the order of the DisplayModes enum
        const char *DisplayModesStrings[]
        {
            "SDR",
            "FSHDR_Gamma22",
            "FSHDR_SCRGB",
            "HDR10_2084",
            "HDR10_SCRGB"
        };

        return DisplayModesStrings[displayMode];
    }

    //--------------------------------------------------------------------------------------
    //
    // fsHdrGetFormat
    //
    //--------------------------------------------------------------------------------------
    VkSurfaceFormatKHR fsHdrGetFormat(DisplayMode displayMode)
    {
        VkSurfaceFormatKHR surfaceFormat;

        surfaceFormat = availableDisplayModeSurfaceformats[displayMode];

        return surfaceFormat;
    }

    //--------------------------------------------------------------------------------------
    //
    // fsHdrSetDisplayMode
    //
    //--------------------------------------------------------------------------------------
    bool fsHdrSetDisplayMode(DisplayMode displayMode, VkSwapchainKHR swapChain)
    {
        if (!ExtAreHDRExtensionsPresent())
            return false;

        switch (displayMode)
        {
        case DISPLAYMODE_SDR:
            // rec 709 primaries
            s_HdrMetadataEXT.displayPrimaryRed.x = 0.64f;
            s_HdrMetadataEXT.displayPrimaryRed.y = 0.33f;
            s_HdrMetadataEXT.displayPrimaryGreen.x = 0.30f;
            s_HdrMetadataEXT.displayPrimaryGreen.y = 0.60f;
            s_HdrMetadataEXT.displayPrimaryBlue.x = 0.15f;
            s_HdrMetadataEXT.displayPrimaryBlue.y = 0.06f;
            s_HdrMetadataEXT.whitePoint.x = 0.3127f;
            s_HdrMetadataEXT.whitePoint.y = 0.3290f;
            s_HdrMetadataEXT.minLuminance = 0.0f;
            s_HdrMetadataEXT.maxLuminance = 300.0f;
            break;
        case DISPLAYMODE_FSHDR_Gamma22:
            // use the values that we queried using VkGetPhysicalDeviceSurfaceCapabilities2KHR
            break;
        case DISPLAYMODE_FSHDR_SCRGB:
            // use the values that we queried using VkGetPhysicalDeviceSurfaceCapabilities2KHR
            break;
        case DISPLAYMODE_HDR10_2084:
            // rec 2020 primaries
            s_HdrMetadataEXT.displayPrimaryRed.x = 0.708f;
            s_HdrMetadataEXT.displayPrimaryRed.y = 0.292f;
            s_HdrMetadataEXT.displayPrimaryGreen.x = 0.170f;
            s_HdrMetadataEXT.displayPrimaryGreen.y = 0.797f;
            s_HdrMetadataEXT.displayPrimaryBlue.x = 0.131f;
            s_HdrMetadataEXT.displayPrimaryBlue.y = 0.046f;
            s_HdrMetadataEXT.whitePoint.x = 0.3127f;
            s_HdrMetadataEXT.whitePoint.y = 0.3290f;
            s_HdrMetadataEXT.minLuminance = 0.0f;
            s_HdrMetadataEXT.maxLuminance = 1000.0f; // This will cause tonemapping to happen on display end as long as it's greater than display's actual queried max luminance. The look will change and it will be display dependent!
            s_HdrMetadataEXT.maxContentLightLevel = 1000.0f;
            s_HdrMetadataEXT.maxFrameAverageLightLevel = 400.0f; // max and average content light level data will be used to do tonemapping on display
            break;
        case DISPLAYMODE_HDR10_SCRGB:
            // rec 709 primaries
            s_HdrMetadataEXT.displayPrimaryRed.x = 0.64f;
            s_HdrMetadataEXT.displayPrimaryRed.y = 0.33f;
            s_HdrMetadataEXT.displayPrimaryGreen.x = 0.30f;
            s_HdrMetadataEXT.displayPrimaryGreen.y = 0.60f;
            s_HdrMetadataEXT.displayPrimaryBlue.x = 0.15f;
            s_HdrMetadataEXT.displayPrimaryBlue.y = 0.06f;
            s_HdrMetadataEXT.whitePoint.x = 0.3127f;
            s_HdrMetadataEXT.whitePoint.y = 0.3290f;
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
    // fsHdrSetLocalDimmingMode
    //
    //--------------------------------------------------------------------------------------
    void fsHdrSetLocalDimmingMode(VkSwapchainKHR swapchain, VkBool32 localDimmingEnable)
    {
        g_vkSetLocalDimmingAMD(s_device, swapchain, localDimmingEnable);
        VkResult res = g_vkGetPhysicalDeviceSurfaceCapabilities2KHR(s_physicalDevice, &s_PhysicalDeviceSurfaceInfo2KHR, &s_SurfaceCapabilities2KHR);
        assert(res == VK_SUCCESS);
        g_vkSetHdrMetadataEXT(s_device, 1, &swapchain, &s_HdrMetadataEXT);
    }

    //--------------------------------------------------------------------------------------
    //
    // fsHdrSetFullscreenState
    //
    //--------------------------------------------------------------------------------------
    void fsHdrSetFullscreenState(bool fullscreen, VkSwapchainKHR swapchain)
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
    }

    //--------------------------------------------------------------------------------------
    //
    // fsHdrGetDisplayInfo
    //
    //--------------------------------------------------------------------------------------
    const VkHdrMetadataEXT* fsHdrGetDisplayInfo()
    {
        return &s_HdrMetadataEXT;
    }

    void fsHdrGetPhysicalDeviceSurfaceCapabilities2KHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR *pSurfCapabilities)
    {
        assert(surface == s_surface);
        assert(physicalDevice == s_physicalDevice);

        VkResult res = g_vkGetPhysicalDeviceSurfaceCapabilities2KHR(s_physicalDevice, &s_PhysicalDeviceSurfaceInfo2KHR, &s_SurfaceCapabilities2KHR);
        assert(res == VK_SUCCESS);

        if (pSurfCapabilities)
            *pSurfCapabilities = s_SurfaceCapabilities2KHR.surfaceCapabilities;
    }

    // We are turning off this HDR path for now
    // Driver update to get this path working is coming soon.
    #define WINDOW_HDR_PATH 0
    const bool CheckIfWindowModeHdrOn()
    {
        #if WINDOW_HDR_PATH
            return s_windowsHdrtoggle;
        #else
            return false;
        #endif
    }
}
