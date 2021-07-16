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
// 

//--------------------------------------------------------------------------------------
// Constant Buffers
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// PerFrame structure, must match the one in GlTFCommon.h
//--------------------------------------------------------------------------------------

#include "perFrameStruct.h"

cbuffer cbPerFrame : register(b0)
{
    PerFrame myPerFrame;
};

//--------------------------------------------------------------------------------------
// Texture definitions
//--------------------------------------------------------------------------------------
Texture2D DepthBuffer : register(t1);
RWTexture2D<float4> OutputBuffer : register(u0);

//--------------------------------------------------------------------------------------
// Compute Shader
//--------------------------------------------------------------------------------------

#include "common.h"
#include "shadowFiltering.h"

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void mainCS(uint3 Tid : SV_DispatchThreadID)
{
    uint2 dims;
    OutputBuffer.GetDimensions(dims.x, dims.y);
    float result[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    // Retrieve the iterated texel
    if (any(Tid.xy >= dims)) return;    // out of bounds
    const float2 uv = (Tid.xy + 0.5f) / float2(dims);
    const float depth = DepthBuffer.Load(int3(Tid.xy, 0)).x;
    const float2 ndc = 2.0f * float2(uv.x, 1.0f - uv.y) - 1.0f;

    // No need to evaluate sky pixels
    if (depth < 1.0f)
    {
        // Recover the world-space position
        const float4 homogeneous = mul(myPerFrame.u_mCameraViewProjInverse, float4(ndc, depth, 1.0f));
        const float3 world_position = homogeneous.xyz / homogeneous.w;  // perspective divide

        // Iterate the lights and resolve the shadows
        for (int i = 0; i < myPerFrame.u_lightCount; ++i)
        {
            Light light = myPerFrame.u_lights[i];

            if (light.type != LightType_Spot && light.type != LightType_Directional)
            {
                continue;   // only spotlight shadows are currently supported
            }

            result[i] = DoSpotShadow(world_position, light);
        }
    }

    // Write the results out to memory
    OutputBuffer[Tid.xy] = float4(result[0], result[1], result[2], result[3]);
}
