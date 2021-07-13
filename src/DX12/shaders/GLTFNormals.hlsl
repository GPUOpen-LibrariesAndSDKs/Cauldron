// Portions Copyright 2019 Advanced Micro Devices, Inc.All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#define TEXCOORD(id) CONCAT(Input.UV, id)
#include "GLTFPbrPass-IO.h"

#ifdef ID_normalTexture
Texture2D        normalTexture                 :register(TEX(ID_normalTexture));
SamplerState     samNormal                     :register(SMP(ID_normalTexture));
#endif      

float2 getNormalUV(VS_OUTPUT_SCENE Input)
{
    float3 uv = float3(0.0, 0.0, 1.0);
#ifdef ID_normalTexCoord
    uv.xy = TEXCOORD(ID_normalTexCoord);
#ifdef HAS_NORMAL_UV_TRANSFORM
    uv *= u_NormalUVTransform;
#endif
#endif
    return uv.xy;
}

float3 getNormalTexture(VS_OUTPUT_SCENE Input)
{
#ifdef ID_normalTexture
    float2 xy = 2.0 * normalTexture.SampleBias(samNormal, getNormalUV(Input), myPerFrame.u_LodBias).rg - 1.0;
    float  z = sqrt(1.0f - dot(xy, xy));
    return float3(xy, z);
#else 
    return float3(0, 0, 0);
#endif
}

// Find the normal for this fragment, pulling either from a predefined normal map
// or from the interpolated mesh normal and tangent attributes.
float3 getPixelNormal(VS_OUTPUT_SCENE Input, bool bIsFontFacing = false)
{
    // Retrieve the tangent space matrix
#ifndef HAS_TANGENT
    float2 UV = getNormalUV(Input);   
    float3 pos_dx = ddx(Input.WorldPos);
    float3 pos_dy = ddy(Input.WorldPos);
    float3 tex_dx = ddx(float3(UV, 0.0));
    float3 tex_dy = ddy(float3(UV, 0.0));
    float3 t = (tex_dy.y * pos_dx - tex_dx.y * pos_dy) / (tex_dx.x * tex_dy.y - tex_dy.x * tex_dx.y);

#ifdef HAS_NORMAL
    float3 ng = normalize(Input.Normal);
#else
    float3 ng = cross(pos_dx, pos_dy);
#endif

    t = normalize(t - ng * dot(ng, t));
    float3 b = normalize(cross(ng, t));
    float3x3 tbn = float3x3(t, b, ng);
#else // HAS_TANGENTS
    float3x3 tbn = float3x3(Input.Tangent, Input.Binormal, Input.Normal);
#endif

#ifdef ID_normalTexture
    float3 n = getNormalTexture(Input);
    n = normalize(mul(transpose(tbn),(n /* * float3(u_NormalScale, u_NormalScale, 1.0) */)));
#else
    // The tbn matrix is linearly interpolated, so we need to re-normalize
    float3 n = normalize(tbn[2].xyz);
#endif

    return n * (bIsFontFacing ? -1 : 1);
}

