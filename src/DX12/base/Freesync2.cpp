// AMD AMDUtils code
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

#include "Freesync2.h"
#include "Misc\Error.h"

#include <string.h>
#include <assert.h>

namespace CAULDRON_DX12
{
    int             s_displayIndex;
    AGSDisplayInfo  s_AGSDisplayInfo;
    AGSContext     *s_pAGSContext;
    AGSGPUInfo     *s_pGPUInfo;
    static HWND     s_hWnd = NULL;

    bool fs2Init(AGSContext *pAGSContext, AGSGPUInfo *pGPUInfo, HWND hWnd)
    {
        s_displayIndex = -1;
        s_AGSDisplayInfo = {};
        s_pAGSContext = pAGSContext;
        s_pGPUInfo = pGPUInfo;
        s_hWnd = hWnd;

        if (s_pAGSContext == NULL)
            return false;
        return true;
    }

    //--------------------------------------------------------------------------------------
    // To detect HDR support, we will need to check the color space in the primary DXGI output associated with the app at
    // this point in time (using window/display intersection).

    // Compute the overlay area of two rectangles, A and B.
    // (ax1, ay1) = left-top coordinates of A; (ax2, ay2) = right-bottom coordinates of A
    // (bx1, by1) = left-top coordinates of B; (bx2, by2) = right-bottom coordinates of B
    // based on https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12HDR/src/D3D12HDR.cpp
    //--------------------------------------------------------------------------------------
    inline int ComputeIntersectionArea(int ax1, int ay1, int ax2, int ay2, int bx1, int by1, int bx2, int by2)
    {
        return max(0, min(ax2, bx2) - max(ax1, bx1)) * max(0, min(ay2, by2) - max(ay1, by1));
    }

    //--------------------------------------------------------------------------------------
    //
    // fs2EnumerateDisplayModes, enumerates availabe modes
    //
    //--------------------------------------------------------------------------------------
    bool fs2EnumerateDisplayModes(std::vector<DisplayModes> *pModes)
    {
        pModes->clear();
        pModes->push_back(DISPLAYMODE_SDR);

        if (s_pAGSContext == NULL)
            return true;

        assert(s_pGPUInfo->numDevices >= 0);
        if (s_pGPUInfo->numDevices == 0)
            return false;

        int numDevices = s_pGPUInfo->numDevices;
        AGSDeviceInfo* devices = s_pGPUInfo->devices;

        // Find display, app window is rendering to
        int numDisplays = devices[0].numDisplays;
        float bestIntersectArea = -1;
        int bestDisplayIndex = -1;
        for (int i = 0; i < numDisplays; i++)
        {
            AGSRect monitorRect = devices[0].displays[i].currentResolution;
            RECT windowRect;
            GetWindowRect(s_hWnd, &windowRect);

            // Get the retangle bounds of the app window
            int ax1 = windowRect.left;
            int ay1 = windowRect.top;
            int ax2 = windowRect.right;
            int ay2 = windowRect.bottom;

            int bx1 = monitorRect.offsetX;
            int by1 = monitorRect.offsetY;
            int bx2 = monitorRect.offsetX + monitorRect.width;
            int by2 = monitorRect.offsetY + monitorRect.height;

            // Compute the intersection
            int intersectArea = ComputeIntersectionArea(ax1, ay1, ax2, ay2, bx1, by1, bx2, by2);
            if (intersectArea > bestIntersectArea)
            {
                bestDisplayIndex = i;
                bestIntersectArea = static_cast<float>(intersectArea);
            }
        }

        s_displayIndex = bestDisplayIndex;

        // First check for FS2 support
        if (devices[0].displays[bestDisplayIndex].displayFlags & AGSDisplayFlags::AGS_DISPLAYFLAG_FREESYNC_2)
        {
            pModes->push_back(DISPLAYMODE_FS2_Gamma22);
            pModes->push_back(DISPLAYMODE_FS2_SCRGB);
            pModes->push_back(DISPLAYMODE_HDR10_2084);
            pModes->push_back(DISPLAYMODE_HDR10_SCRGB);
        }
        else // Check for HDR support
        {
            if (devices[0].displays[bestDisplayIndex].displayFlags & AGSDisplayFlags::AGS_DISPLAYFLAG_HDR10)
            {
                pModes->push_back(DISPLAYMODE_HDR10_2084);
                pModes->push_back(DISPLAYMODE_HDR10_SCRGB);
            }
        }

        return true;
    }

