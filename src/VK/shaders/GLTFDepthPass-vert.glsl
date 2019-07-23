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

#include "skinning.h"

//--------------------------------------------------------------------------------------
// Constant Buffer
//--------------------------------------------------------------------------------------

layout (std140, binding = ID_PER_FRAME) uniform myFrame 
{
    mat4 u_MVPMatrix;
} myPerFrame;

layout (std140, binding = ID_PER_OBJECT) uniform perObject
{
    mat4 u_ModelMatrix;
} myPerObject;

//--------------------------------------------------------------------------------------
// I/O Structures
//--------------------------------------------------------------------------------------

/////////////////   4VS

#ifdef ID_4VS_POSITION
    layout (location = ID_4VS_POSITION) in vec4 a_Position;
#endif

#ifdef ID_4VS_TEXCOORD_0
    layout (location = ID_4VS_TEXCOORD_0) in  vec2 a_UV;
#endif

/////////////////   4PS

#ifdef ID_4PS_TEXCOORD_0
    layout (location = ID_4PS_TEXCOORD_0) out vec2 v_UV;
#endif

//--------------------------------------------------------------------------------------
// main
//--------------------------------------------------------------------------------------
void main()
{
    mat4 transMatrix = ApplySkinning(myPerObject.u_ModelMatrix);

    vec4 pos = transMatrix * a_Position;

    gl_Position = myPerFrame.u_MVPMatrix * pos;

#if defined(ID_4VS_TEXCOORD_0) && defined(ID_4PS_TEXCOORD_0)
    v_UV = a_UV;
#endif
}
