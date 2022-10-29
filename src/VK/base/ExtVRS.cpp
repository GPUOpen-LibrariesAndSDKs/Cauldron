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

#include "ExtVRS.h"
#include "Misc/Misc.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

namespace CAULDRON_VK
{
    static VkPhysicalDeviceFragmentShadingRateFeaturesKHR VRSQueryFeatures = {};

    void ExtVRSCheckExtensions(DeviceProperties* pDP, bool& Tier1Supported, bool& Tier2Supported, VkExtent2D& MaxFragmentSize, VkExtent2D& FragmentShadingRateAttachmentTexelSize)
    {
        if (pDP->AddDeviceExtensionName(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME))
        {
            pDP->AddDeviceExtensionName(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);

            VRSQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;
            VRSQueryFeatures.attachmentFragmentShadingRate = VK_TRUE;
            VRSQueryFeatures.pipelineFragmentShadingRate = VK_TRUE;
            VRSQueryFeatures.primitiveFragmentShadingRate = VK_TRUE;
            VkPhysicalDeviceFeatures2 features = {};
            features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            features.pNext = &VRSQueryFeatures;
            vkGetPhysicalDeviceFeatures2(pDP->GetPhysicalDevice(), &features);

            Tier1Supported = VRSQueryFeatures.pipelineFragmentShadingRate;
            Tier2Supported = VRSQueryFeatures.attachmentFragmentShadingRate && VRSQueryFeatures.primitiveFragmentShadingRate;
 
            VkPhysicalDeviceFragmentShadingRatePropertiesKHR physicalDeviceFragmentShadingRateProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR};
            {
                VkPhysicalDeviceProperties2KHR deviceProperties = {};
                {
                    deviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
                    deviceProperties.pNext = &physicalDeviceFragmentShadingRateProperties;
                }
                vkGetPhysicalDeviceProperties2(pDP->GetPhysicalDevice(), &deviceProperties);
            }
            MaxFragmentSize = physicalDeviceFragmentShadingRateProperties.maxFragmentSize;
            FragmentShadingRateAttachmentTexelSize = physicalDeviceFragmentShadingRateProperties.maxFragmentShadingRateAttachmentTexelSize;
        }
    }
}
