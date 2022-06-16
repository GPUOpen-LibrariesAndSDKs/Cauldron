// Portions Copyright 2019 Advanced Micro Devices, Inc.All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
 
// This shader code was ported from https://github.com/KhronosGroup/glTF-WebGL-PBR
// All credits should go to his original author.
 
//
// This fragment shader defines a reference implementation for Physically Based Shading of
// a microfacet surface material defined by a glTF model.
//
// References:
// [1] Real Shading in Unreal Engine 4
//     http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
// [2] Physically Based Shading at Disney
//     http://blog.selfshadow.com/publications/s2012-shading-course/burley/s2012_pbs_disney_brdf_notes_v3.pdf
// [3] README.md - Environment Maps
//     https://github.com/KhronosGroup/glTF-WebGL-PBR/#environment-maps
// [4] "An Inexpensive BRDF Model for Physically based Rendering" by Christophe Schlick
//     https://www.cs.virginia.edu/~jdl/bib/appearance/analytic%20models/schlick94b.pdf
//
#include "common.h"

//--------------------------------------------------------------------------------------
//  Include IO structures
//--------------------------------------------------------------------------------------
#include "GLTFPbrPass-IO.h"

//--------------------------------------------------------------------------------------
//  Remove texture references if the material doesn't have texture coordinates
//--------------------------------------------------------------------------------------

#ifndef HAS_TEXCOORD_0
    #if ID_normalTexCoord == 0
    #undef ID_normalTexCoord
    #endif
    #if ID_emissiveTexCoord == 0
    #undef ID_emissiveTexCoord
    #endif
    #if ID_occlusionTexCoord == 0
    #undef ID_occlusionTexCoord
    #endif
    #if ID_baseTexCoord == 0
    #undef ID_baseTexCoord
    #endif
    #if ID_metallicRoughnessTexCoord == 0
    #undef ID_metallicRoughnessTexCoord
    #endif
#endif

#ifndef HAS_TEXCOORD_1
    #if ID_normalTexCoord == 1
    #undef ID_normalTexCoord
    #endif
    #if ID_emissiveTexCoord == 1
    #undef ID_emissiveTexCoord
    #endif
    #if ID_occlusionTexCoord == 1
    #undef ID_occlusionTexCoord
    #endif
    #if ID_baseTexCoord == 1
    #undef ID_baseTexCoord
    #endif
    #if ID_metallicRoughnessTexCoord == 1
    #undef ID_metallicRoughnessTexCoord
    #endif
#endif

//--------------------------------------------------------------------------------------
//
// Constant Buffers 
//
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// PerFrame structure, must match the one in GlTFCommon.h
//--------------------------------------------------------------------------------------

#include "perFrameStruct.h"

cbuffer cbPerFrame : register(b0)
{
    PerFrame myPerFrame;
};

//--------------------------------------------------------------------------------------
// PerObject structure, must match the one in GltfPbrPass.h
//--------------------------------------------------------------------------------------

#include "PBRPixelParams.hlsl"

cbuffer cbPerObject : register(b1)
{
    matrix        myPerObject_u_mCurrWorld;
    matrix        myPerObject_u_mPrevWorld;
    
    PBRFactors    u_pbrParams;
};

#include "functions.hlsl"
#include "shadowFiltering.h"
#include "GLTFPBRLighting.hlsl"

//--------------------------------------------------------------------------------------
// mainPS DEFERRED & FORWARD pass
//--------------------------------------------------------------------------------------

struct Output
{
#ifdef HAS_MOTION_VECTORS_RT
    float2 motionVectors : TARGET(HAS_MOTION_VECTORS_RT);
#endif    

#ifdef HAS_FORWARD_RT
    float4 finalColor : TARGET(HAS_FORWARD_RT);
#endif    

#ifdef HAS_SPECULAR_ROUGHNESS_RT
    float4 specularRoughness : TARGET(HAS_SPECULAR_ROUGHNESS_RT);
#endif   

#ifdef HAS_DIFFUSE_RT
    float4 diffuseColor : TARGET(HAS_DIFFUSE_RT);
#endif

#ifdef HAS_NORMALS_RT
    float4 normals : TARGET(HAS_NORMALS_RT);
#endif

#ifdef HAS_UPSCALE_REACTIVE_RT
    float upscaleReactive : TARGET(HAS_UPSCALE_REACTIVE_RT);
#endif

#ifdef HAS_UPSCALE_TRANSPARENCY_AND_COMPOSITION_RT
    float upscaleTransparencyAndComposition : TARGET(HAS_UPSCALE_TRANSPARENCY_AND_COMPOSITION_RT);
#endif
};

Output mainPS(VS_OUTPUT_SCENE Input, bool bIsFontFacing : SV_IsFrontFace)
{
    discardPixelIfAlphaCutOff(Input);

    float alpha;
    float perceptualRoughness;
    float3 diffuseColor;
    float3 specularColor;
    getPBRParams(Input, u_pbrParams, diffuseColor, specularColor, perceptualRoughness, alpha);

    // Roughness is authored as perceptual roughness; as is convention,
    // convert to material roughness by squaring the perceptual roughness [2].
    float alphaRoughness = perceptualRoughness * perceptualRoughness;

    Output output;

#ifdef HAS_MOTION_VECTORS_RT
    float2 cancelJitter = myPerFrame.u_mCameraPrevJitter - myPerFrame.u_mCameraCurrJitter;
    output.motionVectors =
        (Input.svPrevPosition.xy / Input.svPrevPosition.w) - (Input.svCurrPosition.xy / Input.svCurrPosition.w)
                + cancelJitter;

    // Transform motion vector from NDC space to UV space (+Y is top-down).
    output.motionVectors *= float2(0.5f, -0.5f);
#endif       
    
#ifdef HAS_SPECULAR_ROUGHNESS_RT
    output.specularRoughness = float4(specularColor, alphaRoughness);
#endif

#ifdef HAS_DIFFUSE_RT
    output.diffuseColor = float4(diffuseColor, alpha);
#endif

#ifdef HAS_FORWARD_RT
    output.finalColor = float4(doPbrLighting(Input, myPerFrame, diffuseColor, specularColor, perceptualRoughness), alpha);
    output.finalColor = lerp(output.finalColor, float4(myPerFrame.u_WireframeOptions.rgb, 1.0), myPerFrame.u_WireframeOptions.w);
#endif            

#ifdef HAS_NORMALS_RT
    output.normals = float4(getPixelNormal(Input, bIsFontFacing) / 2 + 0.5f, 0);
#endif
    
#ifdef HAS_UPSCALE_TRANSPARENCY_AND_COMPOSITION_RT
    #if defined(HAS_NORMAL_UV_TRANSFORM)
        || defined(HAS_EMISSIVE_UV_TRANSFORM)
        || defined(HAS_OCCLSION_UV_TRANSFORM)
        || defined(HAS_BASECOLOR_UV_TRANSFORM)
        || defined(HAS_METALLICROUGHNESS_UV_TRANSFORM)
        || defined(HAS_SPECULARGLOSSINESS_UV_TRANSFORM)
        || defined(HAS_DIFFUSE_UV_TRANSFORM)
        float hasAnimatedTexture = 1.f;
    #else
        float hasAnimatedTexture = 0.f;
    #endif
    output.upscaleTransparencyAndComposition = max(alpha, hasAnimatedTexture);
#endif

    return output;
}