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
#include "FrameworkWindows.h"
#include "Misc/Misc.h"

#include <array>

using namespace CAULDRON_DX12;

LRESULT CALLBACK WindowProc(HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam);


static const char* const WINDOW_CLASS_NAME = "RadeonCauldron";
static FrameworkWindows* pFrameworkInstance = nullptr;

static bool bIsMinimized = false;
static RECT m_windowRect;
static LONG lBorderedStyle = 0;
static LONG lBorderlessStyle = 0;
static UINT lwindowStyle = 0;

// Default values for validation layers - applications can override these values in their constructors
#if _DEBUG
static constexpr bool ENABLE_CPU_VALIDATION_DEFAULT = true;
static constexpr bool ENABLE_GPU_VALIDATION_DEFAULT = true;
#else // RELEASE
static constexpr bool ENABLE_CPU_VALIDATION_DEFAULT = false;
static constexpr bool ENABLE_GPU_VALIDATION_DEFAULT = false;
#endif

int RunFramework(HINSTANCE hInstance, LPSTR lpCmdLine, int nCmdShow, FrameworkWindows* pFramework)
{
    // Init logging
    int result = Log::InitLogSystem();
    assert(!result);

    // Init window class
    HWND hWnd;
    WNDCLASSEX windowClass;

    ZeroMemory(&windowClass, sizeof(WNDCLASSEX));
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.lpszClassName = WINDOW_CLASS_NAME;
    windowClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(101)); // '#define ICON_IDI 101' is assumed to be included with resource.h
    if (windowClass.hIcon == NULL)
    {
        DWORD dw = GetLastError();
        if (dw == ERROR_RESOURCE_TYPE_NOT_FOUND)
            Trace("Warning: Icon file or .rc file not found, using default Windows app icon.");
        else
            Trace("Warning: error loading icon, using default Windows app icon.");
    }
    RegisterClassEx(&windowClass);

    // If this is null, nothing to do, bail
    assert(pFramework);
    if (!pFramework) return -1;
    pFrameworkInstance = pFramework;
    
    // Get command line and config file parameters for app run
    uint32_t Width = 1920;
    uint32_t Height = 1080;
    pFramework->OnParseCommandLine(lpCmdLine, &Width, &Height);

    // This makes sure that in a multi-monitor setup with different resolutions, get monitor info returns correct dimensions
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    // Window setup based on config params
    lwindowStyle = WS_OVERLAPPEDWINDOW;
    RECT windowRect = { 0, 0, (LONG)Width, (LONG)Height };
    AdjustWindowRect(&windowRect, lwindowStyle, FALSE);    // adjust the size

    // Create the window
    hWnd = CreateWindowEx(NULL,
        WINDOW_CLASS_NAME,    // name of the window class
        pFramework->GetName(),
        lwindowStyle,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        NULL,    // we have no parent window, NULL
        NULL,    // we aren't using menus, NULL
        hInstance,    // application handle
        NULL);    // used with multiple windows, NULL

    // Framework owns device and swapchain, so initialize them
    pFramework->DeviceInit(hWnd);
    
    // Sample create callback
    pFramework->OnCreate();

    // show the window
    ShowWindow(hWnd, nCmdShow);
    lBorderedStyle = GetWindowLong(hWnd, GWL_STYLE);
    lBorderlessStyle = lBorderedStyle & ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);

    // main loop
    MSG msg = { 0 };
    while (msg.message != WM_QUIT)
    {
        // check to see if any messages are waiting in the queue
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg); // translate keystroke messages into the right format            
            DispatchMessage(&msg); // send the message to the WindowProc function
        }
        else if (!bIsMinimized)
            pFramework->OnRender();
    }

    // Destroy app-side framework 
    pFramework->OnDestroy();

    // Shutdown all the device stuff before quitting
    pFramework->DeviceShutdown();

    // Delete the framework created by the sample
    pFrameworkInstance = nullptr;
    delete pFramework;

    // Shutdown logging before quitting the application
    Log::TerminateLogSystem();

    // return this part of the WM_QUIT message to Sample
    return static_cast<char>(msg.wParam);
}

