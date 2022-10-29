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
#include "Helper.h"
#include "ExtDebugUtils.h"

namespace CAULDRON_VK
{
    // this is when you need to clear the attachment, for example when you are not rendering the full screen.
    //
    void AttachClearBeforeUse(VkFormat format, VkSampleCountFlagBits sampleCount, VkImageLayout initialLayout, VkImageLayout finalLayout, VkAttachmentDescription *pAttachDesc)
    {
        pAttachDesc->format = format;
        pAttachDesc->samples = sampleCount;
        pAttachDesc->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        pAttachDesc->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        pAttachDesc->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        pAttachDesc->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        pAttachDesc->initialLayout = initialLayout;
        pAttachDesc->finalLayout = finalLayout;
        pAttachDesc->flags = 0;
    }

    void AttachClearBeforeUse(VkFormat format, VkSampleCountFlagBits sampleCount, VkImageLayout initialLayout, VkImageLayout finalLayout, VkAttachmentDescription2 *pAttachDesc)
    {
        pAttachDesc->sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
        pAttachDesc->format = format;
        pAttachDesc->samples = sampleCount;
        pAttachDesc->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        pAttachDesc->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        pAttachDesc->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        pAttachDesc->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        pAttachDesc->initialLayout = initialLayout;
        pAttachDesc->finalLayout = finalLayout;
        pAttachDesc->flags = 0;
    }

    // No clear, attachment will keep data that was not written (if this is the first pass make sure you are filling the whole screen)
    void AttachNoClearBeforeUse(VkFormat format, VkSampleCountFlagBits sampleCount, VkImageLayout initialLayout, VkImageLayout finalLayout, VkAttachmentDescription *pAttachDesc)
    {
        pAttachDesc->format = format;
        pAttachDesc->samples = sampleCount;
        pAttachDesc->loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        pAttachDesc->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        pAttachDesc->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        pAttachDesc->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        pAttachDesc->initialLayout = initialLayout;
        pAttachDesc->finalLayout = finalLayout;
        pAttachDesc->flags = 0;
    }

    void AttachNoClearBeforeUse(VkFormat format, VkSampleCountFlagBits sampleCount, VkImageLayout initialLayout, VkImageLayout finalLayout, VkAttachmentDescription2 *pAttachDesc)
    {
        pAttachDesc->sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
        pAttachDesc->format = format;
        pAttachDesc->samples = sampleCount;
        pAttachDesc->loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        pAttachDesc->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        pAttachDesc->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        pAttachDesc->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        pAttachDesc->initialLayout = initialLayout;
        pAttachDesc->finalLayout = finalLayout;
        pAttachDesc->flags = 0;
    }

    // Attanchment where we will be using alpha blending, this means we care about the previous contents
    void AttachBlending(VkFormat format, VkSampleCountFlagBits sampleCount, VkImageLayout initialLayout, VkImageLayout finalLayout, VkAttachmentDescription *pAttachDesc)
    {
        pAttachDesc->format = format;
        pAttachDesc->samples = sampleCount;
        pAttachDesc->loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        pAttachDesc->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        pAttachDesc->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        pAttachDesc->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        pAttachDesc->initialLayout = initialLayout;
        pAttachDesc->finalLayout = finalLayout;
        pAttachDesc->flags = 0;
    }

    void AttachBlending(VkFormat format, VkSampleCountFlagBits sampleCount, VkImageLayout initialLayout, VkImageLayout finalLayout, VkAttachmentDescription2 *pAttachDesc)
    {
        pAttachDesc->sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
        pAttachDesc->format = format;
        pAttachDesc->samples = sampleCount;
        pAttachDesc->loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        pAttachDesc->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        pAttachDesc->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        pAttachDesc->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        pAttachDesc->initialLayout = initialLayout;
        pAttachDesc->finalLayout = finalLayout;
        pAttachDesc->flags = 0;
    }


