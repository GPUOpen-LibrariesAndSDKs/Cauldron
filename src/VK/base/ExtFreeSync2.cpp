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


#include "ExtFreeSync2.h"
#include "Misc/Misc.h"
#include <vulkan/vulkan.h>
#ifdef _WIN32
#include <vulkan/vulkan_win32.h>
#endif

namespace CAULDRON_VK
{

    static PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR g_vkGetPhysicalDeviceSurfaceCapabilities2KHR =  nullptr;
    static PFN_vkGetDeviceProcAddr g_vkGetDeviceProcAddr =  nullptr;
    static PFN_vkSetHdrMetadataEXT g_vkSetHdrMetadataEXT =  nullptr;
    #ifdef _WIN32
    static PFN_vkAcquireFullScreenExclusiveModeEXT g_vkAcquireFullScreenExclusiveModeEXT =  nullptr;
    static PFN_vkReleaseFullScreenExclusiveModeEXT g_vkReleaseFullScreenExclusiveModeEXT =  nullptr;
    #endif
    static PFN_vkGetPhysicalDeviceSurfaceFormats2KHR      g_vkGetPhysicalDeviceSurfaceFormats2KHR =  nullptr;
    #ifdef _WIN32
    static PFN_vkGetDeviceGroupSurfacePresentModes2EXT    g_vkGetDeviceGroupSurfacePresentModes2EXT =  nullptr;
    static PFN_vkGetPhysicalDeviceSurfacePresentModes2EXT g_vkGetPhysicalDeviceSurfacePresentModes2EXT =  nullptr;
    static PFN_vkSetLocalDimmingAMD                       g_vkSetLocalDimmingAMD =  nullptr;
    #endif
    static PFN_vkGetPhysicalDevicePresentRectanglesKHR    g_vkGetPhysicalDevicePresentRectanglesKHR =  nullptr;


    bool ExtFreeSync2CheckExtensions(DeviceProperties *pDP)
    {
        bool res = true;

        #ifdef _WIN32
        std::vector<const char *> required_extension_names = { VK_EXT_HDR_METADATA_EXTENSION_NAME, VK_AMD_DISPLAY_NATIVE_HDR_EXTENSION_NAME, VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME };
    
        for (auto& ext : required_extension_names)
        {
            if (pDP->Add(ext) == false)
            {                
                Trace(format("FreeSync2 disabled, missing extension: %s\n", ext));
                res = false;
            }
            
        }
        #else
        res = false;
        Trace(format("FreeSync2 disabled, unsupported platform"));
        #endif

        return res;
    }

    void ExtFreeSync2Init(VkInstance instance)
    {
        g_vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)vkGetInstanceProcAddr(instance, "vkGetDeviceProcAddr");
        assert(g_vkGetDeviceProcAddr);

        g_vkGetPhysicalDeviceSurfaceFormats2KHR = (PFN_vkGetPhysicalDeviceSurfaceFormats2KHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceFormats2KHR");
        assert(g_vkGetPhysicalDeviceSurfaceFormats2KHR);

#ifdef _WIN32
        g_vkGetPhysicalDeviceSurfacePresentModes2EXT = (PFN_vkGetPhysicalDeviceSurfacePresentModes2EXT)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfacePresentModes2EXT");
        assert(g_vkGetPhysicalDeviceSurfacePresentModes2EXT);

        g_vkGetDeviceGroupSurfacePresentModes2EXT = (PFN_vkGetDeviceGroupSurfacePresentModes2EXT)vkGetInstanceProcAddr(instance, "vkGetDeviceGroupSurfacePresentModes2EXT");
        assert(g_vkGetDeviceGroupSurfacePresentModes2EXT);
#endif
    }

    void ExtFreeSync2Init(VkDevice device)
    {
        g_vkSetHdrMetadataEXT = (PFN_vkSetHdrMetadataEXT)g_vkGetDeviceProcAddr(device, "vkSetHdrMetadataEXT");
        assert(g_vkSetHdrMetadataEXT);

#ifdef _WIN32
        g_vkAcquireFullScreenExclusiveModeEXT = (PFN_vkAcquireFullScreenExclusiveModeEXT)g_vkGetDeviceProcAddr(device, "vkAcquireFullScreenExclusiveModeEXT");
        assert(g_vkAcquireFullScreenExclusiveModeEXT);

        g_vkReleaseFullScreenExclusiveModeEXT = (PFN_vkReleaseFullScreenExclusiveModeEXT)g_vkGetDeviceProcAddr(device, "vkReleaseFullScreenExclusiveModeEXT");
        assert(g_vkReleaseFullScreenExclusiveModeEXT);

        g_vkSetLocalDimmingAMD = (PFN_vkSetLocalDimmingAMD)g_vkGetDeviceProcAddr(device, "vkSetLocalDimmingAMD");
        assert(g_vkSetLocalDimmingAMD);
#endif
        g_vkGetPhysicalDevicePresentRectanglesKHR = (PFN_vkGetPhysicalDevicePresentRectanglesKHR)g_vkGetDeviceProcAddr(device, "vkGetPhysicalDevicePresentRectanglesKHR");
        assert(g_vkGetPhysicalDevicePresentRectanglesKHR);
    }

}