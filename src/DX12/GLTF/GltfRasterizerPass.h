// AMD Cauldron code
// AMD Work graph Rasterizer code
// 
// Copyright(c) 2024 Advanced Micro Devices, Inc.All rights reserved.
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

#include "base/Buffer.h"
#include "GLTFTexturesAndBuffers.h"

namespace CAULDRON_DX12
{
	// Material, primitive and mesh structs specific for the depth pass (you might want to compare these structs with the ones used for the depth pass in GltfPbrPass.h)

#define GPU_NUM_THREADS		32

	// Data structure to match the command signature used for ExecuteIndirect.
	struct RasterizerArguments
	{
		UINT constant;
		D3D12_DISPATCH_ARGUMENTS dispatchArguments;
	};

	struct RasterizerTriangleState
	{
		float triangleCorners[3][4];
		float triangleBBMin     [2];
		float triangleBBMax     [2];
	};

	struct RasterizerSplitState
	{
		D3D12_DISPATCH_ARGUMENTS dispatchArguments;
		struct RasterizerTriangleState tri;
		int DispatchDims[2];
		int DispatchFrag[2];
	};

	struct RasterizerWorkload {
		int offsetIndex;  // local cluster location in the global shared index buffer
		int offsetVertex; // local cluster location in the global shared vertex buffer (indices are cluster local)

		int startIndex;   // start of the triangle-range in the index buffer
		int stopIndex;    // end of the triangle range in the index buffer

		math::Vector4 center;
		math::Vector4 radius;

		math::Matrix4 mWorldCurrent;
		math::Matrix4 mWorldPrevious;
	};

	struct RasterizerMaterial
	{
		int m_textureCount = 0;
		CBV_SRV_UAV *m_pTransparency = NULL;

		DefineList m_defines;
		bool m_doubleSided = false;
		bool m_transparent = false;
	};

	struct RasterizerPrimitives
	{
		uint32_t m_WorkloadOffset;
		Geometry m_geometry;

		int offsetIndex;  // local cluster location in the global shared index buffer
		int offsetVertex; // local cluster location in the global shared vertex buffer (indices are cluster local)

		int startIndex;   // start of the triangle-range in the index buffer
		int stopIndex;    // end of the triangle range in the index buffer

		RasterizerMaterial* m_pMaterial = NULL;
		DefineList m_defines;
	};

	struct RasterizerMesh
	{
		std::vector<RasterizerPrimitives> m_pPrimitives;
	};

	class GltfRasterizerPass
	{
	public:
		struct per_object
		{
			math::Matrix4 mWorld;
		};

		void OnCreate(
			Device *pDevice,
			UploadHeap *pUploadHeap,
			ResourceViewHeaps *pHeaps,
			DynamicBufferRing *pDynamicBufferRing,
			StaticBufferPool *pStaticBufferPool,
			GLTFTexturesAndBuffers* pGLTFTexturesAndBuffers,
			Texture* pVBuffer,
			CBV_SRV_UAV* pVBufferUAV,
			CBV_SRV_UAV* pVBufferUAVCPU,
			AsyncPool *pAsyncPool = NULL,
			DXGI_FORMAT depthFormat = DXGI_FORMAT_D32_FLOAT);

		void ReCreate(
			int numberOfBins,
			bool homogeneous,
			bool showBins,
			bool fixExpansion,
			bool threadLaunch,
			int* feedback,
			AsyncPool* pAsyncPool = NULL);

		void OnDestroy();
		void SetPerFrameSceneGraph();
		per_frame *SetPerFrameConstants(int passIndex = 0);
		void Draw(ID3D12GraphicsCommandList* pCommandList, int submissionMode, int passIndex = 0);
	private:
		Device* m_pDevice;
#ifdef ENABLE_WORKGRAPHS
		ID3D12Device8* m_pDevice9;
#endif
		UploadHeap* m_pUploadHeap;
		ResourceViewHeaps *m_pResourceViewHeaps;
		DynamicBufferRing *m_pDynamicBufferRing;
		StaticBufferPool *m_pStaticBufferPool;

		std::vector<RasterizerArguments> m_Arguments;
		std::vector<RasterizerWorkload> m_Workloads;
		std::vector<RasterizerMesh> m_meshes;
		std::vector<RasterizerMaterial> m_materialsData;

		RasterizerMaterial m_defaultMaterial;

		GLTFTexturesAndBuffers *m_pGLTFTexturesAndBuffers;
		D3D12_STATIC_SAMPLER_DESC m_samplerDesc;
		D3D12_GPU_VIRTUAL_ADDRESS m_perFrameDesc[5];

		Sync creating;

		ID3D12RootSignature* m_rootSignature;
		ID3D12CommandSignature* m_commandSignature;
		ID3D12PipelineState* m_pipelineRender;
		ID3D12PipelineState* m_pipelineRenderMS[4];
#ifdef ENABLE_WORKGRAPHS
		size_t m_memorySize[2];
		ID3D12Resource* m_memoryWorkGraph[2];
		bool m_memoryInitialized[2];
		ID3D12StateObject* m_pipelineWorkGraph[2];
#endif
		
		int m_numberOfBins;
		bool m_homogeneous;
		bool m_showBins;
		bool m_fixExpansion;
		bool m_threadLaunch;
		int* m_feedback;
		int m_feedbackBuffer[6];

		Buffer m_EIBuffer;
		CBV_SRV_UAV m_pEIBufferSRV;

		Buffer m_WLBuffer;
		CBV_SRV_UAV m_pWLBufferSRV;
		CBV_SRV_UAV m_pIndex16BufferSRV;
		CBV_SRV_UAV m_pIndex32BufferSRV;
		CBV_SRV_UAV m_PositionBufferSRV;

		int m_MSAllocationRange;
		Buffer m_MSCountBuffer; // size: 1
		Buffer m_MSArgsSBuffer; // size: ALLOCATION_RANGE
		Buffer m_MSArgsRBuffer; // size: TRIANGLE_BINS
		Buffer m_MSSplitBuffer; // size: ALLOCATION_RANGE
		Buffer m_MSRastrBuffer; // size: ALLOCATION_RANGE * (TRIANGLE_BINS)
		CBV_SRV_UAV m_pMSCountBufferUAV;
		CBV_SRV_UAV m_pMSArgsSBufferUAV;
		CBV_SRV_UAV m_pMSSplitBufferUAV;
		CBV_SRV_UAV m_pMSArgsRBufferUAV;
		CBV_SRV_UAV m_pMSRastrBufferUAV;

		Texture* m_pVBuffer;
		CBV_SRV_UAV* m_pVBufferUAV;
		CBV_SRV_UAV* m_pVBufferUAVCPU;

		void CreateRootSignature();
		void CreateCommandSignature();
		void CreateExecuteIndirectPipeline(bool bUsingSkinning, DefineList& defines, RasterizerPrimitives* pPrimitive);
		void CreateExecuteIndirectMultiStagePipeline(bool bUsingSkinning, DefineList& defines, RasterizerPrimitives* pPrimitive);
#ifdef ENABLE_WORKGRAPHS
		void CreateWorkGraphPipeline(bool bUsingSkinning, DefineList& defines, RasterizerPrimitives* pPrimitive);
#endif
	};
}


