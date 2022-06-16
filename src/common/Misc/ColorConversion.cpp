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

#include "stdafx.h"
#include "ColorConversion.h"

#define FFX_CPU
#include "FidelityFX/include/gpu/ffx_core.h"

static float    fs2S;
static float    hdr10S;
static uint32_t ctl[24 * 4];

static void LpmSetupOut(uint32_t i, uint32_t* v)
{
    for (int j = 0; j < 4; ++j)
    {
        ctl[i * 4 + j] = v[j];
    }
}
#include "FidelityFX/include/gpu/ffx_lpm.h"

float ColorSpacePrimaries[4][4][2] = {
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

math::Matrix4 CalculateRGBToXYZMatrix(float xw, float yw, float xr, float yr, float xg, float yg, float xb, float yb, bool scaleLumaFlag)
{
    float Xw = xw / yw;
    float Yw = 1;
    float Zw = (1 - xw - yw) / yw;

    float Xr = xr / yr;
    float Yr = 1;
    float Zr = (1 - xr - yr) / yr;

    float Xg = xg / yg;
    float Yg = 1;
    float Zg = (1 - xg - yg) / yg;

    float Xb = xb / yb;
    float Yb = 1;
    float Zb = (1 - xb - yb) / yb;

    math::Matrix4 XRGB = math::Matrix4(
        math::Vector4(Xr, Xg, Xb, 0),
        math::Vector4(Yr, Yg, Yb, 0),
        math::Vector4(Zr, Zg, Zb, 0),
        math::Vector4(0, 0, 0, 1));
    math::Matrix4 XRGBInverse =  math::inverse(XRGB);

    math::Vector4 referenceWhite = math::Vector4(Xw, Yw, Zw, 0);
    math::Vector4 SRGB = math::transpose(XRGBInverse) * referenceWhite;
    
    math::Matrix4 RegularResult = math::Matrix4(
                                        math::Vector4(SRGB.getX() * Xr, SRGB.getY() * Xg, SRGB.getZ() * Xb, 0),
                                        math::Vector4(SRGB.getX() * Yr, SRGB.getY() * Yg, SRGB.getZ() * Yb, 0),
                                        math::Vector4(SRGB.getX() * Zr, SRGB.getY() * Zg, SRGB.getZ() * Zb, 0),
                                        math::Vector4(0, 0, 0, 1));

    if (!scaleLumaFlag)
        return RegularResult;

    math::Matrix4 Scale = math::Matrix4::scale(math::Vector3(100, 100, 100));
    math::Matrix4 Result = RegularResult * Scale;
    return Result;
}

math::Matrix4 CalculateXYZToRGBMatrix(float xw, float yw, float xr, float yr, float xg, float yg, float xb, float yb, bool scaleLumaFlag)
{
    auto RGBToXYZ = CalculateRGBToXYZMatrix(xw, yw, xr, yr, xg, yg, xb, yb, scaleLumaFlag);
    return math::inverse(RGBToXYZ);
}

void FillDisplaySpecificPrimaries(float xw, float yw, float xr, float yr, float xg, float yg, float xb, float yb)
{
    ColorSpacePrimaries[ColorSpace_Display][ColorPrimaries_WHITE][ColorPrimariesCoordinates_X] = xw;
    ColorSpacePrimaries[ColorSpace_Display][ColorPrimaries_WHITE][ColorPrimariesCoordinates_Y] = yw;

    ColorSpacePrimaries[ColorSpace_Display][ColorPrimaries_RED][ColorPrimariesCoordinates_X] = xr;
    ColorSpacePrimaries[ColorSpace_Display][ColorPrimaries_RED][ColorPrimariesCoordinates_Y] = yr;

    ColorSpacePrimaries[ColorSpace_Display][ColorPrimaries_GREEN][ColorPrimariesCoordinates_X] = xg;
    ColorSpacePrimaries[ColorSpace_Display][ColorPrimaries_GREEN][ColorPrimariesCoordinates_Y] = yg;

    ColorSpacePrimaries[ColorSpace_Display][ColorPrimaries_BLUE][ColorPrimariesCoordinates_X] = xb;
    ColorSpacePrimaries[ColorSpace_Display][ColorPrimaries_BLUE][ColorPrimariesCoordinates_Y] = yb;
}

void SetupGamutMapperMatrices(ColorSpace gamutIn, ColorSpace gamutOut, math::Matrix4* inputToOutputRecMatrix)
{
    math::Matrix4 intputGamut_To_XYZ = CalculateRGBToXYZMatrix(
        ColorSpacePrimaries[gamutIn][ColorPrimaries_WHITE][ColorPrimariesCoordinates_X], ColorSpacePrimaries[gamutIn][ColorPrimaries_WHITE][ColorPrimariesCoordinates_Y],
        ColorSpacePrimaries[gamutIn][ColorPrimaries_RED][ColorPrimariesCoordinates_X], ColorSpacePrimaries[gamutIn][ColorPrimaries_RED][ColorPrimariesCoordinates_Y],
        ColorSpacePrimaries[gamutIn][ColorPrimaries_GREEN][ColorPrimariesCoordinates_X], ColorSpacePrimaries[gamutIn][ColorPrimaries_GREEN][ColorPrimariesCoordinates_Y],
        ColorSpacePrimaries[gamutIn][ColorPrimaries_BLUE][ColorPrimariesCoordinates_X], ColorSpacePrimaries[gamutIn][ColorPrimaries_BLUE][ColorPrimariesCoordinates_Y],
        false);

    math::Matrix4 XYZ_To_OutputGamut = CalculateXYZToRGBMatrix(
        ColorSpacePrimaries[gamutOut][ColorPrimaries_WHITE][ColorPrimariesCoordinates_X], ColorSpacePrimaries[gamutOut][ColorPrimaries_WHITE][ColorPrimariesCoordinates_Y],
        ColorSpacePrimaries[gamutOut][ColorPrimaries_RED][ColorPrimariesCoordinates_X], ColorSpacePrimaries[gamutOut][ColorPrimaries_RED][ColorPrimariesCoordinates_Y],
        ColorSpacePrimaries[gamutOut][ColorPrimaries_GREEN][ColorPrimariesCoordinates_X], ColorSpacePrimaries[gamutOut][ColorPrimaries_GREEN][ColorPrimariesCoordinates_Y],
        ColorSpacePrimaries[gamutOut][ColorPrimaries_BLUE][ColorPrimariesCoordinates_X], ColorSpacePrimaries[gamutOut][ColorPrimaries_BLUE][ColorPrimariesCoordinates_Y],
        false);

    math::Matrix4 intputGamut_To_OutputGamut = intputGamut_To_XYZ * XYZ_To_OutputGamut;
    *inputToOutputRecMatrix = math::transpose(intputGamut_To_OutputGamut);
}

void SetLPMConfig(bool conIn, bool softIn, bool con2In, bool clipIn, bool scaleOnlyIn, LPMConfig& lpmConfig)
{
    lpmConfig.con       = conIn;
    lpmConfig.soft      = softIn;
    lpmConfig.con2      = con2In;
    lpmConfig.clip      = clipIn;
    lpmConfig.scaleOnly = scaleOnlyIn;
}

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
                  float&        scaleCOut)
{
    workingPrimaries.RedPrimary[0] = xyRedWIn[0];
    workingPrimaries.RedPrimary[1] = xyRedWIn[1];

    workingPrimaries.GreenPrimary[0] = xyGreenWIn[0];
    workingPrimaries.GreenPrimary[1] = xyGreenWIn[1];

    workingPrimaries.BluePrimary[0] = xyBlueWIn[0];
    workingPrimaries.BluePrimary[1] = xyBlueWIn[1];

    workingPrimaries.WhitePoint[0] = xyWhiteWIn[0];
    workingPrimaries.WhitePoint[1] = xyWhiteWIn[1];

    outputPrimaries.RedPrimary[0] = xyRedOIn[0];
    outputPrimaries.RedPrimary[1] = xyRedOIn[1];

    outputPrimaries.GreenPrimary[0] = xyGreenOIn[0];
    outputPrimaries.GreenPrimary[1] = xyGreenOIn[1];

    outputPrimaries.BluePrimary[0] = xyBlueOIn[0];
    outputPrimaries.BluePrimary[1] = xyBlueOIn[1];

    outputPrimaries.WhitePoint[0] = xyWhiteOIn[0];
    outputPrimaries.WhitePoint[1] = xyWhiteOIn[1];

    containerPrimaries.RedPrimary[0] = xyRedCIn[0];
    containerPrimaries.RedPrimary[1] = xyRedCIn[1];

    containerPrimaries.GreenPrimary[0] = xyGreenCIn[0];
    containerPrimaries.GreenPrimary[1] = xyGreenCIn[1];

    containerPrimaries.BluePrimary[0] = xyBlueCIn[0];
    containerPrimaries.BluePrimary[1] = xyBlueCIn[1];

    containerPrimaries.WhitePoint[0] = xyWhiteCIn[0];
    containerPrimaries.WhitePoint[1] = xyWhiteCIn[1];

    scaleCOut = scaleCIn;
}

