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

#include "PBRTextures.glsl"

vec4 getPixelColor(VS2PS Input)
{
   vec4 color = vec4(1.0, 1.0, 1.0, 1.0);

#ifdef ID_COLOR_0
    color.xyz = Input.Color0;
#endif

   return color;
}

// Find the normal for this fragment, pulling either from a predefined normal map
// or from the interpolated mesh normal and tangent attributes.
vec3 getPixelNormal(VS2PS Input)
{
    vec2 UV = getNormalUV(Input);

    // Retrieve the tangent space matrix
#ifndef ID_TANGENT
    vec3 pos_dx = dFdx(Input.WorldPos);
    vec3 pos_dy = dFdy(Input.WorldPos);
    vec3 tex_dx = dFdx(vec3(UV, 0.0));
    vec3 tex_dy = dFdy(vec3(UV, 0.0));
    vec3 t = (tex_dy.y * pos_dx - tex_dx.y * pos_dy) / (tex_dx.x * tex_dy.y - tex_dy.x * tex_dx.y);

#ifdef ID_NORMAL
    vec3 ng = normalize(Input.Normal);
#else
    vec3 ng = cross(pos_dx, pos_dy);
#endif

    t = normalize(t - ng * dot(ng, t));
    vec3 b = normalize(cross(ng, t));
    mat3 tbn = mat3(t, b, ng);
#else // HAS_TANGENTS
    mat3 tbn = mat3(Input.Tangent, Input.Binormal, Input.Normal);
#endif

#ifdef ID_normalTexture
    vec2 xy = 2.0 * texture(u_NormalSampler, UV, myPerFrame.u_LodBias).rg - 1.0;
    float z = sqrt(1.0 - dot(xy, xy));
    vec3 n = vec3(xy, z);
    n = normalize(tbn * (n /* * vec3(u_NormalScale, u_NormalScale, 1.0) */));
#else
    // The tbn matrix is linearly interpolated, so we need to re-normalize
    vec3 n = normalize(tbn[2].xyz);
#endif
    
    return n;
}

//------------------------------------------------------------
// PBR getters
//------------------------------------------------------------

struct PBRFactors
{
    vec4 myPerObject_u_EmissiveFactor;

    // pbrMetallicRoughness
    vec4 u_BaseColorFactor;
    float u_MetallicFactor;
    float u_RoughnessFactor;
    vec2 padding;

    // KHR_materials_pbrSpecularGlossiness
    vec4 u_DiffuseFactor;
    vec3 u_SpecularFactor;
    float u_GlossinessFactor;
};

vec4 getBaseColor(VS2PS Input)
{
    vec4 baseColor = vec4(0.0, 0.0, 0.0, 1.0);
#ifdef MATERIAL_SPECULARGLOSSINESS
    baseColor = getDiffuseTexture(Input);
#endif

#ifdef MATERIAL_METALLICROUGHNESS
    // The albedo may be defined from a base texture or a flat color
    baseColor = getBaseColorTexture(Input);
#endif
    return baseColor;
}

vec4 getBaseColor(VS2PS Input, PBRFactors params)
{
    vec4 baseColor = getBaseColor(Input);

#ifdef MATERIAL_SPECULARGLOSSINESS
    baseColor *= params.u_DiffuseFactor;
#endif

#ifdef MATERIAL_METALLICROUGHNESS
    baseColor *= params.u_BaseColorFactor;
#endif

    baseColor *= getPixelColor(Input);
    return baseColor;
}

void discardPixelIfAlphaCutOff(VS2PS Input)
{
    vec4 baseColor = getBaseColor(Input);

#if defined(DEF_alphaMode_BLEND)
        if (baseColor.a == 0)
            discard;
#elif defined(DEF_alphaMode_MASK) && defined(DEF_alphaCutoff)
        if (baseColor.a < DEF_alphaCutoff)
            discard;
#else
        //OPAQUE
#endif
}

void getPBRParams(VS2PS Input, PBRFactors params, out vec3 diffuseColor, out vec3  specularColor, out float perceptualRoughness, out float alpha)
{
    // Metallic and Roughness material properties are packed together
    // In glTF, these factors can be specified by fixed scalar values
    // or from a metallic-roughness map
    alpha = 0.0;
    perceptualRoughness = 0.0;
    diffuseColor = vec3(0.0, 0.0, 0.0);
    specularColor = vec3(0.0, 0.0, 0.0);
    float metallic = 0.0;
    vec3 f0 = vec3(0.04, 0.04, 0.04);

    vec4 baseColor = getBaseColor(Input, params);

#ifdef MATERIAL_SPECULARGLOSSINESS
    vec4 sgSample = getSpecularGlossinessTexture(Input);
    perceptualRoughness = (1.0 - sgSample.a * params.u_GlossinessFactor); // glossiness to roughness
    f0 = sgSample.rgb * params.u_SpecularFactor; // specular

    // f0 = specular
    specularColor = f0;
    float oneMinusSpecularStrength = 1.0 - max(max(f0.r, f0.g), f0.b);
    diffuseColor = baseColor.rgb * oneMinusSpecularStrength;

#ifdef DEBUG_METALLIC
    // do conversion between metallic M-R and S-G metallic
    metallic = solveMetallic(baseColor.rgb, specularColor, oneMinusSpecularStrength);
#endif // ! DEBUG_METALLIC
#endif // ! MATERIAL_SPECULARGLOSSINESS

#ifdef MATERIAL_METALLICROUGHNESS
    // Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
    // This layout intentionally reserves the 'r' channel for (optional) occlusion map data
    vec4 mrSample = getMetallicRoughnessTexture(Input);
    perceptualRoughness = mrSample.g * params.u_RoughnessFactor;
    metallic = mrSample.b * params.u_MetallicFactor;

    diffuseColor = baseColor.rgb * (vec3(1.0, 1.0, 1.0) - f0) * (1.0 - metallic);
    specularColor = mix(f0, baseColor.rgb, metallic);
#endif // ! MATERIAL_METALLICROUGHNESS

    perceptualRoughness = clamp(perceptualRoughness, 0.0, 1.0);

    alpha = baseColor.a;
}
