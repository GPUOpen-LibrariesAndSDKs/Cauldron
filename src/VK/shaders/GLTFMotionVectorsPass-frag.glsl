#version 450

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
//

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

precision highp float;

//--------------------------------------------------------------------------------------
// I/O Structures
//--------------------------------------------------------------------------------------

#include "GLTF_VS2PS_IO.glsl"
layout (location = 0) in VS2PS Input;

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

#include "PixelParams.glsl"

#ifdef HAS_MOTION_VECTORS
    layout (location = HAS_MOTION_VECTORS) out vec2 motionVect;
#endif

#ifdef HAS_NORMALS
    layout (location = HAS_NORMALS) out vec4 normals;
#endif
 
//--------------------------------------------------------------------------------------
// main
//--------------------------------------------------------------------------------------

void main()
{
    discardPixelIfAlphaCutOff(Input);

#ifdef HAS_MOTION_VECTORS
    motionVect = Input.CurrPosition.xy / Input.CurrPosition.w -
                 Input.PrevPosition.xy / Input.PrevPosition.w;
#endif

#ifdef HAS_NORMALS
    normals = vec4((getPixelNormal(Input) + 1) / 2, 0);
#endif
}