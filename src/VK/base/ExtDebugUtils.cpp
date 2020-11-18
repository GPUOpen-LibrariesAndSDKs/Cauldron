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
#include "ExtDebugUtils.h"
#include "Misc/Misc.h"

namespace CAULDRON_VK
{
    static PFN_vkSetDebugUtilsObjectNameEXT    s_vkSetDebugUtilsObjectName = NULL;
    
    static bool s_bCanUseDebugUtils;
    static std::mutex s_mutex;

    bool ExtDebugUtilsCheckInstanceExtensions(InstanceProperties *pDP)
    {
        s_bCanUseDebugUtils = pDP->AddInstanceExtensionName("VK_EXT_debug_utils");
        if (s_bCanUseDebugUtils)
        {
            Trace("Note that the extension 'VK_EXT_debug_utils' is only available under tools that enable them, like RenderDoc\n");
        }

        return s_bCanUseDebugUtils;
    }

    //
    //
    void ExtDebugUtilsGetProcAddresses(VkDevice device)
    {
        if (s_bCanUseDebugUtils)
        {
            s_vkSetDebugUtilsObjectName = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT");
        }
    }

    void SetResourceName(VkDevice device, VkObjectType objectType, uint64_t handle, const char *pMsg)
    {
        assert(handle != VK_NULL_HANDLE);

        size_t size = strlen(pMsg);
        char  *uniName = (char  *)malloc(size +1); //yes, this will be causing leaks!
        strcpy_s(uniName, size+1, pMsg);

        const VkDebugUtilsObjectNameInfoEXT imageNameInfo =
        {
            VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, // sType
            NULL,                                               // pNext
            objectType,                                         // objectType
            (uint64_t)handle,                                   // object
            uniName,                                            // pObjectName
        };

        if (s_vkSetDebugUtilsObjectName != NULL)
        {
           std::unique_lock<std::mutex> lock(s_mutex);
           s_vkSetDebugUtilsObjectName(device, &imageNameInfo);
        }
    }
}
