// AMD Cauldron code
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

//--------------------------------------------------------------------------------------
// Texture definitions
//--------------------------------------------------------------------------------------
Texture2D<uint>        inputTex         :register(t0);
RWTexture2D<uint>      outputTex0       :register(u0); // 4x4
RWTexture2D<uint>      outputTex1       :register(u1); // 2x2
RWTexture2D<uint>      outputTex2       :register(u2); // 1x1

//--------------------------------------------------------------------------------------
// Main function
//--------------------------------------------------------------------------------------

groupshared uint lower0[4][4];
groupshared uint lower1[2][2];
groupshared uint lower2[1][1];

[numthreads(4, 4, 1)]
float4 ProduceHiZ(
	uint2	GroupLocation : SV_GroupID
	uint2	SIMDLaneIndex : SV_GroupThreadID)
{
	{
		uint val00 = inputTex[(GroupLocation + SIMDLaneIndex) * 2 + uint2(0, 0)];
		uint val01 = inputTex[(GroupLocation + SIMDLaneIndex) * 2 + uint2(0, 1)];
		uint val10 = inputTex[(GroupLocation + SIMDLaneIndex) * 2 + uint2(1, 0)];
		uint val11 = inputTex[(GroupLocation + SIMDLaneIndex) * 2 + uint2(1, 1)];

		uint val = min(min(val00, val01), min(val10, val11));

		lower0[SIMDLaneIndex.y][SIMDLaneIndex.x] = val;
		outputTex0[(GroupLocation + SIMDLaneIndex)] = val;
	}

	GroupMemoryBarrierWithGroupSync();

	{
		uint val00 = lower0[SIMDLaneIndex.y + 0][SIMDLaneIndex.x + 0];
		uint val01 = lower0[SIMDLaneIndex.y + 0][SIMDLaneIndex.x + 1];
		uint val10 = lower0[SIMDLaneIndex.y + 1][SIMDLaneIndex.x + 0];
		uint val11 = lower0[SIMDLaneIndex.y + 1][SIMDLaneIndex.x + 1];

		uint val = min(min(val00, val01), min(val10, val11));

		lower1[SIMDLaneIndex.y / 2][SIMDLaneIndex.x / 2] = val;
		outputTex1[(GroupLocation + SIMDLaneIndex) / 2] = val;
	}

	GroupMemoryBarrierWithGroupSync();

	{
		uint val00 = lower0[SIMDLaneIndex.y / 2 + 0][SIMDLaneIndex.x / 2 + 0];
		uint val01 = lower0[SIMDLaneIndex.y / 2 + 0][SIMDLaneIndex.x / 2 + 1];
		uint val10 = lower0[SIMDLaneIndex.y / 2 + 1][SIMDLaneIndex.x / 2 + 0];
		uint val11 = lower0[SIMDLaneIndex.y / 2 + 1][SIMDLaneIndex.x / 2 + 1];

		uint val = min(min(val00, val01), min(val10, val11));

		lower2[SIMDLaneIndex.y / 4][SIMDLaneIndex.x / 4] = val;
		outputTex2[(GroupLocation + SIMDLaneIndex) / 4] = val;
	}
}
