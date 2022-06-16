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

#define DisplayMode_SDR 0
#define DisplayMode_FS2_Gamma22 1
#define DisplayMode_FS2_SCRGB 2
#define DisplayMode_HDR10_PQ 3
#define DisplayMode_HDR10_SCRGB 4

vec3 ApplyGamma(vec3 color)
{
    float invGamma = 1.0 / 2.2;
    color.xyz = pow(color.rgb, vec3(invGamma, invGamma, invGamma));
    return color;
}

vec3 ApplyPQ(vec3 color)
{
    // Apply ST2084 curve
    float m1 = 2610.0 / 4096.0 / 4;
    float m2 = 2523.0 / 4096.0 * 128;
    float c1 = 3424.0 / 4096.0;
    float c2 = 2413.0 / 4096.0 * 32;
    float c3 = 2392.0 / 4096.0 * 32;
    vec3 cp = pow(abs(color.xyz), vec3(m1));
    color.xyz = pow((c1 + c2 * cp) / (1 + c3 * cp), vec3(m2));
    return color;
}

vec3 ApplyscRGBScale(vec3 color, float minLuminancePerNits, float maxLuminancePerNits)
{
    color.xyz = (color.xyz * (maxLuminancePerNits - minLuminancePerNits)) + vec3(minLuminancePerNits, minLuminancePerNits, minLuminancePerNits);
    return color;
}