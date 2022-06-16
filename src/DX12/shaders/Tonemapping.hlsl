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

#include "tonemappers.hlsl"

//--------------------------------------------------------------------------------------
// Constant Buffer
//--------------------------------------------------------------------------------------
cbuffer cbPerFrame : register(b0)
{
    float u_exposure;
    int u_toneMapper;
    int u_gamma2;
    float u_pad1;
    uint u_shoulder;
    uint u_con;
    uint u_soft;
    uint u_con2;
    uint u_clip;
    uint u_scaleOnly;
    uint u_displayMode;
    uint u_pad;
    matrix u_inputToOutputMatrix;
    uint4 u_ctl[24];
}

//--------------------------------------------------------------------------------------
// I/O Structures
//--------------------------------------------------------------------------------------
struct VERTEX
{
    float2 vTexcoord : TEXCOORD;
};

//--------------------------------------------------------------------------------------
// Texture definitions
//--------------------------------------------------------------------------------------
Texture2D        HDR              :register(t0);
SamplerState     samLinearWrap    :register(s0);


float3 Pattern(float2 vTexcoord)
{
    if (vTexcoord.x < .5)
        return float3(.5, .5, .5);

    uint y = vTexcoord.y * 720;
    if ((y & 1) == 1)
        return float3(1.0, 1.0, 1.0);


    return float3(0, 0, 0);
}

float3 ApplyGamma(float3 color)
{
    return pow(abs(color.rgb), 1.0f / 2.2f);
}

#include "LPMTonemapperHelper.hlsl"

float3 Tonemap(float3 color, float exposure, int tonemapper)
{
    color *= exposure;

    switch (tonemapper)
    {
    case 0: return AMDTonemapper(color);
    case 1: return DX11DSK(color);
    case 2: return Reinhard(color);
    case 3: return Uncharted2Tonemap(color);
    case 4: return ACESFilm(color);
    case 5: return color;
    case 6: return LPMTonemapper(color, u_shoulder, u_con, u_soft, u_con2, u_clip, u_scaleOnly, u_inputToOutputMatrix);
    default: return float3(1, 1, 1);
    }
}

//--------------------------------------------------------------------------------------
// Main function
//--------------------------------------------------------------------------------------

float4 mainPS(VERTEX Input) : SV_Target
{
    if (u_exposure < 0)
    {
        return HDR.Sample(samLinearWrap, Input.vTexcoord);
    }

    float4 texColor = HDR.Sample(samLinearWrap, Input.vTexcoord);

    float3 color = Tonemap(texColor.rgb, u_exposure, u_toneMapper);
    if (u_gamma2 == 1)
        color = sqrt(color);
    return float4(color, 1);
}
