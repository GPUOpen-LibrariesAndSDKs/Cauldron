#define USE_PUNCTUAL

#include "GLTFNormals.hlsl"

struct MaterialInfo
{
    float perceptualRoughness; // roughness value, as authored by the model creator (input to shader)
    float3 reflectance0; // full reflectance color (normal incidence angle)

    float alphaRoughness; // roughness mapped to a more linear change in the roughness (proposed by [2])
    float3 diffuseColor; // color contribution from diffuse lighting

    float3 reflectance90; // reflectance color at grazing angle
    float3 specularColor; // color contribution from specular lighting
};

//
//  Get SSAO from texture
//
#ifdef ID_SSAO
Texture2D<float>  ssaoTexture                :register(TEX(ID_SSAO));
SamplerState      ssaoSampler                :register(SMP(ID_SSAO));

float GetSSAO(float2 coords)
{
    return ssaoTexture.Sample(ssaoSampler, coords);
}
#else
float GetSSAO(float2 coords)
{
    return 1.0f;
}
#endif

// Calculation of the lighting contribution from an optional Image Based Light source.
// Precomputed Environment Maps are required uniform inputs and are computed as outlined in [1].
// See our README.md on Environment Maps [3] for additional discussion.
#ifdef USE_IBL
float3 getIBLContribution(MaterialInfo materialInfo, float3 n, float3 v)
{
    float NdotV = clamp(dot(n, v), 0.0, 1.0);

    float u_MipCount = 9.0; // resolution of 512x512 of the IBL
    float lod = clamp(materialInfo.perceptualRoughness * float(u_MipCount), 0.0, float(u_MipCount));
    float3 reflection = normalize(reflect(-v, n));

    float2 brdfSamplePoint = clamp(float2(NdotV, materialInfo.perceptualRoughness), float2(0.0, 0.0), float2(1.0, 1.0));
    // retrieve a scale and bias to F0. See [1], Figure 3
    float2 brdf = brdfTexture.Sample(samBRDF, brdfSamplePoint).rg;

    float3 diffuseLight = diffuseCube.Sample(samDiffuseCube, n).rgb;

#ifdef USE_TEX_LOD
    float3 specularLight = specularCube.SampleLevel(samSpecularCube, reflection, lod).rgb;
#else
    float3 specularLight = specularCube.Sample(samSpecularCube, reflection).rgb;
#endif

    float3 diffuse = diffuseLight * materialInfo.diffuseColor;
    float3 specular = specularLight * (materialInfo.specularColor * brdf.x + brdf.y);

    return diffuse + specular;
}
#endif

// Lambert lighting
// see https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
float3 diffuse(MaterialInfo materialInfo)
{
    return materialInfo.diffuseColor / M_PI;
}

// The following equation models the Fresnel reflectance term of the spec equation (aka F())
// Implementation of fresnel from [4], Equation 15
float3 specularReflection(MaterialInfo materialInfo, AngularInfo angularInfo)
{
    return materialInfo.reflectance0 + (materialInfo.reflectance90 - materialInfo.reflectance0) * pow(clamp(1.0 - angularInfo.VdotH, 0.0, 1.0), 5.0);
}

// Smith Joint GGX
// Note: Vis = G / (4 * NdotL * NdotV)
// see Eric Heitz. 2014. Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs. Journal of Computer Graphics Techniques, 3
// see Real-Time Rendering. Page 331 to 336.
// see https://google.github.io/filament/Filament.md.html#materialsystem/specularbrdf/geometricshadowing(specularg)
float visibilityOcclusion(MaterialInfo materialInfo, AngularInfo angularInfo)
{
    float NdotL = angularInfo.NdotL;
    float NdotV = angularInfo.NdotV;
    float alphaRoughnessSq = materialInfo.alphaRoughness * materialInfo.alphaRoughness;

    float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);
    float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);

    float GGX = GGXV + GGXL;
    if (GGX > 0.0)
    {
        return 0.5 / GGX;
    }
    return 0.0;
}

