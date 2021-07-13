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

namespace CAULDRON_DX12
{
	// Forward declarations
	class DynamicBufferRing;

	class MagnifierPS
	{
	//--------------------------------------------------------------------------------------------------------
	// DEFINITIONS
	//--------------------------------------------------------------------------------------------------------
	public:
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
			, DXGI_FORMAT outFormat
			, bool bOutputsToSwapchain = false
		);

		void OnDestroy();
		void OnCreateWindowSizeDependentResources(const Texture* pTexture);
		void OnDestroyWindowSizeDependentResources();

		// Draws a magnified region on top of the given image (on a separate output)
		// @SRVInputImage should be the same size as the Texture resource given on OnCreateWindowSizeDependentResources() call.
		// if @pSwapchainRTVDescHandle is provided, expects swapchain to be sycned before this call.
		// Barriers are to be managed by the caller.
		void Draw(
			ID3D12GraphicsCommandList* pCmd
			, const PassParameters&    params
			, CBV_SRV_UAV              SRVInputImage
			, D3D12_CPU_DESCRIPTOR_HANDLE* pSwapchainRTVDescHandle = nullptr
		);

		inline ID3D12Resource* GetPassOutputResource() const { return m_TexPassOutput.GetResource(); }
		inline CBV_SRV_UAV GetPassOutputUAV()     const { return m_UAVOutput; }
		inline RTV         GetPassOutputRTV()     const { return m_RTVOutput; }
		inline RTV         GetPassOutputRTVSrgb()     const { return m_RTVOutputSrgb; }
		inline CBV_SRV_UAV GetPassOutputSRV()     const { return m_SRVOutput; }
		void UpdatePipeline(DXGI_FORMAT outFormat);

	private:
		void InitializeHeaps();
		void CompileShaders(StaticBufferPool* pStaticBufferPool, DXGI_FORMAT outFormat);
		void DestroyShaders();

		D3D12_GPU_VIRTUAL_ADDRESS SetConstantBufferData(const PassParameters& params);
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
		// STATE
		//
		// None.

		//
		// RESOURCE VIEWS
		//
		CBV_SRV_UAV m_UAVOutput;
		RTV         m_RTVOutput;
		RTV         m_RTVOutputSrgb;
		CBV_SRV_UAV m_SRVOutput;

		//
		// BUFFERS
		//
		// None.

		//
		// TEXTURES
		//
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