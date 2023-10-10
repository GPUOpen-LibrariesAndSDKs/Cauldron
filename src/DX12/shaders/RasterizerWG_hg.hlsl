// AMD Workgraph Rasterizer code
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

#define TRIANGLE_BIN_BASE		2	// NOTE: base-2 seems rather optimal
#define TRIANGLE_BIN_MIN		0	// NOTE: performance drops with increasing magnitude, 1 & 2 look interesting though
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
	float4 triangleCorners[3];
	float2 triangleBBMin;
	float2 triangleBBMax;
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

struct ExecutionRecord
{
	// Broadcast Grid dimension - set by the application based on the loaded index buffers's size
	uint3 DispatchGrid : SV_DispatchGrid;
	uint  workloadOffset;
};

struct DrawRecord
{
	// Compatible with Argument buffer
	uint  workloadOffset;
	uint3 DispatchGrid : SV_DispatchGrid;
};

struct SplitRecord
{
	uint3 DispatchGrid : SV_DispatchGrid;

	struct TriangleStateStorage tri;
	uint2 DispatchDims;
	uint2 DispatchFrag;
};

struct SplitFixedRecord
{
	struct TriangleStateStorage tri;
	uint2 DispatchDims;
	uint2 DispatchFrag;
};

struct RasterizeRecord
{
	struct TriangleStateStorage tri;
};

// --------------------------------------------------------------------------------------
#if DISTRIBUTOR
[Shader("node")]
[NodeLaunch("broadcasting")]
[NodeMaxDispatchGrid(32768, 1, 1)] // Allow scenes upto 32k objects
[NodeIsProgramEntry] // optional
[NumThreads(DISTRB_NUM_THREADS, 1, 1)]
void DistributeArguments(
	uint	argumentOffset : SV_DispatchThreadID,

	// Input record that contains the dispatch grid size.
	// Set up by the application.
	DispatchNodeInputRecord<ExecutionRecord> launchRecord,

	[MaxRecords(DISTRB_NUM_THREADS)] // 32 threads that each output 1 draw
	[NodeID("TriangleFetchAndTransform")]
	NodeOutput<DrawRecord> drawRecord
)
{
	struct Argument arguments = argumentBuffer[argumentOffset];

	const bool allocateDrawRecordForThisThread = true;

	ThreadNodeOutputRecords<DrawRecord> outputRecord =
		drawRecord.GetThreadNodeOutputRecords(allocateDrawRecordForThisThread);

	if (allocateDrawRecordForThisThread)
	{
		outputRecord.Get().workloadOffset = arguments.constant;
		outputRecord.Get().DispatchGrid = arguments.dispatchArguments;
	}

	outputRecord.OutputComplete();
}
#endif

