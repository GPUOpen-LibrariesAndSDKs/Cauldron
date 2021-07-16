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

#include "MagnifierPS.h"

#include "Base/Device.h"
#include "Base/DynamicBufferRing.h"
#include "Base/UserMarkers.h"
#include "Misc/Misc.h"
#include "Misc/DxgiFormatHelper.h"

#include <fstream>
#include <sstream>

using namespace CAULDRON_DX12;

static const char* SHADER_FILE_NAME_MAGNIFIER = "MagnifierPS.hlsl";
using cbHandle_t = D3D12_GPU_VIRTUAL_ADDRESS;

void MagnifierPS::OnCreate(
	Device* pDevice
	, ResourceViewHeaps* pResourceViewHeaps
	, DynamicBufferRing* pDynamicBufferRing
	, StaticBufferPool* pStaticBufferPool
	, DXGI_FORMAT outFormat
	, bool bOutputsToSwapchain /*= false*/
)
{
	m_pDevice = pDevice;
	m_pResourceViewHeaps = pResourceViewHeaps;
	m_pDynamicBufferRing = pDynamicBufferRing;
	m_bOutputsToSwapchain = bOutputsToSwapchain;

	InitializeHeaps();
	CompileShaders(pStaticBufferPool, outFormat);
}


void MagnifierPS::OnDestroy()
{
	DestroyShaders();
}


void MagnifierPS::OnCreateWindowSizeDependentResources(const Texture* pTexture)
{
	// Create pTexture-sized output resource
	CD3DX12_RESOURCE_DESC textureDesc = {};
	textureDesc.Width  = pTexture->GetWidth();
	textureDesc.Height = pTexture->GetHeight();
	textureDesc.MipLevels = 1;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	constexpr D3D12_RESOURCE_STATES RSC_STATE = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
	const D3D12_CLEAR_VALUE CLR_VAL = { textureDesc.Format, 0.0f, 0.0f, 0.0f, 1.0f };

	//
	// Inititalize Textures
	//
	textureDesc.Format = pTexture->GetFormat();
	if (!m_bOutputsToSwapchain)
	{
		m_TexPassOutput.Init(m_pDevice, "TexMagnifierOutput", &textureDesc, RSC_STATE, NULL);
		m_TexPassOutput.CreateSRV(0, &m_SRVOutput);
		m_TexPassOutput.CreateUAV(0, &m_UAVOutput);
		m_TexPassOutput.CreateRTV(0, &m_RTVOutput);
		m_TexPassOutput.CreateRTV(0, &m_RTVOutputSrgb, -1, -1, -1, ConvertIntoGammaFormat(textureDesc.Format));
	}
}


void MagnifierPS::OnDestroyWindowSizeDependentResources()
{
	if (!m_bOutputsToSwapchain)
	{
		m_TexPassOutput.OnDestroy();
	}
}

void MagnifierPS::Draw(
	ID3D12GraphicsCommandList* pCmd
	, const PassParameters&    params
	, CBV_SRV_UAV              SRVInputImage
	, D3D12_CPU_DESCRIPTOR_HANDLE* pSwapchainRTVDescHandle /*= nullptr*/
)
{
	UserMarker marker(pCmd, "Magnifier");

	if(m_bOutputsToSwapchain && !pSwapchainRTVDescHandle)
	{
		Trace("Warning: MagnifierPS::Draw() is called with NULL swapchain RTV handle, bOutputsToSwapchain was true during MagnifierPS::OnCreate(). No Draw calls will be issued.");
		return;
	}

	D3D12_GPU_VIRTUAL_ADDRESS cbHandle = SetConstantBufferData(params);
	D3D12_CPU_DESCRIPTOR_HANDLE* pRTV = pSwapchainRTVDescHandle ? pSwapchainRTVDescHandle : &m_RTVOutput.GetCPU();

	pCmd->OMSetRenderTargets(1, pRTV, true, NULL);

	m_ShaderMagnify.Draw(pCmd, 1, &SRVInputImage, cbHandle);
}


void MagnifierPS::InitializeHeaps()
{
	m_pResourceViewHeaps->AllocCBV_SRV_UAVDescriptor(1, &m_UAVOutput);
	m_pResourceViewHeaps->AllocCBV_SRV_UAVDescriptor(1, &m_SRVOutput);
	m_pResourceViewHeaps->AllocRTVDescriptor(1, &m_RTVOutput);
	m_pResourceViewHeaps->AllocRTVDescriptor(1, &m_RTVOutputSrgb);
}

void MagnifierPS::CompileShaders(StaticBufferPool* pStaticBufferPool, DXGI_FORMAT outFormat)
{
	//
	// Static Sampler States
	//
	D3D12_STATIC_SAMPLER_DESC SamplerDescs[3] = {};
	for (uint32_t i = 0; i < 3; ++i)
	{
		SamplerDescs[i].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		SamplerDescs[i].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		SamplerDescs[i].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		SamplerDescs[i].MipLODBias = 0;
		SamplerDescs[i].MaxAnisotropy = 0;
		SamplerDescs[i].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		SamplerDescs[i].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		SamplerDescs[i].MinLOD = 0;
		SamplerDescs[i].MaxLOD = D3D12_FLOAT32_MAX;
		SamplerDescs[i].ShaderRegister = i;
		SamplerDescs[i].RegisterSpace = 0;
		SamplerDescs[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	}

	// [0] : Nearest Sampler
	SamplerDescs[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;

	// [1] : Linear Sampler
	SamplerDescs[1].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;

	// [2] : Linear Black Border Sampler
	SamplerDescs[2].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	SamplerDescs[2].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	SamplerDescs[2].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	SamplerDescs[2].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	SamplerDescs[2].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;

	//
	// Compile Magnifier Pixel Shader
	//
	constexpr int NUM_SRVS = 1;
	constexpr int NUM_SAMPLERS = 3;
	m_ShaderMagnify.OnCreate(m_pDevice
		, SHADER_FILE_NAME_MAGNIFIER
		, m_pResourceViewHeaps
		, pStaticBufferPool
		, NUM_SRVS
		, NUM_SAMPLERS, SamplerDescs
		, outFormat);
	return;
}

void MagnifierPS::UpdatePipeline(DXGI_FORMAT outFormat)
{
	m_ShaderMagnify.UpdatePipeline(outFormat);
}

void MagnifierPS::DestroyShaders()
{
	m_ShaderMagnify.OnDestroy();
}

D3D12_GPU_VIRTUAL_ADDRESS CAULDRON_DX12::MagnifierPS::SetConstantBufferData(const PassParameters& params)
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

void CAULDRON_DX12::MagnifierPS::KeepMagnifierOnScreen(const CAULDRON_DX12::MagnifierPS::PassParameters& params)
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
