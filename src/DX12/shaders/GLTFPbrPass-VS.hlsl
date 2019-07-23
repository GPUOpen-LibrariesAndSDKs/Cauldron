// AMD AMDUtils code
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

//--------------------------------------------------------------------------------------
//  Include IO structures
//--------------------------------------------------------------------------------------

#include "GLTFPbrPass-IO.hlsl"

//--------------------------------------------------------------------------------------
//  Constant buffers
//--------------------------------------------------------------------------------------

cbuffer cbPerFrame : register(b0)
{
    matrix        myPerFrame_u_mCameraViewProj;
    float4        myPerFrame_u_CameraPos;
    float         myPerFrame_u_iblFactor;
    float         myPerFrame_u_EmissiveFactor;
};

cbuffer cbPerObject : register(b1)
{
    matrix        myPerObject_u_mWorld;
}

cbuffer cbPerSkeleton : register(b2)
{
    matrix        myPerSkeleton_u_ModelMatrix[200];
};


//--------------------------------------------------------------------------------------
// mainVS
//--------------------------------------------------------------------------------------
VS_OUTPUT_SCENE mainVS(VS_INPUT_SCENE input)
{
    VS_OUTPUT_SCENE Output;

    matrix transMatrix = myPerObject_u_mWorld;

    //
    // skinning
    //
#ifdef HAS_WEIGHTS_0
    matrix skinMatrix =
        input.Weights0.x * myPerSkeleton_u_ModelMatrix[input.Joints0.x] +
        input.Weights0.y * myPerSkeleton_u_ModelMatrix[input.Joints0.y] +
        input.Weights0.z * myPerSkeleton_u_ModelMatrix[input.Joints0.z] +
        input.Weights0.w * myPerSkeleton_u_ModelMatrix[input.Joints0.w];
    
#ifdef HAS_WEIGHTS_1
    skinMatrix +=
        input.Weights1.x * myPerSkeleton_u_ModelMatrix[input.Joints1.x] +
        input.Weights1.y * myPerSkeleton_u_ModelMatrix[input.Joints1.y] +
        input.Weights1.z * myPerSkeleton_u_ModelMatrix[input.Joints1.z] +
        input.Weights1.w * myPerSkeleton_u_ModelMatrix[input.Joints1.w];
#endif

    transMatrix = mul(transMatrix , skinMatrix);
#endif

    Output.WorldPos = mul(transMatrix, float4(input.Position, 1)).xyz;

    Output.svPosition = mul(myPerFrame_u_mCameraViewProj, float4(Output.WorldPos,1));
#ifdef HAS_NORMAL
    Output.Normal  = normalize(mul(transMatrix, float4(input.Normal, 0)).xyz);
#endif    
#ifdef HAS_TANGENT    
    Output.Tangent = normalize(mul(transMatrix, float4(input.Tangent.xyz, 0)).xyz);
    Output.Binormal = cross(Output.Normal, Output.Tangent) *input.Tangent.w;
#endif    

#ifdef HAS_COLOR_0
    Output.Color0 = input.Color0;
#endif

#ifdef HAS_TEXCOORD_0
    Output.UV0 = input.UV0;
#endif

#ifdef HAS_TEXCOORD_1
    Output.UV1 = input.UV1;
#endif

    return Output;
}

