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

#ifdef ID_shadowMap
layout(set = 1, binding = ID_shadowMap) uniform sampler2DShadow u_shadowMap[MAX_SHADOW_INSTANCES];
#endif

// shadowmap filtering
float FilterShadow(int shadowIndex, vec3 uv)
{
    float shadow = 0.0;
#ifdef ID_shadowMap
    ivec2 texDim = textureSize(u_shadowMap[shadowIndex], 0);
    float scale = 1.0;
    float dx = scale * 1.0 / float(texDim.x);
    float dy = scale * 1.0 / float(texDim.y);


    int kernelLevel = 2;
    int kernelWidth = 2 * kernelLevel + 1;
    for (int i = -kernelLevel; i <= kernelLevel; i++)
    {
        for (int j = -kernelLevel; j <= kernelLevel; j++)
        {
            shadow += texture(u_shadowMap[shadowIndex], uv + vec3(dx*i, dy*j, 0)).r;
        }
    }

    shadow /= (kernelWidth*kernelWidth);
#endif
    return shadow;
}

//
// Project world space point onto shadowmap
//
float DoSpotShadow(vec3 vPosition, Light light)
{
#ifdef ID_shadowMap
    if (light.shadowMapIndex < 0)
        return 1.0f;

    if (light.type != LightType_Spot && light.type != LightType_Directional)
        return 1.0; // no other light types cast shadows for now

    vec4 shadowTexCoord = light.mLightViewProj * vec4(vPosition, 1.0);
    shadowTexCoord.xyz = shadowTexCoord.xyz / shadowTexCoord.w;

    // Re-scale to 0-1
    shadowTexCoord.x = (1.0 + shadowTexCoord.x) * 0.5;
    shadowTexCoord.y = (1.0 - shadowTexCoord.y) * 0.5;

    if (light.type == LightType_Spot)
    {
        if ((shadowTexCoord.y < 0) || (shadowTexCoord.y > 1)) return 0;
        if ((shadowTexCoord.x < 0) || (shadowTexCoord.x > 1)) return 0;
        if (shadowTexCoord.z < 0.0f) return 0.0f;
        if (shadowTexCoord.z > 1.0f) return 1.0f;
    }
    else if (light.type == LightType_Directional)
    {
        // This is the sun, so outside of the volume we do have light
        if ((shadowTexCoord.y < 0) || (shadowTexCoord.y > 1)) return 1.0f;
        if ((shadowTexCoord.x < 0) || (shadowTexCoord.x > 1)) return 1.0f;
        if (shadowTexCoord.z < 0.0f) return 1.0f;
        if (shadowTexCoord.z > 1.0f) return 1.0f;
    }

    // offsets of the center of the shadow map atlas
    /*float offsetsX[4] = { 0.0, 1.0, 0.0, 1.0 };
    float offsetsY[4] = { 0.0, 0.0, 1.0, 1.0 };
    shadowTexCoord.x += offsetsX[light.shadowMapIndex] * .5;
    shadowTexCoord.y += offsetsY[light.shadowMapIndex] * .5;*/

    shadowTexCoord.z -= light.depthBias;

    return FilterShadow(light.shadowMapIndex, shadowTexCoord.xyz);
#else
    return 1.0f;
#endif
}
