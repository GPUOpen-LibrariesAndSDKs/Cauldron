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

#include "common.h"


ByteAddressBuffer Position : register(TEX(HAS_POSITION));

#if defined(HAS_WEIGHTS_0) && defined(HAS_JOINTS_0)
ByteAddressBuffer Weights0 : register(TEX(HAS_WEIGHTS_0));
ByteAddressBuffer Joints0  : register(TEX(HAS_JOINTS_0));
#endif

#if defined(HAS_WEIGHTS_1) && defined(HAS_JOINTS_1)
ByteAddressBuffer Weights1 : register(TEX(HAS_WEIGHTS_1));
ByteAddressBuffer Joints1  : register(TEX(HAS_JOINTS_1));
#endif

RWStructuredBuffer<float3> OutputBuffer : register(UA(ID_OUTPUT));

#include "Skinning.hlsl"

float3 Getfloat3(ByteAddressBuffer w, uint offset)
{
    return asfloat(w.Load3(3 * 4 * offset));
}

float4 Getfloat4(ByteAddressBuffer w, uint offset)
{
    return asfloat(w.Load4(4 * 4 * offset));
}

uint4 GetShort4(ByteAddressBuffer w, uint offset)
{
    uint2 j = w.Load2(2 * 4 * offset);
    return uint4(j.x & 0xffff, j.x >> 16, j.y & 0xffff, j.y >> 16);
}

[numthreads(64, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint i = dtID.x;

    uint2 dims;
    OutputBuffer.GetDimensions(dims.x, dims.y);
    if (i>=dims.x)
        return;

    float3 pos = Getfloat3(Position, i);
    
#ifdef HAS_WEIGHTS_0
    matrix skinningMatrix;
    float4 w0 = Getfloat4(Weights0, i);
    uint4  j0 = GetShort4(Joints0, i);
    skinningMatrix  = GetSkinningMatrix(w0, j0);
#ifdef HAS_WEIGHTS_1
    float4 w1 = Getfloat4(Weights1, i);
    uint4  j1 = GetShort4(Joints1, i);
    skinningMatrix += GetSkinningMatrix(w1, j1);
#endif
#else
    matrix skinningMatrix =
    {
        { 1, 0, 0, 0 },
        { 0, 1, 0, 0 },
        { 0, 0, 1, 0 },
        { 0, 0, 0, 1 }
    };
#endif

    OutputBuffer[i] = mul(skinningMatrix, float4(pos, 1)).xyz;
}
