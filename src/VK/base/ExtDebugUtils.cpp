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
    static PFN_vkSetDebugUtilsObjectNameEXT     s_vkSetDebugUtilsObjectName = nullptr;
    static PFN_vkCmdBeginDebugUtilsLabelEXT     s_vkCmdBeginDebugUtilsLabel = nullptr;
    static PFN_vkCmdEndDebugUtilsLabelEXT       s_vkCmdEndDebugUtilsLabel = nullptr;
    static bool s_bCanUseDebugUtils = false;
    static std::mutex s_mutex;

    bool ExtDebugUtilsCheckInstanceExtensions(InstanceProperties *pDP)
    {
        s_bCanUseDebugUtils = pDP->AddInstanceExtensionName("VK_EXT_debug_utils");
        return s_bCanUseDebugUtils;
    }

    //
    //
    void ExtDebugUtilsGetProcAddresses(VkDevice device)
    {
        if (s_bCanUseDebugUtils)
        {
            s_vkSetDebugUtilsObjectName = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT");
            s_vkCmdBeginDebugUtilsLabel = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetDeviceProcAddr(device, "vkCmdBeginDebugUtilsLabelEXT");
            s_vkCmdEndDebugUtilsLabel = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetDeviceProcAddr(device, "vkCmdEndDebugUtilsLabelEXT");
        }
    }

    void SetResourceName(VkDevice device, VkObjectType objectType, uint64_t handle, const char* name)
    {
        if (s_vkSetDebugUtilsObjectName && handle && name)
        {
            std::unique_lock<std::mutex> lock(s_mutex);

            VkDebugUtilsObjectNameInfoEXT nameInfo = {};
            nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            nameInfo.objectType = objectType;
            nameInfo.objectHandle = handle;
            nameInfo.pObjectName = name;
            s_vkSetDebugUtilsObjectName(device, &nameInfo);
        }
    }

    void SetPerfMarkerBegin(VkCommandBuffer cmd_buf, const char* name)
    {
        if (s_vkCmdBeginDebugUtilsLabel)
        {
            VkDebugUtilsLabelEXT label = {};
            label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
            label.pLabelName = name;
            const float color[] = { 1.0f, 0.0f, 0.0f, 1.0f };
            memcpy( label.color, color, sizeof( color ) );
            s_vkCmdBeginDebugUtilsLabel( cmd_buf, &label );
        }
    }

    void SetPerfMarkerEnd(VkCommandBuffer cmd_buf)
    {
        if (s_vkCmdEndDebugUtilsLabel)
        {
            s_vkCmdEndDebugUtilsLabel(cmd_buf);
        }
    }
}
