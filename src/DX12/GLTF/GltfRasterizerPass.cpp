// AMD Cauldron code
// AMD Work graph Rasterizer code
// 
// Copyright(c) 2023 Advanced Micro Devices, Inc.All rights reserved.
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
#include <format>
#include "GLTF/GltfHelpers.h"
#include "Base/ShaderCompilerHelper.h"
#include "Misc/ThreadPool.h"
#include "Misc/Misc.h"

#include "GLTF/GltfPbrMaterial.h"
#include "GltfRasterizerPass.h"
#include "GltfPbrPass.h"

#ifdef ENABLE_EXPERIMENTAL_WORKGRAPHS
#include <wrl.h>
#include <comdef.h>
using namespace Microsoft::WRL;
#endif

#include <random>
#include <iostream>

// TODO: use BakeSkinning to not needing to support skinning
#ifdef _DEBUG
#define OPT	""
#else
#define OPT	" -O3"
#endif

namespace CAULDRON_DX12
{
	//--------------------------------------------------------------------------------------
	//
	// OnCreate
	//
	//--------------------------------------------------------------------------------------
	void GltfRasterizerPass::OnCreate(
		Device *pDevice,
		UploadHeap *pUploadHeap,
		ResourceViewHeaps *pHeaps,
		DynamicBufferRing *pDynamicBufferRing,
		StaticBufferPool *pStaticBufferPool,
		GLTFTexturesAndBuffers* pGLTFTexturesAndBuffers,
		Texture* pVBuffer,
		CBV_SRV_UAV* pVBufferUAV,
		CBV_SRV_UAV* pVBufferUAVCPU,
		AsyncPool* pAsyncPool,
		DXGI_FORMAT depthFormat)
	{
		m_pDevice = pDevice;
#ifdef ENABLE_EXPERIMENTAL_WORKGRAPHS
		ThrowIfFailed(m_pDevice->GetDevice()->QueryInterface(IID_PPV_ARGS(&m_pDevice9)));
#endif
		m_pUploadHeap = pUploadHeap;
		m_pResourceViewHeaps = pHeaps;
		m_pStaticBufferPool = pStaticBufferPool;
		m_pDynamicBufferRing = pDynamicBufferRing;
		m_pGLTFTexturesAndBuffers = pGLTFTexturesAndBuffers;

		m_rootSignature = nullptr;
		m_commandSignature = nullptr;
		m_pipelineRender = nullptr;
		for (int t = 0; t < 4; ++t)
			m_pipelineRenderMS[t] = nullptr;

#ifdef ENABLE_EXPERIMENTAL_WORKGRAPHS
		m_memoryInitialized[0] = false;
		m_memoryInitialized[1] = false;
		for (int t = 0; t < 2; ++t)
		{
			m_pipelineWorkGraph[t] = nullptr;
			m_memoryWorkGraph[t] = nullptr;
			m_memorySize[t] = 0;
		}
#endif

		m_numberOfBins = 9;
		m_homogeneous = true;
		m_showBins = false;
		m_fixExpansion = true;
		m_threadLaunch = true;
		m_feedback = m_feedbackBuffer;

		m_pVBuffer = pVBuffer;
		m_pVBufferUAV = pVBufferUAV;
		m_pVBufferUAVCPU = pVBufferUAVCPU;

		CreateRootSignature();
		CreateCommandSignature();

		const json &j3 = pGLTFTexturesAndBuffers->m_pGLTFCommon->j3;

		/////////////////////////////////////////////
		// Create default material

		m_defaultMaterial.m_textureCount = 0;
		m_defaultMaterial.m_doubleSided = false;
		m_defaultMaterial.m_transparent = false;

		// Create static sampler in case there is transparency
		//
		ZeroMemory(&m_samplerDesc, sizeof(m_samplerDesc));
		m_samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		m_samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		m_samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		m_samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		m_samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		m_samplerDesc.MinLOD = 0.0f;
		m_samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
		m_samplerDesc.MipLODBias = 0;
		m_samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		m_samplerDesc.MaxAnisotropy = 1;
		m_samplerDesc.ShaderRegister = 0;
		m_samplerDesc.RegisterSpace = 0;
		m_samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		// Create materials (in a depth pass materials are still needed to handle non opaque textures
		//
		if (j3.find("materials") != j3.end())
		{
			const json &materials = j3["materials"];

			m_materialsData.resize(materials.size());
			for (uint32_t i = 0; i < materials.size(); i++)
			{
				const json &material = materials[i];

				RasterizerMaterial *tfmat = &m_materialsData[i];

				// Load material constants. This is a depth pass and we are only interested in the mask texture
				//               
				tfmat->m_doubleSided = GetElementBoolean(material, "doubleSided", false);
				std::string alphaMode = GetElementString(material, "alphaMode", "OPAQUE");
				tfmat->m_defines["DEF_alphaMode_" + alphaMode] = std::to_string(1);

				// NOTE: Transparent materials not supported for z-buffer rasterizer
				tfmat->m_transparent = (alphaMode != "OPAQUE" && alphaMode != "MASK");

				// If transparent use the baseColorTexture for alpha
				//
				if (alphaMode == "MASK")
				{
					tfmat->m_defines["DEF_alphaCutoff"] = std::to_string(GetElementFloat(material, "alphaCutoff", 0.5));

					auto pbrMetallicRoughnessIt = material.find("pbrMetallicRoughness");
					if (pbrMetallicRoughnessIt != material.end())
					{
						const json &pbrMetallicRoughness = pbrMetallicRoughnessIt.value();

						int id = GetElementInt(pbrMetallicRoughness, "baseColorTexture/index", -1);
						if (id >= 0)
						{
							tfmat->m_defines["MATERIAL_METALLICROUGHNESS"] = "1";

							// allocate descriptor table for the texture
							tfmat->m_textureCount = 1;
							tfmat->m_pTransparency = new CBV_SRV_UAV();
							pHeaps->AllocCBV_SRV_UAVDescriptor(tfmat->m_textureCount, tfmat->m_pTransparency);
							Texture *pTexture = pGLTFTexturesAndBuffers->GetTextureViewByID(id);
							pTexture->CreateSRV(0, tfmat->m_pTransparency);
							tfmat->m_defines["ID_baseColorTexture"] = "0";
							tfmat->m_defines["ID_baseTexCoord"] = std::to_string(GetElementInt(pbrMetallicRoughness, "baseColorTexture/texCoord", 0));
						}
					}
				}
			}
		}

		// Calculate argument-buffer sizes for all Meshes
		//
		size_t primitiveCount = 0;

		if (j3.find("meshes") != j3.end())
		{
			const json& meshes = j3["meshes"];
			m_meshes.resize(meshes.size());
			for (uint32_t i = 0; i < meshes.size(); i++)
			{
				// NOTE: Skinned meshes not supported for z-buffer rasterizer (can be pre-skinned)
				bool bUsingSkinning = pGLTFTexturesAndBuffers->m_pGLTFCommon->FindMeshSkinId(i) != -1;
				if (bUsingSkinning)
					continue;

				const json& primitives = meshes[i]["primitives"];

				primitiveCount += primitives.size();
			}
		}

		m_Arguments.resize(primitiveCount);
		m_Workloads.resize(primitiveCount);

		uint32_t primitiveOffset = 0;

		// Load Meshes
		//
		if (j3.find("meshes") != j3.end())
		{
			const json &meshes = j3["meshes"];
			m_meshes.resize(meshes.size());
			for (uint32_t i = 0; i < meshes.size(); i++)
			{
				// NOTE: Skinned meshes not supported for z-buffer rasterizer (can be pre-skinned)
				bool bUsingSkinning = pGLTFTexturesAndBuffers->m_pGLTFCommon->FindMeshSkinId(i) != -1;
				if (bUsingSkinning)
					continue;

				RasterizerMesh *tfmesh = &m_meshes[i];

				const json &primitives = meshes[i]["primitives"];
				tfmesh->m_pPrimitives.resize(primitives.size());
				for (uint32_t p = 0; p < primitives.size(); p++)
				{
					const json &primitive = primitives[p];
					RasterizerPrimitives *pPrimitive = &tfmesh->m_pPrimitives[p];

					pPrimitive->m_WorkloadOffset = primitiveOffset++;

					ExecAsyncIfThereIsAPool(pAsyncPool, [this, i, &primitive, pPrimitive, depthFormat]()
					{
						bool bUsingSkinning = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->FindMeshSkinId(i) != -1;

						// Set Material
						//
						auto mat = primitive.find("material");
						pPrimitive->m_pMaterial = (mat != primitive.end()) ? &m_materialsData[mat.value()] : &m_defaultMaterial;

						// holds all the #defines from materials, geometry and texture IDs, the VS & PS shaders need this to get the bindings and code paths
						//
						DefineList defines = pPrimitive->m_pMaterial->m_defines;

						// make a list of all the attribute names our pass requires, in the case of a depth pass we only need the position and a few other things. 
						// also track the IDs of required buffers for the z-buffer rasterizer
						//
						int posId = 0;
						int wg0Id = 0;
						int wg1Id = 0;
						int jn0Id = 0;
						int jn1Id = 0;
						std::vector<std::string > requiredAttributes;
						for (auto const & it : primitive["attributes"].items())
						{
							const std::string semanticName = it.key();
							if ((semanticName == "POSITION"))
								posId = it.value();
							if ((semanticName == "WEIGHTS_0"))
								wg0Id = it.value();
							if ((semanticName == "WEIGHTS_1"))
								wg0Id = it.value();
							if ((semanticName == "JOINTS_0"))
								jn0Id = it.value();
							if ((semanticName == "JOINTS_1"))
								jn1Id = it.value();
							if (
								(semanticName == "POSITION") || 
								(semanticName.substr(0, 7) == "WEIGHTS") || // for skinning
								(semanticName.substr(0, 6) == "JOINTS") || // for skinning
								(DoesMaterialUseSemantic(pPrimitive->m_pMaterial->m_defines, semanticName) == true) // if there is transparency this will make sure we use the texture coordinates of that texture
								)
							{
								requiredAttributes.push_back(semanticName);
							}
						}

						// create an input layout from the required attributes
						// shader's can tell the slots from the #defines
						//
						std::vector<std::string> semanticNames;
						std::vector<D3D12_INPUT_ELEMENT_DESC> layout;
						m_pGLTFTexturesAndBuffers->CreateGeometry(primitive, requiredAttributes, semanticNames, layout, defines, &pPrimitive->m_geometry);

						// - Get global indices/location of this mesh in the global pool ----------------------------
						auto pBufferPool = m_pGLTFTexturesAndBuffers->GetStaticBufferPool();
						ID3D12Resource* pMeshData = pBufferPool->GetResource();
						size_t sMeshPool = pBufferPool->GetSize();
						D3D12_GPU_VIRTUAL_ADDRESS aMeshPool = pMeshData->GetGPUVirtualAddress();
						D3D12_GPU_VIRTUAL_ADDRESS aIndexData = pPrimitive->m_geometry.m_IBV.BufferLocation;
						D3D12_GPU_VIRTUAL_ADDRESS aPostnData = m_pGLTFTexturesAndBuffers->GetVBVFromAttribute(posId).BufferLocation;
						D3D12_GPU_VIRTUAL_ADDRESS aWght0Data = m_pGLTFTexturesAndBuffers->GetVBVFromAttribute(wg0Id).BufferLocation;
						D3D12_GPU_VIRTUAL_ADDRESS aWght1Data = m_pGLTFTexturesAndBuffers->GetVBVFromAttribute(wg1Id).BufferLocation;
						D3D12_GPU_VIRTUAL_ADDRESS aJont0Data = m_pGLTFTexturesAndBuffers->GetVBVFromAttribute(jn0Id).BufferLocation;
						D3D12_GPU_VIRTUAL_ADDRESS aJont1Data = m_pGLTFTexturesAndBuffers->GetVBVFromAttribute(jn1Id).BufferLocation;

						// - Workload calculation and upload to buffer ----------------------------------------------
						const size_t idxSize = pPrimitive->m_geometry.m_indexType == DXGI_FORMAT_R32_UINT ? sizeof(long) : sizeof(short);
						const size_t posSize = sizeof(float) * 3;
						const size_t wgtSize = sizeof(float) * 4;
						const size_t jntSize = sizeof(unsigned int) * 4;

						assert(((aIndexData - aMeshPool) % idxSize) == 0);
						assert(((aPostnData - aMeshPool) % posSize) == 0);
						assert(((aWght0Data - aMeshPool) % wgtSize) == 0 || !bUsingSkinning);
						assert(((aWght1Data - aMeshPool) % wgtSize) == 0 || !bUsingSkinning);
						assert(((aJont0Data - aMeshPool) % jntSize) == 0 || !bUsingSkinning);
						assert(((aJont1Data - aMeshPool) % jntSize) == 0 || !bUsingSkinning);

						struct RasterizerWorkload location =
						{
							int((aIndexData - aMeshPool) / idxSize),
							int((aPostnData - aMeshPool) / posSize),
							int(0),
							int(0 + pPrimitive->m_geometry.m_NumIndices - 1)
						};

						m_Workloads[pPrimitive->m_WorkloadOffset] = location;

						pPrimitive->offsetIndex  = location.offsetIndex;
						pPrimitive->offsetVertex = location.offsetVertex;
						pPrimitive->startIndex   = location.startIndex;
						pPrimitive->stopIndex    = location.stopIndex;

						// Create Pipeline
						//
						creating.Wait();
						creating.Inc();

						CreateExecuteIndirectPipeline(bUsingSkinning, defines, pPrimitive);
						CreateExecuteIndirectMultiStagePipeline(bUsingSkinning, defines, pPrimitive);

#ifdef ENABLE_EXPERIMENTAL_WORKGRAPHS
						CreateWorkGraphPipeline(bUsingSkinning, defines, pPrimitive);
#endif

						creating.Dec();

						pPrimitive->m_defines = defines;
					});
				}
			}
		}

		// - Create global workload array ----------------------------------------------
		auto pBufferPool = m_pGLTFTexturesAndBuffers->GetStaticBufferPool();

		m_EIBuffer.InitFromMem(m_pDevice, "Arguments", m_pUploadHeap, &m_Arguments[0], int(m_Arguments.size()), int(sizeof(RasterizerArguments)), 1024 * 1024);
		m_WLBuffer.InitFromMem(m_pDevice, "Workloads", m_pUploadHeap, &m_Workloads[0], int(m_Workloads.size()), int(sizeof(RasterizerWorkload )), 1024 * 1024);

		m_pResourceViewHeaps->AllocCBV_SRV_UAVDescriptor(1, &m_pEIBufferSRV);
		m_pResourceViewHeaps->AllocCBV_SRV_UAVDescriptor(1, &m_pWLBufferSRV);
		m_pResourceViewHeaps->AllocCBV_SRV_UAVDescriptor(1, &m_pIndex16BufferSRV);
		m_pResourceViewHeaps->AllocCBV_SRV_UAVDescriptor(1, &m_pIndex32BufferSRV);
		m_pResourceViewHeaps->AllocCBV_SRV_UAVDescriptor(1, &m_PositionBufferSRV);

		m_EIBuffer.CreateSRV(0, &m_pEIBufferSRV);
		m_WLBuffer.CreateSRV(0, &m_pWLBufferSRV);
		pBufferPool->CreateSRV(0, &m_pIndex16BufferSRV, DXGI_FORMAT_R16_UINT, sizeof(unsigned short) * 1);
		pBufferPool->CreateSRV(0, &m_pIndex32BufferSRV, DXGI_FORMAT_R32_UINT, sizeof(unsigned int) * 1);
		pBufferPool->CreateSRV(0, &m_PositionBufferSRV, DXGI_FORMAT_R32G32B32_FLOAT, sizeof(float) * 3);

		// - Create multi-stage compute rasterizer resources ----------------------------------------------
		m_MSAllocationRange = 4 * 1024 * 1024;

		int numArgS = m_MSAllocationRange;
		int numArgR = m_numberOfBins;
		int numSplt = m_MSAllocationRange;
		int numTris = m_MSAllocationRange * m_numberOfBins;

		int szeArgs = int(sizeof(RasterizerArguments));
		int szeSplt = int(sizeof(RasterizerSplitState));
		int szeTris = int(sizeof(RasterizerTriangleState));

		m_MSCountBuffer.InitFromMem(m_pDevice, "MS-Counts", m_pUploadHeap, nullptr, 1      , 4      , 1024 * 1024);
		m_MSArgsSBuffer.InitFromMem(m_pDevice, "MS-ArgsS" , m_pUploadHeap, nullptr, numArgS, szeArgs, 1024 * 1024);
		m_MSArgsRBuffer.InitFromMem(m_pDevice, "MS-ArgsR" , m_pUploadHeap, nullptr, numArgR, szeArgs, 1024 * 1024);
		m_MSSplitBuffer.InitFromMem(m_pDevice, "MS-Splits", m_pUploadHeap, nullptr, numSplt, szeSplt, 1024 * 1024);
		m_MSRastrBuffer.InitFromMem(m_pDevice, "MS-Tris"  , m_pUploadHeap, nullptr, numTris, szeTris, 1024 * 1024);

		m_pResourceViewHeaps->AllocCBV_SRV_UAVDescriptor(1, &m_pMSCountBufferUAV);
		m_pResourceViewHeaps->AllocCBV_SRV_UAVDescriptor(1, &m_pMSArgsSBufferUAV);
		m_pResourceViewHeaps->AllocCBV_SRV_UAVDescriptor(1, &m_pMSArgsRBufferUAV);
		m_pResourceViewHeaps->AllocCBV_SRV_UAVDescriptor(1, &m_pMSSplitBufferUAV);
		m_pResourceViewHeaps->AllocCBV_SRV_UAVDescriptor(1, &m_pMSRastrBufferUAV);

		m_MSCountBuffer.CreateUAV(0, &m_pMSCountBufferUAV);
		m_MSArgsSBuffer.CreateUAV(0, &m_pMSArgsSBufferUAV);
		m_MSArgsRBuffer.CreateUAV(0, &m_pMSArgsRBufferUAV);
		m_MSSplitBuffer.CreateUAV(0, &m_pMSSplitBufferUAV);
		m_MSRastrBuffer.CreateUAV(0, &m_pMSRastrBufferUAV);

		struct {
			uint32_t constant;
			uint32_t dispatchArguments[3];
		} args[20] = {
			 0, { 0, 1, 1 },  1, { 0, 1, 1 },  2, { 0, 1, 1 },  3, { 0, 1, 1 },
			 4, { 0, 1, 1 },  5, { 0, 1, 1 },  6, { 0, 1, 1 },  7, { 0, 1, 1 },
			 8, { 0, 1, 1 },  9, { 0, 1, 1 }, 10, { 0, 1, 1 }, 11, { 0, 1, 1 },
			12, { 0, 1, 1 }, 13, { 0, 1, 1 }, 14, { 0, 1, 1 }, 15, { 0, 1, 1 },
			16, { 0, 1, 1 }, 17, { 0, 1, 1 }, 18, { 0, 1, 1 }, 19, { 0, 1, 1 },
		};

		m_MSCountBuffer.UpdateFromMem(m_pUploadHeap, args, 1, int(sizeof(uint32_t)));
		m_MSArgsRBuffer.UpdateFromMem(m_pUploadHeap, args, m_numberOfBins, szeArgs);

		m_pUploadHeap->FlushAndFinish();
	}
	
