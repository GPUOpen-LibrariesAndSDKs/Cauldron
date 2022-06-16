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

// KHR_lights_punctual extension.
// see https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_lights_punctual

#define MAX_LIGHT_INSTANCES  80
#define MAX_SHADOW_INSTANCES 32

struct Light
{
    matrix        mLightViewProj;
    matrix        mLightView;

    float3        direction;
    float         range;

    float3        color;
    float         intensity;

    float3        position;
    float         innerConeCos;

    float         outerConeCos;
    int           type;
    float         depthBias;
    int           shadowMapIndex;
};

struct LightInstance
{
    matrix        mLightViewProj;
    float3        direction;
    float3        position;
    int           shadowMapIndex;
    float         depthBias;
};

static const int LightType_Directional = 0;
static const int LightType_Point = 1;
static const int LightType_Spot = 2;

struct PerFrame
{
    matrix        u_mCameraCurrViewProj;
    matrix        u_mCameraPrevViewProj;
    matrix        u_mCameraCurrViewProjInverse;
    float4        u_CameraPos;
    float         u_iblFactor;
    float         u_EmissiveFactor;
    float2        u_invScreenResolution;

    float4        u_WireframeOptions;

    float2        u_mCameraCurrJitter;
    float2        u_mCameraPrevJitter;

    Light         u_lights[MAX_LIGHT_INSTANCES];
    int           u_lightCount;
    float         u_LodBias;
};