static LPMOutputParams s_lpmOutputParams;

void SetupLPM(LPMInputParams& lpmInputParams)
{
    SetupGamutMapperMatrices(ColorSpace_REC709, lpmInputParams.colorSpace, &s_lpmOutputParams.inputToOutputMatrix);

    FfxFloat32x2 fs2R;
    FfxFloat32x2 fs2G;
    FfxFloat32x2 fs2B;
    FfxFloat32x2 fs2W;
    FfxFloat32x2 displayMinMaxLuminance;
    if (lpmInputParams.displayMode != DISPLAYMODE_SDR)
    {
        // Only used in fs2 modes
        fs2R[0] = lpmInputParams.fs2Primaries.RedPrimary[0];
        fs2R[1] = lpmInputParams.fs2Primaries.RedPrimary[1];
        fs2G[0] = lpmInputParams.fs2Primaries.GreenPrimary[0];
        fs2G[1] = lpmInputParams.fs2Primaries.GreenPrimary[1];
        fs2B[0] = lpmInputParams.fs2Primaries.BluePrimary[0];
        fs2B[1] = lpmInputParams.fs2Primaries.BluePrimary[1];
        fs2W[0] = lpmInputParams.fs2Primaries.WhitePoint[0];
        fs2W[1] = lpmInputParams.fs2Primaries.WhitePoint[1];
        // Only used in fs2 modes

        displayMinMaxLuminance[0] = lpmInputParams.fs2Primaries.MinLuminance;
        displayMinMaxLuminance[1] = lpmInputParams.fs2Primaries.MaxLuminance;
    }

    LPMPrimaries workingPrimaries;
    LPMPrimaries outputPrimaries;
    LPMPrimaries containerPrimaries;
    float        scaleC = 0.0f;

    s_lpmOutputParams.shoulder    = lpmInputParams.shoulder;
    s_lpmOutputParams.displayMode = lpmInputParams.displayMode;

    switch (lpmInputParams.colorSpace)
    {
    case ColorSpace_REC709:
    {
        switch (lpmInputParams.displayMode)
        {
        case DISPLAYMODE_SDR:
            SetLPMConfig(LPM_CONFIG_709_709, s_lpmOutputParams.lpmConfig);
            SetLPMColors(LPM_COLORS_709_709, workingPrimaries, outputPrimaries, containerPrimaries, scaleC);
            break;

        case DISPLAYMODE_FSHDR_Gamma22:
            SetLPMConfig(LPM_CONFIG_FS2RAW_709, s_lpmOutputParams.lpmConfig);
            SetLPMColors(LPM_COLORS_FS2RAW_709, workingPrimaries, outputPrimaries, containerPrimaries, scaleC);
            break;

        case DISPLAYMODE_FSHDR_SCRGB:
            fs2S = LpmFs2ScrgbScalar(displayMinMaxLuminance[0], displayMinMaxLuminance[1]);
            SetLPMConfig(LPM_CONFIG_FS2SCRGB_709, s_lpmOutputParams.lpmConfig);
            SetLPMColors(LPM_COLORS_FS2SCRGB_709, workingPrimaries, outputPrimaries, containerPrimaries, scaleC);
            break;

        case DISPLAYMODE_HDR10_2084:
            hdr10S = LpmHdr10RawScalar(displayMinMaxLuminance[1]);
            SetLPMConfig(LPM_CONFIG_HDR10RAW_709, s_lpmOutputParams.lpmConfig);
            SetLPMColors(LPM_COLORS_HDR10RAW_709, workingPrimaries, outputPrimaries, containerPrimaries, scaleC);
            break;

        case DISPLAYMODE_HDR10_SCRGB:
            hdr10S = LpmHdr10ScrgbScalar(displayMinMaxLuminance[1]);
            SetLPMConfig(LPM_CONFIG_HDR10SCRGB_709, s_lpmOutputParams.lpmConfig);
            SetLPMColors(LPM_COLORS_HDR10SCRGB_709, workingPrimaries, outputPrimaries, containerPrimaries, scaleC);
            break;

        default:
            break;
        }
        break;
    }

    case ColorSpace_P3:
    {
        switch (lpmInputParams.displayMode)
        {
        case DISPLAYMODE_SDR:
            SetLPMConfig(LPM_CONFIG_709_P3, s_lpmOutputParams.lpmConfig);
            SetLPMColors(LPM_COLORS_709_P3, workingPrimaries, outputPrimaries, containerPrimaries, scaleC);
            break;

        case DISPLAYMODE_FSHDR_Gamma22:
            SetLPMConfig(LPM_CONFIG_FS2RAW_P3, s_lpmOutputParams.lpmConfig);
            SetLPMColors(LPM_COLORS_FS2RAW_P3, workingPrimaries, outputPrimaries, containerPrimaries, scaleC);
            break;

        case DISPLAYMODE_FSHDR_SCRGB:
            fs2S = LpmFs2ScrgbScalar(displayMinMaxLuminance[0], displayMinMaxLuminance[1]);
            SetLPMConfig(LPM_CONFIG_FS2SCRGB_P3, s_lpmOutputParams.lpmConfig);
            SetLPMColors(LPM_COLORS_FS2SCRGB_P3, workingPrimaries, outputPrimaries, containerPrimaries, scaleC);
            break;

        case DISPLAYMODE_HDR10_2084:
            hdr10S = LpmHdr10RawScalar(displayMinMaxLuminance[1]);
            SetLPMConfig(LPM_CONFIG_HDR10RAW_P3, s_lpmOutputParams.lpmConfig);
            SetLPMColors(LPM_COLORS_HDR10RAW_P3, workingPrimaries, outputPrimaries, containerPrimaries, scaleC);
            break;

        case DISPLAYMODE_HDR10_SCRGB:
            hdr10S = LpmHdr10ScrgbScalar(displayMinMaxLuminance[1]);
            SetLPMConfig(LPM_CONFIG_HDR10SCRGB_P3, s_lpmOutputParams.lpmConfig);
            SetLPMColors(LPM_COLORS_HDR10SCRGB_P3, workingPrimaries, outputPrimaries, containerPrimaries, scaleC);
            break;

        default:
            break;
        }
        break;
    }

    case ColorSpace_REC2020:
    {
        switch (lpmInputParams.displayMode)
        {
        case DISPLAYMODE_SDR:
            SetLPMConfig(LPM_CONFIG_709_2020, s_lpmOutputParams.lpmConfig);
            SetLPMColors(LPM_COLORS_709_2020, workingPrimaries, outputPrimaries, containerPrimaries, scaleC);
            break;

        case DISPLAYMODE_FSHDR_Gamma22:
            SetLPMConfig(LPM_CONFIG_FS2RAW_2020, s_lpmOutputParams.lpmConfig);
            SetLPMColors(LPM_COLORS_FS2RAW_2020, workingPrimaries, outputPrimaries, containerPrimaries, scaleC);
            break;

        case DISPLAYMODE_FSHDR_SCRGB:
            fs2S = LpmFs2ScrgbScalar(displayMinMaxLuminance[0], displayMinMaxLuminance[1]);
            SetLPMConfig(LPM_CONFIG_FS2SCRGB_2020, s_lpmOutputParams.lpmConfig);
            SetLPMColors(LPM_COLORS_FS2SCRGB_2020, workingPrimaries, outputPrimaries, containerPrimaries, scaleC);
            break;

        case DISPLAYMODE_HDR10_2084:
            hdr10S = LpmHdr10RawScalar(displayMinMaxLuminance[1]);
            SetLPMConfig(LPM_CONFIG_HDR10RAW_2020, s_lpmOutputParams.lpmConfig);
            SetLPMColors(LPM_COLORS_HDR10RAW_2020, workingPrimaries, outputPrimaries, containerPrimaries, scaleC);
            break;

        case DISPLAYMODE_HDR10_SCRGB:
            hdr10S = LpmHdr10ScrgbScalar(displayMinMaxLuminance[1]);
            SetLPMConfig(LPM_CONFIG_HDR10SCRGB_2020, s_lpmOutputParams.lpmConfig);
            SetLPMColors(LPM_COLORS_HDR10SCRGB_2020, workingPrimaries, outputPrimaries, containerPrimaries, scaleC);
            break;

        default:
            break;
        }
        break;
    }

    default:
        break;
    }

    // saturation passed by value as it gets written into in LPMSetup
    float saturation[3] = {lpmInputParams.saturationR, lpmInputParams.saturationG, lpmInputParams.saturationB};

    LpmSetup(lpmInputParams.shoulder,
             s_lpmOutputParams.lpmConfig.con,
             s_lpmOutputParams.lpmConfig.soft,
             s_lpmOutputParams.lpmConfig.con2,
             s_lpmOutputParams.lpmConfig.clip,
             s_lpmOutputParams.lpmConfig.scaleOnly,
             workingPrimaries.RedPrimary,
             workingPrimaries.GreenPrimary,
             workingPrimaries.BluePrimary,
             workingPrimaries.WhitePoint,
             outputPrimaries.RedPrimary,
             outputPrimaries.GreenPrimary,
             outputPrimaries.BluePrimary,
             outputPrimaries.WhitePoint,
             containerPrimaries.RedPrimary,
             containerPrimaries.GreenPrimary,
             containerPrimaries.BluePrimary,
             containerPrimaries.WhitePoint,
             scaleC,
             lpmInputParams.softGap,
             lpmInputParams.hdrMax,
             lpmInputParams.lpmExposure,
             lpmInputParams.contrast,
             lpmInputParams.shoulderContrast,
             saturation,
             lpmInputParams.crosstalk);

    memcpy(s_lpmOutputParams.ctl, ctl, sizeof(ctl));
}

const LPMOutputParams& GetLPMParameters()
{
    return s_lpmOutputParams;
}