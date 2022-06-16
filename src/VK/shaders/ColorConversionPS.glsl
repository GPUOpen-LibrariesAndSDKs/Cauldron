#version 450

// AMD Cauldron code
// 
// Copyright(c) 2019 Advanced Micro Devices, Inc.All rights reserved.
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

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

//--------------------------------------------------------------------------------------
// Constant Buffer
//--------------------------------------------------------------------------------------
layout (set=0, binding = 0, std140) uniform perFrame
{
    mat4          u_contentToMonitorRecMatrix;
    uint          u_displayMode;
    float         u_displayMinLuminancePerNits; // display min luminanace in units of 80 nits
    float         u_displayMaxLuminancePerNits; // display max luminanace in units of 80 nits
    uint          u_isLPMToneMapperSelected;
} myPerFrame;

//--------------------------------------------------------------------------------------
// I/O Structures
//--------------------------------------------------------------------------------------
layout (location = 0) in vec2 inTexCoord;
layout (location = 0) out vec4 outColor;

//--------------------------------------------------------------------------------------
// Texture definitions
//--------------------------------------------------------------------------------------
layout(set=0, binding=1) uniform sampler2D sSampler;

#include "transferFunction.h"

//--------------------------------------------------------------------------------------
// Main function
//--------------------------------------------------------------------------------------
void main() 
{
    vec4 color = texture(sSampler, inTexCoord.st);

    if (myPerFrame.u_isLPMToneMapperSelected == 1)
    {
        switch (myPerFrame.u_displayMode)
        {
            case DisplayMode_FS2_Gamma22:
                // Apply gamma
                color.xyz = ApplyGamma(color.xyz);
                break;

            case DisplayMode_HDR10_PQ:
                // Apply ST2084 curve
                color.xyz = ApplyPQ(color.xyz);
                break;
        }

        outColor = color;
        return;
    }

    switch (myPerFrame.u_displayMode)
    {
        case DisplayMode_SDR:
            // this shader should never get called for SDR
            break;

        case DisplayMode_FS2_Gamma22:
            color.xyz = (myPerFrame.u_contentToMonitorRecMatrix * color).xyz;

			// Apply gamma
            outColor.xyz =  ApplyGamma(color.xyz);  
            break;

        case DisplayMode_FS2_SCRGB:
            // Scale to maxdisplayLuminanace / 80
            // In this case luminanace value queried from
            outColor.xyz = ApplyscRGBScale(color.xyz, myPerFrame.u_displayMinLuminancePerNits, myPerFrame.u_displayMaxLuminancePerNits);
            break;
        
        case DisplayMode_HDR10_PQ:
            // Convert to rec2020 colour space
            color.xyz = (myPerFrame.u_contentToMonitorRecMatrix * color).xyz;

            // myPerFrame.u_displayMaxLuminancePerNits = max nits set / 80
            // ST2084 convention when they define PQ formula is 1 = 10,000 nits.
            // MS/driver convention is 1 = 80 nits, so when we apply the formula, we need to scale by 80/10000 to get us on the same page as ST2084 spec
            // So, scaling should be color *= (nits_when_color_is_1 / 10,000).
            // If color is normalized so that 1.0 = max nits set in vk_hdrmetadata
            // Lets look at whole caluculation with example: max nits = 1000, color = 1
            // 1 * ((1000 / 80) * (80 / 10000)) = 1 / 10 = 0.1
            //
            color.xyz *= (myPerFrame.u_displayMaxLuminancePerNits * (80.0f / 10000.0f));

            // Apply ST2084 curve
            outColor.xyz = ApplyPQ(color.xyz);
            break;

        case DisplayMode_HDR10_SCRGB:
            outColor.xyz = ApplyscRGBScale(color.xyz, myPerFrame.u_displayMinLuminancePerNits, myPerFrame.u_displayMaxLuminancePerNits);
            break;
    }

	outColor.a = color.a;
}
