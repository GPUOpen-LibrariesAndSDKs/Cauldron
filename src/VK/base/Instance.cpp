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
#include <algorithm>
#include "Instance.h"
#include "InstanceProperties.h"
#include <vulkan/vulkan_win32.h>
#include "ExtValidation.h"
#include "ExtFreeSyncHDR.h"
#include "ExtDebugUtils.h"


namespace CAULDRON_VK
{
	uint32_t GetScore(VkPhysicalDevice physicalDevice)
	{
		uint32_t score = 0;

		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
		// Use the features for a more precise way to select the GPU
		//VkPhysicalDeviceFeatures deviceFeatures;
		//vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
		switch (deviceProperties.deviceType)
		{
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
			score += 1000;
			break;
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
			score += 10000;
			break;
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
			score += 100;
			break;
		case VK_PHYSICAL_DEVICE_TYPE_CPU:
			score += 10;
			break;
		default:
			break;
		}

		// TODO: add other constraints

		return score;
	}

	// Select the best physical device.
	// For now, the code just gets the first discrete GPU.
	// If none is found, it default to an integrated then virtual then cpu one
	VkPhysicalDevice SelectPhysicalDevice(std::vector<VkPhysicalDevice>& physicalDevices)
	{
		assert(physicalDevices.size() > 0 && "No GPU found");

		std::multimap<uint32_t, VkPhysicalDevice> ratings;

		for (auto it = physicalDevices.begin(); it != physicalDevices.end(); ++it)
			ratings.insert(std::make_pair(GetScore(*it), *it));

		return ratings.rbegin()->second;
	}

    bool CreateInstance(const char *pAppName, const char *pEngineName, VkInstance *pVulkanInstance, VkPhysicalDevice *pPhysicalDevice, InstanceProperties *pIp)
    {
        VkApplicationInfo app_info = {};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pNext = NULL;
        app_info.pApplicationName = pAppName;
        app_info.applicationVersion = 1;
        app_info.pEngineName = pEngineName;
        app_info.engineVersion = 1;
        app_info.apiVersion = VK_API_VERSION_1_2;
        VkInstance instance = CreateInstance(app_info, pIp);

		// Enumerate physical devices
		uint32_t physicalDeviceCount = 1;
		uint32_t const req_count = physicalDeviceCount;
		VkResult res = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
		assert(physicalDeviceCount > 0 && "No GPU found");

		std::vector<VkPhysicalDevice> physicalDevices;
		physicalDevices.resize(physicalDeviceCount);
		res = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());
		assert(res == VK_SUCCESS && physicalDeviceCount >= req_count && "Unable to enumerate physical devices");

		// get the best available gpu
        *pPhysicalDevice = SelectPhysicalDevice(physicalDevices);
        *pVulkanInstance = instance;

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



