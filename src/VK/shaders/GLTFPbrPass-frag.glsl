#version 450

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

//#extension GL_OES_standard_derivatives : enable

#extension GL_EXT_shader_texture_lod: enable
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
// this makes the structures declared with a scalar layout match the c structures
#extension GL_EXT_scalar_block_layout : enable
#ifdef HAS_VRS_RT
//#extension GL_EXT_fragment_shading_rate : require
#endif

precision highp float;

#define USE_PUNCTUAL

//--------------------------------------------------------------------------------------
//  PS Inputs
//--------------------------------------------------------------------------------------

#include "GLTF_VS2PS_IO.glsl"
layout (location = 0) in VS2PS Input;

//--------------------------------------------------------------------------------------
// PS Outputs
//--------------------------------------------------------------------------------------

#ifdef HAS_MOTION_VECTORS_RT
layout(location = HAS_MOTION_VECTORS_RT) out vec2 Output_motionVect;
#endif

#ifdef HAS_FORWARD_RT
    layout (location = HAS_FORWARD_RT) out vec4 Output_finalColor;
#endif

#ifdef HAS_SPECULAR_ROUGHNESS_RT
    layout (location = HAS_SPECULAR_ROUGHNESS_RT) out vec4 Output_specularRoughness;
#endif

#ifdef HAS_DIFFUSE_RT
    layout (location = HAS_DIFFUSE_RT) out vec4 Output_diffuseColor;
#endif

#ifdef HAS_NORMALS_RT
    layout (location = HAS_NORMALS_RT) out vec4 Output_normal;
#endif

#ifdef HAS_UPSCALE_REACTIVE_RT
    layout (location = HAS_UPSCALE_REACTIVE_RT) out float Output_upscaleReactive;
#endif

#ifdef HAS_UPSCALE_TRANSPARENCY_AND_COMPOSITION_RT
    layout (location = HAS_UPSCALE_TRANSPARENCY_AND_COMPOSITION_RT) out float Output_upscaleTransparencyAndComposition;
#endif

//--------------------------------------------------------------------------------------
//
// Constant Buffers 
//
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Per Frame structure, must match the one in GlTFCommon.h
//--------------------------------------------------------------------------------------

#include "perFrameStruct.h"

layout (scalar, set=0, binding = 0) uniform perFrame 
{
	PerFrame myPerFrame;
};

//--------------------------------------------------------------------------------------
// PerFrame structure, must match the one in GltfPbrPass.h
//--------------------------------------------------------------------------------------

#include "PixelParams.glsl"

layout (scalar, set=0, binding = 1) uniform perObject 
{
    mat4 myPerObject_u_mCurrWorld;
    mat4 myPerObject_u_mPrevWorld;

	PBRFactors u_pbrParams;
};


//--------------------------------------------------------------------------------------
// mainPS
//--------------------------------------------------------------------------------------

#include "functions.glsl"
#include "shadowFiltering.h"
#include "GLTFPBRLighting.h"

void main()
{
    discardPixelIfAlphaCutOff(Input);

    float alpha;
    float perceptualRoughness;
    vec3 diffuseColor;
    vec3 specularColor;
	getPBRParams(Input, u_pbrParams, diffuseColor, specularColor, perceptualRoughness, alpha);

    // Roughness is authored as perceptual roughness; as is convention,
    // convert to material roughness by squaring the perceptual roughness [2].
    float alphaRoughness = perceptualRoughness * perceptualRoughness;

#ifdef HAS_MOTION_VECTORS_RT
    vec2 cancelJitter = myPerFrame.u_mCameraPrevJitter - myPerFrame.u_mCameraCurrJitter;
    Output_motionVect =
        (Input.PrevPosition.xy / Input.PrevPosition.w) - (Input.CurrPosition.xy / Input.CurrPosition.w)
                + cancelJitter;

    // Transform motion vector from NDC space to UV space (+Y is top-down).
    Output_motionVect *= vec2(0.5f, -0.5f);
#endif

#ifdef HAS_SPECULAR_ROUGHNESS_RT
    Output_specularRoughness = vec4(specularColor, alphaRoughness);
#endif

#ifdef HAS_DIFFUSE_RT
    Output_diffuseColor = vec4(diffuseColor, 0);
#endif

#ifdef HAS_NORMALS_RT
    Output_normal = vec4((getPixelNormal(Input) + 1) / 2, 0);
#endif

#ifdef HAS_FORWARD_RT
	Output_finalColor = vec4(doPbrLighting(Input, myPerFrame, diffuseColor, specularColor, perceptualRoughness), alpha);
    Output_finalColor = mix(Output_finalColor, vec4(myPerFrame.u_WireframeOptions.rgb, 1.0), myPerFrame.u_WireframeOptions.w);
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
    Output_upscaleTransparencyAndComposition = max(alpha, hasAnimatedTexture);
#endif

}