    //--------------------------------------------------------------------------------------
    //
    // fs2GetDisplayModeString
    //
    //--------------------------------------------------------------------------------------
    const char *fs2GetDisplayModeString(DisplayModes displayMode)
    {
        // note that these string must match the order of the DisplayModes enum
        const char *DisplayModesStrings[]
        {
            "SDR",
            "FS2_Gamma22",
            "FS2_SCRGB",
            "HDR10_2084",
            "HDR10_SCRGB"
        };

        return DisplayModesStrings[displayMode];
    }

    //--------------------------------------------------------------------------------------
    //
    // fs2GetFormat
    //
    //--------------------------------------------------------------------------------------
    DXGI_FORMAT fs2GetFormat(DisplayModes displayMode)
    {
        if (s_pAGSContext==NULL)
            displayMode = DISPLAYMODE_SDR;

        switch (displayMode)
        {
        case DISPLAYMODE_SDR:
            return DXGI_FORMAT_R8G8B8A8_UNORM;

        case DISPLAYMODE_FS2_Gamma22:
            return DXGI_FORMAT_R10G10B10A2_UNORM;

        case DISPLAYMODE_FS2_SCRGB:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;

        case DISPLAYMODE_HDR10_2084:
            return DXGI_FORMAT_R10G10B10A2_UNORM;

        case DISPLAYMODE_HDR10_SCRGB:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;

        default:
            return DXGI_FORMAT_UNKNOWN;
        }
    }

