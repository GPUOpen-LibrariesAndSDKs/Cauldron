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

#include "Skinning.hlsl"

//--------------------------------------------------------------------------------------
//  For VS input struct
//--------------------------------------------------------------------------------------

struct VS_INPUT_SCENE
{
    float3 Position : POSITION; // vertex position
#ifdef HAS_NORMAL
    float3 Normal       :    NORMAL;        // this normal comes in per-vertex
#endif
#ifdef HAS_TANGENT
    float4 Tangent      :    TANGENT;       // this normal comes in per-vertex
#endif
#ifdef HAS_TEXCOORD_0
    float2 UV0          :    TEXCOORD0;    // vertex texture coords
#endif
#ifdef HAS_TEXCOORD_1
    float2 UV1          :    TEXCOORD1;    // vertex texture coords
#endif

#ifdef HAS_COLOR_0
    float4 Color0       :    COLOR0;
#endif

    // joints weights
    //
#ifdef HAS_WEIGHTS_0
    float4 Weights0     :    WEIGHTS0;
#endif
#ifdef HAS_WEIGHTS_1
    float4 Weights1     :    WEIGHTS1;
#endif

    // joints indices
    //
#ifdef HAS_JOINTS_0
    uint4 Joints0       :    JOINTS0;
#endif
#ifdef HAS_JOINTS_1
    uint4 Joints1       :    JOINTS1;
#endif
};

//--------------------------------------------------------------------------------------
// mainVS
//--------------------------------------------------------------------------------------
VS_OUTPUT_SCENE gltfVertexFactory(VS_INPUT_SCENE input)
{
    VS_OUTPUT_SCENE Output;

#ifdef HAS_WEIGHTS_0
    matrix skinningMatrix;
    skinningMatrix  = GetCurrentSkinningMatrix(input.Weights0, input.Joints0);
#ifdef HAS_WEIGHTS_1
    skinningMatrix += GetCurrentSkinningMatrix(input.Weights1, input.Joints1);
#endif
#else
    matrix skinningMatrix =
    {
        { 1, 0, 0, 0 },
        { 0, 1, 0, 0 },
        { 0, 0, 1, 0 },
        { 0, 0, 0, 1 }
    };
#endif

    matrix transMatrix = mul(GetWorldMatrix(), skinningMatrix);
    Output.WorldPos = mul(transMatrix, float4(input.Position, 1)).xyz;
    Output.svPosition = mul(GetCameraViewProj(), float4(Output.WorldPos, 1));

#ifdef HAS_MOTION_VECTORS

    #ifdef HAS_WEIGHTS_0
        matrix prevSkinningMatrix;
        prevSkinningMatrix  = GetPreviousSkinningMatrix(input.Weights0, input.Joints0);
    #ifdef HAS_WEIGHTS_1
        prevSkinningMatrix += GetPreviousSkinningMatrix(input.Weights1, input.Joints1);
    #endif
    #else
        matrix prevSkinningMatrix =
        {
            { 1, 0, 0, 0 },
            { 0, 1, 0, 0 },
            { 0, 0, 1, 0 },
            { 0, 0, 0, 1 }
        };
    #endif

    Output.svCurrPosition = Output.svPosition; // current's frame vertex position 

    matrix prevTransMatrix = mul(GetPrevWorldMatrix(), prevSkinningMatrix);
    float3 worldPrevPos = mul(prevTransMatrix, float4(input.Position, 1)).xyz;
    Output.svPrevPosition = mul(GetPrevCameraViewProj(), float4(worldPrevPos, 1));
#endif

#ifdef HAS_NORMAL
    Output.Normal  = normalize(mul(transMatrix, float4(input.Normal, 0)).xyz);
#endif

#ifdef HAS_TANGENT
    Output.Tangent = normalize(mul(transMatrix, float4(input.Tangent.xyz, 0)).xyz);
    Output.Binormal = cross(Output.Normal, Output.Tangent);
    // Fix broken tangent space from the Blender glTF exporter
    if (dot(cross(Output.Normal, Output.Tangent), Output.Binormal) < 0.0f) {
        Output.Binormal = Output.Binormal * -1.0f;
    }
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
