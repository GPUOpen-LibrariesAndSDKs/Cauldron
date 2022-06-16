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

#include "Base/SwapChain.h"
#include "Base/Device.h"
#include "Base/Texture.h"
#include "Base/ShaderCompilerHelper.h"
#include "Base/ResourceViewHeaps.h"

namespace CAULDRON_DX12
{
    typedef enum GBufferFlagBits
    {
        GBUFFER_NONE = 0,
        GBUFFER_DEPTH = 1,
        GBUFFER_FORWARD = 2,
        GBUFFER_MOTION_VECTORS = 4,
        GBUFFER_NORMAL_BUFFER = 8,
        GBUFFER_DIFFUSE = 16,
        GBUFFER_SPECULAR_ROUGHNESS = 32,
        GBUFFER_INVERTED_DEPTH = 64,
        GBUFFER_UPSCALEREACTIVE = 128,
        GBUFFER_UPSCALE_TRANSPARENCY_AND_COMPOSITION = 256,
    } GBufferFlagBits;

    typedef uint32_t GBufferFlags;

    class GBuffer;

    class GBufferRenderPass
    {
    public:
        void OnCreate(GBuffer *pGBuffer, GBufferFlags flags);
        void OnDestroy();
        void OnCreateWindowSizeDependentResources(uint32_t Width, uint32_t Height);
        void OnDestroyWindowSizeDependentResources();
        void BeginPass(ID3D12GraphicsCommandList* pCommandList, bool bClear);
        void EndPass();
        void GetCompilerDefinesAndGBufferFormats(DefineList &defines, std::vector<DXGI_FORMAT> &outFormats, DXGI_FORMAT &depthFormat);
        int32_t GetSampleCount();
        int GetRtIndex(GBufferFlagBits pass);    // return RT index for a given GBuffer target
    private:
        GBufferFlags                    m_GBufferFlags;
        GBuffer                        *m_pGBuffer;

        D3D12_CPU_DESCRIPTOR_HANDLE m_DepthBuffer;
        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>  m_RenderTargetViewList;
    };

    class GBuffer
    {
    public:
        void OnCreate(Device* pDevice, ResourceViewHeaps *pHeaps, const std::map<GBufferFlags, DXGI_FORMAT> &formats, uint32_t sampleCount, GBufferFlags flag = GBUFFER_NONE);
        void OnDestroy();

        void OnCreateWindowSizeDependentResources(SwapChain *pSwapChain, uint32_t Width, uint32_t Height);
        void OnDestroyWindowSizeDependentResources();

        uint32_t GetSampleCount() {  return m_sampleCount; }

        // depth buffer
        Texture                         m_DepthBuffer;
        DSV                             m_DepthBufferDSV;
        CBV_SRV_UAV                     m_DepthBufferSRV;

        // diffuse
        Texture                         m_Diffuse;
        RTV                             m_DiffuseRTV;
        CBV_SRV_UAV                     m_DiffuseSRV;

        // specular
        Texture                         m_SpecularRoughness;
        RTV                             m_SpecularRoughnessRTV;
        CBV_SRV_UAV                     m_SpecularRoughnessSRV;

        // upscale reactive data
        Texture                         m_UpscaleReactive;
        RTV                             m_UpscaleReactiveRTV;
        CBV_SRV_UAV                     m_UpscaleReactiveSRV;

        // upscale transparency and composition data
        Texture                         m_UpscaleTransparencyAndComposition;
        RTV                             m_UpscaleTransparencyAndCompositionRTV;
        CBV_SRV_UAV                     m_UpscaleTransparencyAndCompositionSRV;

        // motion vectors
        Texture                         m_MotionVectors;
        RTV                             m_MotionVectorsRTV;
        CBV_SRV_UAV                     m_MotionVectorsSRV;

        // normal buffer
        Texture                         m_NormalBuffer;
        RTV                             m_NormalBufferRTV;
        CBV_SRV_UAV                     m_NormalBufferSRV;

        // HDR
        Texture                         m_HDR;
        CBV_SRV_UAV                     m_HDRSRV;
        CBV_SRV_UAV                     m_HDRUAV;
        RTV                             m_HDRRTV;

    private:
        Device                         *m_pDevice;

        GBufferFlags                    m_GBufferFlags;
        int                             m_sampleCount;

        std::map<GBufferFlags, DXGI_FORMAT> m_formats;
    };

}