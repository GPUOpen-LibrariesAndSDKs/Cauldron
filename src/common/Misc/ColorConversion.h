// AMD Cauldron code
// 
// Copyright(c) 2020 Advanced Micro Devices, Inc.All rights reserved.
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

#include "../../libs/vectormath/vectormath.hpp"
#include <stdint.h>
#include <minmax.h>

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

extern float ColorSpacePrimaries[4][4][2];

void FillDisplaySpecificPrimaries(float xw, float yw, float xr, float yr, float xg, float yg, float xb, float yb);

math::Matrix4 CalculateRGBToXYZMatrix(float xw, float yw, float xr, float yr, float xg, float yg, float xb, float yb, bool scaleLumaFlag);
math::Matrix4 CalculateXYZToRGBMatrix(float xw, float yw, float xr, float yr, float xg, float yg, float xb, float yb, bool scaleLumaFlag);

void SetupGamutMapperMatrices(ColorSpace gamutIn, ColorSpace gamutOut, math::Matrix4* inputToOutputRecMatrix);

enum PresentationMode
{
    PRESENTATIONMODE_WINDOWED,
    PRESENTATIONMODE_BORDERLESS_FULLSCREEN,
    PRESENTATIONMODE_EXCLUSIVE_FULLSCREEN
};

enum DisplayMode
{
    DISPLAYMODE_SDR,
    DISPLAYMODE_FSHDR_Gamma22,
    DISPLAYMODE_FSHDR_SCRGB,
    DISPLAYMODE_HDR10_2084,
    DISPLAYMODE_HDR10_SCRGB
};

struct LPMPrimaries
{
    float RedPrimary[2];
    float GreenPrimary[2];
    float BluePrimary[2];
    float WhitePoint[2];
    float MinLuminance;
    float MaxLuminance;
};

struct LPMConfig
{
    bool con;
    bool soft;
    bool con2;
    bool clip;
    bool scaleOnly;
};

struct LPMInputParams
{
    DisplayMode  displayMode;
    ColorSpace   colorSpace;
    bool         shoulder;
    float        softGap;
    float        hdrMax;
    float        lpmExposure;
    float        contrast;
    float        shoulderContrast;
    float        saturationR;
    float        saturationG;
    float        saturationB;
    float        crosstalk[3];
    LPMPrimaries fs2Primaries;
};

struct LPMOutputParams
{
    bool          shoulder;
    LPMConfig     lpmConfig;
    DisplayMode   displayMode;
    math::Matrix4 inputToOutputMatrix;
    uint32_t      ctl[24 * 4];
};

struct LPMConsts
{
    uint32_t      shoulder;            // Use optional extra shoulderContrast tuning (set to false if shoulderContrast is 1.0).
    uint32_t      con;                 // Use first RGB conversion matrix, if 'soft' then 'con' must be true also.
    uint32_t      soft;                // Use soft gamut mapping.
    uint32_t      con2;                // Use last RGB conversion matrix.
    uint32_t      clip;                // Use clipping in last conversion matrix.
    uint32_t      scaleOnly;           // Scale only for last conversion matrix (used for 709 HDR to scRGB).
    uint32_t      displayMode;         // Display mode to target
    uint32_t      pad;
    math::Matrix4 inputToOutputMatrix; // Conversion matrix to set into right colourspace pre LPM
    uint32_t      ctl[24 * 4];         // Store data from LPMSetup call
};

void SetLPMConfig(bool conIn, bool softIn, bool con2In, bool clipIn, bool scaleOnlyIn, LPMConfig& lpmConfig);
void SetLPMColors(float         xyRedWIn[2],
                  float         xyGreenWIn[2],
                  float         xyBlueWIn[2],
                  float         xyWhiteWIn[2],
                  float         xyRedOIn[2],
                  float         xyGreenOIn[2],
                  float         xyBlueOIn[2],
                  float         xyWhiteOIn[2],
                  float         xyRedCIn[2],
                  float         xyGreenCIn[2],
                  float         xyBlueCIn[2],
                  float         xyWhiteCIn[2],
                  float         scaleCIn,
                  LPMPrimaries& workingPrimaries,
                  LPMPrimaries& outputPrimaries,
                  LPMPrimaries& containerPrimaries,
                  float&        scaleCOut);

void SetupLPM(LPMInputParams& lpmInputParams);

const LPMOutputParams& GetLPMParameters();