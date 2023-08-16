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

#include "ExtRayTracing.h"
#include "Misc/Misc.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

namespace CAULDRON_VK
{
    static VkPhysicalDeviceRayTracingPipelineFeaturesKHR RTPipelinesFeatures = {};
    static VkPhysicalDeviceRayQueryFeaturesKHR RTQueryFeatures = {};
    static VkPhysicalDeviceAccelerationStructureFeaturesKHR AccelerationStructureFeatures = {};
    static VkPhysicalDeviceBufferDeviceAddressFeatures BufferDeviceAddressFeatures = {};
    static VkPhysicalDeviceDescriptorIndexingFeatures DescriptorIndexingFeatures = {};
    void ExtRTCheckExtensions(DeviceProperties* pDP, bool& RT10Supported, bool& RT11Supported)
    {
        bool RT10ExtensionPresent = pDP->AddDeviceExtensionName(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
        bool RT11ExtensionPresent = pDP->AddDeviceExtensionName(VK_KHR_RAY_QUERY_EXTENSION_NAME);
        bool AccelerationStructureExtensionPresent = pDP->AddDeviceExtensionName(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
        bool BufferDeviceAddressPresent = pDP->AddDeviceExtensionName(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
        bool DescriptorIndexingExtensionPresent = pDP->AddDeviceExtensionName(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
        pDP->AddDeviceExtensionName(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
        pDP->AddDeviceExtensionName(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
        pDP->AddDeviceExtensionName(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);

        if (RT10ExtensionPresent)
        {
            RTPipelinesFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
            VkPhysicalDeviceFeatures2 features = {};
            features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            features.pNext = &RTPipelinesFeatures;
            vkGetPhysicalDeviceFeatures2(pDP->GetPhysicalDevice(), &features);
            
            RT10Supported = RTPipelinesFeatures.rayTracingPipeline;

            if (RT10Supported)
            {
                RTPipelinesFeatures.pNext = pDP->GetNext();
                pDP->SetNewNext(&RTPipelinesFeatures);
            }
        }

        if (RT11ExtensionPresent)
        {
            RTQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
            VkPhysicalDeviceFeatures2 features = {};
            features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            features.pNext = &RTQueryFeatures;
            vkGetPhysicalDeviceFeatures2(pDP->GetPhysicalDevice(), &features);

            RT11Supported = RTQueryFeatures.rayQuery;

            if (RT11Supported)
            {
                RTQueryFeatures.pNext = pDP->GetNext();
                pDP->SetNewNext(&RTQueryFeatures);
            }
        }

        if (AccelerationStructureExtensionPresent)
        {
            AccelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
            VkPhysicalDeviceFeatures2 features = {};
            features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            features.pNext = &AccelerationStructureFeatures;
            vkGetPhysicalDeviceFeatures2(pDP->GetPhysicalDevice(), &features);

            if (AccelerationStructureFeatures.accelerationStructure == VK_TRUE)
            {
                // we already have 2 feature structures chained, so we aren't setting pNext to the same structure that will become the Next one
                AccelerationStructureFeatures.pNext = pDP->GetNext();
                pDP->SetNewNext(&AccelerationStructureFeatures);
            }
        }

        if (DescriptorIndexingExtensionPresent)
        {
            DescriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
            VkPhysicalDeviceFeatures2 features = {};
            features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            features.pNext = &DescriptorIndexingFeatures;
            vkGetPhysicalDeviceFeatures2(pDP->GetPhysicalDevice(), &features);

            DescriptorIndexingFeatures.pNext = pDP->GetNext();
            pDP->SetNewNext(&DescriptorIndexingFeatures);
        }

        if (BufferDeviceAddressPresent)
        {
            BufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
            VkPhysicalDeviceFeatures2 features = {};
            features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            features.pNext = &BufferDeviceAddressFeatures;
            vkGetPhysicalDeviceFeatures2(pDP->GetPhysicalDevice(), &features);

            if (BufferDeviceAddressFeatures.bufferDeviceAddress == VK_TRUE)
            {
                BufferDeviceAddressFeatures.pNext = pDP->GetNext();
                pDP->SetNewNext(&BufferDeviceAddressFeatures);
            }
        }
    }
}
