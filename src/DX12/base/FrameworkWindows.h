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
#include "misc/Misc.h"
#include "FreesyncHDR.h"
#include "Device.h"
#include "SwapChain.h"

namespace CAULDRON_DX12
{
	class FrameworkWindows
	{
	public:
		FrameworkWindows(LPCSTR name);
		virtual ~FrameworkWindows() {}

		// Client (Sample) application interface
		virtual void OnParseCommandLine(LPSTR lpCmdLine, uint32_t* pWidth, uint32_t* pHeight) = 0;
		virtual void OnCreate() = 0;
		virtual void OnDestroy() = 0;
		virtual void OnRender() = 0;
		virtual bool OnEvent(MSG msg) = 0;
		virtual void OnResize(bool resizeRender) = 0;
		virtual void OnUpdateDisplay() = 0;

		// Device & swapchain management
		void DeviceInit(HWND WindowsHandle);
		void DeviceShutdown();
		void BeginFrame();
		void EndFrame();

		// Fullscreen management & window events are handled by Cauldron instead of the client applications.
		void ToggleFullScreen();
		void HandleFullScreen();
		void OnActivate(bool windowActive);
		void OnWindowMove();
		void UpdateDisplay(bool disableLocalDimming);
		void OnResize(uint32_t Width, uint32_t Height);
		
		// Getters
		inline LPCSTR             GetName() const { return m_Name; }
		inline uint32_t           GetWidth() const { return m_Width; }
		inline uint32_t           GetHeight() const { return m_Height; }
		inline DisplayMode        GetCurrentDisplayMode() const { return m_currentDisplayModeNamesIndex; }
		inline size_t             GetNumDisplayModes() const { return m_displayModesAvailable.size(); }
		inline const char* const* GetDisplayModeNames() const { return &m_displayModesNamesAvailable[0]; }
		inline bool               GetLocalDimmingDisableState() const { return m_disableLocalDimming; }


	protected:
		LPCSTR m_Name; // sample application name
		int m_Width ;  // application window dimensions
		int m_Height;  // application window dimensions

		// Simulation management
		double  m_lastFrameTime;
		double  m_deltaTime;

		// Device management
		HWND   m_windowHwnd;
		Device m_device;
		bool   m_stablePowerState;
		bool   m_isCpuValidationLayerEnabled;
		bool   m_isGpuValidationLayerEnabled;
		bool   m_initializeAGS;

		// Swapchain management
		SwapChain         m_swapChain;
		bool              m_VsyncEnabled;
		PresentationMode  m_fullscreenMode;
		PresentationMode  m_previousFullscreenMode;

		// Display management
		HMONITOR                  m_monitor;
		bool                      m_FreesyncHDROptionEnabled;
		DisplayMode               m_previousDisplayModeNamesIndex;
		DisplayMode               m_currentDisplayModeNamesIndex;
		std::vector<DisplayMode>  m_displayModesAvailable;
		std::vector<const char*>  m_displayModesNamesAvailable;
		bool                      m_disableLocalDimming;

		// System info
		struct SystemInfo
		{
			std::string mCPUName = "UNAVAILABLE";
			std::string mGPUName = "UNAVAILABLE";
			std::string mGfxAPI  = "UNAVAILABLE";
		};
		SystemInfo  m_systemInfo;
	};
} // CAULDRON_DX12

int  RunFramework(HINSTANCE hInstance, LPSTR lpCmdLine, int nCmdShow, CAULDRON_DX12::FrameworkWindows* pFramework);
void SetFullscreen(HWND hWnd, bool fullscreen);