void SetFullscreen(HWND hWnd, bool fullscreen)
{
    if (fullscreen)
    {
        // Save the old window rect so we can restore it when exiting fullscreen mode.
        GetWindowRect(hWnd, &m_windowRect);

        // Make the window borderless so that the client area can fill the screen.
        SetWindowLong(hWnd, GWL_STYLE, lwindowStyle & ~(WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU | WS_THICKFRAME));

        MONITORINFO monitorInfo;
        monitorInfo.cbSize = sizeof(monitorInfo);
        GetMonitorInfo(MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST), &monitorInfo);

        SetWindowPos(
            hWnd,
            HWND_NOTOPMOST,
            monitorInfo.rcMonitor.left,
            monitorInfo.rcMonitor.top,
            monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
            monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
            SWP_FRAMECHANGED | SWP_NOACTIVATE);

        ShowWindow(hWnd, SW_MAXIMIZE);
    }
    else
    {
        // Restore the window's attributes and size.
        SetWindowLong(hWnd, GWL_STYLE, lwindowStyle);

        SetWindowPos(
            hWnd,
            HWND_NOTOPMOST,
            m_windowRect.left,
            m_windowRect.top,
            m_windowRect.right - m_windowRect.left,
            m_windowRect.bottom - m_windowRect.top,
            SWP_FRAMECHANGED | SWP_NOACTIVATE);

        ShowWindow(hWnd, SW_NORMAL);
    }
}

// this is the main message handler for the program
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // sort through and find what code to run for the message given
    switch (message)
    {
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }

        // When close button is clicked on window
        case WM_CLOSE:
        {
            PostQuitMessage(0);
            return 0;
        }

        case WM_KEYDOWN:
        {
            if (wParam == VK_ESCAPE)
            {
                PostQuitMessage(0);
            }

            break;
        }

        case WM_SYSKEYDOWN:
        {
            const bool bAltKeyDown = (lParam & (1 << 29));
            if ((wParam == VK_RETURN) && bAltKeyDown) // For simplicity, alt+enter only toggles in/out windowed and borderless fullscreen
                pFrameworkInstance->ToggleFullScreen();
            break;
        }

        case WM_SIZE:
        {
            if (pFrameworkInstance)
            {
                RECT clientRect = {};
                GetClientRect(hWnd, &clientRect);
                pFrameworkInstance->OnResize(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
                bIsMinimized = (IsIconic(hWnd) == TRUE);
                return 0;
            }
            break;
        }

        // When window goes outof focus, use this event to fall back on SDR.
        // If we don't gracefully fallback to SDR, the renderer will output HDR colours which will look extremely bright and washed out.
        // However if you want to use breakpoints in HDR mode to inspect/debug values, you will have to comment this function call.
        case WM_ACTIVATE:
        {
            if (pFrameworkInstance)
            {
                pFrameworkInstance->OnActivate(wParam != WA_INACTIVE);
            }

            break;
        }

        case WM_MOVE:
        {
            if (pFrameworkInstance)
            {
                pFrameworkInstance->OnWindowMove();

                return 0;
            }
            break;
        }

        // Turn off MessageBeep sound on Alt+Enter
        case WM_MENUCHAR: return MNC_CLOSE << 16;
    }

    if (pFrameworkInstance)
    {
        MSG msg;
        msg.hwnd = hWnd;
        msg.message = message;
        msg.wParam = wParam;
        msg.lParam = lParam;
        pFrameworkInstance->OnEvent(msg);
    }

    // Handle any messages the switch statement didn't
    return DefWindowProc(hWnd, message, wParam, lParam);
}

static std::string GetCPUNameString()
{
	int nIDs = 0;
	int nExIDs = 0;

	char strCPUName[0x40] = { };

	std::array<int, 4> cpuInfo;
	std::vector<std::array<int, 4>> extData;

	__cpuid(cpuInfo.data(), 0);

	// Calling __cpuid with 0x80000000 as the function_id argument
	// gets the number of the highest valid extended ID.
	__cpuid(cpuInfo.data(), 0x80000000);

	nExIDs = cpuInfo[0];
	for (int i = 0x80000000; i <= nExIDs; ++i)
	{
		__cpuidex(cpuInfo.data(), i, 0);
		extData.push_back(cpuInfo);
	}

	// Interpret CPU strCPUName string if reported
	if (nExIDs >= 0x80000004)
	{
		memcpy(strCPUName, extData[2].data(), sizeof(cpuInfo));
		memcpy(strCPUName + 16, extData[3].data(), sizeof(cpuInfo));
		memcpy(strCPUName + 32, extData[4].data(), sizeof(cpuInfo));
	}

	return strlen(strCPUName) != 0 ? strCPUName : "UNAVAILABLE";
}

