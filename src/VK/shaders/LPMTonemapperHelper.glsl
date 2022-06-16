// AMD Cauldron code
// 
// Copyright(c) 2022 Advanced Micro Devices, Inc.All rights reserved.
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


#define FFX_GPU 1
#define FFX_GLSL 1
#include "ffx_core.h"

FfxUInt32x4 LpmFilterCtl(FfxUInt32 i)
{
    return myPerScene.u_ctl[i];
}

#define LPM_NO_SETUP 1
#include "ffx_lpm.h"

vec3 LPMTonemapper(vec3 color, bool shoulder, bool con, bool soft, bool con2, bool clip, bool scaleOnly, mat4 inputToOutputMatrix)
{
    color = (inputToOutputMatrix * vec4(color, 0.0f)).xyz;

    // This code is there to make sure no negative values make it down to tonemappers.
    // Make sure to never hit this case and convert content to correct colourspace
    color.r = max(0, color.r);
    color.g = max(0, color.g);
    color.b = max(0, color.b);
    //

    LpmFilter(color.r, color.g, color.b, shoulder, con, soft, con2, clip, scaleOnly);
    return color;
}