	void GltfRasterizerPass::ReCreate(
		int numberOfBins,
		bool homogeneous,
		bool showBins,
		bool fixExpansion,
		bool threadLaunch,
		int* feedback,
		AsyncPool* pAsyncPool)
	{
		if (numberOfBins == m_numberOfBins)
		if ( homogeneous == m_homogeneous )
		if (    showBins == m_showBins    )
		if (fixExpansion == m_fixExpansion)
		if (threadLaunch == m_threadLaunch)
			return;

		// Flush the gpu to make sure we don't change anything still active
		m_pDevice->GPUFlush();

#ifdef ENABLE_EXPERIMENTAL_WORKGRAPHS
		for (int t = 0; t < 2; ++t)
		{
			if (m_memoryWorkGraph[t])
				m_memoryWorkGraph[t]->Release();

			m_memorySize[t] = 0;
			m_memoryWorkGraph[t] = nullptr;
		}
#endif

		// - Recreate multi-stage compute rasterizer resources ----------------------------------------------
		if (numberOfBins != m_numberOfBins)
		{
			m_numberOfBins = std::clamp(numberOfBins, 1, 15);

			m_MSCountBuffer.Release();
			m_MSArgsSBuffer.Release();
			m_MSArgsRBuffer.Release();
			m_MSSplitBuffer.Release();
			m_MSRastrBuffer.Release();

			int numArgS = m_MSAllocationRange;
			int numArgR = m_numberOfBins;
			int numSplt = m_MSAllocationRange;
			int numTris = m_MSAllocationRange * m_numberOfBins;

			int szeArgs = int(sizeof(RasterizerArguments));
			int szeSplt = int(sizeof(RasterizerSplitState));
			int szeTris = int(sizeof(RasterizerTriangleState));

			m_MSCountBuffer.InitFromMem(m_pDevice, "MS-Counts", m_pUploadHeap, nullptr, 1      , 4      , 1024 * 1024);
			m_MSArgsSBuffer.InitFromMem(m_pDevice, "MS-ArgsS" , m_pUploadHeap, nullptr, numArgS, szeArgs, 1024 * 1024);
			m_MSArgsRBuffer.InitFromMem(m_pDevice, "MS-ArgsR" , m_pUploadHeap, nullptr, numArgR, szeArgs, 1024 * 1024);
			m_MSSplitBuffer.InitFromMem(m_pDevice, "MS-Splits", m_pUploadHeap, nullptr, numSplt, szeSplt, 1024 * 1024);
			m_MSRastrBuffer.InitFromMem(m_pDevice, "MS-Tris"  , m_pUploadHeap, nullptr, numTris, szeTris, 1024 * 1024);

			m_MSCountBuffer.CreateUAV(0, &m_pMSCountBufferUAV);
			m_MSArgsSBuffer.CreateUAV(0, &m_pMSArgsSBufferUAV);
			m_MSArgsRBuffer.CreateUAV(0, &m_pMSArgsRBufferUAV);
			m_MSSplitBuffer.CreateUAV(0, &m_pMSSplitBufferUAV);
			m_MSRastrBuffer.CreateUAV(0, &m_pMSRastrBufferUAV);
		}

		m_numberOfBins = std::clamp(numberOfBins, 1, 15);
		m_homogeneous  = homogeneous;
		m_showBins     = showBins;
		m_fixExpansion = fixExpansion;
		m_threadLaunch = threadLaunch;

		if (m_pipelineRender)
		{
			m_pipelineRender->Release();
			m_pipelineRender = nullptr;
		}

		for (int t = 0; t < 4; ++t)
		{
			if (m_pipelineRenderMS[t])
			{
				m_pipelineRenderMS[t]->Release();
				m_pipelineRenderMS[t] = nullptr;
			}
		}

#ifdef ENABLE_EXPERIMENTAL_WORKGRAPHS
		m_memoryInitialized[0] = false;
		m_memoryInitialized[1] = false;
		for (int t = 0; t < 2; ++t)
		{
			if (m_pipelineWorkGraph[t])
			{
				m_pipelineWorkGraph[t]->Release();
				m_pipelineWorkGraph[t] = nullptr;
			}
		}
#endif

		const json& j3 = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->j3;

		if (j3.find("meshes") != j3.end())
		{
			const json &meshes = j3["meshes"];
			m_meshes.resize(meshes.size());
			for (uint32_t i = 0; i < meshes.size(); i++)
			{
				// NOTE: Skinned meshes not supported for z-buffer rasterizer (can be pre-skinned)
				bool bUsingSkinning = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->FindMeshSkinId(i) != -1;
				if (bUsingSkinning)
					continue;

				RasterizerMesh *tfmesh = &m_meshes[i];

				const json &primitives = meshes[i]["primitives"];
				tfmesh->m_pPrimitives.resize(primitives.size());
				for (uint32_t p = 0; p < primitives.size(); p++)
				{
					const json &primitive = primitives[p];
					RasterizerPrimitives *pPrimitive = &tfmesh->m_pPrimitives[p];

					ExecAsyncIfThereIsAPool(pAsyncPool, [this, i, &primitive, pPrimitive]()
					{
						bool bUsingSkinning = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->FindMeshSkinId(i) != -1;

						creating.Wait();
						creating.Inc();

						CreateExecuteIndirectPipeline(bUsingSkinning, pPrimitive->m_defines, pPrimitive);
						CreateExecuteIndirectMultiStagePipeline(bUsingSkinning, pPrimitive->m_defines, pPrimitive);

#ifdef ENABLE_EXPERIMENTAL_WORKGRAPHS
						CreateWorkGraphPipeline(bUsingSkinning, pPrimitive->m_defines, pPrimitive);
#endif

						creating.Dec();
					});
				}
			}
		}

		m_feedback = feedback;
	}

