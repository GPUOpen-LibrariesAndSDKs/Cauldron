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

#include "Base/Device.h"
#include "Base/SwapChain.h"
#include "Base/Texture.h"
#include "Base/ExtDebugUtils.h"
#include "Base/ShaderCompilerHelper.h"
#include "Base/ResourceViewHeaps.h"

namespace CAULDRON_VK
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
        GBUFFER_FRAGMENT_SHADING_RATE = 512,
    } GBufferFlagBits;

    typedef uint32_t GBufferFlags;

    class GBuffer;

    class GBufferRenderPass
    {
    public:
        void OnCreate(GBuffer *pGBuffer, GBufferFlags flags, bool bClear, const std::string &name);
        void OnCreate(GBuffer *pGBuffer, GBufferFlags flags, bool bClear, VkImageLayout previousDepth, VkImageLayout currentDepth, const std::string &name);
        void OnDestroy();
        void OnCreateWindowSizeDependentResources(uint32_t Width, uint32_t Height);
        void OnDestroyWindowSizeDependentResources();
        void BeginPass(VkCommandBuffer commandList, VkRect2D renderArea);
        void EndPass(VkCommandBuffer commandList);
        void GetCompilerDefines(DefineList &defines);
        VkRenderPass GetRenderPass() { return m_renderPass; }
        VkFramebuffer GetFramebuffer() { return m_frameBuffer; }
        VkSampleCountFlagBits  GetSampleCount();
    private:
        Device                         *m_pDevice;
        GBufferFlags                    m_flags;
        GBuffer                        *m_pGBuffer;
        VkRenderPass                    m_renderPass;
        VkFramebuffer                   m_frameBuffer;
        std::vector<VkClearValue>       m_clearValues;
    };

    class GBuffer
    {
    public:

        void OnCreate(Device* pDevice, ResourceViewHeaps *pHeaps, const std::map<GBufferFlags, VkFormat> &m_formats, int sampleCount);
        void OnDestroy();

        void OnCreateWindowSizeDependentResources(SwapChain* pSwapChain, uint32_t Width, uint32_t Height, VkImageUsageFlagBits depthUsageFlags = {});
        void OnDestroyWindowSizeDependentResources();

        void GetAttachmentList(GBufferFlags flags, std::vector<VkImageView> *pAttachments, std::vector<VkClearValue> *pClearValues);
        VkRenderPass CreateRenderPass(GBufferFlags flags, bool bClear);
        VkRenderPass CreateRenderPass(GBufferFlags flags, bool bClear, VkImageLayout previousDepth, VkImageLayout currentDepth);

        void GetCompilerDefines(DefineList &defines);
        VkSampleCountFlagBits  GetSampleCount() { return m_sampleCount; }
        Device *GetDevice() { return m_pDevice; }

        // depth buffer
        Texture                         m_DepthBuffer;
        VkImageView                     m_DepthBufferDSV;
        VkImageView                     m_DepthBufferSRV;

        // diffuse
        Texture                         m_Diffuse;
        VkImageView                     m_DiffuseSRV;

        // specular
        Texture                         m_SpecularRoughness;
        VkImageView                     m_SpecularRoughnessSRV;

        // upscale reactive data
        Texture                         m_UpscaleReactive;
        VkImageView                     m_UpscaleReactiveSRV;

        // upscale transparency and composition data
        Texture                         m_UpscaleTransparencyAndComposition;
        VkImageView                     m_UpscaleTransparencyAndCompositionSRV;

        // motion vectors
        Texture                         m_MotionVectors;
        VkImageView                     m_MotionVectorsSRV;

        // normal buffer
        Texture                         m_NormalBuffer;
        VkImageView                     m_NormalBufferSRV;

        // HDR
        Texture                         m_HDR;
        VkImageView                     m_HDRSRV;

        // VRS
        Texture                         m_VRS;
        VkImageView                     m_VRSSRV;

    private:
        Device                         *m_pDevice;

        VkSampleCountFlagBits           m_sampleCount;

        GBufferFlags                    m_GBufferFlags;
        std::vector<VkClearValue>       m_clearValues;

        std::map<GBufferFlags, VkFormat> m_formats;
    };

}