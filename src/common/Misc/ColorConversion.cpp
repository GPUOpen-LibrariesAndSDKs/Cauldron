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

#include "stdafx.h"

XMMATRIX AmdCalculateRGBToXYZMatrix(float xw, float yw, float xr, float yr, float xg, float yg, float xb, float yb, bool scaleLumaFlag)
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

    XMMATRIX XRGB = XMMatrixSet(Xr, Xg, Xb, 0, Yr, Yg, Yb, 0, Zr, Zg, Zb, 0, 0, 0, 0, 1);
    XMMATRIX XRGBInverse = XMMatrixInverse(nullptr, XRGB);

    XMVECTOR referenceWhite = XMVectorSet(Xw, Yw, Zw, 0);
    XMVECTOR SRGB = XMVector3Transform(referenceWhite, XMMatrixTranspose(XRGBInverse));

    XMMATRIX RegularResult = XMMatrixSet(XMVectorGetX(SRGB) * Xr, XMVectorGetY(SRGB) * Xg, XMVectorGetZ(SRGB) * Xb, 0,
        XMVectorGetX(SRGB) * Yr, XMVectorGetY(SRGB) * Yg, XMVectorGetZ(SRGB) * Yb, 0,
        XMVectorGetX(SRGB) * Zr, XMVectorGetY(SRGB) * Zg, XMVectorGetZ(SRGB) * Zb, 0,
        0, 0, 0, 1);
    if (!scaleLumaFlag)
        return RegularResult;

    XMMATRIX Scale = XMMatrixScaling(100, 100, 100);
    XMMATRIX Result = Scale * RegularResult;
    return Result;
}

XMMATRIX AmdCalculateXYZToRGBMatrix(float xw, float yw, float xr, float yr, float xg, float yg, float xb, float yb, bool scaleLumaFlag)
{
    auto RGBToXYZ = AmdCalculateRGBToXYZMatrix(xw, yw, xr, yr, xg, yg, xb, yb, scaleLumaFlag);
    return XMMatrixInverse(nullptr, RGBToXYZ);
}

void SetupGamutMapperMatrices(float xw, float yw, float xr, float yr, float xg, float yg, float xb, float yb, XMMATRIX* contentToMonitorRecMatrix)
{
    XMMATRIX Rec709_To_XYZ = AmdCalculateRGBToXYZMatrix(0.3127f, 0.3290f, 0.64f, 0.33f, 0.30f, 0.60f, 0.15f, 0.06f, false);
    XMMATRIX XYZ_To_MonitorRec = AmdCalculateXYZToRGBMatrix(xw, yw, xr, yr, xg, yg, xb, yb, false);
    XMMATRIX rec709_To_Display = XYZ_To_MonitorRec * Rec709_To_XYZ;
    *contentToMonitorRecMatrix = XMMatrixTranspose(rec709_To_Display);
}