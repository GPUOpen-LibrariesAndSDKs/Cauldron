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
#include <algorithm>
#include "Instance.h"
#include "InstanceProperties.h"
#include <vulkan/vulkan_win32.h>
#include "ExtValidation.h"
#include "ExtFreeSyncHDR.h"
#include "ExtDebugUtils.h"
#include "ExtGPUValidation.h"


namespace CAULDRON_VK
{
    bool CreateInstance(const char *pAppName, const char *pEngineName, VkInstance *pVulkanInstance, VkPhysicalDevice *pPhysicalDevice, InstanceProperties *pIp)
    {
        VkApplicationInfo app_info = {};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pNext = NULL;
        app_info.pApplicationName = pAppName;
        app_info.applicationVersion = 1;
        app_info.pEngineName = pEngineName;
        app_info.engineVersion = 1;
        app_info.apiVersion = VK_API_VERSION_1_1;
        VkInstance instance = CreateInstance(app_info, pIp);

        // Enumerate physical devices
        //
        uint32_t gpu_count = 1;
        uint32_t const req_count = gpu_count;
        VkResult res = vkEnumeratePhysicalDevices(instance, &gpu_count, NULL);
        assert(gpu_count);

        std::vector<VkPhysicalDevice> gpus;
        gpus.resize(gpu_count);

        res = vkEnumeratePhysicalDevices(instance, &gpu_count, gpus.data());
        assert(!res && gpu_count >= req_count);

        *pVulkanInstance = instance;
        *pPhysicalDevice = gpus[0];

        return true;
    }

    VkInstance CreateInstance(VkApplicationInfo app_info, InstanceProperties *pIp)
    {
        VkInstance instance;

        // prepare existing extensions and layer names into a buffer for vkCreateInstance
        std::vector<const char *> instance_layer_names;
        std::vector<const char *> instance_extension_names;
        pIp->GetExtensionNamesAndConfigs(&instance_layer_names, &instance_extension_names);

        // do create the instance
        VkInstanceCreateInfo inst_info = {};
        inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        inst_info.pNext = pIp->GetNext();
        inst_info.flags = 0;
        inst_info.pApplicationInfo = &app_info;
        inst_info.enabledLayerCount = (uint32_t)instance_layer_names.size();
        inst_info.ppEnabledLayerNames = (uint32_t)instance_layer_names.size() ? instance_layer_names.data() : NULL;
        inst_info.enabledExtensionCount = (uint32_t)instance_extension_names.size();
        inst_info.ppEnabledExtensionNames = instance_extension_names.data();           
        VkResult res = vkCreateInstance(&inst_info, NULL, &instance);
        assert(res == VK_SUCCESS);

        // Init the extensions (if they have been enabled successfuly)
        //
        //
        ExtDebugReportGetProcAddresses(instance);
        ExtDebugReportOnCreate(instance);

        return instance;
    }

    void DestroyInstance(VkInstance instance)
    {
        vkDestroyInstance(instance, nullptr);
    }
}



