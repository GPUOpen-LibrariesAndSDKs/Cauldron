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

//#extension GL_ARB_separate_shader_objects : enable
//#extension GL_ARB_shading_language_420pack : enable

//--------------------------------------------------------------------------------------
//  Include IO structures
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
//  For VS layout
//--------------------------------------------------------------------------------------

#ifdef ID_4VS_POSITION
    layout (location = ID_4VS_POSITION) in vec4 a_Position;
#endif

#ifdef ID_4VS_COLOR_0
    layout (location = ID_4VS_COLOR_0) in  vec3 a_Color0;
#endif

#ifdef ID_4VS_TEXCOORD_0
    layout (location = ID_4VS_TEXCOORD_0) in  vec2 a_UV0;
#endif

#ifdef ID_4VS_TEXCOORD_1
    layout (location = ID_4VS_TEXCOORD_1) in  vec2 a_UV1;
#endif

#ifdef ID_4VS_NORMAL
    layout (location = ID_4VS_NORMAL) in  vec3 a_Normal;
#endif

#ifdef ID_4VS_TANGENT
    layout (location = ID_4VS_TANGENT) in vec4 a_Tangent;
#endif

//--------------------------------------------------------------------------------------
//  For PS layout (make sure this layout matches the one in glTF20-frag.glsl)
//--------------------------------------------------------------------------------------

#ifdef ID_4PS_COLOR_0
    layout (location = ID_4PS_COLOR_0) out  vec3 v_Color0;
#endif

#ifdef ID_4PS_TEXCOORD_0
    layout (location = ID_4PS_TEXCOORD_0) out vec2 v_UV0;
#endif

#ifdef ID_4PS_TEXCOORD_1
    layout (location = ID_4PS_TEXCOORD_1) out vec2 v_UV1;
#endif
    
#ifdef ID_4PS_NORMAL
    layout (location = ID_4PS_NORMAL) out vec3 v_Normal;
#endif

#ifdef ID_4PS_TANGENT
    layout (location = ID_4PS_TANGENT) out vec3 v_Tangent;
    layout (location = ID_4PS_LASTID+2) out vec3 v_Binormal;
#endif

layout (location = ID_4PS_LASTID+1) out vec3 v_Position;

//--------------------------------------------------------------------------------------
// Constant buffers
//--------------------------------------------------------------------------------------

layout (std140, binding = ID_PER_FRAME) uniform perBatch 
{
    mat4 u_MVPMatrix;
} myPerFrame;

layout (std140, binding = ID_PER_OBJECT) uniform perObject
{
    mat4 u_ModelMatrix;
} myPerObject;

#include "skinning.h"

//--------------------------------------------------------------------------------------
// Main
//--------------------------------------------------------------------------------------
void main()
{
  mat4 transMatrix = ApplySkinning(myPerObject.u_ModelMatrix);

  vec4 pos = transMatrix * a_Position;

  v_Position = vec3(pos.xyz) / pos.w;

  #ifdef ID_4VS_NORMAL
  v_Normal = normalize(vec3(transMatrix * vec4(a_Normal.xyz, 0.0)));

  #ifdef ID_4VS_TANGENT
    v_Tangent = normalize(vec3(transMatrix * vec4(a_Tangent.xyz, 0.0)));
    v_Binormal = cross(v_Normal, v_Tangent) * a_Tangent.w;
  #endif
  #endif

  #ifdef ID_4VS_COLOR_0  
    v_Color0 = a_Color0;
  #endif

  #ifdef ID_4VS_TEXCOORD_0
    v_UV0 = a_UV0;
  #endif

  #ifdef ID_4VS_TEXCOORD_1
    v_UV1 = a_UV1;
  #endif

  gl_Position = myPerFrame.u_MVPMatrix * pos; // needs w for proper perspective correction
}

