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

#include "stdafx.h"
#include "FreesyncHDR.h"

#include <string.h>
#include <dxgi1_6.h>

namespace CAULDRON_DX12
{
    int                         s_displayIndexAGS;
    int                         s_displayDeviceAGS;
    int                         s_displayIndexDXGI;
    AGSContext                 *s_pAGSContext;
    AGSGPUInfo                 *s_pGPUInfo;
    static HWND                 s_hWnd = NULL;
    std::vector<IDXGIOutput6*>  s_pOutputs;
    DXGI_OUTPUT_DESC1           s_desc1;
    bool                        s_windowsHdrtoggle = false;
    DisplayMode                 s_previousDisplayMode = DISPLAYMODE_SDR;

    bool fsHdrInit(AGSContext *pAGSContext, AGSGPUInfo *pGPUInfo, HWND hWnd)
    {
        s_displayIndexAGS = -1;
        s_displayDeviceAGS = -1;
        s_displayIndexDXGI = -1;
        s_pAGSContext = pAGSContext;
        s_pGPUInfo = pGPUInfo;
        s_hWnd = hWnd;

        // Collect all monitor outputs to adapter selected
        UINT i = 0;
        IDXGIOutput *pOutput;
        IDXGIFactory* pFactory;
        ThrowIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&pFactory)));
        IDXGIAdapter* pa(0);
        unsigned int a = 0;
        HRESULT res(S_OK);
        while ((res = pFactory->EnumAdapters(a, &pa)) == S_OK)
        {
            a++;
            while (pa->EnumOutputs(i, &pOutput) != DXGI_ERROR_NOT_FOUND)
            {
                IDXGIOutput6* output6;
                ThrowIfFailed(pOutput->QueryInterface(__uuidof(IDXGIOutput6), (void**)&output6));
                s_pOutputs.push_back(output6);
                pOutput->Release();
                ++i;
            }
            pa->Release();
        }
        assert(res == DXGI_ERROR_NOT_FOUND);
        pFactory->Release();

        return s_pAGSContext != NULL;
    }

    void fsHdrDestroy()
    {
        for (int i = 0; i < s_pOutputs.size(); ++i)
        {
            s_pOutputs[i]->Release();
        }
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

    bool FindBestDisplayIndex(RECT *windowRect, RECT *monitorRect, float *bestIntersectArea)
    {
        // Get the retangle bounds of the app window
        int ax1 = windowRect->left;
        int ay1 = windowRect->top;
        int ax2 = windowRect->right;
        int ay2 = windowRect->bottom;

        int bx1 = monitorRect->left;
        int by1 = monitorRect->top;
        int bx2 = monitorRect->right;
        int by2 = monitorRect->bottom;

        // Compute the intersection
        int intersectArea = ComputeIntersectionArea(ax1, ay1, ax2, ay2, bx1, by1, bx2, by2);
        if (intersectArea > *bestIntersectArea)
        {
            *bestIntersectArea = static_cast<float>(intersectArea);
            return true;
        }

        return false;
    }

    //--------------------------------------------------------------------------------------
    //
    // fsHdrEnumerateDisplayModes, enumerates availabe modes
    //
    //--------------------------------------------------------------------------------------
    bool fsHdrEnumerateDisplayModes(std::vector<DisplayMode> *pModes)
    {
        pModes->clear();
        pModes->push_back(DISPLAYMODE_SDR);

        float bestIntersectArea = -1;
        bool hdrModesAdded = false;
        if (s_pAGSContext != NULL)
        {
            assert(s_pGPUInfo->numDevices >= 0);
            if (s_pGPUInfo->numDevices == 0)
                return false;

            int numDevices = s_pGPUInfo->numDevices;
            AGSDeviceInfo *devices = s_pGPUInfo->devices;

            // Find display, app window is rendering to in ags display list
            for( int dv = 0; dv < s_pGPUInfo->numDevices; dv++ )
                for (int i = 0; i < devices[dv].numDisplays; i++)
                {
                    RECT windowRect;
                    GetWindowRect(s_hWnd, &windowRect);
                    AGSRect AGSmonitorRect = devices[dv].displays[i].currentResolution;
                    RECT monitorRect = { AGSmonitorRect.offsetX, AGSmonitorRect.offsetY, AGSmonitorRect.offsetX + AGSmonitorRect.width, AGSmonitorRect.offsetY + AGSmonitorRect.height };

                    if (FindBestDisplayIndex(&windowRect, &monitorRect, &bestIntersectArea))
                    {
                        s_displayIndexAGS = i;
                        s_displayDeviceAGS = dv;
                    }
                }



            // Check for FS HDR support
            if (devices[s_displayDeviceAGS].displays[s_displayIndexAGS].freesyncHDR)
            {
                pModes->push_back(DISPLAYMODE_FSHDR_Gamma22);
                pModes->push_back(DISPLAYMODE_FSHDR_SCRGB);
            }

            // Check for HDR10 support
            if (devices[s_displayDeviceAGS].displays[s_displayIndexAGS].HDR10)
            {
                hdrModesAdded = true;
                pModes->push_back(DISPLAYMODE_HDR10_2084);
                pModes->push_back(DISPLAYMODE_HDR10_SCRGB);
            }
        }

        // Find display, app window is rendering to in dxgi display list
        bestIntersectArea = -1;
        for (UINT i = 0; i < (UINT)s_pOutputs.size(); i++)
        {
            RECT windowRect;
            GetWindowRect(s_hWnd, &windowRect);
            // Get the rectangle bounds of current output
            DXGI_OUTPUT_DESC desc;
            ThrowIfFailed(s_pOutputs[i]->GetDesc(&desc));
            RECT monitorRect = desc.DesktopCoordinates;

            if (FindBestDisplayIndex(&windowRect, &monitorRect, &bestIntersectArea))
            {
                s_displayIndexDXGI = i;
            }
        }

        DXGI_OUTPUT_DESC1 desc1;
        ThrowIfFailed(s_pOutputs[s_displayIndexDXGI]->GetDesc1(&desc1));

        if (desc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020) // Only works when windows settings hdr toggle is on
        {
            s_windowsHdrtoggle = true;

            if (!hdrModesAdded)
            {
                pModes->push_back(DISPLAYMODE_HDR10_2084);
                pModes->push_back(DISPLAYMODE_HDR10_SCRGB);
            }
        }

        return true;
    }

    //--------------------------------------------------------------------------------------
    //
    // fsHdrGetDisplayModeString
    //
    //--------------------------------------------------------------------------------------
    const char *fsHdrGetDisplayModeString(DisplayMode displayMode)
    {
        // note that these string must match the order of the DisplayModes enum
        const char *DisplayModesStrings[]
        {
            "SDR",
            "FSHDR_Gamma22",
            "FSHDR_SCRGB",
            "HDR10_2084",
            "HDR10_SCRGB"
        };

        return DisplayModesStrings[displayMode];
    }

    //--------------------------------------------------------------------------------------
    //
    // fsHdrGetFormat
    //
    //--------------------------------------------------------------------------------------
    DXGI_FORMAT fsHdrGetFormat(DisplayMode displayMode)
    {
        switch (displayMode)
        {
        case DISPLAYMODE_SDR:
            return DXGI_FORMAT_R8G8B8A8_UNORM;

        case DISPLAYMODE_FSHDR_Gamma22:
            return DXGI_FORMAT_R10G10B10A2_UNORM;

        case DISPLAYMODE_FSHDR_SCRGB:
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
    // fsHdrSetDisplayMode
    //
    //--------------------------------------------------------------------------------------
    bool fsHdrSetDisplayMode(DisplayMode displayMode, bool disableLocalDimming, IDXGISwapChain4 *pSwapchain)
    {
        AGSDisplaySettings agsDisplaySettings = {};
        DXGI_HDR_METADATA_HDR10 HDR10MetaData = {};

        // Get Display Metadata
        ThrowIfFailed(s_pOutputs[s_displayIndexDXGI]->GetDesc1(&s_desc1));

        switch (displayMode)
        {
            case DISPLAYMODE_SDR:
            {
                // rec 709 primaries
                s_desc1.RedPrimary[0] = 0.64f;
                s_desc1.RedPrimary[1] = 0.33f;
                s_desc1.GreenPrimary[0] = 0.30f;
                s_desc1.GreenPrimary[1] = 0.60f;
                s_desc1.BluePrimary[0] = 0.15f;
                s_desc1.BluePrimary[1] = 0.06f;
                s_desc1.WhitePoint[0] = 0.3127f;
                s_desc1.WhitePoint[1] = 0.3290f;
                s_desc1.MinLuminance = 0.0f;
                s_desc1.MaxLuminance = 100.0f;
                s_desc1.ColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
                break;
            }

            case DISPLAYMODE_HDR10_2084:
            {
                // rec 2020 primaries
                s_desc1.RedPrimary[0] = 0.708f;
                s_desc1.RedPrimary[1] = 0.292f;
                s_desc1.GreenPrimary[0] = 0.170f;
                s_desc1.GreenPrimary[1] = 0.797f;
                s_desc1.BluePrimary[0] = 0.131f;
                s_desc1.BluePrimary[1] = 0.046f;
                s_desc1.WhitePoint[0] = 0.3127f;
                s_desc1.WhitePoint[1] = 0.3290f;
                s_desc1.MinLuminance = 0.0f;
                s_desc1.MaxLuminance = 1000.0f; // This will cause tonemapping to happen on display end as long as it's greater than display's actual queried max luminance. The look will change and it will be display dependent!
                HDR10MetaData.MaxContentLightLevel = 1000;
                HDR10MetaData.MaxFrameAverageLightLevel = 400; // max and average content ligt level data will be used to do tonemapping on display
                s_desc1.ColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
                break;
            }

            case DISPLAYMODE_HDR10_SCRGB:
            {
                // rec 709 primaries
                s_desc1.RedPrimary[0] = 0.64f;
                s_desc1.RedPrimary[1] = 0.33f;
                s_desc1.GreenPrimary[0] = 0.30f;
                s_desc1.GreenPrimary[1] = 0.60f;
                s_desc1.BluePrimary[0] = 0.15f;
                s_desc1.BluePrimary[1] = 0.06f;
                s_desc1.WhitePoint[0] = 0.3127f;
                s_desc1.WhitePoint[1] = 0.3290f;
                s_desc1.MinLuminance = 0.0f;
                s_desc1.MaxLuminance = 1000.0f; // This will cause tonemapping to happen on display end as long as it's greater than display's actual queried max luminance. The look will change and it will be display dependent!
                HDR10MetaData.MaxContentLightLevel = 1000;
                HDR10MetaData.MaxFrameAverageLightLevel = 400; // max and average content ligt level data will be used to do tonemapping on display
                s_desc1.ColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
                break;
            }

            case DISPLAYMODE_FSHDR_Gamma22:
            {
                agsDisplaySettings.mode = AGSDisplaySettings::Mode::Mode_FreesyncHDR_Gamma22;
                break;
            }

            case DISPLAYMODE_FSHDR_SCRGB:
            {
                agsDisplaySettings.mode = AGSDisplaySettings::Mode::Mode_FreesyncHDR_scRGB;
                break;
            }
        }

        switch (displayMode)
        {
            case DISPLAYMODE_SDR:
            case DISPLAYMODE_HDR10_2084:
            case DISPLAYMODE_HDR10_SCRGB:
            {
                // If we are going from any of the ags modes to dxgi modes, we need to flip back to SDR to avoid issues with AGS + DXGI usage.
                // Driver does take care of this but it is done for safety
                if (s_previousDisplayMode == DISPLAYMODE_FSHDR_Gamma22 || s_previousDisplayMode == DISPLAYMODE_FSHDR_SCRGB)
                {
                    agsDisplaySettings.mode = AGSDisplaySettings::Mode::Mode_SDR;
                    AGSReturnCode rc = agsSetDisplayMode(s_pAGSContext, s_displayDeviceAGS, s_displayIndexAGS, &agsDisplaySettings);
                    assert(rc == AGS_SUCCESS);
                    s_previousDisplayMode = displayMode;
                }

                // Set HDR meta data
                // The values are normalized to 50,000.
                HDR10MetaData.RedPrimary[0] = static_cast<UINT16>(s_desc1.RedPrimary[0] * 50000.0f);
                HDR10MetaData.RedPrimary[1] = static_cast<UINT16>(s_desc1.RedPrimary[1] * 50000.0f);
                HDR10MetaData.GreenPrimary[0] = static_cast<UINT16>(s_desc1.GreenPrimary[0] * 50000.0f);
                HDR10MetaData.GreenPrimary[1] = static_cast<UINT16>(s_desc1.GreenPrimary[1] * 50000.0f);
                HDR10MetaData.BluePrimary[0] = static_cast<UINT16>(s_desc1.BluePrimary[0] * 50000.0f);
                HDR10MetaData.BluePrimary[1] = static_cast<UINT16>(s_desc1.BluePrimary[1] * 50000.0f);
                HDR10MetaData.WhitePoint[0] = static_cast<UINT16>(s_desc1.WhitePoint[0] * 50000.0f);
                HDR10MetaData.WhitePoint[1] = static_cast<UINT16>(s_desc1.WhitePoint[1] * 50000.0f);
                HDR10MetaData.MaxMasteringLuminance = static_cast<UINT>(s_desc1.MaxLuminance);
                HDR10MetaData.MinMasteringLuminance = static_cast<UINT>(s_desc1.MinLuminance * 10000.0f);
                ThrowIfFailed(pSwapchain->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_HDR10, sizeof(DXGI_HDR_METADATA_HDR10), &HDR10MetaData));
                ThrowIfFailed(pSwapchain->SetColorSpace1(s_desc1.ColorSpace));
                return true;
            }

            case DISPLAYMODE_FSHDR_Gamma22:
            case DISPLAYMODE_FSHDR_SCRGB:
            {
                agsDisplaySettings.disableLocalDimming = disableLocalDimming;
                if (agsDisplaySettings.disableLocalDimming)
                    s_desc1.MaxLuminance = s_desc1.MaxFullFrameLuminance;

                agsDisplaySettings.chromaticityRedX = s_desc1.RedPrimary[0];
                agsDisplaySettings.chromaticityRedY = s_desc1.RedPrimary[1];
                agsDisplaySettings.chromaticityGreenX = s_desc1.GreenPrimary[0];
                agsDisplaySettings.chromaticityGreenY = s_desc1.GreenPrimary[1];
                agsDisplaySettings.chromaticityBlueX = s_desc1.BluePrimary[0];
                agsDisplaySettings.chromaticityBlueY = s_desc1.BluePrimary[1];
                agsDisplaySettings.chromaticityWhitePointX = s_desc1.WhitePoint[0];
                agsDisplaySettings.chromaticityWhitePointY = s_desc1.WhitePoint[1];
                agsDisplaySettings.minLuminance = s_desc1.MinLuminance;
                agsDisplaySettings.maxLuminance = s_desc1.MaxLuminance;
                AGSReturnCode rc = agsSetDisplayMode(s_pAGSContext, s_displayDeviceAGS, s_displayIndexAGS, &agsDisplaySettings);
                assert (rc == AGS_SUCCESS);
                s_previousDisplayMode = displayMode;
                return true;
            }
        }

        return false;
    }

    const DXGI_OUTPUT_DESC1 *GetDisplayInfo()
    {
        return &s_desc1;
    }

    const bool CheckIfWindowModeHdrOn()
    {
        return s_windowsHdrtoggle;
    }
}
