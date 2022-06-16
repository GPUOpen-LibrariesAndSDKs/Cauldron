// AMD Cauldron code
// 
// Copyright(c) 2020 Advanced Micro Devices, Inc.All rights reserved.
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
#include "MagnifierPS.h"

#include "Base/Device.h"
#include "Base/DynamicBufferRing.h"
#include "Base/ExtDebugUtils.h"
#include "Base/Helper.h"
#include "base/SwapChain.h"
#include "Misc/Misc.h"

#include <cassert>
#include <fstream>
#include <sstream>

using namespace CAULDRON_VK;

static const char* SHADER_FILE_NAME_MAGNIFIER = "MagnifierPS.glsl";
static const char* SHADER_ENTRY_POINT         = "main";
using cbHandle_t = VkDescriptorBufferInfo;



void MagnifierPS::OnCreate(
	Device* pDevice
	, ResourceViewHeaps* pResourceViewHeaps
	, DynamicBufferRing* pDynamicBufferRing
	, StaticBufferPool* pStaticBufferPool
	, VkFormat outFormat
	, bool bOutputsToSwapchain /*= false*/
)
{
	m_pDevice = pDevice;
	m_pResourceViewHeaps = pResourceViewHeaps;
	m_pDynamicBufferRing = pDynamicBufferRing;
	m_bOutputsToSwapchain = bOutputsToSwapchain;
	m_OutFormat = outFormat;
	m_SRVOutput = nullptr;

	InitializeDescriptorSets();
	
	m_RenderPass = SimpleColorBlendRenderPass(m_pDevice->GetDevice(), m_OutFormat, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	CompileShaders(pStaticBufferPool, outFormat); // expects a non-null Render Pass!
}


void MagnifierPS::OnDestroy()
{
	vkDestroyRenderPass(m_pDevice->GetDevice(), m_RenderPass, NULL);

	DestroyShaders();
	DestroyDescriptorSets();
}


void MagnifierPS::OnCreateWindowSizeDependentResources(uint32_t Width, uint32_t Height)
{
	if (!m_bOutputsToSwapchain)
	{
		VkDevice device = m_pDevice->GetDevice();
		const uint32_t ImgWidth = Width;
		const uint32_t ImgHeight = Height;

		// create pass output image and its views
		VkImageCreateInfo image_info = {};
		image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image_info.pNext = NULL;
		image_info.imageType = VK_IMAGE_TYPE_2D;
		image_info.format = m_OutFormat;
		image_info.extent.width = ImgWidth;
		image_info.extent.height = ImgHeight;
		image_info.extent.depth = 1;
		image_info.mipLevels = 1;
		image_info.arrayLayers = 1;
		image_info.samples = VK_SAMPLE_COUNT_1_BIT;
		image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		image_info.queueFamilyIndexCount = 0;
		image_info.pQueueFamilyIndices = NULL;
		image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		image_info.usage = (VkImageUsageFlags)(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
		image_info.flags = 0;
		image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		m_TexPassOutput.Init(m_pDevice, &image_info, "TexMagnifierOutput");
		m_TexPassOutput.CreateSRV(&m_SRVOutput);
		m_TexPassOutput.CreateRTV(&m_RTVOutput);

		// create framebuffer
		VkImageView attachments[] = { m_RTVOutput };
		VkFramebufferCreateInfo fb_info = {};
		fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fb_info.pNext = NULL;
		fb_info.renderPass = m_RenderPass;
		fb_info.attachmentCount = 1;
		fb_info.pAttachments = attachments;
		fb_info.width = ImgWidth;
		fb_info.height = ImgHeight;
		fb_info.layers = 1;
		VkResult res = vkCreateFramebuffer(device, &fb_info, NULL, &m_FrameBuffer);
		assert(res == VK_SUCCESS);
		SetResourceName(device, VK_OBJECT_TYPE_FRAMEBUFFER, (uint64_t)m_FrameBuffer, "MagnifierPS");
	}
}


void MagnifierPS::OnDestroyWindowSizeDependentResources()
{
	VkDevice device = m_pDevice->GetDevice();
	if (m_SamplerSrc)
	{
		vkDestroySampler(m_pDevice->GetDevice(), m_SamplerSrc, nullptr);
		m_SamplerSrc = nullptr;
	}

	if (!m_bOutputsToSwapchain)
	{
		vkDestroyImageView(device, m_SRVOutput, NULL);
		vkDestroyImageView(device, m_RTVOutput, NULL);
		m_TexPassOutput.OnDestroy();

		vkDestroyFramebuffer(device, m_FrameBuffer, NULL);

		m_SRVOutput = nullptr;
		m_RTVOutput = nullptr;
		m_FrameBuffer = nullptr;
	}
}

void MagnifierPS::CompileShaders(StaticBufferPool* pStaticBufferPool, VkFormat outFormat)
{
	//
	// Compile Magnifier Pixel Shader
	//
	m_ShaderMagnify.OnCreate(m_pDevice,
		m_RenderPass
		, SHADER_FILE_NAME_MAGNIFIER
		, SHADER_ENTRY_POINT
		, ""
		, pStaticBufferPool
		, m_pDynamicBufferRing
		, m_DescriptorSetLayout
		, NULL, VK_SAMPLE_COUNT_1_BIT
	);
	return;
}

void MagnifierPS::DestroyShaders()
{
	m_ShaderMagnify.OnDestroy();
}

void MagnifierPS::InitializeDescriptorSets()
{
	std::vector<VkDescriptorSetLayoutBinding> layoutBindingsMagnifier(2);
	layoutBindingsMagnifier[0] = {};
	layoutBindingsMagnifier[0].binding = 0;
	layoutBindingsMagnifier[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	layoutBindingsMagnifier[0].descriptorCount = 1;
	layoutBindingsMagnifier[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	layoutBindingsMagnifier[0].pImmutableSamplers = NULL;

	layoutBindingsMagnifier[1] = {};
	layoutBindingsMagnifier[1].binding = 1;
	layoutBindingsMagnifier[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	layoutBindingsMagnifier[1].descriptorCount = 1;
	layoutBindingsMagnifier[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	layoutBindingsMagnifier[1].pImmutableSamplers = NULL;

	m_pResourceViewHeaps->CreateDescriptorSetLayout(&layoutBindingsMagnifier, &m_DescriptorSetLayout);

	for (int i = 0; i < 3; i++)
	{
		m_pResourceViewHeaps->AllocDescriptor(m_DescriptorSetLayout, &m_DescriptorSet[i]);
		m_pDynamicBufferRing->SetDescriptorSet(1, sizeof(PassParameters), m_DescriptorSet[i]);
	}
}
void MagnifierPS::UpdateDescriptorSets(VkImageView ImageViewSrc)
{
	// nearest sampler
	{
		VkSamplerCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		info.magFilter = VK_FILTER_NEAREST;
		info.minFilter = VK_FILTER_NEAREST;
		info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		info.minLod = -1000;
		info.maxLod = 1000;
		info.maxAnisotropy = 1.0f;
		VkResult res = vkCreateSampler(m_pDevice->GetDevice(), &info, NULL, &m_SamplerSrc);
		assert(res == VK_SUCCESS);
	}

	constexpr size_t NUM_WRITES = 1;
	constexpr size_t NUM_IMAGES = 1;

	VkDescriptorImageInfo ImgInfo[NUM_IMAGES] = {};
	VkWriteDescriptorSet  SetWrites[NUM_WRITES] = {};
	
	for (size_t i = 0; i < NUM_IMAGES; ++i) { ImgInfo[i].sampler = m_SamplerSrc; }
	for (size_t i = 0; i < NUM_WRITES; ++i)
	{
		SetWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		SetWrites[i].dstSet = m_DescriptorSet[m_DescriptorSetIndex];
		SetWrites[i].descriptorCount = 1;
		SetWrites[i].pImageInfo = ImgInfo + i;
	}

	SetWrites[0].dstBinding = 0;
	SetWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	ImgInfo[0].imageView = ImageViewSrc;
	ImgInfo[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	vkUpdateDescriptorSets(m_pDevice->GetDevice(), _countof(SetWrites), SetWrites, 0, NULL);
}

void MagnifierPS::DestroyDescriptorSets()
{
	for (int i = 0; i < 3; i++)
		m_pResourceViewHeaps->FreeDescriptor(m_DescriptorSet[i]);
	vkDestroyDescriptorSetLayout(m_pDevice->GetDevice(), m_DescriptorSetLayout, NULL);
}

void MagnifierPS::UpdatePipelines(VkRenderPass renderPass)
{
	m_ShaderMagnify.UpdatePipeline(renderPass, NULL, VK_SAMPLE_COUNT_1_BIT);
}

void MagnifierPS::BeginPass(VkCommandBuffer cmd, VkRect2D renderArea, SwapChain* pSwapChain)
{
	VkRenderPassBeginInfo rp_begin = {};
	rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rp_begin.pNext = NULL;
	rp_begin.renderPass = pSwapChain ? pSwapChain->GetRenderPass() : m_RenderPass;
	rp_begin.framebuffer = pSwapChain ? pSwapChain->GetCurrentFramebuffer() : m_FrameBuffer;
	rp_begin.renderArea = renderArea;
	rp_begin.clearValueCount = 0;
	rp_begin.pClearValues = NULL;

	vkCmdBeginRenderPass(cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
}

void MagnifierPS::EndPass(VkCommandBuffer cmd)
{
	vkCmdEndRenderPass(cmd);
}


void MagnifierPS::Draw(
	VkCommandBuffer           cmd
	, const PassParameters&   params
	, VkImageView			imageViewSrc
	, SwapChain* pSwapChain /* = nullptr*/
)
{
	SetPerfMarkerBegin(cmd, "Magnifier");

	if (m_bOutputsToSwapchain && !pSwapChain)
	{
		Trace("Warning: MagnifierPS::Draw() is called with NULL swapchain RTV handle, bOutputsToSwapchain was true during MagnifierPS::OnCreate(). No Draw calls will be issued.");
		return;
	}

	UpdateDescriptorSets(imageViewSrc);

	cbHandle_t cbHandle = SetConstantBufferData(params);

	VkRect2D renderArea;
	renderArea.offset.x = 0;
	renderArea.offset.y = 0;
	renderArea.extent.width = params.uImageWidth;
	renderArea.extent.height = params.uImageHeight;
	BeginPass(cmd, renderArea, pSwapChain);

	m_ShaderMagnify.Draw(cmd, &cbHandle, m_DescriptorSet[m_DescriptorSetIndex]);
	EndPass(cmd);
	SetPerfMarkerEnd(cmd);

	m_DescriptorSetIndex++;

	if (m_DescriptorSetIndex >= 3)
		m_DescriptorSetIndex = 0;
}


VkDescriptorBufferInfo CAULDRON_VK::MagnifierPS::SetConstantBufferData(const PassParameters& params)
{
	KeepMagnifierOnScreen(params);

	// memcpy to cbuffer
	cbHandle_t cbHandle = {};
	uint32_t* pConstMem = nullptr;
	const uint32_t szConstBuff = sizeof(PassParameters);
	void const* pConstBufSrc = static_cast<void const*>(&params);
	m_pDynamicBufferRing->AllocConstantBuffer(szConstBuff, reinterpret_cast<void**>(&pConstMem), &cbHandle);
	memcpy(pConstMem, pConstBufSrc, szConstBuff);

	return cbHandle;
}

void CAULDRON_VK::MagnifierPS::KeepMagnifierOnScreen(const PassParameters& params)
{
	const int IMAGE_SIZE[2] = { static_cast<int>(params.uImageWidth), static_cast<int>(params.uImageHeight) };
	const int& W = IMAGE_SIZE[0];
	const int& H = IMAGE_SIZE[1];

	const int radiusInPixelsMagifier     = static_cast<int>(params.fMagnifierScreenRadius * H);
	const int radiusInPixelsMagifiedArea = static_cast<int>(params.fMagnifierScreenRadius * H / params.fMagnificationAmount);

	const bool bCirclesAreOverlapping = radiusInPixelsMagifiedArea + radiusInPixelsMagifier > std::sqrt(params.iMagnifierOffset[0] * params.iMagnifierOffset[0] + params.iMagnifierOffset[1] * params.iMagnifierOffset[1]);


	if (bCirclesAreOverlapping) // don't let the two circles overlap
	{
		params.iMagnifierOffset[0] = radiusInPixelsMagifiedArea + radiusInPixelsMagifier + 1;
		params.iMagnifierOffset[1] = radiusInPixelsMagifiedArea + radiusInPixelsMagifier + 1;
	}

	for (int i = 0; i < 2; ++i) // try to move the magnified area to be fully on screen, if possible
	{
		const bool bMagnifierOutOfScreenRegion = params.iMousePos[i] + params.iMagnifierOffset[i] + radiusInPixelsMagifier > IMAGE_SIZE[i] 
			                                  || params.iMousePos[i] + params.iMagnifierOffset[i] - radiusInPixelsMagifier < 0;
		if (bMagnifierOutOfScreenRegion)
		{
			if (!(params.iMousePos[i] - params.iMagnifierOffset[i] + radiusInPixelsMagifier > IMAGE_SIZE[i]
				|| params.iMousePos[i] - params.iMagnifierOffset[i] - radiusInPixelsMagifier < 0))
			{
				// flip offset if possible
				params.iMagnifierOffset[i] = -params.iMagnifierOffset[i];
			}
			else
			{
				// otherwise clamp
				if (params.iMousePos[i] + params.iMagnifierOffset[i] + radiusInPixelsMagifier > IMAGE_SIZE[i])
					params.iMagnifierOffset[i] = IMAGE_SIZE[i] - params.iMousePos[i] - radiusInPixelsMagifier;
				if (params.iMousePos[i] + params.iMagnifierOffset[i] - radiusInPixelsMagifier < 0)
					params.iMagnifierOffset[i] = -params.iMousePos[i] + radiusInPixelsMagifier;
			}
		}
	}
}