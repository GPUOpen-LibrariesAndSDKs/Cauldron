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

#include "Device.h"
#include <vulkan\vulkan_win32.h>
#include "Instance.h"
#include "InstanceProperties.h"
#include "DeviceProperties.h"
#include "DebugMarkersExt.h"
#include "ExtFreeSync2.h"
#include "ExtFp16.h"
#include "ValidationExt.h"

#ifdef USE_VMA
#define VMA_IMPLEMENTATION
#include "..\VulkanMemoryAllocator\vk_mem_alloc.h"
#endif

namespace CAULDRON_VK
{
    Device::Device()
    {
    }

    Device::~Device()
    {
    }

    void Device::OnCreate(const char *pAppName, const char *pEngineName, bool bValidationEnabled, HWND hWnd)
    {        
        InstanceProperties ip;
        ip.Init();

        if (bValidationEnabled)
            m_usingValidationLayer = ValidationCheckExtensions(&ip);

        VkApplicationInfo app_info = {};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pNext = NULL;
        app_info.pApplicationName = pAppName;
        app_info.applicationVersion = 1;
        app_info.pEngineName = pEngineName;
        app_info.engineVersion = 1;
        app_info.apiVersion = VK_API_VERSION_1_1;
        m_instance = CreateInstance(app_info, &ip);

        if (m_usingValidationLayer)
            ValidationOnCreate(m_instance);

        // Enumerate physical devices
        //
        uint32_t gpu_count = 1;
        uint32_t const req_count = gpu_count;
        VkResult res = vkEnumeratePhysicalDevices(m_instance, &gpu_count, NULL);
        assert(gpu_count);

        std::vector<VkPhysicalDevice> gpus;
        gpus.resize(gpu_count);

        res = vkEnumeratePhysicalDevices(m_instance, &gpu_count, gpus.data());
        assert(!res && gpu_count >= req_count);

        m_physicaldevice = gpus[0];

        // Get queue/memory/device properties
        //
        uint32_t queue_family_count;
        vkGetPhysicalDeviceQueueFamilyProperties(m_physicaldevice, &queue_family_count, NULL);
        assert(queue_family_count >= 1);

        std::vector<VkQueueFamilyProperties> queue_props;
        queue_props.resize(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(m_physicaldevice, &queue_family_count, queue_props.data());
        assert(queue_family_count >= 1);

        vkGetPhysicalDeviceMemoryProperties(m_physicaldevice, &m_memoryProperties);
        vkGetPhysicalDeviceProperties(m_physicaldevice, &m_deviceProperties);

        // Crate a Win32 Surface
        //
        VkWin32SurfaceCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.pNext = NULL;
        createInfo.hinstance = NULL;
        createInfo.hwnd = hWnd;
        res = vkCreateWin32SurfaceKHR(m_instance, &createInfo, NULL, &m_surface);

        // Find a graphics device and a queue that can present to the above surface
        //
        graphics_queue_family_index = UINT32_MAX;
        present_queue_family_index = UINT32_MAX;
        for (uint32_t i = 0; i < queue_family_count; ++i)
        {
            if ((queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
            {
                if (graphics_queue_family_index == UINT32_MAX) graphics_queue_family_index = i;

                VkBool32 supportsPresent;
                vkGetPhysicalDeviceSurfaceSupportKHR(gpus[0], i, m_surface, &supportsPresent);
                if (supportsPresent == VK_TRUE)
                {
                    graphics_queue_family_index = i;
                    present_queue_family_index = i;
                    break;
                }
            }
        }

        // If didn't find a queue that supports both graphics and present, then
        // find a separate present queue.
        if (present_queue_family_index == UINT32_MAX)
        {
            for (uint32_t i = 0; i < queue_family_count; ++i)
            {
                VkBool32 supportsPresent;
                vkGetPhysicalDeviceSurfaceSupportKHR(gpus[0], i, m_surface, &supportsPresent);
                if (supportsPresent == VK_TRUE)
                {
                    present_queue_family_index = (uint32_t)i;
                    break;
                }
            }
        }

        // Read device properties 
        DeviceProperties dp;
        dp.Init(m_physicaldevice);

        // Check required extensions are present
        //
        void *pNext = NULL;
        m_usingFp16 = ExtFp16CheckExtensions(&dp, &pNext);
        m_usingFreeSync2 = ExtFreeSync2CheckExtensions(&dp);
        m_usingValidationLayer = ValidationCheckExtensions(&dp, &pNext);

        std::vector<const char *> required_extension_names = { VK_KHR_SWAPCHAIN_EXTENSION_NAME , VK_KHR_MAINTENANCE1_EXTENSION_NAME, VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME };
        for (auto& ext : required_extension_names)
        {
            if (dp.Add(ext)==false)
            {
                MessageBoxA(NULL, "Panic: Required extension not found, device creation will fail. Update your driver.", ext, 0);
            }
        }

        // collect existing extensions
        std::vector<const char *> extension_names;        
        dp.GetExtensionNamesAndConfigs(&extension_names);

        // Create device 
        //
        float queue_priorities[1] = { 0.0 };
        VkDeviceQueueCreateInfo queue_info = {};
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.pNext = NULL;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = queue_priorities;
        queue_info.queueFamilyIndex = graphics_queue_family_index;

        VkPhysicalDeviceFeatures physicalDeviceFeatures = {};
        physicalDeviceFeatures.fillModeNonSolid = true;
        physicalDeviceFeatures.pipelineStatisticsQuery = true;
        physicalDeviceFeatures.wideLines = true; //needed for drawing lines with a specific width.

        VkDeviceCreateInfo device_info = {};
        device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_info.pNext = pNext;
        device_info.queueCreateInfoCount = 1;
        device_info.pQueueCreateInfos = &queue_info;
        device_info.enabledExtensionCount = (uint32_t)extension_names.size();
        device_info.ppEnabledExtensionNames = device_info.enabledExtensionCount ? extension_names.data() : NULL;
        device_info.pEnabledFeatures = &physicalDeviceFeatures;
        res = vkCreateDevice(gpus[0], &device_info, NULL, &m_device);
        assert(res == VK_SUCCESS);

#ifdef USE_VMA
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = GetPhysicalDevice();
        allocatorInfo.device = GetDevice();
        vmaCreateAllocator(&allocatorInfo, &m_hAllocator);
#endif

        // create queues
        //
        vkGetDeviceQueue(m_device, graphics_queue_family_index, 0, &graphics_queue);
        if (graphics_queue_family_index == present_queue_family_index)
        {
            present_queue = graphics_queue;
        }
        else
        {
            vkGetDeviceQueue(m_device, present_queue_family_index, 0, &present_queue);
        }

        // Init debug marker extension if found
        //
        if (m_usingValidationLayer)
        {
            ExtDebugMarkersInit(m_device);
        }

        if (m_usingFreeSync2)
        {
            ExtFreeSync2Init(m_instance);
            ExtFreeSync2Init(m_device);
        }
    }


    void Device::CreatePipelineCache()
    {
        // create pipeline cache

        VkPipelineCacheCreateInfo pipelineCache;
        pipelineCache.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        pipelineCache.pNext = NULL;
        pipelineCache.initialDataSize = 0;
        pipelineCache.pInitialData = NULL;
        pipelineCache.flags = 0;
        VkResult res = vkCreatePipelineCache(m_device, &pipelineCache, NULL, &m_pipelineCache);
        assert(res == VK_SUCCESS);
    }

    void Device::DestroyPipelineCache()
    {
        vkDestroyPipelineCache(m_device, m_pipelineCache, NULL);
    }

    VkPipelineCache Device::GetPipelineCache()
    {
        return m_pipelineCache;
    }

    void Device::OnDestroy()
    {

#ifdef USE_VMA
        vmaDestroyAllocator(m_hAllocator);
        m_hAllocator = NULL;
#endif

        if (m_device != VK_NULL_HANDLE)
        {
            vkDestroyDevice(m_device, nullptr);
            m_device = VK_NULL_HANDLE;
        }

        if (m_usingValidationLayer)
        {
            ValidationOnDestroy(m_instance);
        }

        DestroyInstance(m_instance);

        m_instance = VK_NULL_HANDLE;
    }

    void Device::GPUFlush()
    {
        vkDeviceWaitIdle(m_device);
    }

    bool memory_type_from_properties(VkPhysicalDeviceMemoryProperties &memory_properties, uint32_t typeBits, VkFlags requirements_mask, uint32_t *typeIndex) {
        // Search memtypes to find first index with those properties
        for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++) {
            if ((typeBits & 1) == 1) {
                // Type is available, does it match user properties?
                if ((memory_properties.memoryTypes[i].propertyFlags & requirements_mask) == requirements_mask) {
                    *typeIndex = i;
                    return true;
                }
            }
            typeBits >>= 1;
        }
        // No memory types matched, return failure
        return false;
    }
}