namespace CAULDRON_DX12
{
    FrameworkWindows::FrameworkWindows(LPCSTR name)
        : m_Name(name)
        , m_Width(0)
        , m_Height(0)

        // Simulation management
        , m_lastFrameTime(MillisecondsNow())
        , m_deltaTime(0.0)

        // Device management
        , m_windowHwnd(NULL)
        , m_device()
        , m_stablePowerState(false)
        , m_isCpuValidationLayerEnabled(ENABLE_CPU_VALIDATION_DEFAULT)
        , m_isGpuValidationLayerEnabled(ENABLE_GPU_VALIDATION_DEFAULT)
        , m_initializeAGS(false)

        // Swapchain management
        , m_swapChain()
        , m_VsyncEnabled(false)
        , m_fullscreenMode(PRESENTATIONMODE_WINDOWED)
        , m_previousFullscreenMode(PRESENTATIONMODE_WINDOWED)

        // Display management
        , m_monitor()
        , m_FreesyncHDROptionEnabled(false)
        , m_previousDisplayModeNamesIndex(DISPLAYMODE_SDR)
        , m_currentDisplayModeNamesIndex (DISPLAYMODE_SDR)
        , m_displayModesAvailable()
        , m_displayModesNamesAvailable()
        , m_disableLocalDimming(false)

        // System info
        , m_systemInfo() // initialized after device
    {
    }
    
    void FrameworkWindows::DeviceInit(HWND WindowsHandle)
    {
        const char* STR_CAULDRON = "Cauldron v1.4";

        // Store the windows handle (other things need it later)
        m_windowHwnd = WindowsHandle;

        // Create Device
        m_device.OnCreate(m_Name, STR_CAULDRON, m_isCpuValidationLayerEnabled, m_isGpuValidationLayerEnabled, m_windowHwnd, m_initializeAGS);

        // set stable power state (only works when Developer Mode is enabled)
        if (m_stablePowerState)
        {
            HRESULT hr = m_device.GetDevice()->SetStablePowerState(TRUE);

            // handle failure / device removed
            if (FAILED(hr))
            {
                HRESULT reason = m_device.GetDevice()->GetDeviceRemovedReason();

                Trace("Warning: ID3D12Device::SetStablePowerState(TRUE) failed: Reason 0x%x (DXGI_ERROR). Recreating device, setting m_stablePowerState = false.", reason);

                // device removed, so recreate
                m_device.OnDestroy();
                m_device.OnCreate(m_Name, STR_CAULDRON, m_isCpuValidationLayerEnabled, m_isGpuValidationLayerEnabled, m_windowHwnd, m_initializeAGS);
                m_stablePowerState = false;
            }
        }

        // Get the monitor
        m_monitor = MonitorFromWindow(m_windowHwnd, MONITOR_DEFAULTTONEAREST);

        // Create Swapchain
        uint32_t dwNumberOfBackBuffers = 2;
        m_swapChain.OnCreate(&m_device, dwNumberOfBackBuffers, m_windowHwnd);

        m_swapChain.EnumerateDisplayModes(&m_displayModesAvailable, &m_displayModesNamesAvailable);

        if (m_previousFullscreenMode != m_fullscreenMode)
        {
            HandleFullScreen();
            m_previousFullscreenMode = m_fullscreenMode;
        }

		// Get system info
		std::string dummyStr;
		m_device.GetDeviceInfo(&m_systemInfo.mGPUName, &dummyStr); // 2nd parameter is unused
		m_systemInfo.mCPUName = GetCPUNameString();
		m_systemInfo.mGfxAPI = "DirectX 12";
    }

    void FrameworkWindows::DeviceShutdown()
    {
        // Fullscreen state should always be false before exiting the app.
        if (m_fullscreenMode == PRESENTATIONMODE_EXCLUSIVE_FULLSCREEN)
            m_swapChain.SetFullScreen(false);

        // Fall back to SDR when app closes
        if (m_displayModesAvailable[m_currentDisplayModeNamesIndex] != DISPLAYMODE_SDR)
            m_swapChain.OnCreateWindowSizeDependentResources(m_Width, m_Height, m_VsyncEnabled, DISPLAYMODE_SDR, false);

        m_swapChain.OnDestroyWindowSizeDependentResources();
        m_swapChain.OnDestroy();

        m_device.OnDestroy();
    }

