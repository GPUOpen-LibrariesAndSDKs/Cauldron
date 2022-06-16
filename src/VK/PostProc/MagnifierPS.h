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

#pragma once

#include "PostProcPS.h"

#include "Base/Texture.h"

namespace CAULDRON_VK
{
	// Forward declarations
	class DynamicBufferRing;
	class SwapChain;

	class MagnifierPS
	{
	public:
	//--------------------------------------------------------------------------------------------------------
	// DEFINITIONS
	//--------------------------------------------------------------------------------------------------------
		struct PassParameters
		{
			PassParameters()
				: uImageWidth(1)
				, uImageHeight(1)
				, iMousePos{ 0, 0 }
				, fBorderColorRGB{ 1, 1, 1, 1 }
				, fMagnificationAmount(6.0f)
				, fMagnifierScreenRadius(0.35f)
				, iMagnifierOffset{ 500, -500 }
			{}

			uint32_t    uImageWidth;
			uint32_t    uImageHeight;
			int         iMousePos[2];            // in pixels, driven by ImGuiIO.MousePos.xy

			float       fBorderColorRGB[4];      // Linear RGBA

			float       fMagnificationAmount;    // [1-...]
			float       fMagnifierScreenRadius;  // [0-1]
			mutable int iMagnifierOffset[2];     // in pixels
		};

	//--------------------------------------------------------------------------------------------------------
	// INTERFACE
	//--------------------------------------------------------------------------------------------------------
	public:
		void OnCreate(
			Device* pDevice
			, ResourceViewHeaps* pResourceViewHeaps
			, DynamicBufferRing* pDynamicBufferRing
			, StaticBufferPool* pStaticBufferPool
			, VkFormat outFormat
			, bool bOutputsToSwapchain = false
		);

		void OnDestroy();
		void OnCreateWindowSizeDependentResources(uint32_t Width, uint32_t Height);
		void OnDestroyWindowSizeDependentResources();
		void BeginPass(VkCommandBuffer cmd, VkRect2D renderArea, SwapChain* pSwapChain = nullptr);
		void EndPass(VkCommandBuffer cmd);


		// Draws a magnified region on top of the given image to OnCreateWindowSizeDependentResources()
		// if @pSwapChain is provided, expects swapchain to be sycned before this call.
		// Barriers are to be managed by the caller.
		void Draw(
			VkCommandBuffer         cmd
			, const PassParameters& params
			, VkImageView			imageViewSrc
			, SwapChain*            pSwapChain = nullptr
		);

		inline Texture& GetPassOutput() { assert(!m_bOutputsToSwapchain); return m_TexPassOutput; }
		inline VkImage GetPassOutputResource() const { assert(!m_bOutputsToSwapchain); return m_TexPassOutput.Resource(); }
		inline VkImageView GetPassOutputRTV() const { assert(!m_bOutputsToSwapchain); return m_RTVOutput; }
		inline VkImageView GetPassOutputSRV() const  { assert(!m_bOutputsToSwapchain); return m_SRVOutput; }
		inline VkRenderPass GetPassRenderPass() const { assert(!m_bOutputsToSwapchain); return m_RenderPass; }
		void UpdatePipelines(VkRenderPass renderPass);
	private:

		void CompileShaders(StaticBufferPool* pStaticBufferPool, VkFormat outFormat);
		void DestroyShaders();

		void InitializeDescriptorSets();
		void UpdateDescriptorSets(VkImageView ImageViewSrc);
		void DestroyDescriptorSets();
		VkDescriptorBufferInfo SetConstantBufferData(const PassParameters& params);
		static void KeepMagnifierOnScreen(const PassParameters& params);

	//--------------------------------------------------------------------------------------------------------
	// DATA
	//--------------------------------------------------------------------------------------------------------
	private:
		//
		// DEVICE & MEMORY
		//
		Device* m_pDevice = nullptr;
		ResourceViewHeaps* m_pResourceViewHeaps = nullptr;
		DynamicBufferRing* m_pDynamicBufferRing = nullptr;

		//
		// DESCRIPTOR SETS & LAYOUTS
		//
		VkDescriptorSet m_DescriptorSet[3];
		VkDescriptorSetLayout m_DescriptorSetLayout;
		uint32_t m_DescriptorSetIndex = 0;

		//
		// RENDER PASSES & RESOURCE VIEWS
		//
		VkSampler   m_SamplerSrc;

		VkRenderPass  m_RenderPass;
		VkFramebuffer m_FrameBuffer;
		VkImageView   m_RTVOutput;
		VkImageView   m_SRVOutput;

		//
		// BUFFERS
		//
		// None.

		//
		// TEXTURES
		//
		VkFormat m_OutFormat;
		Texture m_TexPassOutput;

		// 
		// SHADERS
		//
		PostProcPS m_ShaderMagnify;

		//
		// MISC
		//
		bool m_bOutputsToSwapchain = false;
	};
}