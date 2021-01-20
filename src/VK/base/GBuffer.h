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
        GBUFFER_WORLD_COORD = 64
    } GBufferFlagBits;

    typedef uint32_t GBufferFlags;

    class GBuffer;

    class GBufferRenderPass
    {
    public:
        void OnCreate(GBuffer *pGBuffer, GBufferFlags flags, bool bClear, const std::string &name);
        void OnDestroy();
        void OnCreateWindowSizeDependentResources(uint32_t Width, uint32_t Height);
        void OnDestroyWindowSizeDependentResources();
        void BeginPass(VkCommandBuffer commandList, VkRect2D renderArea);
        void EndPass(VkCommandBuffer commandList);
        void GetCompilerDefines(DefineList &defines);
        VkRenderPass GetRenderPass() { return m_renderPass; }
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

        void OnCreateWindowSizeDependentResources(SwapChain *pSwapChain, uint32_t Width, uint32_t Height);
        void OnDestroyWindowSizeDependentResources();

        void GetAttachmentList(GBufferFlags flags, std::vector<VkImageView> *pAttachments, std::vector<VkClearValue> *pClearValues);
        VkRenderPass CreateRenderPass(GBufferFlags flags, bool bClear);

        void GetCompilerDefines(DefineList &defines);
        VkSampleCountFlagBits  GetSampleCount() { return m_sampleCount; }
        Device *GetDevice() { return m_pDevice; }

        // depth buffer
        Texture                         m_DepthBuffer;
        VkImageView                     m_DepthBufferDSV;
        VkImageView                     m_DepthBufferSRV;

        // difuse
        Texture                         m_Diffuse;
        VkImageView                     m_DiffuseSRV;

        // specular
        Texture                         m_SpecularRoughness;
        VkImageView                     m_SpecularRoughnessSRV;

        // motion vectors
        Texture                         m_MotionVectors;
        VkImageView                     m_MotionVectorsSRV;

        // normal buffer
        Texture                         m_NormalBuffer;
        VkImageView                     m_NormalBufferSRV;

        // world space coordinates
        Texture                         m_WorldCoord;
        VkImageView                     m_WorldCoordSRV;

        // HDR
        Texture                         m_HDR;
        VkImageView                     m_HDRSRV;

    private:
        Device                         *m_pDevice;

        VkSampleCountFlagBits           m_sampleCount;

        GBufferFlags                    m_GBufferFlags;
        std::vector<VkClearValue>       m_clearValues;

        std::map<GBufferFlags, VkFormat> m_formats;
    };

}