    void FrameworkWindows::ToggleFullScreen()
    {
        if (m_fullscreenMode == PRESENTATIONMODE_WINDOWED)
        {
            m_fullscreenMode = PRESENTATIONMODE_BORDERLESS_FULLSCREEN;
        }
        else
        {
            m_fullscreenMode = PRESENTATIONMODE_WINDOWED;
        }

        HandleFullScreen();
        m_previousFullscreenMode = m_fullscreenMode;
    }

    void FrameworkWindows::HandleFullScreen()
    {
        // Flush the gpu to make sure we don't change anything still active
        m_device.GPUFlush();

        // -------------------- For HDR only.
        // If FS2 modes, always fallback to SDR
        if (m_fullscreenMode == PRESENTATIONMODE_WINDOWED &&
            (m_displayModesAvailable[m_currentDisplayModeNamesIndex] == DISPLAYMODE_FSHDR_Gamma22 ||
                m_displayModesAvailable[m_currentDisplayModeNamesIndex] == DISPLAYMODE_FSHDR_SCRGB))
        {
            m_currentDisplayModeNamesIndex = DISPLAYMODE_SDR;
        }
        // when hdr10 modes, fall back to SDR unless windowMode hdr is enabled
        else if (m_fullscreenMode == PRESENTATIONMODE_WINDOWED && !CheckIfWindowModeHdrOn() &&
            (m_displayModesAvailable[m_currentDisplayModeNamesIndex] != DISPLAYMODE_SDR))
        {
            m_currentDisplayModeNamesIndex = DISPLAYMODE_SDR;
        }
        // For every other case go back to previous state
        else
        {
            m_currentDisplayModeNamesIndex = m_previousDisplayModeNamesIndex;
        }
        // -------------------- For HDR only.
        bool resizeResources = false;
        switch (m_fullscreenMode)
        {
        case PRESENTATIONMODE_WINDOWED:
        {
            if (m_previousFullscreenMode == PRESENTATIONMODE_EXCLUSIVE_FULLSCREEN)
            {
                m_swapChain.SetFullScreen(false);
                resizeResources = true;
            }

            SetFullscreen(m_windowHwnd, false);

            break;
        }

        case PRESENTATIONMODE_BORDERLESS_FULLSCREEN:
        {
            if (m_previousFullscreenMode == PRESENTATIONMODE_WINDOWED)
            {
                SetFullscreen(m_windowHwnd, true);
            }
            else if (m_previousFullscreenMode == PRESENTATIONMODE_EXCLUSIVE_FULLSCREEN)
            {
                m_swapChain.SetFullScreen(false);
                resizeResources = true;
            }

            break;
        }

        case PRESENTATIONMODE_EXCLUSIVE_FULLSCREEN:
        {
            if (m_previousFullscreenMode == PRESENTATIONMODE_WINDOWED)
            {
                SetFullscreen(m_windowHwnd, true);
            }

            m_swapChain.SetFullScreen(true);
            resizeResources = true;

            break;
        }
        }

        RECT clientRect = {};
        GetClientRect(m_windowHwnd, &clientRect);
        uint32_t nw = clientRect.right - clientRect.left;
        uint32_t nh = clientRect.bottom - clientRect.top;
        resizeResources = (resizeResources && nw == m_Width && nh == m_Height);
        OnResize(nw, nh);
        if (resizeResources)
        {
            UpdateDisplay(m_disableLocalDimming);
            OnResize(true);
        }
    }