// The following equation(s) model the distribution of microfacet normals across the area being drawn (aka D())
// Implementation from "Average Irregularity Representation of a Roughened Surface for Ray Reflection" by T. S. Trowbridge, and K. P. Reitz
// Follows the distribution function recommended in the SIGGRAPH 2013 course notes from EPIC Games [1], Equation 3.
float microfacetDistribution(MaterialInfo materialInfo, AngularInfo angularInfo)
{
    float alphaRoughnessSq = materialInfo.alphaRoughness * materialInfo.alphaRoughness;
    float f = (angularInfo.NdotH * alphaRoughnessSq - angularInfo.NdotH) * angularInfo.NdotH + 1.0;
    return alphaRoughnessSq / (M_PI * f * f + 0.000001f);
}

float3 getPointShade(float3 pointToLight, MaterialInfo materialInfo, float3 normal, float3 view)
{
    AngularInfo angularInfo = getAngularInfo(pointToLight, normal, view);

    if (angularInfo.NdotL > 0.0 || angularInfo.NdotV > 0.0)
    {
        // Calculate the shading terms for the microfacet specular shading model
        float3 F = specularReflection(materialInfo, angularInfo);
        float Vis = visibilityOcclusion(materialInfo, angularInfo);
        float D = microfacetDistribution(materialInfo, angularInfo);

        // Calculation of analytical lighting contribution
        float3 diffuseContrib = (1.0 - F) * diffuse(materialInfo);
        float3 specContrib = F * Vis * D;

        // Obtain final intensity as reflectance (BRDF) scaled by the energy of the light (cosine law)
        return angularInfo.NdotL * (diffuseContrib + specContrib);
    }

    return float3(0.0, 0.0, 0.0);
}

// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_lights_punctual/README.md#range-property
float getRangeAttenuation(float range, float distance)
{
    if (range < 0.0)
    {
        // negative range means unlimited
        return 1.0;
    }
    return max(lerp(1, 0, distance / range), 0);
    //return max(min(1.0 - pow(distance / range, 4.0), 1.0), 0.0) / pow(distance, 2.0);
}

// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_lights_punctual/README.md#inner-and-outer-cone-angles
float getSpotAttenuation(float3 pointToLight, float3 spotDirection, float outerConeCos, float innerConeCos)
{
    float actualCos = dot(normalize(spotDirection), normalize(-pointToLight));
    if (actualCos > outerConeCos)
    {
        if (actualCos < innerConeCos)
        {
            return smoothstep(outerConeCos, innerConeCos, actualCos);
        }
        return 1.0;
    }
    return 0.0;
}

float3 applyDirectionalLight(Light light, MaterialInfo materialInfo, float3 normal, float3 view)
{
    float3 pointToLight = light.direction;
    float3 shade = getPointShade(pointToLight, materialInfo, normal, view);
    return light.intensity * light.color * shade;
}

float3 applyPointLight(Light light, MaterialInfo materialInfo, float3 normal, float3 worldPos, float3 view)
{
    float3 pointToLight = light.position - worldPos;
    float distance = length(pointToLight);
    float attenuation = getRangeAttenuation(light.range, distance);
    float3 shade = getPointShade(pointToLight, materialInfo, normal, view);
    return attenuation * light.intensity * light.color * shade;
}

float3 applySpotLight(Light light, MaterialInfo materialInfo, float3 normal, float3 worldPos, float3 view)
{
    float3 pointToLight = light.position - worldPos;
    float distance = length(pointToLight);
    float rangeAttenuation = getRangeAttenuation(light.range, distance);
    float spotAttenuation = getSpotAttenuation(pointToLight, -light.direction, light.outerConeCos, light.innerConeCos);
    float3 shade = getPointShade(pointToLight, materialInfo, normal, view);
    return rangeAttenuation * spotAttenuation * light.intensity * light.color * shade;
}


