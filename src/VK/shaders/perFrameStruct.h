// AMD Cauldron code
//
// Copyright(c) 2018 Advanced Micro Devices, Inc.All rights reserved.
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
    mat4          mLightViewProj;
    mat4          mLightView;

    vec3          direction;
    float         range;

    vec3          color;
    float         intensity;

    vec3          position;
    float         innerConeCos;

    float         outerConeCos;
    int           type;
    float         depthBias;
    int           shadowMapIndex;
};

const int LightType_Directional = 0;
const int LightType_Point = 1;
const int LightType_Spot = 2;

struct PerFrame
{
    mat4          u_mCameraCurrViewProj;
    mat4          u_mCameraPrevViewProj;
    mat4          u_mCameraCurrViewProjInverse;

    vec4          u_CameraPos;
    float         u_iblFactor;
    float         u_EmissiveFactor;
    vec2          u_invScreenResolution;

    vec4          u_WireframeOptions;

    vec2          u_mCameraCurrJitter;
    vec2          u_mCameraPrevJitter; 

    Light         u_lights[MAX_LIGHT_INSTANCES];
    int           u_lightCount;
    float         u_LodBias;

    vec2          u_padding;
};
