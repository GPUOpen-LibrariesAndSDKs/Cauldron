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

//--------------------------------------------------------------------------------------
// Texture definitions
//--------------------------------------------------------------------------------------
Texture2D<float4> TAABuffer : register(t0);

RWTexture2D<float4> HDR : register(u0);
RWTexture2D<float4> History : register(u1);

//--------------------------------------------------------------------------------------
// Helper functions
//--------------------------------------------------------------------------------------

float3 RGBToYCoCg(in float3 rgb)
{
    return float3(
        0.25f * rgb.r + 0.5f * rgb.g + 0.25f * rgb.b,
        0.5f * rgb.r - 0.5f * rgb.b,
        -0.25f * rgb.r + 0.5f * rgb.g - 0.25f * rgb.b);
}

float3 YCoCgToRGB(in float3 yCoCg)
{
    return float3(
        yCoCg.x + yCoCg.y - yCoCg.z,
        yCoCg.x + yCoCg.z,
        yCoCg.x - yCoCg.y - yCoCg.z);
}

float3 ApplySharpening(in float3 center, in float3 top, in float3 left, in float3 right, in float3 bottom)
{
    float3 result = RGBToYCoCg(center);
    float unsharpenMask = 4.0f * result.x;
    unsharpenMask -= RGBToYCoCg(top).x;
    unsharpenMask -= RGBToYCoCg(bottom).x;
    unsharpenMask -= RGBToYCoCg(left).x;
    unsharpenMask -= RGBToYCoCg(right).x;
    result.x = min(result.x + 0.25f * unsharpenMask, 1.1f * result.x);
    return YCoCgToRGB(result);
}

float3 ReinhardInverse(in float3 sdr)
{
    return sdr / max(1.0f - sdr, 1e-2f);
}

//--------------------------------------------------------------------------------------
// Main function
//--------------------------------------------------------------------------------------
[numthreads(8, 8, 1)]
void mainCS(uint3 globalID : SV_DispatchThreadID, uint3 localID : SV_GroupThreadID, uint localIndex : SV_GroupIndex, uint3 groupID : SV_GroupID)
{
    const float3 center = TAABuffer[globalID.xy].xyz;
    const float3 top    = TAABuffer[globalID.xy + uint2( 0,  1)].xyz;
    const float3 left   = TAABuffer[globalID.xy + uint2( 1,  0)].xyz;
    const float3 right  = TAABuffer[globalID.xy + uint2(-1,  0)].xyz;
    const float3 bottom = TAABuffer[globalID.xy + uint2( 0, -1)].xyz;

    const float3 color = ApplySharpening(center, top, left, right, bottom);

    HDR[globalID.xy] = float4(ReinhardInverse(color), 1.0f);
    History[globalID.xy] = float4(center, 1.0f);
}

[numthreads(8, 8, 1)]
void postCS(uint3 globalID : SV_DispatchThreadID, uint3 localID : SV_GroupThreadID, uint localIndex : SV_GroupIndex, uint3 groupID : SV_GroupID)
{
    const float3 center = TAABuffer[globalID.xy].xyz;

    HDR[globalID.xy] = float4(ReinhardInverse(center), 1.0f);
    History[globalID.xy] = float4(center, 1.0f);
}
