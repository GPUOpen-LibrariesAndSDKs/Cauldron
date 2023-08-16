#version 420

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

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_compute_shader  : enable

layout (std140, binding = 0) uniform perBatch 
{
    float u_exposure; 
    int u_toneMapper;
    float u_pad0;
    float u_pad1;
    bool u_shoulder;
    bool u_con;
    bool u_soft;
    bool u_con2;
    bool u_clip;
    bool u_scaleOnly;
    uint u_displayMode;
    uint u_pad;
    mat4 u_inputToOutputMatrix;
    ivec4 u_ctl[24];
} myPerScene;

layout (local_size_x = 8, local_size_y = 8) in;
layout (binding = 1, rgba16f) uniform image2D HDR;

#include "tonemappers.glsl"
#include "LPMTonemapperHelper.glsl"

vec3 Tonemap(vec3 color, float exposure, int tonemapper)
{
    color *= exposure;

    switch (tonemapper)
    {
        case 0: return AMDTonemapper(color);
        case 1: return DX11DSK(color);
        case 2: return Reinhard(color);
        case 3: return Uncharted2Tonemap(color);
        case 4: return tonemapACES( color );
        case 5: return color;
        case 6: return LPMTonemapper(color, myPerScene.u_shoulder, myPerScene.u_con, myPerScene.u_soft, myPerScene.u_con2, myPerScene.u_clip, myPerScene.u_scaleOnly, myPerScene.u_inputToOutputMatrix);
        default: return vec3(1, 1, 1);
    } 
}

void main() 
{
	ivec2 coords = ivec2(gl_GlobalInvocationID.xy);

    vec4 texColor = imageLoad(HDR, coords).rgba;

    vec3 color = Tonemap(texColor.rgb, myPerScene.u_exposure, myPerScene.u_toneMapper);

    imageStore(HDR, coords, vec4(color, texColor.a));
}
