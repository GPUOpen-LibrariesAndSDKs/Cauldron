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
#include "Misc/Misc.h"
#include "Misc/Error.h"

namespace CAULDRON_VK
{
    extern PFN_vkGetDeviceProcAddr                        g_vkGetDeviceProcAddr = NULL;

    // Functions for regular HDR ex: HDR10
    extern PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR g_vkGetPhysicalDeviceSurfaceCapabilities2KHR = NULL;
    extern PFN_vkGetPhysicalDeviceSurfaceFormats2KHR      g_vkGetPhysicalDeviceSurfaceFormats2KHR = NULL;
    extern PFN_vkSetHdrMetadataEXT                        g_vkSetHdrMetadataEXT = NULL;

    // Functions for FSE required if trying to use Freesync HDR
    extern PFN_vkAcquireFullScreenExclusiveModeEXT        g_vkAcquireFullScreenExclusiveModeEXT = NULL;
    extern PFN_vkReleaseFullScreenExclusiveModeEXT        g_vkReleaseFullScreenExclusiveModeEXT = NULL;

    // Functions for Freesync HDR
    extern PFN_vkSetLocalDimmingAMD                       g_vkSetLocalDimmingAMD = NULL;

    static VkPhysicalDeviceSurfaceInfo2KHR s_PhysicalDeviceSurfaceInfo2KHR;

    static VkSurfaceFullScreenExclusiveWin32InfoEXT s_SurfaceFullScreenExclusiveWin32InfoEXT;
    static VkSurfaceFullScreenExclusiveInfoEXT s_SurfaceFullScreenExclusiveInfoEXT;

    static VkDisplayNativeHdrSurfaceCapabilitiesAMD s_DisplayNativeHdrSurfaceCapabilitiesAMD;

    static bool s_isHdrInstanceExtensionPresent = false;
    static bool s_isHdrDeviceExtensionsPresent = false;
    static bool s_isFSEDeviceExtensionsPresent = false;
    static bool s_isFSHDRDeviceExtensionsPresent = false;

    void ExtCheckHDRInstanceExtensions(InstanceProperties *pIP)
    {
        s_isHdrInstanceExtensionPresent = pIP->AddInstanceExtensionName(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
    }

    void ExtCheckHDRDeviceExtensions(DeviceProperties *pDP)
    {
        s_isHdrDeviceExtensionsPresent = pDP->AddDeviceExtensionName(VK_EXT_HDR_METADATA_EXTENSION_NAME);
    }

    void ExtCheckFSEDeviceExtensions(DeviceProperties *pDP)
    {
        s_isFSEDeviceExtensionsPresent = pDP->AddDeviceExtensionName(VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME);
    }

    void ExtCheckFreeSyncHDRDeviceExtensions(DeviceProperties *pDP)
    {
        s_isFSHDRDeviceExtensionsPresent = pDP->AddDeviceExtensionName(VK_AMD_DISPLAY_NATIVE_HDR_EXTENSION_NAME);
    }

    void ExtGetHDRFSEFreesyncHDRProcAddresses(VkInstance instance, VkDevice device)
    {
        if (s_isHdrInstanceExtensionPresent)
        {
            g_vkGetPhysicalDeviceSurfaceCapabilities2KHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceCapabilities2KHR");
            assert(g_vkGetPhysicalDeviceSurfaceCapabilities2KHR);

            g_vkGetPhysicalDeviceSurfaceFormats2KHR = (PFN_vkGetPhysicalDeviceSurfaceFormats2KHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceFormats2KHR");
            assert(g_vkGetPhysicalDeviceSurfaceFormats2KHR);
        }

        g_vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)vkGetInstanceProcAddr(instance, "vkGetDeviceProcAddr");
        assert(g_vkGetDeviceProcAddr);

        if (s_isHdrDeviceExtensionsPresent)
        {
            g_vkSetHdrMetadataEXT = (PFN_vkSetHdrMetadataEXT)g_vkGetDeviceProcAddr(device, "vkSetHdrMetadataEXT");
            assert(g_vkSetHdrMetadataEXT);
        }

        if (s_isFSEDeviceExtensionsPresent)
        {
            g_vkAcquireFullScreenExclusiveModeEXT = (PFN_vkAcquireFullScreenExclusiveModeEXT)g_vkGetDeviceProcAddr(device, "vkAcquireFullScreenExclusiveModeEXT");
            assert(g_vkAcquireFullScreenExclusiveModeEXT);

            g_vkReleaseFullScreenExclusiveModeEXT = (PFN_vkReleaseFullScreenExclusiveModeEXT)g_vkGetDeviceProcAddr(device, "vkReleaseFullScreenExclusiveModeEXT");
            assert(g_vkReleaseFullScreenExclusiveModeEXT);
        }

        if (s_isFSHDRDeviceExtensionsPresent)
        {
            g_vkSetLocalDimmingAMD = (PFN_vkSetLocalDimmingAMD)g_vkGetDeviceProcAddr(device, "vkSetLocalDimmingAMD");
            assert(g_vkSetLocalDimmingAMD);
        }
    }

    bool ExtAreHDRExtensionsPresent()
    {
        return s_isHdrInstanceExtensionPresent && s_isHdrDeviceExtensionsPresent;
    }

    bool ExtAreFSEExtensionsPresent()
    {
        return s_isFSEDeviceExtensionsPresent;
    }

    bool ExtAreFreeSyncHDRExtensionsPresent()
    {
        return s_isHdrInstanceExtensionPresent &&
               s_isHdrDeviceExtensionsPresent &&
               s_isFSEDeviceExtensionsPresent &&
               s_isFSHDRDeviceExtensionsPresent;
    }
}
