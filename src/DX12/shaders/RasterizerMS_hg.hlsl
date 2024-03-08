// AMD Workgraph Rasterizer code
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

#define TRIANGLE_BIN_BASE		2
#define TRIANGLE_BIN_MIN		0
#define binlog(v)				log2(v) - TRIANGLE_BIN_MIN
#define binexp(v)				1U << (v + TRIANGLE_BIN_MIN)
//#define binlog(v)				log(v) / log(TRIANGLE_BIN_BASE) - TRIANGLE_BIN_MIN
//#define binexp(v)				pow(TRIANGLE_BIN_BASE, v + TRIANGLE_BIN_MIN)
#define binrnd(v)				ceil(v)

#define DISTRB_NUM_THREADS		32
#define TRANSF_NUM_THREADS		32
#define SPLITR_NUM_THREADS		32
#define RASTER_NUM_THREADS		32

#include "common.h"

//--------------------------------------------------------------------------------------
// Constant Buffer
//--------------------------------------------------------------------------------------
#include "perFrameStruct.h"

cbuffer cbConstants : register(b0)
{
	uint workloadOffset;
	uint pad[3];
};

cbuffer cbPerFrame : register(b3)
{
	PerFrame myPerFrame;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------

matrix GetCameraViewProj()
{
	return myPerFrame.u_mCameraCurrViewProj;
}

//--------------------------------------------------------------------------------------
// Compute Shader
//--------------------------------------------------------------------------------------

#include "Rasterizer_common.h"

struct Argument
{
	uint  constant;
	uint3 dispatchArguments;
};

struct Workload
{
	uint offsetIndex;  // local cluster location in the global shared index buffer
	uint offsetVertex; // local cluster location in the global shared vertex buffer (indices are cluster local)

	uint startIndex;   // start of the triangle-range in the index buffer
	uint stopIndex;    // end of the triangle range in the index buffer, inclusive

	float4 vCenter;
	float4 vRadius;

	matrix mWorldCurrent;
	matrix mWorldPrevious;
};

StructuredBuffer<Argument> argumentBuffer : register(t1);
StructuredBuffer<Workload> workloadBuffer : register(t2);

#if defined(HAS_INDEXSHORT) || defined(HAS_INDEXLONG)
Buffer<uint>                  indexBuffer : register(t3); // casts short to ints automatically
#endif

Buffer<float3>             positionBuffer : register(t4);

//RWTexture2D<uint64_t>  visibilityBuffer : register(u0);
RWTexture2D<uint>        visibilityBuffer : register(u0);

// --------------------------------------------------------------------------------------
struct TriangleState
{
	float4 triangleCorners[3];
	float2 triangleBBMin;
	float2 triangleBBMax;
	uint   triangleBBBin;
	bool   triangleValid;
};

struct TriangleStateStorage
{
	// 64 bytes
	float4 triangleCorners[3]; // 4*4*3 = 48
	float2 triangleBBMin;      // 4*2*1 = 8
	float2 triangleBBMax;      // 4*2*1 = 8
};

struct TriangleStateStorage StoreTriangleState(struct TriangleState ts) {
	struct TriangleStateStorage tss;
	
	tss.triangleCorners[0]    = ts.triangleCorners[0];
	tss.triangleCorners[1]    = ts.triangleCorners[1];
	tss.triangleCorners[2]    = ts.triangleCorners[2];
	tss.triangleBBMin         = ts.triangleBBMin;
	tss.triangleBBMax         = ts.triangleBBMax;

	return tss;
}

struct TriangleState RetrieveTriangleState(struct TriangleStateStorage tss) {
	struct TriangleState ts;

	ts.triangleCorners[0]    = tss.triangleCorners[0];
	ts.triangleCorners[1]    = tss.triangleCorners[1];
	ts.triangleCorners[2]    = tss.triangleCorners[2];
	ts.triangleBBMin         = tss.triangleBBMin;
	ts.triangleBBMax         = tss.triangleBBMax;

	return ts;
}

// --------------------------------------------------------------------------------------
// Draw(numtriangles) -> DispatchGrid(numtriangles, 1, 1)

struct SplitterRecord
{
	struct TriangleStateStorage tri;
	uint2 dims;
	uint2 frag;
};

struct RasterizeRecord
{
	struct TriangleStateStorage tri;
};

RWBuffer<uint>                      counterWorkloads : register(u1); // size: 1, [0] = splitter count, IndirectArgumentCounter
RWStructuredBuffer<Argument>       splitterArguments : register(u2); // size: ALLOCATION_RANGE
RWStructuredBuffer<SplitterRecord> splitterWorkloads : register(u3); // size: ALLOCATION_RANGE
// rasterArguments is pre-filled with:
//  constant = bin 0, dispatch = 0,1,1
//  constant = bin 1, dispatch = 0,1,1
//  constant = bin 2, dispatch = 0,1,1
//  constant = ....., dispatch = 0,1,1
RWStructuredBuffer<Argument>        rasterArguments : register(u4); // size: TRIANGLE_BINS - 1, reading this back gives required ALLOCATION_RANGE
RWStructuredBuffer<RasterizeRecord> rasterWorkloads : register(u5); // size: ALLOCATION_RANGE * (TRIANGLE_BINS - 1)

// --------------------------------------------------------------------------------------
// Draw(numtriangles) -> Dispatch(numtriangles, 1, 1)
[NumThreads(TRANSF_NUM_THREADS, 1, 1)]
void TriangleFetchAndTransform(
	uint	WorkloadIndex : SV_GroupID,
	uint	SIMDLaneIndex : SV_GroupIndex)
{
	struct TriangleState ts;

	// Uniform permitting construct (see spec)
//	do
	{
		// - CS ----------------------------------------------------------------------
		struct Workload workload = workloadBuffer[workloadOffset];
		uint triangleNumber = WorkloadIndex * TRANSF_NUM_THREADS + SIMDLaneIndex;
		uint triangleLocation = workload.startIndex + triangleNumber * 3;

		// Drop out of bounds triangles
		if ((triangleLocation + 2) > workload.stopIndex)
			return;

	#if defined(HAS_INDEXSHORT) || defined(HAS_INDEXLONG)
		uint vertexIndex0 = indexBuffer[workload.offsetIndex + triangleLocation + 0];
		uint vertexIndex1 = indexBuffer[workload.offsetIndex + triangleLocation + 1];
		uint vertexIndex2 = indexBuffer[workload.offsetIndex + triangleLocation + 2];
	#else
		uint vertexIndex0 =                                    triangleLocation + 0;
		uint vertexIndex1 =                                    triangleLocation + 1;
		uint vertexIndex2 =                                    triangleLocation + 2;
	#endif

		// Drop degenerate triangles
		if ((vertexIndex0 == vertexIndex1) ||
			(vertexIndex1 == vertexIndex2) ||
			(vertexIndex2 == vertexIndex0))
		{
			return;
		}

		float4 vertexPosition0 = float4(positionBuffer[workload.offsetVertex + vertexIndex0], 1.0f);
		float4 vertexPosition1 = float4(positionBuffer[workload.offsetVertex + vertexIndex1], 1.0f);
		float4 vertexPosition2 = float4(positionBuffer[workload.offsetVertex + vertexIndex2], 1.0f);

		// - VS ----------------------------------------------------------------------
		uint2 Dim; visibilityBuffer.GetDimensions(Dim.x, Dim.y);

		matrix transMatrix = workload.mWorldCurrent /*GetWorldMatrix()*/;
		float3 WorldPos0 = mul(transMatrix, vertexPosition0).xyz;
		float3 WorldPos1 = mul(transMatrix, vertexPosition1).xyz;
		float3 WorldPos2 = mul(transMatrix, vertexPosition2).xyz;
		vertexPosition0 = mul(GetCameraViewProj(), float4(WorldPos0, 1));
		vertexPosition1 = mul(GetCameraViewProj(), float4(WorldPos1, 1));
		vertexPosition2 = mul(GetCameraViewProj(), float4(WorldPos2, 1));

		//	if ((vertexPosition0.z < 0.0f) &&
		//		(vertexPosition1.z < 0.0f) &&
		//		(vertexPosition2.z < 0.0f))
		//		return;

		// Drop triangles on opposite side of camera
		if ((asuint(vertexPosition0.z) & asuint(vertexPosition1.z) & asuint(vertexPosition2.z)) > 0x80000000U)
			return;

		float4 homogenousPosition0 = vertexPosition0;
		float4 homogenousPosition1 = vertexPosition1;
		float4 homogenousPosition2 = vertexPosition2;
		
		// Clip space to homogenous space
		homogenousPosition0.xy = (homogenousPosition0.xy * float2(0.5f, -0.5f) + homogenousPosition0.w * 0.5f) * Dim.xy;
		homogenousPosition1.xy = (homogenousPosition1.xy * float2(0.5f, -0.5f) + homogenousPosition1.w * 0.5f) * Dim.xy;
		homogenousPosition2.xy = (homogenousPosition2.xy * float2(0.5f, -0.5f) + homogenousPosition2.w * 0.5f) * Dim.xy;

		// Homogeneous space equations
		float3 equationX = float3(homogenousPosition0.x, homogenousPosition1.x, homogenousPosition2.x);
		float3 equationY = float3(homogenousPosition0.y, homogenousPosition1.y, homogenousPosition2.y);
		float3 equationW = float3(homogenousPosition0.w, homogenousPosition1.w, homogenousPosition2.w);

		// CCW + BFC (counter-clockwise backface-cull)
		float det = SafeDeterminant(equationX, equationY, equationW);
		if (det >= 0.0f)
			return;

		float3 einverse0;
		float3 einverse1;
		float3 einverse2;

		SafeInverse(det,
			equationX, equationY, equationW,
			einverse0, einverse1, einverse2
		);

		float3 edge0 = einverse0; // / (abs(einverse0.x) + abs(einverse0.y));
		float3 edge1 = einverse1; // / (abs(einverse1.x) + abs(einverse1.y));
		float3 edge2 = einverse2; // / (abs(einverse2.x) + abs(einverse2.y));

		// Near plane intersection fix (TODO: should be much more reliable)
		vertexPosition0.w = max(vertexPosition0.w, 0.001f);
		vertexPosition1.w = max(vertexPosition1.w, 0.001f);
		vertexPosition2.w = max(vertexPosition2.w, 0.001f);

		// Perspective divide, keep 1/w for barycentrics later
		vertexPosition0.w = rcp(vertexPosition0.w); vertexPosition0.xy *= vertexPosition0.w;
		vertexPosition1.w = rcp(vertexPosition1.w); vertexPosition1.xy *= vertexPosition1.w;
		vertexPosition2.w = rcp(vertexPosition2.w); vertexPosition2.xy *= vertexPosition2.w;

		// Clip space to screen space
		vertexPosition0.xy = (vertexPosition0.xy * float2(0.5f, -0.5f) + 0.5f) * Dim.xy;
		vertexPosition1.xy = (vertexPosition1.xy * float2(0.5f, -0.5f) + 0.5f) * Dim.xy;
		vertexPosition2.xy = (vertexPosition2.xy * float2(0.5f, -0.5f) + 0.5f) * Dim.xy;

		// Prepare for max()
		Dim -= 1;
		// ---------------------------------------------------------------------------

		float2 BBMinF = min3(vertexPosition0.xy, vertexPosition1.xy, vertexPosition2.xy);
		float2 BBMaxF = max3(vertexPosition0.xy, vertexPosition1.xy, vertexPosition2.xy);

		// Tie-breaker for BBox calculation
		// Top-left tie-breaker is: take
		// Bottom-right tie-breaker is: leave
		uint2  BBMinI = uint2(BBMinF) + select(frac(BBMinF) <= 0.5f, 0, 1);
		uint2  BBMaxI = uint2(BBMaxF) - select(frac(BBMaxF) <= 0.5f, 1, 0);

		// Clamp to resource dimensions and pixel center placement
		float2 BBMin = float2(min(max(BBMinI, uint2(0, 0)), Dim)) + 0.5f;
		float2 BBMax = float2(min(max(BBMaxI, uint2(0, 0)), Dim)) + 0.5f;

		// Calculate rasterization area and bin
		// log2() binning guarantees ~75% occupancy:
		//  for bin 7 it's [128 min,192 avg,256 max), 192 of 256 is 75%
		//  for bin 6 it's [ 64 min, 96 avg,128 max),  96 of 128 is 75%
		//  for bin 5 it's [ 32 min, 48 avg, 64 max),  48 of  64 is 75%
		//  ...
		float2 BBSize = max(BBMax - BBMin + float2(1.0f, 1.0f), float2(0.0f, 0.0f));
		float  BBArea = BBSize.x * BBSize.y;
		float  BBBin  = binlog(BBArea);

		// Cull triangles completely between sample positions
#if 0	// fast
		if (any(frac(BBMinF) != 0.5f) && (BBBin < 0.0f))
			return;
#else	// precise
		if ((frac(BBMinF.x) > 0.5f) && (frac(BBMaxF.x) <= 0.5f) && (BBSize.x < 1.0f))
			return;
		if ((frac(BBMinF.y) > 0.5f) && (frac(BBMaxF.y) <= 0.5f) && (BBSize.y < 1.0f))
			return;
#endif

		ts.triangleCorners[0] = float4(edge0.xyz, homogenousPosition0.z);
		ts.triangleCorners[1] = float4(edge1.xyz, homogenousPosition1.z);
		ts.triangleCorners[2] = float4(edge2.xyz, homogenousPosition2.z);
		ts.triangleBBMin = BBMin;
		ts.triangleBBMax = BBMax;
		ts.triangleBBBin = min(uint(binrnd(BBBin)), TRIANGLE_BINS /* - 1 */);
		ts.triangleValid = (ts.triangleBBBin < TRIANGLE_BINS);
	}
//	while (false);
	
	// - CS ----------------------------------------------------------------------
	uint3 DispatchGrid;
	uint2 DispatchDims;
	uint2 DispatchFrag;

	if (!ts.triangleValid)
	{
		uint2 triangleBBoxSize = uint2(ts.triangleBBMax - ts.triangleBBMin + float2(1.0f, 1.0f));

		// ...
		// we aim at max 32*32=1024 loop iterations for the rasterizer => 1024=2^10 => 10 bins + splitter => 11
		// we aim at max 16*16= 256 loop iterations for the rasterizer =>   256=2^8 =>  8 bins + splitter =>  9
		// we aim at max  8* 8=  64 loop iterations for the rasterizer =>    64=2^6 =>  6 bins + splitter =>  7
		// we aim at max  4* 4=  16 loop iterations for the rasterizer =>    16=2^4 =>  4 bins + splitter =>  5
		// ...
		const float maxArea = float(binexp(TRIANGLE_BINS - 1)); // 2^TB = 16,...,64,...,256,...,1024
		const float maxAreaRoot = sqrt(maxArea); // sqrt(area) = 4,...,8,...,16,...,32

		uint tileVTest = uint(ceil(float(triangleBBoxSize.y) / maxAreaRoot));
		uint tileVSize = uint(ceil(float(triangleBBoxSize.y) / tileVTest));
		uint tileHSize = uint(floor(maxArea / tileVSize)); // V*H <= maxAreaRoot guaranteed

		uint tileVCount = uint(ceil(float(triangleBBoxSize.y) / tileVSize));
		uint tileHCount = uint(ceil(float(triangleBBoxSize.x) / tileHSize));

		DispatchGrid = uint3((tileHCount * tileVCount + SPLITR_NUM_THREADS - 1) / SPLITR_NUM_THREADS, 1, 1);
		DispatchDims = uint2(tileHCount, tileVCount);
		DispatchFrag = uint2(tileHSize, tileVSize);
	}

	// - WG ----------------------------------------------------------------------
	const uint triangleBin = ts.triangleBBBin;
	const bool allocateRecordForThisThread = ts.triangleValid;

	[allow_uav_condition]
	if (allocateRecordForThisThread)
	{
		uint positionWorkload = 0;

		InterlockedAdd(rasterArguments[triangleBin].dispatchArguments.x, 1, positionWorkload);

		uint triangleNumber = triangleBin * ALLOCATION_RANGE + positionWorkload % ALLOCATION_RANGE;

		rasterWorkloads[triangleNumber].tri = StoreTriangleState(ts);
	}
	else
	{
		uint positionWorkload = 0;

		InterlockedAdd(counterWorkloads[0], 1, positionWorkload); // each split is one dispatch

		uint splitNumber = positionWorkload % ALLOCATION_RANGE;

		splitterArguments[splitNumber].constant          = splitNumber;
		splitterArguments[splitNumber].dispatchArguments = DispatchGrid;
		splitterWorkloads[splitNumber].dims              = DispatchDims;
		splitterWorkloads[splitNumber].frag              = DispatchFrag;
		splitterWorkloads[splitNumber].tri               = StoreTriangleState(ts);
	}
}

// --------------------------------------------------------------------------------------
// TRIANGLE_BIN is defined by the application to auto-generate different binned rasterizer
[NumThreads(SPLITR_NUM_THREADS, 1, 1)]
void Split(
	uint 	WorkSIMDIndex : SV_DispatchThreadID)
{
	struct TriangleState ts;

	ts = RetrieveTriangleState(splitterWorkloads[workloadOffset].tri);
	ts.triangleBBBin = TRIANGLE_BINS;

	uint2 Posn = uint2(WorkSIMDIndex % splitterWorkloads[workloadOffset].dims.x, WorkSIMDIndex / splitterWorkloads[workloadOffset].dims.x);
	uint2 Frag = splitterWorkloads[workloadOffset].frag.xy;

	// triangleBBMin has the 0.5f pixel-center offset, Frag is an integer, clamp result to valid area (removing oversize from rounding)
	float2 BBMin = ts.triangleBBMin + Posn * Frag;
	float2 BBMax = min(BBMin + Frag - float2(1.0f, 1.0f), ts.triangleBBMax);

	// Drop unused tiles (caused by fixed thread-group size)
	if ((BBMax.x < BBMin.x) ||
		(BBMax.y < BBMin.y))
	{
		return;
	}

	float3 edge0 = ts.triangleCorners[0].xyz;
	float3 edge1 = ts.triangleCorners[1].xyz;
	float3 edge2 = ts.triangleCorners[2].xyz;

	// Special case of Triangle<->Rectangle overlap detection:
	// As the sub-BBoxes are all contained in the triangle's BBox
	// it's sufficient to ask if all of the BBox corners are on the
	// outside of any of the triangle's edges
	float2 bboxCorners[4] = {
		float2(BBMin.x, BBMin.y),
		float2(BBMax.x, BBMin.y),
		float2(BBMin.x, BBMax.y),
		float2(BBMax.x, BBMax.y)
	};

	int outside[3] = { 0, 0, 0 };

	[unroll]
	for (int c = 0; c < 4; ++c) {
		float x = bboxCorners[c][0];
		float y = bboxCorners[c][1];

		float3 S = float3(x, y, 1.0f);

		float SE0 = dot(edge0, S);
		float SE1 = dot(edge1, S);
		float SE2 = dot(edge2, S);

		// create condition, "are all bbox points on the outside of each edge?"
		// TODO: Tie-breaker for edge rasterization
		outside[0] += (SE0 <= 0.0f);
		outside[1] += (SE1 <= 0.0f);
		outside[2] += (SE2 <= 0.0f);
	}

	const bool part_covered = (outside[0] != 4) & (outside[1] != 4) & (outside[2] != 4);
	const bool full_covered = (outside[0] == 0) & (outside[1] == 0) & (outside[2] == 0);
	if (part_covered)
	{
		// The BBox created here can be oversized, as we don't
		// actually clip the triangles against the BBox and recalculate
		// the true BBox of the resulting triangle, which can be smaller.

		// Calculate rasterization area and bin
		// log2() binning guarantees ~75% occupancy:
		//  ...
		//  for bin 7 it's [128 min,192 avg,256 max), 192 of 256 is 75%
		//  for bin 6 it's [ 64 min, 96 avg,128 max),  96 of 128 is 75%
		//  for bin 5 it's [ 32 min, 48 avg, 64 max),  48 of  64 is 75%
		//  ...
		float2 BBSize = max(BBMax - BBMin + float2(1.0f, 1.0f), float2(0.0f, 0.0f));
		float  BBArea = BBSize.x * BBSize.y;
		float  BBBin  = binlog(BBArea);

		// Here we can't detect easily if the covered triangle-piece falls completely between sample positions
		if (BBBin >= 0.0f)
		{
			ts.triangleBBMin = BBMin;
			ts.triangleBBMax = BBMax;
			ts.triangleBBBin = min(uint(binrnd(BBBin)), TRIANGLE_BINS - 1);
		}
	}

	ts.triangleValid = (ts.triangleBBBin < TRIANGLE_BINS);

	// - WG ----------------------------------------------------------------------
	const uint triangleBin = ts.triangleBBBin;
	const bool allocateRecordForThisThread = ts.triangleValid;

	[allow_uav_condition]
	if (allocateRecordForThisThread)
	{
		uint positionWorkload = 0;
		
		InterlockedAdd(rasterArguments[triangleBin].dispatchArguments.x, 1, positionWorkload);

		uint triangleNumber = triangleBin * ALLOCATION_RANGE + positionWorkload % ALLOCATION_RANGE;

		rasterWorkloads[triangleNumber].tri = StoreTriangleState(ts);
	}
}

// --------------------------------------------------------------------------------------
[NumThreads(32, 1, 1)]
void GroupTasks(
	uint	WorkSIMDIndex : SV_DispatchThreadID)
{
	uint triangleBin = WorkSIMDIndex;
	if (triangleBin < TRIANGLE_BINS)
	{
		uint originalArguments =
			rasterArguments[triangleBin].dispatchArguments.x;
		uint dispatchArguments =
			(originalArguments + RASTER_NUM_THREADS - 1) / RASTER_NUM_THREADS;
		uint dispatchExtent =
			dispatchArguments * RASTER_NUM_THREADS;

		// Clear overhanging states (invalid but processed by group)
		struct TriangleState ts = (struct TriangleState)0;
		for (uint t = originalArguments; t < dispatchExtent; ++t)
		{
			uint triangleNumber = triangleBin * ALLOCATION_RANGE + t % ALLOCATION_RANGE;

			rasterWorkloads[triangleNumber].tri = StoreTriangleState(ts);
		}

		rasterArguments[triangleBin].dispatchArguments.x = dispatchArguments;
	}
}

// --------------------------------------------------------------------------------------
// TRIANGLE_BIN is defined by the application to auto-generate different binned rasterizer
[NumThreads(RASTER_NUM_THREADS, 1, 1)]
void Rasterize(
	uint	WorkSIMDIndex : SV_DispatchThreadID)
{
	struct TriangleState ts;

	uint triangleBin = workloadOffset;
	uint triangleNumber = triangleBin * ALLOCATION_RANGE + WorkSIMDIndex % ALLOCATION_RANGE;

	ts = RetrieveTriangleState(rasterWorkloads[triangleNumber].tri);

	// Homogeneous space equations
	float3 edge0 = ts.triangleCorners[0].xyz;
	float3 edge1 = ts.triangleCorners[1].xyz;
	float3 edge2 = ts.triangleCorners[2].xyz;

	float3 O = float3(1.0f, 1.0f, 1.0f);
	float3 Z = float3(ts.triangleCorners[0].w, ts.triangleCorners[1].w, ts.triangleCorners[2].w);

	float3 einverseX = float3(edge0.x, edge1.x, edge2.x);
	float3 einverseY = float3(edge0.y, edge1.y, edge2.y);
	float3 einverseZ = float3(edge0.z, edge1.z, edge2.z);

	float3 oneOW = float3(
		dot(einverseX, O),
		dot(einverseY, O),
		dot(einverseZ, O)
	);

	float3 zetOW = float3(
		dot(einverseX, Z),
		dot(einverseY, Z),
		dot(einverseZ, Z)
	);

	// edge0 /= (abs(einverse0.x) + abs(einverse0.y));
	// edge1 /= (abs(einverse1.x) + abs(einverse1.y));
	// edge2 /= (abs(einverse2.x) + abs(einverse2.y));

	// Bounding box
	float2 BBMin = ts.triangleBBMin;
	float2 BBMax = ts.triangleBBMax;

	// - CS ----------------------------------------------------------------------
	// Area loop
	// Need to bin for 1D permutations space: BBox.x * BBox.y
	// to have balanced occupancy in the loop-iteration count over the given triangles
	float BBStrideX = (BBMax.x - BBMin.x + 1);
	float BBStrideY = (BBMax.y - BBMin.y + 1);
	float BBCoverArea = BBStrideY * BBStrideX - 1.0f;

	// There are numerical imprecisions, min can be slightly too high and max too low
#define EPS	0.00005f
	BBMin = BBMin - float2(EPS, EPS);
	BBMax = BBMax + float2(EPS, EPS);

	float y = BBMin.y;
	float x = BBMin.x - 1.0f;

	[allow_uav_condition]
	while (BBCoverArea >= 0.0f)
	{
		x += 1.0f;
		y += x > BBMax.x;
		if (x > BBMax.x)
			x = BBMin.x;

		BBCoverArea -= 1.0f;

		{
			float3 S = float3(x, y, 1.0f);

			float SE0 = dot(edge0, S);
			float SE1 = dot(edge1, S);
			float SE2 = dot(edge2, S);

			//	if ((SE0 <= 0.0f) ||
			//		(SE1 <= 0.0f) ||
			//		(SE2 <= 0.0f))
			//		continue;

			// TODO: Tie-breaker for edge rasterization
			if ((asuint(SE0) | asuint(SE1) | asuint(SE2)) > 0x80000000U)
				continue;

			// Get interpolated 1/W & Z/W
			float _oneOW = dot(oneOW, S);
			float _zetOW = dot(zetOW, S);

			// Get interpolated W & Z
			float w = rcp(_oneOW);
			float z = _zetOW * w;
			
			// Drop pixels on opposite side of camera
			if (z < 0.0f)
				continue;

			// NOTE: the identifier will be tie-breaker when z-test is equal, and the triangle with the largest identifier wins
		//	uint64_t depthIndexPair = (asuint(z) << 32) | (identifier);
			uint2 location = uint2(x, y);

#if SHOWBINS
#define	TRIANGLE_BIN	triangleBin
			z = float(TRIANGLE_BIN) / 16.0f;
#endif
		 
		//	InterlockedMin(visibilityBuffer[location], depthIndexPair);
			InterlockedMin(visibilityBuffer[location], asuint(z));
		}
	}
}
