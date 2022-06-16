#version 400

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

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable



layout (location = 0) in vec2 inTexCoord;

layout (location = 0) out vec4 outColor;

layout(set=0, binding=0) uniform sampler2D smpColor;
layout(set=0, binding=1) uniform const_buffer
{
    uint     uImageWidth;
    uint     uImageHeight;
    ivec2    iMousePos; // driven by ImGuiIO.MousePos.xy
    
    vec4     fBorderColorRGB;
    
    float    fMagnificationAmount;
    float    fMagnifierScreenRadius;  // [0-1]
    int      iMagnifierOffsetX;
    int      iMagnifierOffsetY;
    //int      iEnable;
};

void main() 
{
    //if(iEnable == 0)
        //    return;

    outColor = texture(smpColor, inTexCoord.st);
    
    vec2 uv = inTexCoord.st;
    
    const vec2 ImageSize = vec2(uImageWidth, uImageHeight);
    const float AspectRatio    = ImageSize.x / ImageSize.y;
    const float AspectRatioInv = ImageSize.y / ImageSize.x;
    const vec2 MagnifierAreaCenter = vec2(iMousePos[0], iMousePos[1]);
    const vec2 MagnifierOnScreenLocation = vec2(iMousePos[0] + iMagnifierOffsetX, iMousePos[1] + iMagnifierOffsetY);
    
    const vec2 CurrPixel = inTexCoord.st * ImageSize;
    const vec2 uvMagnifiedArea = vec2(MagnifierAreaCenter / ImageSize);
    const vec2 uvMagnifierOnScreen = vec2(MagnifierOnScreenLocation / ImageSize);

    const float MagnifiedAreaR = fMagnifierScreenRadius / fMagnificationAmount;
    const float MagnifierOnScreenR = fMagnifierScreenRadius;


    vec2 uvRArea = uv - uvMagnifiedArea;
    if (ImageSize.x > ImageSize.y)
        uvRArea.x *= AspectRatio;
    else
        uvRArea.y *= AspectRatioInv;
        
    vec2 uvROnScreen = uv - uvMagnifierOnScreen;
    if (ImageSize.x > ImageSize.y)
        uvROnScreen.x *= AspectRatio;
    else
        uvROnScreen.y *= AspectRatioInv;

    const float BORDER_THICKNESS_OFFSET = 0.001f;

    const bool bAreWeInMagnifiedAreaRegion = length(uvRArea) < MagnifiedAreaR;
    const bool bAreWeInMagnifiedAreaBorderRegion = length(uvRArea) < (MagnifiedAreaR + BORDER_THICKNESS_OFFSET);

    
    const bool bAreWeInMagnifierOnScreenRegion       = length(uvROnScreen) < MagnifierOnScreenR;
    const bool bAreWeInMagnifierOnScreenBorderRegion = length(uvROnScreen) < (MagnifierOnScreenR + BORDER_THICKNESS_OFFSET);
        
    if (bAreWeInMagnifiedAreaRegion) 
    {
        return;
    }
    if (bAreWeInMagnifierOnScreenBorderRegion || bAreWeInMagnifiedAreaBorderRegion)
    {
        outColor.rgb = fBorderColorRGB.rgb;        
    }
    if (bAreWeInMagnifierOnScreenRegion)
    {
        vec2 sampleUVOffsetFromCenter = uv - uvMagnifierOnScreen;
        sampleUVOffsetFromCenter /= fMagnificationAmount;        
        const vec2 magnifierUV = uvMagnifiedArea + sampleUVOffsetFromCenter;
        outColor = texture(smpColor, magnifierUV);        
        return;
    }


    // draw lines tangent to both circles
    vec2 currentToCenter = uv - uvMagnifiedArea;
    vec2 centerToCenterLine = uvMagnifierOnScreen - uvMagnifiedArea;

    
    // coorect for aspect ration
    if (ImageSize.x > ImageSize.y) 
    {
        currentToCenter.x *= AspectRatio;
        centerToCenterLine.x *= AspectRatio;
    }
    else 
    {
        currentToCenter.y *= AspectRatio;
        centerToCenterLine.y *= AspectRatio;
    }


    float centerToCenterLength = length(centerToCenterLine);
    centerToCenterLine /= centerToCenterLength;

    float distanceOnCenterline = dot(currentToCenter, centerToCenterLine);
    vec2 projOnCenterline = distanceOnCenterline * centerToCenterLine;
    vec2 perpToCenterline = currentToCenter - projOnCenterline;

    
    // cosine of angle between center line and tangent radius line
    float alphaFactor = (MagnifierOnScreenR - MagnifiedAreaR) / centerToCenterLength;

    float centerToCenterPlusAlphaLength = centerToCenterLength - alphaFactor * (MagnifierOnScreenR - MagnifiedAreaR);
    
    distanceOnCenterline += alphaFactor * MagnifiedAreaR;
    if (distanceOnCenterline > 0 && distanceOnCenterline < centerToCenterPlusAlphaLength) 
    {    
        
        float t = distanceOnCenterline / centerToCenterPlusAlphaLength;

        // the tangent line is also tangent to a circle of a radius that is a linear interpolation of the two circles and the alpha factor
        float radiusAtPoint = ((1 - t) * MagnifiedAreaR + t * MagnifierOnScreenR) * sqrt(1 - alphaFactor * alphaFactor);
        
        if (abs(length(perpToCenterline) - radiusAtPoint) < BORDER_THICKNESS_OFFSET) 
        {
            outColor.rgb = fBorderColorRGB.rgb;
        }
    }

}