	//--------------------------------------------------------------------------------------
	//
	// OnDestroy
	//
	//--------------------------------------------------------------------------------------
	void GltfRasterizerPass::OnDestroy()
	{
		if (m_pipelineRender)
			m_pipelineRender->Release();
		
		for (int t = 0; t < 4; ++t)
		{
			if (m_pipelineRenderMS[t])
				m_pipelineRenderMS[t]->Release();
		}

#ifdef ENABLE_EXPERIMENTAL_WORKGRAPHS
		for (int t = 0; t < 2; ++t)
		{
			if (m_pipelineWorkGraph[t])
				m_pipelineWorkGraph[t]->Release();

			if (m_memoryWorkGraph[t])
				m_memoryWorkGraph[t]->Release();
		}
#endif

		if (m_rootSignature)
			m_rootSignature->Release();

		if (m_commandSignature)
			m_commandSignature->Release();

		m_EIBuffer.Release();
		m_WLBuffer.Release();

		m_MSCountBuffer.Release();
		m_MSArgsSBuffer.Release();
		m_MSArgsRBuffer.Release();
		m_MSSplitBuffer.Release();
		m_MSRastrBuffer.Release();
	}

	//--------------------------------------------------------------------------------------
	//
	// CreatePipeline
	//
	//--------------------------------------------------------------------------------------
	void GltfRasterizerPass::CreateRootSignature()
	{
		/////////////////////////////////////////////
		// Create descriptors 

		CD3DX12_DESCRIPTOR_RANGE DescRange[6+5];
		CD3DX12_ROOT_PARAMETER RTSlot[9+5];
		CD3DX12_ROOT_SIGNATURE_DESC descRootSignature = CD3DX12_ROOT_SIGNATURE_DESC();

		DescRange[ 0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);                              // t0 <- per draw
		DescRange[ 1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);                              // t1 <- per scene
		DescRange[ 2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);                              // t2 <- per scene
		DescRange[ 3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3);                              // t3 <- per device
		DescRange[ 4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4);                              // t4 <- per device
		DescRange[ 5].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);                              // u0 <- per frame
		
