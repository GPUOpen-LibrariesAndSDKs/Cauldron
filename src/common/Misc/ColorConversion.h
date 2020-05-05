// AMD AMDUtils code
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
#pragma once

enum ColorSpace
{
    ColorSpace_REC709,
    ColorSpace_P3,
    ColorSpace_REC2020,
    ColorSpace_Display
};

enum ColorPrimaries
{
    ColorPrimaries_WHITE,
    ColorPrimaries_RED,
    ColorPrimaries_GREEN,
    ColorPrimaries_BLUE
};

enum ColorPrimariesCoordinates
{
    ColorPrimariesCoordinates_X,
    ColorPrimariesCoordinates_Y
};

static float ColorSpacePrimaries[4][4][2] = {
    //Rec709
    {
        0.3127f,0.3290f, // White point
        0.64f,0.33f, // Red point
        0.30f,0.60f, // Green point
        0.15f,0.06f, // Blue point
    },
    //P3
    {
        0.3127f,0.3290f, // White point
        0.680f,0.320f, // Red point
        0.265f,0.690f, // Green point
        0.150f,0.060f, // Blue point
    },
    //Rec2020
    {
        0.3127f,0.3290f, // White point
        0.708f,0.292f, // Red point
        0.170f,0.797f, // Green point
        0.131f,0.046f, // Blue point
    },
    //Display Specific zeroed out now Please fill them in once you query them and want to use them for matrix calculations
    {
        0.0f,0.0f, // White point
        0.0f,0.0f, // Red point
        0.0f,0.0f, // Green point
        0.0f,0.0f // Blue point
    }
};

void FillDisplaySpecificPrimaries(float xw, float yw, float xr, float yr, float xg, float yg, float xb, float yb);

XMMATRIX AmdCalculateRGBToXYZMatrix(float xw, float yw, float xr, float yr, float xg, float yg, float xb, float yb, bool scaleLumaFlag);
XMMATRIX AmdCalculateXYZToRGBMatrix(float xw, float yw, float xr, float yr, float xg, float yg, float xb, float yb, bool scaleLumaFlag);

void SetupGamutMapperMatrices(ColorSpace gamutIn, ColorSpace gamutOut, XMMATRIX* inputToOutputRecMatrix);