    VkRenderPass CreateRenderPassOptimal(VkDevice device,
        uint32_t colorAttachments,
        VkAttachmentDescription *pColorAttachments,
        VkAttachmentDescription *pDepthAttachment,
        VkImageLayout depthLayout,
        VkAttachmentDescription *pVRSAttachment,
        VkExtent2D shadingRateAttachmentTexelSize)
    {
        // we need to put all the color and the depth attachments in the same buffer
        //
        VkAttachmentDescription attachments[10];
        assert(colorAttachments < 10); // make sure we don't overflow the scratch buffer above

        memcpy(attachments, pColorAttachments, sizeof(VkAttachmentDescription) * colorAttachments);
        if (pDepthAttachment!=NULL)
            memcpy(&attachments[colorAttachments], pDepthAttachment, sizeof(VkAttachmentDescription));

        //create references for the attachments
        //
        VkAttachmentReference color_reference[10];
        for(uint32_t i=0;i< colorAttachments;i++)
            color_reference[i] = { i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

        VkAttachmentReference depth_reference = { colorAttachments, depthLayout };

        // Create subpass
        //
        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.flags = 0;
        subpass.inputAttachmentCount = 0;
        subpass.pInputAttachments = NULL;
        subpass.colorAttachmentCount = colorAttachments;
        subpass.pColorAttachments = color_reference;
        subpass.pResolveAttachments = NULL;
        subpass.pDepthStencilAttachment = (pDepthAttachment)? &depth_reference : NULL;
        subpass.preserveAttachmentCount = 0;
        subpass.pPreserveAttachments = NULL;

        VkSubpassDependency dep = {};
        dep.dependencyFlags = 0;
        dep.dstAccessMask = VK_ACCESS_SHADER_READ_BIT |
            ((colorAttachments) ? VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT : 0) |
            ((pDepthAttachment) ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : 0);
        dep.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
            ((colorAttachments) ? VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT : 0) |
            ((pDepthAttachment) ? VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT : 0);
        dep.dstSubpass = VK_SUBPASS_EXTERNAL;
        dep.srcAccessMask = ((colorAttachments) ? VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT : 0) |
            ((pDepthAttachment) ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : 0);
        dep.srcStageMask = ((colorAttachments) ? VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT : 0) |
            ((pDepthAttachment) ? VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT : 0);
        dep.srcSubpass = 0;

        // Create render pass
        //
        VkRenderPassCreateInfo rp_info = {};
        rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rp_info.pNext = NULL;
        rp_info.attachmentCount = colorAttachments;
        if (pDepthAttachment != NULL)
            rp_info.attachmentCount++;
        rp_info.pAttachments = attachments;
        rp_info.subpassCount = 1;
        rp_info.pSubpasses = &subpass;
        rp_info.dependencyCount = 1;
        rp_info.pDependencies = &dep;

        VkRenderPass render_pass;
        VkResult res = vkCreateRenderPass(device, &rp_info, NULL, &render_pass);
        assert(res == VK_SUCCESS);

        SetResourceName(device, VK_OBJECT_TYPE_RENDER_PASS, (uint64_t)render_pass, "CreateRenderPassOptimal");

        return render_pass;
    }

    VkRenderPass CreateRenderPassOptimal(VkDevice device,
        uint32_t colorAttachments,
        VkAttachmentDescription2 *pColorAttachments,
        VkAttachmentDescription2 *pDepthAttachment,
        VkImageLayout depthLayout,
        VkAttachmentDescription2 *pVRSAttachment,
        VkExtent2D shadingRateAttachmentTexelSize
        )
    {
        // The passed pointer is valid, but it might point to uninitialized struct
        //
        const bool tDepthAttachmentPresent = pDepthAttachment && pDepthAttachment->sType == VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
        const bool tVRSAttachmentPresent   = pVRSAttachment && pVRSAttachment->sType == VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;

        // we need to put all the color and the depth attachments in the same buffer
        //
        VkAttachmentDescription2 attachments[10];
        assert(colorAttachments < 10); // make sure we don't overflow the scratch buffer above

        memcpy(attachments, pColorAttachments, sizeof(VkAttachmentDescription2) * colorAttachments);

        // Covers the attachment offset change depending on the presence of depth
        //
        int tAttachmentsOffset = 0;
        if (tDepthAttachmentPresent)
        {
            memcpy(&attachments[colorAttachments], pDepthAttachment, sizeof(VkAttachmentDescription2));
            tAttachmentsOffset++;
        }
        if (tVRSAttachmentPresent)
        {
            memcpy(&attachments[colorAttachments + tAttachmentsOffset], pVRSAttachment, sizeof(VkAttachmentDescription2));
        }

        //create references for the attachments
        //
        VkAttachmentReference2 color_reference[10];
        for (uint32_t i = 0; i < colorAttachments; i++)
        {
            color_reference[i].sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
            color_reference[i].attachment = i;
            color_reference[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            color_reference[i].pNext = nullptr;
        }
        VkAttachmentReference2 depth_reference = {};
        {
            depth_reference.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
            depth_reference.attachment = colorAttachments;
            depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depth_reference.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            depth_reference.pNext = nullptr;
        }

        // Set up the attachment reference for the shading rate image attachment in slot 2
        VkAttachmentReference2 fragment_shading_rate_reference = {};
        {
            fragment_shading_rate_reference.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
            fragment_shading_rate_reference.attachment = colorAttachments + tAttachmentsOffset;
            fragment_shading_rate_reference.layout = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
        }

        // Set up the attachment info for the shading rate image, which will be added to the sub pass via structure chaining (in pNext)
        VkFragmentShadingRateAttachmentInfoKHR fragmentShadingRateAttachmentInfo = {};
        {
            fragmentShadingRateAttachmentInfo.sType = VK_STRUCTURE_TYPE_FRAGMENT_SHADING_RATE_ATTACHMENT_INFO_KHR;
            fragmentShadingRateAttachmentInfo.pFragmentShadingRateAttachment = &fragment_shading_rate_reference;
            fragmentShadingRateAttachmentInfo.shadingRateAttachmentTexelSize = shadingRateAttachmentTexelSize;
        }

        // Create subpass
        //
        VkSubpassDescription2 subpass = {};
        {
            
            subpass.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.flags = 0;
            subpass.inputAttachmentCount = 0;
            subpass.pInputAttachments = NULL;
            subpass.colorAttachmentCount = colorAttachments;
            subpass.pColorAttachments = colorAttachments ? color_reference : NULL;
            subpass.pResolveAttachments = NULL;
            subpass.pDepthStencilAttachment = (pDepthAttachment)? &depth_reference : NULL;
            subpass.preserveAttachmentCount = 0;
            subpass.pPreserveAttachments = NULL;
            subpass.pNext = tVRSAttachmentPresent ? &fragmentShadingRateAttachmentInfo : NULL;
        }

        VkSubpassDependency2KHR dep = {};
        dep.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
        dep.dependencyFlags = 0;
        dep.dstAccessMask = VK_ACCESS_SHADER_READ_BIT |
            ((tVRSAttachmentPresent) ? VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT : 0) |
            ((colorAttachments) ? VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT : 0) |
            ((pDepthAttachment) ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : 0);
        dep.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
            ((colorAttachments) ? VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT : 0) |
            ((pDepthAttachment) ? VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT : 0);
        dep.dstSubpass = VK_SUBPASS_EXTERNAL;
        dep.srcAccessMask = ((colorAttachments) ? VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT : 0) |
            ((pDepthAttachment) ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : 0);
        dep.srcStageMask = ((colorAttachments) ? VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT : 0) |
            ((pDepthAttachment) ? VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT : 0);
        dep.srcSubpass = 0;

        // Create render pass
        //
        VkRenderPassCreateInfo2KHR rp_info = {};
        rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
        rp_info.pNext = NULL;
        rp_info.attachmentCount = colorAttachments;
        if (tDepthAttachmentPresent)
        {
            rp_info.attachmentCount++;
        }
        if (tVRSAttachmentPresent)
        {
            rp_info.attachmentCount++;
        }
        rp_info.pAttachments = attachments;
        rp_info.subpassCount = 1;
        rp_info.pSubpasses = &subpass;
        rp_info.dependencyCount = 1;
        rp_info.pDependencies = &dep;

        VkRenderPass render_pass;
        VkResult res = vkCreateRenderPass2(device, &rp_info, NULL, &render_pass);
        assert(res == VK_SUCCESS);

        SetResourceName(device, VK_OBJECT_TYPE_RENDER_PASS, (uint64_t)render_pass, "CreateRenderPassOptimal");

        return render_pass;
    }

    VkRenderPass SimpleColorWriteRenderPass(VkDevice device, VkImageLayout initialLayout, VkImageLayout passLayout, VkImageLayout finalLayout)
    {
        // color RT
        VkAttachmentDescription attachments[1];
        attachments[0].format = VK_FORMAT_R16G16B16A16_SFLOAT;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // we don't care about the previous contents, this is for a full screen pass with no blending
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = initialLayout;
        attachments[0].finalLayout = finalLayout;
        attachments[0].flags = 0;

        VkAttachmentReference color_reference = { 0, passLayout };

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.flags = 0;
        subpass.inputAttachmentCount = 0;
        subpass.pInputAttachments = NULL;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_reference;
        subpass.pResolveAttachments = NULL;
        subpass.pDepthStencilAttachment = NULL;
        subpass.preserveAttachmentCount = 0;
        subpass.pPreserveAttachments = NULL;

        VkSubpassDependency dep = {};
        dep.dependencyFlags = 0;
        dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
        dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dep.dstSubpass = VK_SUBPASS_EXTERNAL;
        dep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dep.srcSubpass = 0;

        VkRenderPassCreateInfo rp_info = {};
        rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rp_info.pNext = NULL;
        rp_info.attachmentCount = 1;
        rp_info.pAttachments = attachments;
        rp_info.subpassCount = 1;
        rp_info.pSubpasses = &subpass;
        rp_info.dependencyCount = 1;
        rp_info.pDependencies = &dep;

        VkRenderPass renderPass;
        VkResult res = vkCreateRenderPass(device, &rp_info, NULL, &renderPass);
        assert(res == VK_SUCCESS);

        SetResourceName(device, VK_OBJECT_TYPE_RENDER_PASS, (uint64_t)renderPass, "SimpleColorWriteRenderPass");

        return renderPass;
    }

    VkRenderPass SimpleColorBlendRenderPass(VkDevice device, VkFormat format, VkImageLayout initialLayout, VkImageLayout passLayout, VkImageLayout finalLayout)
    {
        // color RT
        VkAttachmentDescription attachments[1];
        attachments[0].format = format;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = initialLayout;
        attachments[0].finalLayout = finalLayout;
        attachments[0].flags = 0;

        VkAttachmentReference color_reference = { 0, passLayout };

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.flags = 0;
        subpass.inputAttachmentCount = 0;
        subpass.pInputAttachments = NULL;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_reference;
        subpass.pResolveAttachments = NULL;
        subpass.pDepthStencilAttachment = NULL;
        subpass.preserveAttachmentCount = 0;
        subpass.pPreserveAttachments = NULL;

        VkSubpassDependency dependencies[2];

        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPassCreateInfo rp_info = {};
        rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rp_info.pNext = NULL;
        rp_info.attachmentCount = 1;
        rp_info.pAttachments = attachments;
        rp_info.subpassCount = 1;
        rp_info.pSubpasses = &subpass;
        rp_info.dependencyCount = 2;
        rp_info.pDependencies = dependencies;

        VkRenderPass renderPass;
        VkResult res = vkCreateRenderPass(device, &rp_info, NULL, &renderPass);
        assert(res == VK_SUCCESS);

        SetResourceName(device, VK_OBJECT_TYPE_RENDER_PASS, (uint64_t)renderPass, "SimpleColorBlendRenderPass");

        return renderPass;
    }


    void SetViewportAndScissor(VkCommandBuffer cmd_buf, uint32_t topX, uint32_t topY, uint32_t width, uint32_t height)
    {
        VkViewport viewport;
        viewport.x = static_cast<float>(topX);
        viewport.y = static_cast<float>(topY) + static_cast<float>(height);
        viewport.width = static_cast<float>(width);
        viewport.height = -static_cast<float>(height);
        viewport.minDepth = (float)0.0f;
        viewport.maxDepth = (float)1.0f;
        vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

        VkRect2D   scissor;
        scissor.extent.width = (uint32_t)(width);
        scissor.extent.height = (uint32_t)(height);
        scissor.offset.x = topX;
        scissor.offset.y = topY;
        vkCmdSetScissor(cmd_buf, 0, 1, &scissor);
    }

    void SetDescriptorSet(VkDevice device, uint32_t index, VkImageView imageView, VkImageLayout imageLayout, VkSampler *pSampler, VkDescriptorSet descriptorSet)
    {
        VkDescriptorImageInfo desc_image;
        desc_image.sampler = (pSampler == NULL) ? VK_NULL_HANDLE : *pSampler;
        desc_image.imageView = imageView;
        desc_image.imageLayout = imageLayout;

        VkWriteDescriptorSet write;
        write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = NULL;
        write.dstSet = descriptorSet;
        write.descriptorCount = 1;
        write.descriptorType = (pSampler == NULL) ? VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE : VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = &desc_image;
        write.dstBinding = index;
        write.dstArrayElement = 0;

        vkUpdateDescriptorSets(device, 1, &write, 0, NULL);
    }

    void SetDescriptorSet(VkDevice device, uint32_t index, uint32_t descriptorsCount, const std::vector<VkImageView>& imageViews, VkImageLayout imageLayout, VkSampler* pSampler, VkDescriptorSet descriptorSet)
    {
        std::vector<VkDescriptorImageInfo> desc_images(descriptorsCount);
        uint32_t i = 0;
        for (; i < imageViews.size(); ++i)
        {
            desc_images[i].sampler = (pSampler == NULL) ? VK_NULL_HANDLE : *pSampler;
            desc_images[i].imageView = imageViews[i];
            desc_images[i].imageLayout = imageLayout;
        }
        // we should still assign the remaining descriptors
        // Using the VK_EXT_robustness2 extension, it is possible to assign a NULL one
        for (; i < descriptorsCount; ++i)
        {
            desc_images[i].sampler = (pSampler == NULL) ? VK_NULL_HANDLE : *pSampler;
            desc_images[i].imageView = VK_NULL_HANDLE;
            desc_images[i].imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        }

        VkWriteDescriptorSet write;
        write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = NULL;
        write.dstSet = descriptorSet;
        write.descriptorCount = descriptorsCount;
        write.descriptorType = (pSampler == NULL) ? VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE : VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = desc_images.data();
        write.dstBinding = index;
        write.dstArrayElement = 0;

        vkUpdateDescriptorSets(device, 1, &write, 0, NULL);
    }

    void SetDescriptorSet(VkDevice device, uint32_t index, VkImageView imageView, VkSampler *pSampler, VkDescriptorSet descriptorSet)
    {
        SetDescriptorSet(device, index, imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, pSampler, descriptorSet);
    }

    void SetDescriptorSet(VkDevice device, uint32_t index, uint32_t descriptorsCount, const std::vector<VkImageView>& imageViews, VkSampler* pSampler, VkDescriptorSet descriptorSet)
    {
        SetDescriptorSet(device, index, descriptorsCount, imageViews, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, pSampler, descriptorSet);
    }

    void SetDescriptorSetForDepth(VkDevice device, uint32_t index, VkImageView imageView, VkSampler *pSampler, VkDescriptorSet descriptorSet)
    {
        SetDescriptorSet(device, index, imageView, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, pSampler, descriptorSet);
    }

    void SetDescriptorSet(VkDevice device, uint32_t index, VkImageView imageView, VkDescriptorSet descriptorSet)
    {
        VkDescriptorImageInfo desc_image;
        desc_image.sampler = VK_NULL_HANDLE;
        desc_image.imageView = imageView;
        desc_image.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        VkWriteDescriptorSet write;
        write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = NULL;
        write.dstSet = descriptorSet;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        write.pImageInfo = &desc_image;
        write.dstBinding = index;
        write.dstArrayElement = 0;

        vkUpdateDescriptorSets(device, 1, &write, 0, NULL);
    }

    VkResult CreateDescriptorSetLayoutVK(VkDevice device, std::vector<VkDescriptorSetLayoutBinding> *pDescriptorLayoutBinding, VkDescriptorSetLayout *pDescSetLayout, VkDescriptorSet *pDescriptorSet)
    {
        VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
        descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_layout.pNext = NULL;
        descriptor_layout.bindingCount = (uint32_t)pDescriptorLayoutBinding->size();
        descriptor_layout.pBindings = pDescriptorLayoutBinding->data();

        VkResult res = vkCreateDescriptorSetLayout(device, &descriptor_layout, NULL, pDescSetLayout);
        assert(res == VK_SUCCESS);
        return res;
    }

    VkFramebuffer CreateFrameBuffer(VkDevice device, VkRenderPass renderPass, const std::vector<VkImageView> *pAttachments, uint32_t Width, uint32_t Height)
    {
        VkFramebufferCreateInfo fb_info = {};
        fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fb_info.layers = 1;
        fb_info.pNext = NULL;
        fb_info.width = Width;
        fb_info.height = Height;

        VkFramebuffer frameBuffer;

        VkResult res;
        fb_info.renderPass = renderPass;
        fb_info.pAttachments = pAttachments->data();
        fb_info.attachmentCount = (uint32_t)pAttachments->size();
        res = vkCreateFramebuffer(device, &fb_info, NULL, &frameBuffer);
        assert(res == VK_SUCCESS);

        SetResourceName(device, VK_OBJECT_TYPE_FRAMEBUFFER, (uint64_t)frameBuffer, "HelperCreateFrameBuffer");

        return frameBuffer;
    }

    void BeginRenderPass(VkCommandBuffer commandList, VkRenderPass renderPass, VkFramebuffer frameBuffer, const std::vector<VkClearValue> *pClearValues, uint32_t Width, uint32_t Height)
    {
        VkRenderPassBeginInfo rp_begin;
        rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rp_begin.pNext = NULL;
        rp_begin.renderPass = renderPass;
        rp_begin.framebuffer = frameBuffer;
        rp_begin.renderArea.offset.x = 0;
        rp_begin.renderArea.offset.y = 0;
        rp_begin.renderArea.extent.width = Width;
        rp_begin.renderArea.extent.height = Height;
        rp_begin.pClearValues = pClearValues->data();
        rp_begin.clearValueCount = (uint32_t)pClearValues->size();
        vkCmdBeginRenderPass(commandList, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
    }

}