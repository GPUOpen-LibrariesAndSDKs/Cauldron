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
// Constant Buffer
//--------------------------------------------------------------------------------------

cbuffer cbPerFrame : register(b0)
{
    matrix        u_mViewProj          :    packoffset(c0);
};

cbuffer cbPerObject : register(b1)
{
    matrix        myPerObject_u_mWorld;
};

cbuffer cbPerSkeleton : register(b2)
{
    matrix        myPerSkeleton_u_ModelMatrix[200];
};

//--------------------------------------------------------------------------------------
// I/O Structures
//--------------------------------------------------------------------------------------

struct VS_INPUT_SCENE
{
    float3 Position     :    POSITION;      // vertex position
#ifdef HAS_NORMAL
    float3 Normal       :    NORMAL;        // this normal comes in per-vertex
#endif        
#ifdef HAS_TANGENT
    float4 Tangent      :    TANGENT;       // this normal comes in per-vertex
#endif    
#ifdef HAS_TEXCOORD_0
    float2 Texcoord     :    TEXCOORD;    // vertex texture coords
#endif

    //skinning

#ifdef HAS_WEIGHTS_0
    float4 Weights0       :    WEIGHTS0;    // vertex texture coords
#endif
#ifdef HAS_WEIGHTS_1
    float4 Weights1       :    WEIGHTS1;    // vertex texture coords
#endif

#ifdef HAS_JOINTS_0
    uint4 Joints0       :    JOINTS0;    // vertex texture coords
#endif
#ifdef HAS_JOINTS_1
    uint4 Joints1       :    JOINTS1;    // vertex texture coords
#endif
};

struct VS_OUTPUT_SCENE
{
    float4 svPosition   :    SV_POSITION;   // vertex position
    float2 Texcoord     :    TEXCOORD;      // vertex texture coords
};

//--------------------------------------------------------------------------------------
// Texture definitions
//--------------------------------------------------------------------------------------
#define TEX(b) t ## b

#ifdef ID_baseColorTexture
Texture2D        baseColorTexture              :register(TEX(ID_baseColorTexture));
#endif      

SamplerState     samLinearWrap              :register(s0);

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

    transMatrix = mul(transMatrix, skinMatrix);
#endif

    float3 worldPos = mul(transMatrix, float4(input.Position, 1)).xyz;
    Output.svPosition = mul(u_mViewProj, float4(worldPos, 1));

#ifdef HAS_TEXCOORD_0
    Output.Texcoord = input.Texcoord;
#else
    Output.Texcoord = float2(0, 0);
#endif

    return Output;
}

void mainPS(VS_OUTPUT_SCENE Input)
{
#ifdef ID_baseColorTexture
    float4 baseColor = baseColorTexture.Sample(samLinearWrap, Input.Texcoord);

    #if defined(DEF_alphaMode_BLEND) 
        if (baseColor.a == 0)
            discard;
    #elif defined(DEF_alphaMode_MASK) && defined(DEF_alphaCutoff)
        if (baseColor.a < DEF_alphaCutoff)
            discard;
    #else 
        //OPAQUE
    #endif

#endif
}