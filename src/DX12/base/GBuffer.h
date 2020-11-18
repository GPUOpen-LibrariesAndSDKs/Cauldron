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
        GBUFFER_SPECULAR_ROUGHNESS = 32
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
    private:
        GBufferFlags                    m_GBufferFlags;
        GBuffer                        *m_pGBuffer;

        D3D12_CPU_DESCRIPTOR_HANDLE m_DepthBuffer;
        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>  m_RenderTargetViewList;
    };

    class GBuffer
    {
    public:
        void OnCreate(Device* pDevice, ResourceViewHeaps *pHeaps, const std::map<GBufferFlags, DXGI_FORMAT> &formats, uint32_t sampleCount);
        void OnDestroy();

        void OnCreateWindowSizeDependentResources(SwapChain *pSwapChain, uint32_t Width, uint32_t Height);
        void OnDestroyWindowSizeDependentResources();

        uint32_t GetSampleCount() {  return m_sampleCount; }

        // depth buffer
        Texture                         m_DepthBuffer;
        DSV                             m_DepthBufferDSV;

        // difuse
        Texture                         m_Diffuse;
        RTV                             m_DiffuseRTV;
        CBV_SRV_UAV                     m_DiffuseSRV;

        // specular
        Texture                         m_SpecularRoughness;
        RTV                             m_SpecularRoughnessRTV;
        CBV_SRV_UAV                     m_SpecularRoughnessSRV;

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