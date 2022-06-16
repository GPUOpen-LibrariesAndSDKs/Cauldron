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
#include "common.h"

//--------------------------------------------------------------------------------------
// Constant Buffer
//--------------------------------------------------------------------------------------
#include "perFrameStruct.h"

//--------------------------------------------------------------------------------------
// Constant Buffer
//--------------------------------------------------------------------------------------

cbuffer cbPerFrame : register(b0)
{
    PerFrame myPerFrame;
};

cbuffer cbPerObject : register(b1)
{
    matrix        myPerObject_u_mCurrWorld;
    matrix        myPerObject_u_mPrevWorld;
};

//--------------------------------------------------------------------------------------
// I/O Structures
//--------------------------------------------------------------------------------------

#include "GLTFPbrPass-IO.h"
#include "GLTFNormals.hlsl"

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------

matrix GetWorldMatrix()
{
    return myPerObject_u_mCurrWorld;
}

matrix GetCameraViewProj()
{
    return myPerFrame.u_mCameraCurrViewProj;
}

matrix GetPrevWorldMatrix()
{
    return myPerObject_u_mPrevWorld;
}

matrix GetPrevCameraViewProj()
{
    return myPerFrame.u_mCameraPrevViewProj;
}

#include "GLTFVertexFactory.hlsl"

VS_OUTPUT_SCENE mainVS(VS_INPUT_SCENE input)
{
    VS_OUTPUT_SCENE Output = gltfVertexFactory(input);

    return Output;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

#include "PBRPixelParams.hlsl"

struct Output
{
#ifdef HAS_MOTION_VECTORS
    float2 MotionVect : TARGET(HAS_MOTION_VECTORS);
#endif

#ifdef HAS_NORMALS
    float4 normals : TARGET(HAS_NORMALS);
#endif

};

Output mainPS(VS_OUTPUT_SCENE Input, bool bIsFontFacing : SV_IsFrontFace)
{
    Output output;
    discardPixelIfAlphaCutOff(Input);

#ifdef HAS_MOTION_VECTORS
    output.MotionVect = Input.svCurrPosition.xy / Input.svCurrPosition.w -
                        Input.svPrevPosition.xy / Input.svPrevPosition.w;
#endif

#ifdef HAS_NORMALS
    output.normals = float4(getPixelNormal(Input, bIsFontFacing) / 2 + 0.5f, 0);
#endif

    return output;
}