    //--------------------------------------------------------------------------------------
    //
    // fs2SetDisplayMode
    //
    //--------------------------------------------------------------------------------------
    bool fs2SetDisplayMode(DisplayModes displayMode, bool disableLocalDimming)
    {
        if (s_pAGSContext == NULL)
            return false;

        s_AGSDisplayInfo = s_pGPUInfo->devices[0].displays[s_displayIndex];

        AGSDisplaySettings agsDisplaySettings = {};

        switch (displayMode)
        {
            case DISPLAYMODE_SDR:
            {
                // rec 709 primaries
                s_AGSDisplayInfo.chromaticityRedX = 0.64;
                s_AGSDisplayInfo.chromaticityRedY = 0.33;
                s_AGSDisplayInfo.chromaticityGreenX = 0.30;
                s_AGSDisplayInfo.chromaticityGreenY = 0.60;
                s_AGSDisplayInfo.chromaticityBlueX = 0.15;
                s_AGSDisplayInfo.chromaticityBlueY = 0.06;
                s_AGSDisplayInfo.chromaticityWhitePointX = 0.3127;
                s_AGSDisplayInfo.chromaticityWhitePointY = 0.3290;
                agsDisplaySettings.mode = AGSDisplaySettings::Mode::Mode_SDR;
                agsDisplaySettings.flags = 0; // Local dimming always enabled for SDR, therefore 'disableLocalDimming' flag should be set to false ie 0.
                break;
            }

            case DISPLAYMODE_FS2_Gamma22:
            {
                agsDisplaySettings.mode = AGSDisplaySettings::Mode::Mode_Freesync2_Gamma22;
                agsDisplaySettings.flags = disableLocalDimming; // Local dimming could be enabled or disabled for FS2 based on preference.
                if (disableLocalDimming)
                    s_AGSDisplayInfo.maxLuminance = s_AGSDisplayInfo.avgLuminance;
                break;
            }

            case DISPLAYMODE_FS2_SCRGB:
            {
                agsDisplaySettings.mode = AGSDisplaySettings::Mode::Mode_Freesync2_scRGB;
                agsDisplaySettings.flags = disableLocalDimming; // Local dimming could be enabled or disabled for FS2 based on preference.
                if (disableLocalDimming)
                    s_AGSDisplayInfo.maxLuminance = s_AGSDisplayInfo.avgLuminance;
                break;
            }

            case DISPLAYMODE_HDR10_2084:
            {
                // rec 2020 primaries
                s_AGSDisplayInfo.chromaticityRedX = 0.708;
                s_AGSDisplayInfo.chromaticityRedY = 0.292;
                s_AGSDisplayInfo.chromaticityGreenX = 0.170;
                s_AGSDisplayInfo.chromaticityGreenY = 0.797;
                s_AGSDisplayInfo.chromaticityBlueX = 0.131;
                s_AGSDisplayInfo.chromaticityBlueY = 0.046;
                s_AGSDisplayInfo.chromaticityWhitePointX = 0.3127;
                s_AGSDisplayInfo.chromaticityWhitePointY = 0.3290;
                s_AGSDisplayInfo.minLuminance = 0.0;
                s_AGSDisplayInfo.maxLuminance = 1000.0; // This will cause tonemapping to happen on display end as long as it's greater than display's actual queried max luminance. The look will change and it will be display dependent!
                agsDisplaySettings.maxContentLightLevel = 1000.0;
                agsDisplaySettings.maxFrameAverageLightLevel = 400.0; // max and average content ligt level data will be used to do tonemapping on display
                agsDisplaySettings.mode = AGSDisplaySettings::Mode::Mode_HDR10_PQ;
                agsDisplaySettings.flags = 0; // Local dimming always enabled for HDR, therefore 'disableLocalDimming' flag should be set to false ie 0.
                break;
            }

            case DISPLAYMODE_HDR10_SCRGB:
            {
                // rec 2020 primaries
                s_AGSDisplayInfo.chromaticityRedX = 0.708;
                s_AGSDisplayInfo.chromaticityRedY = 0.292;
                s_AGSDisplayInfo.chromaticityGreenX = 0.170;
                s_AGSDisplayInfo.chromaticityGreenY = 0.797;
                s_AGSDisplayInfo.chromaticityBlueX = 0.131;
                s_AGSDisplayInfo.chromaticityBlueY = 0.046;
                s_AGSDisplayInfo.chromaticityWhitePointX = 0.3127;
                s_AGSDisplayInfo.chromaticityWhitePointY = 0.3290;
                s_AGSDisplayInfo.minLuminance = 0.0;
                s_AGSDisplayInfo.maxLuminance = 1000.0; // This will cause tonemapping to happen on display end as long as it's greater than display's actual queried max luminance. The look will change and it will be display dependent!
                agsDisplaySettings.maxContentLightLevel = 1000.0;
                agsDisplaySettings.maxFrameAverageLightLevel = 400.0; // max and average content ligt level data will be used to do tonemapping on display
                agsDisplaySettings.mode = AGSDisplaySettings::Mode::Mode_HDR10_scRGB;
                agsDisplaySettings.flags = 0; // Local dimming always enabled for HDR, therefore 'disableLocalDimming' flag should be set to false ie 0.
                break;
            }
        }

        agsDisplaySettings.chromaticityRedX = s_AGSDisplayInfo.chromaticityRedX;
        agsDisplaySettings.chromaticityRedY = s_AGSDisplayInfo.chromaticityRedY;
        agsDisplaySettings.chromaticityGreenX = s_AGSDisplayInfo.chromaticityGreenX;
        agsDisplaySettings.chromaticityGreenY = s_AGSDisplayInfo.chromaticityGreenY;
        agsDisplaySettings.chromaticityBlueX = s_AGSDisplayInfo.chromaticityBlueX;
        agsDisplaySettings.chromaticityBlueY = s_AGSDisplayInfo.chromaticityBlueY;
        agsDisplaySettings.chromaticityWhitePointX = s_AGSDisplayInfo.chromaticityWhitePointX;
        agsDisplaySettings.chromaticityWhitePointY = s_AGSDisplayInfo.chromaticityWhitePointY;
        agsDisplaySettings.minLuminance = s_AGSDisplayInfo.minLuminance;
        agsDisplaySettings.maxLuminance = s_AGSDisplayInfo.maxLuminance;

        AGSReturnCode rc = agsSetDisplayMode(s_pAGSContext, 0, s_displayIndex, &agsDisplaySettings);
        return (rc == AGS_SUCCESS);
    }


    //--------------------------------------------------------------------------------------
    //
    // fs2GetDisplayInfo
    //
    //--------------------------------------------------------------------------------------
    const AGSDisplayInfo* fs2GetDisplayInfo()
    {
        if (s_pAGSContext == NULL)
            return NULL;

        return &s_AGSDisplayInfo;
    };
}