float3 doPbrLighting(VS_OUTPUT_SCENE Input, in PerFrame perFrame, in float3 diffuseColor, in float3 specularColor, in float perceptualRoughness)
{
    #ifdef MATERIAL_UNLIT
        return getBaseColor(Input);
    #endif

    // Roughness is authored as perceptual roughness; as is convention,
    // convert to material roughness by squaring the perceptual roughness [2].
    float alphaRoughness = perceptualRoughness * perceptualRoughness;
    
    float3 specularEnvironmentR0 = specularColor.rgb;
    // Anything less than 2% is physically impossible and is instead considered to be shadowing. Compare to "Real-Time-Rendering" 4th editon on page 325.
    float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);
    float3 specularEnvironmentR90 = float3(1.0, 1.0, 1.0) * clamp(reflectance * 50.0, 0.0, 1.0);

    MaterialInfo materialInfo =
    {
        perceptualRoughness,
        specularEnvironmentR0,
        alphaRoughness,
        diffuseColor,
        specularEnvironmentR90,
        specularColor
    };

    // LIGHTING

    float3 color = float3(0.0, 0.0, 0.0);
    float3 normal = getPixelNormal(Input);
    float3 worldPos = Input.WorldPos;
    float3 view = normalize(perFrame.u_CameraPos.xyz - worldPos);

#if (DEF_doubleSided == 1)
    if (dot(normal, view) < 0)
    {
        normal = -normal;
    }
#endif

#ifdef USE_PUNCTUAL
    for (int i = 0; i < perFrame.u_lightCount; ++i)
    {
        Light light = myPerFrame.u_lights[i];
        float shadowFactor = CalcShadows(Input.WorldPos.xyz, int2(Input.svPosition.xy), light);
        if (light.type == LightType_Directional)
        {
            color += applyDirectionalLight(light, materialInfo, normal, view) * shadowFactor;
        }
        else if (light.type == LightType_Point)
        {
            color += applyPointLight(light, materialInfo, normal, worldPos, view) * shadowFactor;
        }
        else if (light.type == LightType_Spot)
        {
            color += applySpotLight(light, materialInfo, normal, worldPos, view) * shadowFactor;
        }
    }
#endif

    // Calculate lighting contribution from image based lighting source (IBL)
#ifdef USE_IBL
    color += getIBLContribution(materialInfo, normal, view) * perFrame.u_iblFactor * GetSSAO(Input.svPosition.xy * perFrame.u_invScreenResolution);
#endif

    float ao = 1.0;
    // Apply optional PBR terms for additional (optional) shading
#ifdef ID_occlusionTexture
    ao = occlusionTexture.Sample(samOcclusion, getOcclusionUV(Input)).r;
    color = color * ao; //mix(color, color * ao, perFrame.u_OcclusionStrength);
#endif

    float3 emissive = float3(0, 0, 0);
#ifdef ID_emissiveTexture
    emissive = (emissiveTexture.SampleBias(samEmissive, getEmissiveUV(Input), myPerFrame.u_LodBias)).rgb * u_pbrParams.myPerObject_u_EmissiveFactor.rgb * perFrame.u_EmissiveFactor;
#else        
    emissive = u_pbrParams.myPerObject_u_EmissiveFactor.rgb * perFrame.u_EmissiveFactor;
#endif
    color += emissive;

#ifndef DEBUG_OUTPUT // no debug

    // regular shading
    float3 outColor = color;

#else // debug output

#ifdef DEBUG_METALLIC
    outColor.rgb = float3(metallic);
#endif

#ifdef DEBUG_ROUGHNESS
    outColor.rgb = float3(perceptualRoughness);
#endif

#ifdef DEBUG_NORMAL
#ifdef ID_normalTexture
    outColor.rgb = texture(u_NormalSampler, getNormalUV(Input)).rgb;
#else
    outColor.rgb = float3(0.5, 0.5, 1.0);
#endif
#endif

#ifdef DEBUG_BASECOLOR
    outColor.rgb = (baseColor.rgb);
#endif

#ifdef DEBUG_OCCLUSION
    outColor.rgb = float3(ao);
#endif

#ifdef DEBUG_EMISSIVE
    outColor.rgb = (emissive);
#endif

#ifdef DEBUG_F0
    outColor.rgb = float3(f0);
#endif

#ifdef DEBUG_ALPHA
    outColor.rgb = float3(alpha);
#endif

#endif // !DEBUG_OUTPUT

    return outColor;    
}