// --------------------------------------------------------------------------------------
#if FRONTEND
[Shader("node")]
[NodeLaunch("broadcasting")]
[NodeMaxDispatchGrid(65535, 1, 1)] // Allow fully utilized 16-bit index buffers, exact triangle number is set via DrawRecord
[NodeIsProgramEntry] // optional
[NumThreads(TRANSF_NUM_THREADS, 1, 1)]
void TriangleFetchAndTransform(
	uint	WorkloadIndex : SV_GroupID,
	uint	SIMDLaneIndex : SV_GroupIndex,

	// Input record that contains the dispatch grid size.
	// Set up by the application.
	DispatchNodeInputRecord<DrawRecord> launchRecord,

#if EXPANSION
	[MaxRecords(TRANSF_NUM_THREADS)] // N threads that each output 1 triangle-split
	[NodeID("SplitFixed")]
	NodeOutput<SplitFixedRecord> splitFixedOutput,

	[MaxRecords(TRANSF_NUM_THREADS)] // N threads that each output 1 triangle-split
	[NodeID("SplitDynamic")]
	NodeOutput<SplitRecord> splitOutput,
#else
	[MaxRecords(TRANSF_NUM_THREADS)] // N threads that each output 1 triangle-split
	[NodeID("Split")]
	NodeOutput<SplitRecord> splitOutput,
#endif

	[MaxRecords(TRANSF_NUM_THREADS)] // N threads that each output 1 triangle
	[NodeID("Rasterize")]
	[NodeArraySize(TRIANGLE_BINS)]  // TRIANGLE_BINS defined by application during compilation
	NodeOutputArray<RasterizeRecord> triangleOutput
)
{
	struct TriangleState ts;

	// Uniform permitting construct (see spec)
//	do
	{
		// - CS ----------------------------------------------------------------------
		uint workloadOffset = launchRecord.Get().workloadOffset;
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

		float3 edge0 = einverse0;
		float3 edge1 = einverse1;
		float3 edge2 = einverse2;

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
		uint2  BBMinI = uint2(BBMinF) + (frac(BBMinF) <= 0.5f ? 0 : 1);
		uint2  BBMaxI = uint2(BBMaxF) - (frac(BBMaxF) <= 0.5f ? 1 : 0);

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
	const bool allocateRasterRecordForThisThread = ts.triangleValid;
#if !EXPANSION
	const bool allocateSplitRecordForThisThread      = !allocateRasterRecordForThisThread;
#else
	const bool allocateSplitRecordForThisThread      = !allocateRasterRecordForThisThread & (DispatchGrid.x != 1);
	const bool allocateSplitFixedRecordForThisThread = !allocateRasterRecordForThisThread & (DispatchGrid.x == 1);
#endif

	ThreadNodeOutputRecords<RasterizeRecord> rasterRecord =
		triangleOutput[triangleBin].GetThreadNodeOutputRecords(allocateRasterRecordForThisThread);
	ThreadNodeOutputRecords<SplitRecord> splitterRecord =
		splitOutput.GetThreadNodeOutputRecords(allocateSplitRecordForThisThread);
#if EXPANSION
	ThreadNodeOutputRecords<SplitFixedRecord> splitterFixedRecord =
		splitFixedOutput.GetThreadNodeOutputRecords(allocateSplitFixedRecordForThisThread);
#endif

	if (allocateRasterRecordForThisThread)
	{
		rasterRecord.Get().tri = StoreTriangleState(ts);
	}
#if EXPANSION
	else if (allocateSplitFixedRecordForThisThread)
	{
		splitterFixedRecord.Get().DispatchDims = DispatchDims;
		splitterFixedRecord.Get().DispatchFrag = DispatchFrag;
		splitterFixedRecord.Get().tri = StoreTriangleState(ts);
	}
#endif
	else if (allocateSplitRecordForThisThread)
	{
		splitterRecord.Get().DispatchGrid = DispatchGrid;
		splitterRecord.Get().DispatchDims = DispatchDims;
		splitterRecord.Get().DispatchFrag = DispatchFrag;
		splitterRecord.Get().tri = StoreTriangleState(ts);
	}

	rasterRecord.OutputComplete();
	splitterRecord.OutputComplete();
#if EXPANSION
	splitterFixedRecord.OutputComplete();
#endif
}
#endif

// --------------------------------------------------------------------------------------
// TRIANGLE_BIN is defined by the application to auto-generate different binned rasterizer
#if SPLITTER
[Shader("node")]
[NodeLaunch("broadcasting")]
#if (EXPANSION == 1) // fixed expansion
[NodeDispatchGrid(1, 1, 1)]
[NumThreads(SPLITR_NUM_THREADS, 1, 1)]
void SplitFixed(
#elif (EXPANSION == 2) // dynamic expansion
[NodeMaxDispatchGrid(240, 135, 1)]
[NumThreads(SPLITR_NUM_THREADS, 1, 1)]
void SplitDynamic(
#else
[NodeMaxDispatchGrid(240, 135, 1)] // Allow breaking up a full 4k triangle into 16x16 tiles: 
[NumThreads(SPLITR_NUM_THREADS, 1, 1)]
void Split(
#endif
	uint 	WorkSIMDIndex : SV_DispatchThreadID,

	// Input record that contains the dispatch grid size.
	// Set up by the application.
#if (EXPANSION == 1) // fixed expansion
	DispatchNodeInputRecord<SplitFixedRecord> splitRecord,
#else
	DispatchNodeInputRecord<SplitRecord> splitRecord,
#endif

	[MaxRecords(SPLITR_NUM_THREADS)] // 32 threads that each output 1 triangle
	[NodeID("Rasterize")]
	[NodeArraySize(TRIANGLE_BINS)]  // TRIANGLE_BINS defined by application during compilation
	NodeOutputArray<RasterizeRecord> triangleOutput
)
{
	struct TriangleState ts;

	ts = RetrieveTriangleState(splitRecord.Get().tri);
	ts.triangleBBBin = TRIANGLE_BINS;

#if 0 // Cull dead lanes
	if (WorkSIMDIndex >= (splitRecord.Get().DispatchDims.x * splitRecord.Get().DispatchDims.y))
		return;
#endif

	uint2 Posn = uint2(WorkSIMDIndex % splitRecord.Get().DispatchDims.x, WorkSIMDIndex / splitRecord.Get().DispatchDims.x);
	uint2 Frag = splitRecord.Get().DispatchFrag.xy;

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
	const bool allocateRasterRecordForThisThread     = ts.triangleValid;

	ThreadNodeOutputRecords<RasterizeRecord> outputRecord =
		triangleOutput[triangleBin].GetThreadNodeOutputRecords(allocateRasterRecordForThisThread);

	if (allocateRasterRecordForThisThread)
	{
		outputRecord.Get().tri = StoreTriangleState(ts);
	}

	outputRecord.OutputComplete();
}
#endif

// --------------------------------------------------------------------------------------
// TRIANGLE_BIN is defined by the application to auto-generate different binned rasterizer
#if BACKEND
[Shader("node")]
#if COALESCING
[NodeLaunch("coalescing")]
[NumThreads(RASTER_NUM_THREADS, 1, 1)]
#else
[NodeLaunch("thread")]
#endif
[NodeID("Rasterize", TRIANGLE_BIN)]
void Rasterize(
#if COALESCING
	uint SIMDLaneIndex : SV_GroupThreadID,

	[MaxRecords(RASTER_NUM_THREADS)]
	GroupNodeInputRecords<RasterizeRecord> triangleRecords
#else
	ThreadNodeInputRecord<RasterizeRecord> triangleRecord
#endif
)
{
	struct TriangleState ts;

#if COALESCING
	ts = RetrieveTriangleState(triangleRecords.Get(SIMDLaneIndex).tri);
#else
	ts = RetrieveTriangleState(triangleRecord.Get().tri);
#endif

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
			
			// Don't drop pixels on opposite side of camera (cost 20% performance here)
			//	if (z < 0.0f)
			//		continue;

			// NOTE: the identifier will be tie-breaker when z-test is equal, and the triangle with the largest identifier wins
		//	uint64_t depthIndexPair = (asuint(z) << 32) | (identifier);
			uint2 location = uint2(x, y);

#if SHOWBINS
			z = ((float(TRIANGLE_BIN) / TRIANGLE_BINS) + 14.0f) / 15.0f;
#endif
		 
		//	InterlockedMin(visibilityBuffer[location], depthIndexPair);
			InterlockedMin(visibilityBuffer[location], asuint(z));
		}
	}
}
#endif