		DescRange[ 6].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1);                              // u1 <- per frame
		DescRange[ 7].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 2);                              // u2 <- per frame
		DescRange[ 8].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 3);                              // u3 <- per frame
		DescRange[ 9].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 4);                              // u4 <- per frame
		DescRange[10].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 5);                              // u5 <- per frame

		RTSlot[ 0].InitAsConstants(1, 0, 0, D3D12_SHADER_VISIBILITY_ALL);                       // b0 <- per draw
																								
		RTSlot[ 1].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_ALL);                 // b2 <- per draw
		RTSlot[ 2].InitAsConstantBufferView(3, 0, D3D12_SHADER_VISIBILITY_ALL);                 // b3 <- per frame
				
		RTSlot[ 3].InitAsDescriptorTable(1, &DescRange[ 0], D3D12_SHADER_VISIBILITY_ALL);       // t0 <- per draw
		RTSlot[ 4].InitAsDescriptorTable(1, &DescRange[ 1], D3D12_SHADER_VISIBILITY_ALL);       // t1 <- per scene
		RTSlot[ 5].InitAsDescriptorTable(1, &DescRange[ 2], D3D12_SHADER_VISIBILITY_ALL);       // t2 <- per scene
		RTSlot[ 6].InitAsDescriptorTable(1, &DescRange[ 3], D3D12_SHADER_VISIBILITY_ALL);       // t3 <- per device
		RTSlot[ 7].InitAsDescriptorTable(1, &DescRange[ 4], D3D12_SHADER_VISIBILITY_ALL);       // t4 <- per device
		RTSlot[ 8].InitAsDescriptorTable(1, &DescRange[ 5], D3D12_SHADER_VISIBILITY_ALL);       // u0 <- per frame
													   
		RTSlot[ 9].InitAsDescriptorTable(1, &DescRange[ 6], D3D12_SHADER_VISIBILITY_ALL);       // u1 <- per frame
		RTSlot[10].InitAsDescriptorTable(1, &DescRange[ 7], D3D12_SHADER_VISIBILITY_ALL);       // u2 <- per frame
		RTSlot[11].InitAsDescriptorTable(1, &DescRange[ 8], D3D12_SHADER_VISIBILITY_ALL);       // u3 <- per frame
		RTSlot[12].InitAsDescriptorTable(1, &DescRange[ 9], D3D12_SHADER_VISIBILITY_ALL);       // u4 <- per frame
		RTSlot[13].InitAsDescriptorTable(1, &DescRange[10], D3D12_SHADER_VISIBILITY_ALL);       // u5 <- per frame

		// the root signature contains 9 slots to be used        
		descRootSignature.NumParameters = 9+5;
		descRootSignature.pParameters = RTSlot;
		descRootSignature.NumStaticSamplers = 0;
		descRootSignature.pStaticSamplers = NULL;

		// deny uneccessary access to certain pipeline stages   
		descRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

		ID3DBlob* pOutBlob, * pErrorBlob = NULL;
		D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &pOutBlob, &pErrorBlob);

		if (pErrorBlob != NULL)
		{
			char* msg = (char*)pErrorBlob->GetBufferPointer();
			MessageBoxA(0, msg, "", 0);
		}

		ThrowIfFailed(
			m_pDevice->GetDevice()->CreateRootSignature(
				0,
				pOutBlob->GetBufferPointer(),
				pOutBlob->GetBufferSize(),
				IID_PPV_ARGS(&m_rootSignature))
		);
		SetName(m_rootSignature, "GltfRasterizerPass::m_RootSignature");

		pOutBlob->Release();
		if (pErrorBlob)
			pErrorBlob->Release();
	}

	void GltfRasterizerPass::CreateCommandSignature()
	{
		/////////////////////////////////////////////
		// Create descriptors 
		D3D12_INDIRECT_ARGUMENT_DESC Args[3];

		// uint32[1]
		Args[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
		Args[0].Constant.RootParameterIndex = 0;
		Args[0].Constant.Num32BitValuesToSet = 1;
		Args[0].Constant.DestOffsetIn32BitValues = 0;
		// uint32[3]: x, y, z
		Args[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;

		D3D12_COMMAND_SIGNATURE_DESC ProgramDesc;
		ProgramDesc.ByteStride = sizeof(RasterizerArguments);
		ProgramDesc.NumArgumentDescs = 2;
		ProgramDesc.pArgumentDescs = Args;
		ProgramDesc.NodeMask = 0;

		ThrowIfFailed(
			m_pDevice->GetDevice()->CreateCommandSignature(
				&ProgramDesc,
				m_rootSignature,
				IID_PPV_ARGS(&m_commandSignature))
		);
	}

	void GltfRasterizerPass::CreateExecuteIndirectPipeline(bool bUsingSkinning, DefineList& defines, RasterizerPrimitives *pPrimitive)
	{
		// NOTE: Skinned meshes not supported for z-buffer rasterizer (can be pre-skinned)
		if (bUsingSkinning)
			return;

		// NOTE: Transparent materials not supported for z-buffer rasterizer
		if (pPrimitive->m_pMaterial->m_transparent)
			return;

		// Single-pass execute-indirect state objects already exists
		if (m_pipelineRender)
			return;

		/////////////////////////////////////////////
		// Compile and create shaders

		const int  triangleBins = std::clamp(m_numberOfBins, 1, 15);
		const bool  homogeneous = m_homogeneous;
		const bool     showBins = m_showBins;
		const bool fixExpansion = m_fixExpansion;
		const bool threadLaunch = m_threadLaunch;

		auto sourceEI = !m_homogeneous ? "RasterizerEI.hlsl" : "RasterizerEI_hg.hlsl";

		defines["SHOWBINS"] = showBins ? "1" : "0";
		defines["COALESCING"] = threadLaunch ? "0" : "1";
		defines["TRIANGLE_BINS"] = std::format("{}", triangleBins);
		
		D3D12_SHADER_BYTECODE shaderCompute;
		CompileShaderFromFile(sourceEI, &defines, "Rasterize", "-enable-16bit-types -T cs_6_5 -Zi" OPT, &shaderCompute);

		/////////////////////////////////////////////
		// Create a Pipeline 

		D3D12_COMPUTE_PIPELINE_STATE_DESC descPso = {};
		descPso.pRootSignature = m_rootSignature;
		descPso.CS = shaderCompute;
		descPso.NodeMask = 0;

		ThrowIfFailed(
			m_pDevice->GetDevice()->CreateComputePipelineState(&descPso, IID_PPV_ARGS(&m_pipelineRender))
		);
		SetName(m_pipelineRender, "GltfRasterizerPass::m_PipelineRender");
	}

	void GltfRasterizerPass::CreateExecuteIndirectMultiStagePipeline(bool bUsingSkinning, DefineList& defines, RasterizerPrimitives* pPrimitive)
	{
		// NOTE: Skinned meshes not supported for z-buffer rasterizer (can be pre-skinned)
		if (bUsingSkinning)
			return;

		// NOTE: Transparent materials not supported for z-buffer rasterizer
		if (pPrimitive->m_pMaterial->m_transparent)
			return;

		// Multi-pass execute-indirect state objects already exists
		if (m_pipelineRenderMS[0] &&
			m_pipelineRenderMS[1] &&
			m_pipelineRenderMS[2])
			return;

		/////////////////////////////////////////////
		// Compile and create shaders

		const int  triangleBins = std::clamp(m_numberOfBins, 1, 15);
		const bool     showBins = m_showBins;
		const bool fixExpansion = m_fixExpansion;
		const bool threadLaunch = m_threadLaunch;
		const int     allocSize = m_MSAllocationRange;

		auto sourceMS = !m_homogeneous ? "RasterizerMS.hlsl" : "RasterizerMS_hg.hlsl";

		defines["SHOWBINS"] = showBins ? "1" : "0";
		defines["COALESCING"] = threadLaunch ? "0" : "1";
		defines["TRIANGLE_BINS"] = std::format("{}", triangleBins);
		defines["ALLOCATION_RANGE"] = std::format("{}", allocSize);

		D3D12_SHADER_BYTECODE shaderCompute[4];
		CompileShaderFromFile(sourceMS, &defines, "TriangleFetchAndTransform", "-enable-16bit-types -T cs_6_5 -Zi" OPT, &shaderCompute[0]);
		CompileShaderFromFile(sourceMS, &defines, "Split", "-enable-16bit-types -T cs_6_5 -Zi" OPT, &shaderCompute[1]);
		CompileShaderFromFile(sourceMS, &defines, "GroupTasks", "-enable-16bit-types -T cs_6_5 -Zi" OPT, &shaderCompute[2]);
		CompileShaderFromFile(sourceMS, &defines, "Rasterize", "-enable-16bit-types -T cs_6_5 -Zi" OPT, &shaderCompute[3]);

		/////////////////////////////////////////////
		// Create a Pipeline 

		for (int t = 0; t < 4; ++t)
		{
			auto pipelineName = std::format("GltfRasterizerPass::m_pipelineRenderMS{}", t);

			D3D12_COMPUTE_PIPELINE_STATE_DESC descPso = {};
			descPso.pRootSignature = m_rootSignature;
			descPso.CS = shaderCompute[t];
			descPso.NodeMask = 0;

			ThrowIfFailed(
				m_pDevice->GetDevice()->CreateComputePipelineState(&descPso, IID_PPV_ARGS(&m_pipelineRenderMS[t]))
			);
			SetName(m_pipelineRenderMS[t], pipelineName.c_str());
		}
	}

#ifdef ENABLE_EXPERIMENTAL_WORKGRAPHS
	void GltfRasterizerPass::CreateWorkGraphPipeline(bool bUsingSkinning, DefineList& defines, RasterizerPrimitives* pPrimitive)
	{
		// NOTE: Skinned meshes not supported for z-buffer rasterizer (can be pre-skinned)
		if (bUsingSkinning)
			return;

		// NOTE: Transparent materials not supported for z-buffer rasterizer
		if (pPrimitive->m_pMaterial->m_transparent)
			return;

		// Work-graph state object already exists
		if (m_pipelineWorkGraph[0] &&
			m_pipelineWorkGraph[1])
			return;

		/////////////////////////////////////////////

		D3D12_NODE_ID entryPointId = {
			.Name = L"DistributeArguments",
			.ArrayIndex = 0,
		};

		D3D12_WORK_GRAPH_DESC workGraphDesc = {
			.ProgramName = L"WorkGraphRasterization",

			.Flags = D3D12_WORK_GRAPH_FLAG_INCLUDE_ALL_AVAILABLE_NODES,

			.NumEntrypoints = 1,
			.pEntrypoints = &entryPointId,

			.NumExplicitlyDefinedNodes = 0,
			.pExplicitlyDefinedNodes = nullptr };

		const D3D12_GLOBAL_ROOT_SIGNATURE globalRootSignature = {
			.pGlobalRootSignature = m_rootSignature };

		/////////////////////////////////////////////
		// Compile and create shaders

		// We can clamp the amount of loop iterations by specifying how large the tiles for rasterization are.
		// If we create smaller tiles, we also cull a lot more of them (and actually launch redundant kernels).
		//
		// ...
		// we aim at max 32*32=1024 loop iterations for the rasterizer => 1024=2^10 => 10 bins + splitter => 11
		// we aim at max 16*16= 256 loop iterations for the rasterizer =>   256=2^8 =>  8 bins + splitter =>  9
		// we aim at max  8* 8=  64 loop iterations for the rasterizer =>    64=2^6 =>  6 bins + splitter =>  7
		// we aim at max  4* 4=  16 loop iterations for the rasterizer =>    16=2^4 =>  4 bins + splitter =>  5
		// ...
		constexpr int triangleBinsLog2 = 4;
		constexpr int triangleBinsMax = 16;
		const int  triangleBins = m_numberOfBins;
		const bool  homogeneous = m_homogeneous;
		const bool     showBins = m_showBins;
		const bool fixExpansion = m_fixExpansion;
		const bool threadLaunch = m_threadLaunch;

		auto sourceWG = !m_homogeneous ? "RasterizerWG.hlsl" : "RasterizerWG_hg.hlsl";

		defines["SHOWBINS"] = showBins ? "1" : "0";
		defines["EXPANSION"] = fixExpansion ? "1" : "0";
		defines["COALESCING"] = threadLaunch ? "0" : "1";
		defines["TRIANGLE_BINS"] = std::format("{}", triangleBins);
		
		D3D12_SHADER_BYTECODE shaderComputeDistributor;
		D3D12_SHADER_BYTECODE shaderComputeFrontend;
		D3D12_SHADER_BYTECODE shaderComputeSplitter[2];
		D3D12_SHADER_BYTECODE shaderComputeRasterize[2 * triangleBinsMax];

		defines["DISTRIBUTOR"] = "1";
		defines["FRONTEND"] = "0";
		defines["SPLITTER"] = "0";
		defines["BACKEND"] = "0";

		CompileShaderFromFile(sourceWG, &defines, "DistributeArguments", "-enable-16bit-types -T lib_6_8" OPT, &shaderComputeDistributor);

		defines["DISTRIBUTOR"] = "0";
		defines["FRONTEND"] = "1";
		defines["SPLITTER"] = "0";
		defines["BACKEND"] = "0";

		CompileShaderFromFile(sourceWG, &defines, "TriangleFetchAndTransform", "-enable-16bit-types -T lib_6_8" OPT, &shaderComputeFrontend);

		for (int expansionBin = 0; expansionBin < (fixExpansion ? 2 : 1); ++expansionBin)
		{
			auto entryPoint = fixExpansion ? (expansionBin ? "SplitDynamic" : "SplitFixed") : "Split";

			defines["DISTRIBUTOR"] = "0";
			defines["FRONTEND"] = "0";
			defines["SPLITTER"] = "1";
			defines["BACKEND"] = "0";
			defines["EXPANSION"] = fixExpansion ? std::format("{}", 1 + expansionBin) : "0";

			CompileShaderFromFile(sourceWG, &defines, entryPoint, "-enable-16bit-types -T lib_6_8" OPT, &shaderComputeSplitter[expansionBin]);
		}

		for (int triangleBin = 0; triangleBin < triangleBins; ++triangleBin)
		{
			for (int spezializeBin = 0; spezializeBin < 1; ++spezializeBin)
			{
				auto entryPoint = "Rasterize";

				defines["DISTRIBUTOR"] = "0";
				defines["FRONTEND"] = "0";
				defines["SPLITTER"] = "0";
				defines["BACKEND"] = "1";
				defines["TRIANGLE_BIN"] = std::format("{}", triangleBin);

				CompileShaderFromFile(sourceWG, &defines, entryPoint, "-enable-16bit-types -T lib_6_8" OPT, &shaderComputeRasterize[triangleBin + (spezializeBin << triangleBinsLog2)]);
			}
		}

		/////////////////////////////////////////////
		// Register SubObjects
		for (int t = 0; t < 2; ++t)
		{
			auto pipelineName = std::format("GltfRasterizerPass::m_pipelineWorkGraph{}", t);

			// NOTE: these structs need persistent memory until CreateStateObject -> use of pointers
			std::list<std::wstring>            refNames;
			std::list<D3D12_EXPORT_DESC>       expNames;
			std::list<D3D12_DXIL_LIBRARY_DESC> libNames;

			std::vector<D3D12_STATE_SUBOBJECT> subObjects = {
				{.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, .pDesc = &globalRootSignature},
				{.Type = D3D12_STATE_SUBOBJECT_TYPE_WORK_GRAPH, .pDesc = &workGraphDesc},
			};

			D3D12_DXIL_LIBRARY_DESC shaderComputeDistributorDXIL = {
				.DXILLibrary = shaderComputeDistributor,
				.NumExports = 0,
				.pExports = nullptr
			};

			D3D12_DXIL_LIBRARY_DESC shaderComputeFrontendDXIL = {
				.DXILLibrary = shaderComputeFrontend,
				.NumExports = 0,
				.pExports = nullptr
			};

			subObjects.emplace_back(D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, &shaderComputeDistributorDXIL);
			subObjects.emplace_back(D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, &shaderComputeFrontendDXIL);

			// -- binning Split() --------------------------------------------------------------
			for (int expansionBin = 0; expansionBin < (fixExpansion ? 2 : 1); ++expansionBin)
			{
				auto entryPoint = fixExpansion ? (expansionBin ? L"SplitDynamic" : L"SplitFixed") : L"Split";

				D3D12_DXIL_LIBRARY_DESC shaderComputeSplitterDXIL = {
					.DXILLibrary = shaderComputeSplitter[expansionBin],
					.NumExports = 0,
					.pExports = nullptr
				};

				libNames.emplace_back(shaderComputeSplitterDXIL);

				subObjects.emplace_back(D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, &libNames.back());
			}

			// -- binning Rasterize() --------------------------------------------------------------
			for (int triangleBin = 0; triangleBin < triangleBins; ++triangleBin)
			{
				for (int spezializeBin = 0; spezializeBin < 1; ++spezializeBin)
				{
					auto entryPoint = L"Rasterize";

					refNames.emplace_back(std::format(L"{}_{}", entryPoint, triangleBin));

					D3D12_EXPORT_DESC shaderComputeRasterizeExport = {
						.Name = refNames.back().c_str(),
						.ExportToRename = entryPoint,
						.Flags = D3D12_EXPORT_FLAG_NONE
					};

					expNames.emplace_back(shaderComputeRasterizeExport);

					D3D12_DXIL_LIBRARY_DESC shaderComputeRasterizeDXIL = {
						.DXILLibrary = shaderComputeRasterize[triangleBin + (spezializeBin << triangleBinsLog2)],
						.NumExports = 1,
						.pExports = &expNames.back()
					};

					libNames.emplace_back(shaderComputeRasterizeDXIL);

					subObjects.emplace_back(D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, &libNames.back());
				}
			}

			/////////////////////////////////////////////
			// Create a StateObject 

			workGraphDesc.ProgramName = t == 0 ? L"WorkGraphRasterization1"   : L"WorkGraphRasterizationN";
			entryPointId.Name         = t == 0 ? L"TriangleFetchAndTransform" : L"DistributeArguments";

			const D3D12_STATE_OBJECT_DESC stateObjectDesc = {
				.Type = D3D12_STATE_OBJECT_TYPE_EXECUTABLE,
				.NumSubobjects = static_cast<UINT>(subObjects.size()),
				.pSubobjects = subObjects.data() };

			ThrowIfFailed(
				m_pDevice9->CreateStateObject(&stateObjectDesc, IID_PPV_ARGS(&m_pipelineWorkGraph[t]))
			);
			SetName(m_pipelineWorkGraph[t], pipelineName.c_str());

			/////////////////////////////////////////////
			// Create a Backing memory

			ComPtr<ID3D12WorkGraphProperties> workGraphProperties;
			ThrowIfFailed(m_pipelineWorkGraph[t]->QueryInterface(IID_PPV_ARGS(&workGraphProperties)));

			D3D12_WORK_GRAPH_MEMORY_REQUIREMENTS workGraphMemoryRequirements;
			workGraphProperties->GetWorkGraphMemoryRequirements(0, &workGraphMemoryRequirements);

			auto memorySize = workGraphMemoryRequirements.MaxSizeInBytes;
			if (m_memorySize[t] != memorySize)
			{
				m_memorySize[t] = memorySize;

				if (m_memoryWorkGraph[t])
					m_memoryWorkGraph[t]->Release();

				const auto workGraphBackingMemoryResourceDesc =
					CD3DX12_RESOURCE_DESC::Buffer(workGraphMemoryRequirements.MaxSizeInBytes);

				const auto defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
				ThrowIfFailed(m_pDevice9->CreateCommittedResource(&defaultHeapProps,
					D3D12_HEAP_FLAG_NONE,
					&workGraphBackingMemoryResourceDesc,
					D3D12_RESOURCE_STATE_COMMON,
					nullptr,
					IID_PPV_ARGS(&m_memoryWorkGraph[t])));

				const D3D12_GPU_VIRTUAL_ADDRESS_RANGE backingMemoryAddrRange = {
					.StartAddress = m_memoryWorkGraph[t]->GetGPUVirtualAddress(),
					.SizeInBytes = workGraphMemoryRequirements.MaxSizeInBytes,
				};
			}
		}
	}
#endif

	//--------------------------------------------------------------------------------------
	//
	// SetPerFrameConstants
	//
	//--------------------------------------------------------------------------------------
	void GltfRasterizerPass::SetPerFrameSceneGraph()
	{
		GLTFCommon* pC = m_pGLTFTexturesAndBuffers->m_pGLTFCommon;
		std::vector<tfNode>* pNodes = &m_pGLTFTexturesAndBuffers->m_pGLTFCommon->m_nodes;
		Matrix2* pNodesMatrices = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->m_worldSpaceMats.data();

		// loop through nodes
		//
		// Instancing can change the number of arguments vs. the number of existing objects
		uint32_t argumentsCount = 0;

		for (uint32_t i = 0; i < pNodes->size(); i++)
		{
			tfNode* pNode = &pNodes->at(i);
			if ((pNode == NULL) || (pNode->meshIndex < 0))
				continue;

			// skinning matrices constant buffer
			D3D12_GPU_VIRTUAL_ADDRESS pPerSkeleton = m_pGLTFTexturesAndBuffers->GetSkinningMatricesBuffer(pNode->skinIndex);

			// NOTE: Skinned meshes not supported for z-buffer rasterizer (can be pre-skinned)
			bool bUsingSkinning = pPerSkeleton != 0;
			if (bUsingSkinning)
				continue;

			math::Matrix4 mModelViewProj = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->m_perFrameData.mCameraCurrViewProj * pNodesMatrices[i].GetCurrent();

			RasterizerMesh* pMesh = &m_meshes[pNode->meshIndex];
			tfMesh* pGlobalMesh = &pC->m_meshes[pNode->meshIndex];
			for (int p = 0; p < pMesh->m_pPrimitives.size(); p++)
			{
				RasterizerPrimitives* pPrimitive = &pMesh->m_pPrimitives[p];

				if (m_pipelineRender == nullptr)
					continue;

				// NOTE: Transparent materials not supported for z-buffer rasterizer
				bool bAlphaTestOrBlend = pPrimitive->m_pMaterial->m_transparent;
				if (bAlphaTestOrBlend)
					continue;

				// do frustum culling
				//
				tfPrimitives boundingBox = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->m_meshes[pNode->meshIndex].m_pPrimitives[p];
				if (CameraFrustumToBoxCollision(mModelViewProj, boundingBox.m_center, boundingBox.m_radius))
					continue;

				++argumentsCount;
			}
		}

		m_Arguments.resize(argumentsCount);
		m_Workloads.resize(argumentsCount);

		if (argumentsCount == 0)
			return;

		// loop through nodes
		//
		uint32_t argumentsOffset = 0;

		for (uint32_t i = 0; i < pNodes->size(); i++)
		{
			tfNode* pNode = &pNodes->at(i);
			if ((pNode == NULL) || (pNode->meshIndex < 0))
				continue;

			// skinning matrices constant buffer
			D3D12_GPU_VIRTUAL_ADDRESS pPerSkeleton = m_pGLTFTexturesAndBuffers->GetSkinningMatricesBuffer(pNode->skinIndex);

			// NOTE: Skinned meshes not supported for z-buffer rasterizer (can be pre-skinned)
			bool bUsingSkinning = pPerSkeleton != 0;
			if (bUsingSkinning)
				continue;

			math::Matrix4 mModelViewProj = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->m_perFrameData.mCameraCurrViewProj * pNodesMatrices[i].GetCurrent();

			RasterizerMesh* pMesh = &m_meshes[pNode->meshIndex];
			tfMesh* pGlobalMesh = &pC->m_meshes[pNode->meshIndex];
			for (int p = 0; p < pMesh->m_pPrimitives.size(); p++)
			{
				RasterizerPrimitives* pPrimitive = &pMesh->m_pPrimitives[p];

				if (m_pipelineRender == nullptr)
					continue;

				// NOTE: Transparent materials not supported for z-buffer rasterizer
				bool bAlphaTestOrBlend = pPrimitive->m_pMaterial->m_transparent;
				if (bAlphaTestOrBlend)
					continue;

				// do frustum culling
				//
				tfPrimitives boundingBox = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->m_meshes[pNode->meshIndex].m_pPrimitives[p];
				if (CameraFrustumToBoxCollision(mModelViewProj, boundingBox.m_center, boundingBox.m_radius))
					continue;

				pPrimitive->m_WorkloadOffset = argumentsOffset;

				uint32_t numTriangles = pPrimitive->m_geometry.m_NumIndices / 3;
				uint32_t numThreadGroups = (numTriangles + GPU_NUM_THREADS - 1) / GPU_NUM_THREADS;

				// Set per Object constants
				//
				m_Workloads[argumentsOffset].offsetIndex  = pPrimitive->offsetIndex;
				m_Workloads[argumentsOffset].offsetVertex = pPrimitive->offsetVertex;
				m_Workloads[argumentsOffset].startIndex   = pPrimitive->startIndex;
				m_Workloads[argumentsOffset].stopIndex    = pPrimitive->stopIndex;

				m_Workloads[argumentsOffset].mWorldCurrent  = pNodesMatrices[i].GetCurrent ();
				m_Workloads[argumentsOffset].mWorldPrevious = pNodesMatrices[i].GetPrevious();

				m_Workloads[argumentsOffset].center = pGlobalMesh->m_pPrimitives[p].m_center;
				m_Workloads[argumentsOffset].radius = pGlobalMesh->m_pPrimitives[p].m_radius;

				// Set per Command arguments
				//
				m_Arguments[argumentsOffset].constant = argumentsOffset;
				m_Arguments[argumentsOffset].dispatchArguments.ThreadGroupCountX = numThreadGroups;
				m_Arguments[argumentsOffset].dispatchArguments.ThreadGroupCountY = 1;
				m_Arguments[argumentsOffset].dispatchArguments.ThreadGroupCountZ = 1;

				++argumentsOffset;
			}
		}

		// Update global workload array
		m_EIBuffer.UpdateFromMem(m_pUploadHeap, &m_Arguments[0], int(m_Arguments.size()), int(sizeof(RasterizerArguments)));
		m_WLBuffer.UpdateFromMem(m_pUploadHeap, &m_Workloads[0], int(m_Workloads.size()), int(sizeof(RasterizerWorkload)));

		// Create multi-stage compute rasterizer resources (initialized with empty dispatch arguments)
		struct {
			uint32_t constant;
			uint32_t dispatchArguments[3];
		} args[20] = {
			 0, { 0, 1, 1 },  1, { 0, 1, 1 },  2, { 0, 1, 1 },  3, { 0, 1, 1 },
			 4, { 0, 1, 1 },  5, { 0, 1, 1 },  6, { 0, 1, 1 },  7, { 0, 1, 1 },
			 8, { 0, 1, 1 },  9, { 0, 1, 1 }, 10, { 0, 1, 1 }, 11, { 0, 1, 1 },
			12, { 0, 1, 1 }, 13, { 0, 1, 1 }, 14, { 0, 1, 1 }, 15, { 0, 1, 1 },
			16, { 0, 1, 1 }, 17, { 0, 1, 1 }, 18, { 0, 1, 1 }, 19, { 0, 1, 1 },
		};

		m_MSCountBuffer.UpdateFromMem(m_pUploadHeap, args, 1, int(sizeof(uint32_t)));
		m_MSArgsRBuffer.UpdateFromMem(m_pUploadHeap, args, m_numberOfBins, int(sizeof(RasterizerArguments)));

		m_pUploadHeap->FlushAndFinish();
	}

	//--------------------------------------------------------------------------------------
	//
	// SetPerFrameConstants
	//
	//--------------------------------------------------------------------------------------
	per_frame* GltfRasterizerPass::SetPerFrameConstants(int passIndex)
	{
		per_frame* cbPerFrame;
		m_pDynamicBufferRing->AllocConstantBuffer(sizeof(per_frame), (void**)&cbPerFrame, &m_perFrameDesc[passIndex]);
		return cbPerFrame;
	}

	//--------------------------------------------------------------------------------------
	//
	// Draw
	//
	//--------------------------------------------------------------------------------------
	void GltfRasterizerPass::Draw(ID3D12GraphicsCommandList* pCommandList, int submissionMode, int passIndex)
	{
		UserMarker marker(pCommandList, "GltfRasterizerPass::Draw");

#ifdef ENABLE_EXPERIMENTAL_WORKGRAPHS
		// Get the interface for the work graphs enabled command list.
		ComPtr<ID3D12GraphicsCommandListExperimental> pCommandListExperimental;
		ThrowIfFailed(pCommandList->QueryInterface(IID_PPV_ARGS(&pCommandListExperimental)));
#endif

		// Set descriptor heaps
		//
		ID3D12DescriptorHeap *pDescriptorHeaps[] = { m_pResourceViewHeaps->GetCBV_SRV_UAVHeap(), m_pResourceViewHeaps->GetSamplerHeap() };
		pCommandList->SetDescriptorHeaps(2, pDescriptorHeaps);

		// Bind Root signature
		//                
		pCommandList->SetComputeRootSignature(m_rootSignature);

		// Bind Pipeline
		//
		pCommandList->SetPipelineState(m_pipelineRender);

		// Bind Descriptor sets
		//                
		pCommandList->SetComputeRootConstantBufferView(2, m_perFrameDesc[passIndex]);

		// Bind workload and geometry buffers
		//
		// StructuredBuffer<Workload> workloadBuffer : register(t1);
		// Buffer<uint>                  indexBuffer : register(t2);
		// Buffer<float3>             positionBuffer : register(t3);
		// RWTexture2D<uint64>      visibilityBuffer : register(u0);
		//
		// RWStructuredBuffer<uint>           counterWorkloads  : register(u1);
		// RWStructuredBuffer<Argument>       splitterArguments : register(u2);
		// RWStructuredBuffer<SplitterRecord> splitterWorkloads : register(u3);
		// RWStructuredBuffer<Argument>        rasterArguments : register(u4);
		// RWStructuredBuffer<RasterizeRecord> rasterWorkloads : register(u5);

		auto argslSRV = &m_pEIBufferSRV;
		auto worklSRV = &m_pWLBufferSRV;
		auto indexSRV = &m_pIndex16BufferSRV;
		auto positSRV = &m_PositionBufferSRV;
		auto outptUAV =  m_pVBufferUAV;

		pCommandList->SetComputeRootDescriptorTable( 4, argslSRV->GetGPU());
		pCommandList->SetComputeRootDescriptorTable( 5, worklSRV->GetGPU());
		pCommandList->SetComputeRootDescriptorTable( 6, indexSRV->GetGPU());
		pCommandList->SetComputeRootDescriptorTable( 7, positSRV->GetGPU());
		pCommandList->SetComputeRootDescriptorTable( 8, outptUAV->GetGPU());
													 
		pCommandList->SetComputeRootDescriptorTable( 9, m_pMSCountBufferUAV.GetGPU());
		pCommandList->SetComputeRootDescriptorTable(10, m_pMSArgsSBufferUAV.GetGPU());
		pCommandList->SetComputeRootDescriptorTable(11, m_pMSSplitBufferUAV.GetGPU());
		pCommandList->SetComputeRootDescriptorTable(12, m_pMSArgsRBufferUAV.GetGPU());
		pCommandList->SetComputeRootDescriptorTable(13, m_pMSRastrBufferUAV.GetGPU());

		/////////////////////////////////////////////

		size_t numDraws = m_Arguments.size();
		size_t numTriangles = 0;
		size_t numThreadGroups = 0;
		for (int a = 0; a < m_Arguments.size(); ++a)
			numThreadGroups += m_Arguments[a].dispatchArguments.ThreadGroupCountX;
		numTriangles += numThreadGroups * GPU_NUM_THREADS / 3;

		m_feedback[0] = int(numDraws);
		m_feedback[1] = int(numTriangles);
#ifdef ENABLE_EXPERIMENTAL_WORKGRAPHS
		m_feedback[2] = int(m_memorySize[0]) / (1024);
		m_feedback[3] = int(m_memorySize[1]) / (1024);
#endif

		size_t numArgS = m_MSAllocationRange;
		size_t numArgR = m_numberOfBins;
		size_t numSplt = m_MSAllocationRange;
		size_t numTris = m_MSAllocationRange * m_numberOfBins;

		size_t szeArgs = sizeof(RasterizerArguments);
		size_t szeSplt = sizeof(RasterizerSplitState);
		size_t szeTris = sizeof(RasterizerTriangleState);

		m_feedback[4] = m_MSAllocationRange;
		m_feedback[5] = int((4ULL + numArgS * szeArgs + numArgR * szeArgs + numSplt * szeSplt + numTris * szeTris) / (1024));

		if (numDraws == 0)
			return;

		if (submissionMode == 0)
		{
			static int counter = 0;
			counter++;

			// Individual Dispatch for each argument
			//
			for (int a = 0; a < m_Arguments.size(); ++a)
			{
				pCommandList->SetComputeRoot32BitConstant(0, m_Arguments[a].constant, 0);
				pCommandList->Dispatch(
					m_Arguments[a].dispatchArguments.ThreadGroupCountX,
					m_Arguments[a].dispatchArguments.ThreadGroupCountY,
					m_Arguments[a].dispatchArguments.ThreadGroupCountZ
				);
			}
		}
		else if (submissionMode == 1)
		{
			// Single-pass ExecuteIndirect
			//
			pCommandList->ExecuteIndirect(m_commandSignature, UINT(m_Arguments.size()), m_EIBuffer.GetResource(), 0, nullptr, 0);
		}
		else if (submissionMode == 2)
		{
			// Prepare barriers for multi-pass ExecuteIndirect
			//
			D3D12_RESOURCE_BARRIER barrier = { D3D12_RESOURCE_BARRIER_TYPE_UAV, D3D12_RESOURCE_BARRIER_FLAG_NONE };
			D3D12_RESOURCE_BARRIER barriers0[] = {
			{
				D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE,
				{{
					m_MSCountBuffer.GetResource(), 0,
					D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
					D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				}}
			},
			{
				D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE,
				{{
					m_MSArgsSBuffer.GetResource(), 0,
					D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
					D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				}}
			},
			{
				D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE,
				{{
					m_MSArgsRBuffer.GetResource(), 0,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
					D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				}}
			} };
			D3D12_RESOURCE_BARRIER barriers1[] = {
			{
				D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE,
				{{
					m_MSCountBuffer.GetResource(), 0,
					D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
					D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
				}}
			},
			{
				D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE,
				{{
					m_MSArgsSBuffer.GetResource(), 0,
					D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
					D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
				}}
			} };
			D3D12_RESOURCE_BARRIER barriers2[] = {
			{
				D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE,
				{{
					m_MSArgsRBuffer.GetResource(), 0,
					D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
					D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
				}}
			}};

			pCommandList->ResourceBarrier(1, &barrier);

			pCommandList->ResourceBarrier(3, barriers0); // args for TriangleFetchAndTransform: init -> UAV (required)
			pCommandList->SetPipelineState(m_pipelineRenderMS[0]);
			pCommandList->ExecuteIndirect(m_commandSignature, UINT(m_Arguments.size()), m_EIBuffer.GetResource(), 0, nullptr, 0);

			pCommandList->ResourceBarrier(1, &barrier);  // all buffers: UAV-barrier

			pCommandList->ResourceBarrier(2, barriers1); // counter/args for splitter: UAV -> IA (required)
			pCommandList->SetPipelineState(m_pipelineRenderMS[1]);
			pCommandList->ExecuteIndirect(m_commandSignature, int(numTriangles), m_MSArgsSBuffer.GetResource(), m_numberOfBins * int(sizeof(RasterizerArguments)), m_MSCountBuffer.GetResource(), 0);

			pCommandList->ResourceBarrier(1, &barrier);  // all buffers: UAV-barrier

			pCommandList->SetPipelineState(m_pipelineRenderMS[2]);
			pCommandList->Dispatch(1, 1, 1);

			pCommandList->ResourceBarrier(1, &barrier);  // all buffers: UAV-barrier

			pCommandList->ResourceBarrier(1, barriers2); // args for rasterizer: UAV -> IA (required)
			pCommandList->SetPipelineState(m_pipelineRenderMS[3]);
			pCommandList->ExecuteIndirect(m_commandSignature, m_numberOfBins, m_MSArgsRBuffer.GetResource(), 0, nullptr, 0);

			pCommandList->ResourceBarrier(1, &barrier);  // all buffers: UAV-barrier
		}

#ifdef ENABLE_EXPERIMENTAL_WORKGRAPHS
		if ((submissionMode == 3) || (submissionMode == 4))
		{
			// Bind WorkGraph pipeline state object and backing buffer
			//                
			ComPtr<ID3D12WorkGraphProperties> workGraphProperties;
			ThrowIfFailed(m_pipelineWorkGraph[0]->QueryInterface(IID_PPV_ARGS(&workGraphProperties)));

			ComPtr<ID3D12StateObjectProperties1> workGraphProperties1;
			ThrowIfFailed(m_pipelineWorkGraph[0]->QueryInterface(IID_PPV_ARGS(&workGraphProperties1)));

			D3D12_WORK_GRAPH_MEMORY_REQUIREMENTS workGraphMemoryRequirements;
			workGraphProperties->GetWorkGraphMemoryRequirements(0, &workGraphMemoryRequirements);

			const D3D12_GPU_VIRTUAL_ADDRESS_RANGE backingMemoryAddrRange = {
				.StartAddress = m_memoryWorkGraph[0]->GetGPUVirtualAddress(),
				.SizeInBytes = workGraphMemoryRequirements.MaxSizeInBytes,
			};

			const D3D12_SET_WORK_GRAPH_FLAGS setWorkGraphFlags =
				m_memoryInitialized[0] ? D3D12_SET_WORK_GRAPH_FLAG_NONE : D3D12_SET_WORK_GRAPH_FLAG_INITIALIZE;
			m_memoryInitialized[0] = true;

			const D3D12_SET_WORK_GRAPH_DESC workGraphProgramDesc = {
				.ProgramIdentifier = workGraphProperties1->GetProgramIdentifier(L"WorkGraphRasterization1"),
				.Flags = setWorkGraphFlags,
				.BackingMemory = backingMemoryAddrRange };

			const D3D12_SET_PROGRAM_DESC programDesc = { .Type = D3D12_PROGRAM_TYPE_WORK_GRAPH,
														.WorkGraph = workGraphProgramDesc };

			pCommandListExperimental->SetProgram(&programDesc);

			if (submissionMode == 3)
			{
				// Individual submission of arguments for DispatchGraph
				//
				for (const auto& arg : m_Arguments)
				{
					const D3D12_NODE_CPU_INPUT nodeCPUInput {
						.EntrypointIndex = 0,
						.NumRecords = 1,
						.pRecords = (void *)&arg,
						.RecordStrideInBytes = sizeof(RasterizerArguments),
					};

					const D3D12_DISPATCH_GRAPH_DESC dispatchGraphDesc = {
						.Mode = D3D12_DISPATCH_MODE_NODE_CPU_INPUT,
						.NodeCPUInput = nodeCPUInput,
					};

					pCommandListExperimental->DispatchGraph(&dispatchGraphDesc);
				}
			}
			else
			{
				// Batch-submission of multiple arguments for DispatchGraph
				//
				const D3D12_NODE_CPU_INPUT nodeCPUInput {
					.EntrypointIndex = 0,
					.NumRecords = UINT(m_Arguments.size()),
					.pRecords = (void*)m_Arguments.data(),
					.RecordStrideInBytes = sizeof(RasterizerArguments),
				};

				const D3D12_DISPATCH_GRAPH_DESC dispatchGraphDesc = {
					.Mode = D3D12_DISPATCH_MODE_NODE_CPU_INPUT,
					.NodeCPUInput = nodeCPUInput,
				};

				pCommandListExperimental->DispatchGraph(&dispatchGraphDesc);
			}
		}
		else if (submissionMode == 5)
		{
			// Bind WorkGraph pipeline state object and backing buffer
			//                
			ComPtr<ID3D12WorkGraphProperties> workGraphProperties;
			ThrowIfFailed(m_pipelineWorkGraph[1]->QueryInterface(IID_PPV_ARGS(&workGraphProperties)));

			ComPtr<ID3D12StateObjectProperties1> workGraphProperties1;
			ThrowIfFailed(m_pipelineWorkGraph[1]->QueryInterface(IID_PPV_ARGS(&workGraphProperties1)));

			D3D12_WORK_GRAPH_MEMORY_REQUIREMENTS workGraphMemoryRequirements;
			workGraphProperties->GetWorkGraphMemoryRequirements(0, &workGraphMemoryRequirements);

			const D3D12_GPU_VIRTUAL_ADDRESS_RANGE backingMemoryAddrRange = {
				.StartAddress = m_memoryWorkGraph[1]->GetGPUVirtualAddress(),
				.SizeInBytes = workGraphMemoryRequirements.MaxSizeInBytes,
			};

			const D3D12_SET_WORK_GRAPH_FLAGS setWorkGraphFlags =
				m_memoryInitialized[1] ? D3D12_SET_WORK_GRAPH_FLAG_NONE : D3D12_SET_WORK_GRAPH_FLAG_INITIALIZE;
			m_memoryInitialized[1] = true;

			const D3D12_SET_WORK_GRAPH_DESC workGraphProgramDesc = {
				.ProgramIdentifier = workGraphProperties1->GetProgramIdentifier(L"WorkGraphRasterizationN"),
				.Flags = setWorkGraphFlags,
				.BackingMemory = backingMemoryAddrRange };

			const D3D12_SET_PROGRAM_DESC programDesc = { .Type = D3D12_PROGRAM_TYPE_WORK_GRAPH,
														.WorkGraph = workGraphProgramDesc };

			pCommandListExperimental->SetProgram(&programDesc);

			// Arguments for ExecuteIndirect-type root node
			//
			struct DispatchGridSize
			{
				UINT x;
				UINT y;
				UINT z;
				UINT i;
			};

			DispatchGridSize dispatchGridSize = {
				.x = UINT(m_Arguments.size()), .y = 1u, .z = 1u, .i = 0
			};

			// Submission of parameters of argument-consumption from the GPU for DispatchGraph
			//
			const D3D12_NODE_CPU_INPUT nodeCPUInput{
				.EntrypointIndex = 0,
				.NumRecords = 1,
				.pRecords = &dispatchGridSize,
				.RecordStrideInBytes = sizeof(DispatchGridSize),
			};

			const D3D12_DISPATCH_GRAPH_DESC dispatchGraphDesc = {
				.Mode = D3D12_DISPATCH_MODE_NODE_CPU_INPUT,
				.NodeCPUInput = nodeCPUInput,
			};

			pCommandListExperimental->DispatchGraph(&dispatchGraphDesc);
		}
#endif
	}
}
