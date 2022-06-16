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
    float u_pad0;
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
// Texture definitions
//--------------------------------------------------------------------------------------
RWTexture2D<float4>      HDR              :register(u0);

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

[numthreads(WIDTH, HEIGHT, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    int2 coord = dtID.xy;

    float4 texColor = HDR.Load(int3(coord, 0));

    float3 color = Tonemap(texColor.rgb, u_exposure, u_toneMapper);

    HDR[coord] = float4(color, texColor.a);
}