    void FrameworkWindows::OnActivate(bool WindowActive)
    {
        // *********************************************************************************
        // Edge case for handling Fullscreen Exclusive (FSE) mode 
        // FSE<->FSB transitions are handled here and at the end of EndFrame().
        if (WindowActive &&
            m_fullscreenMode == PRESENTATIONMODE_BORDERLESS_FULLSCREEN &&
            m_previousFullscreenMode == PRESENTATIONMODE_EXCLUSIVE_FULLSCREEN)
        {
            m_fullscreenMode = PRESENTATIONMODE_EXCLUSIVE_FULLSCREEN;
            m_previousFullscreenMode = PRESENTATIONMODE_BORDERLESS_FULLSCREEN;
            HandleFullScreen();
            m_previousFullscreenMode = m_fullscreenMode;
        }
        // *********************************************************************************

        if (m_displayModesAvailable[m_currentDisplayModeNamesIndex] == DisplayMode::DISPLAYMODE_SDR &&
            m_displayModesAvailable[m_previousDisplayModeNamesIndex] == DisplayMode::DISPLAYMODE_SDR)
            return;

        if (CheckIfWindowModeHdrOn() &&
            (m_displayModesAvailable[m_currentDisplayModeNamesIndex] == DISPLAYMODE_HDR10_2084 ||
                m_displayModesAvailable[m_currentDisplayModeNamesIndex] == DISPLAYMODE_HDR10_SCRGB))
            return;

        // Fall back HDR to SDR when window is fullscreen but not the active window or foreground window
        DisplayMode old = m_currentDisplayModeNamesIndex;
        m_currentDisplayModeNamesIndex = WindowActive && (m_fullscreenMode != PRESENTATIONMODE_WINDOWED) ? m_previousDisplayModeNamesIndex : DisplayMode::DISPLAYMODE_SDR;
        if (old != m_currentDisplayModeNamesIndex)
        {
            UpdateDisplay(m_disableLocalDimming);
            OnResize(true);
        }
    }

    void FrameworkWindows::OnWindowMove()
    {
        HMONITOR currentMonitor = MonitorFromWindow(m_windowHwnd, MONITOR_DEFAULTTONEAREST);
        if (m_monitor != currentMonitor)
        {
            m_swapChain.EnumerateDisplayModes(&m_displayModesAvailable, &m_displayModesNamesAvailable);
            m_monitor = currentMonitor;
            m_previousDisplayModeNamesIndex = m_currentDisplayModeNamesIndex = DISPLAYMODE_SDR;

            UpdateDisplay(m_disableLocalDimming);
            OnResize(true);
        }
    }

    void FrameworkWindows::OnResize(uint32_t width, uint32_t height)
    {
        bool fr = (m_Width != width || m_Height != height);
        m_Width = width;
        m_Height = height;
        if (fr)
        {
            UpdateDisplay(m_disableLocalDimming);
            OnResize(true);
        }
    }

    void FrameworkWindows::UpdateDisplay(bool disableLocalDimming)
    {
        // Nothing was changed in UI
        if (m_displayModesAvailable[m_currentDisplayModeNamesIndex] < 0)
        {
            m_currentDisplayModeNamesIndex = m_previousDisplayModeNamesIndex;
            return;
        }
            
        // Flush GPU
        m_device.GPUFlush();

        m_swapChain.OnDestroyWindowSizeDependentResources();

        m_disableLocalDimming = disableLocalDimming;

        m_swapChain.OnCreateWindowSizeDependentResources(m_Width, m_Height, m_VsyncEnabled, m_displayModesAvailable[m_currentDisplayModeNamesIndex], m_disableLocalDimming);

        // Call sample defined UpdateDisplay()
        OnUpdateDisplay();
    }

    void FrameworkWindows::BeginFrame()
    {
        // Get timings
        double timeNow = MillisecondsNow();
        m_deltaTime = (float)(timeNow - m_lastFrameTime);
        m_lastFrameTime = timeNow;
    }

    // EndFrame will handle Present and other end of frame logic needed
    void FrameworkWindows::EndFrame()
    {
        m_swapChain.Present();

        // If we are doing GPU Validation, flush every frame
        if (m_isGpuValidationLayerEnabled)
            m_device.GPUFlush();

        // *********************************************************************************
        // Edge case for handling Fullscreen Exclusive (FSE) mode
        // Usually OnActivate() detects application changing focus and that's where this transition is handled.
        // However, SwapChain::GetFullScreen() returns true when we handle the event in the OnActivate() block,
        // which is not expected. Hence we handle FSE -> FSB transition here at the end of the frame.
        if (m_fullscreenMode == PRESENTATIONMODE_EXCLUSIVE_FULLSCREEN)
        {
            bool isFullScreen = m_swapChain.GetFullScreen();
            if (!isFullScreen)
            {
                m_fullscreenMode = PRESENTATIONMODE_BORDERLESS_FULLSCREEN;
                HandleFullScreen();
            }
        }
        // *********************************************************************************
    }

} // CAULDRON_DX12