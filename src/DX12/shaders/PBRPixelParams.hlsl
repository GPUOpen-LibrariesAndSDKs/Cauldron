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

#include "PBRTextures.hlsl"

float4 getPixelColor(VS_OUTPUT_SCENE Input)
{
   float4 color = float4(1.0, 1.0, 1.0, 1.0);

#ifdef HAS_COLOR_0
    color = Input.Color0;
#endif

   return color;
}


//------------------------------------------------------------
// PBR getters
//------------------------------------------------------------

struct PBRFactors
{
    float4 myPerObject_u_EmissiveFactor;   
    
    // pbrMetallicRoughness
    float4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;

    float2 u_padding;

    // KHR_materials_pbrSpecularGlossiness
    float4 diffuseFactor;
    float3 specularFactor;
    float glossinessFactor;
};

float4 getBaseColor(VS_OUTPUT_SCENE Input)
{
    float4 baseColor = float4(0.0, 0.0, 0.0, 1.0);
#ifdef MATERIAL_SPECULARGLOSSINESS
    baseColor = getDiffuseTexture(Input);
#endif

#ifdef MATERIAL_METALLICROUGHNESS
    // The albedo may be defined from a base texture or a flat color
    baseColor = getBaseColorTexture(Input);
#endif
    return baseColor;
}

float4 getBaseColor(VS_OUTPUT_SCENE Input, PBRFactors params)
{
    float4 baseColor = getBaseColor(Input);

#ifdef MATERIAL_SPECULARGLOSSINESS
    baseColor *= params.diffuseFactor;
#endif

#ifdef MATERIAL_METALLICROUGHNESS
    baseColor *= params.baseColorFactor;
#endif

    baseColor *= getPixelColor(Input);
    return baseColor;
}

void discardPixelIfAlphaCutOff(VS_OUTPUT_SCENE Input)
{
    float4 baseColor = getBaseColor(Input);

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

void getPBRParams(VS_OUTPUT_SCENE Input, PBRFactors params, out float3 diffuseColor, out float3 specularColor, out float perceptualRoughness, out float alpha)
{
    // Metallic and Roughness material properties are packed together
    // In glTF, these factors can be specified by fixed scalar values
    // or from a metallic-roughness map
    alpha = 0.0;
    perceptualRoughness = 0.0;
    diffuseColor = float3(0.0, 0.0, 0.0);
    specularColor = float3(0.0, 0.0, 0.0);
    float metallic = 0.0;
    float3 f0 = float3(0.04, 0.04, 0.04);

    float4 baseColor = getBaseColor(Input, params);

#ifdef MATERIAL_SPECULARGLOSSINESS
    float4 sgSample = getSpecularGlossinessTexture(Input);
    perceptualRoughness = (1.0 - sgSample.a * params.glossinessFactor); // glossiness to roughness
    f0 = sgSample.rgb * params.specularFactor; // specular

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
    float4 mrSample = getMetallicRoughnessTexture(Input);
    perceptualRoughness = mrSample.g * params.roughnessFactor;
    metallic = mrSample.b * params.metallicFactor;

    diffuseColor = baseColor.rgb * (float3(1.0, 1.0, 1.0) - f0) * (1.0 - metallic);
    specularColor = lerp(f0, baseColor.rgb, metallic);
#endif // ! MATERIAL_METALLICROUGHNESS

    perceptualRoughness = clamp(perceptualRoughness, 0.0, 1.0);

    alpha = baseColor.a;
}
