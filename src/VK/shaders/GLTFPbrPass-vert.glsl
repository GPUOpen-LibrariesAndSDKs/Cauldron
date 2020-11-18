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

//--------------------------------------------------------------------------------------
//  Include IO structures
//--------------------------------------------------------------------------------------

#include "GLTF_VS2PS_IO.glsl"

//--------------------------------------------------------------------------------------
// Constant buffers
//--------------------------------------------------------------------------------------
#include "perFrameStruct.h"

layout (std140, binding = ID_PER_FRAME) uniform _PerFrame 
{
    PerFrame myPerFrame;
};

layout (std140, binding = ID_PER_OBJECT) uniform perObject
{
    mat4 u_mCurrWorld;
    mat4 u_mPrevWorld;
} myPerObject;

mat4 GetWorldMatrix()
{
    return myPerObject.u_mCurrWorld;
}

mat4 GetCameraViewProj()
{
    return myPerFrame.u_mCameraCurrViewProj;
}

mat4 GetPrevWorldMatrix()
{
    return myPerObject.u_mPrevWorld;
}

mat4 GetPrevCameraViewProj()
{
    return myPerFrame.u_mCameraPrevViewProj;
}

#include "GLTFVertexFactory.glsl"

//--------------------------------------------------------------------------------------
// Main
//--------------------------------------------------------------------------------------
void main()
{
	gltfVertexFactory();
}

