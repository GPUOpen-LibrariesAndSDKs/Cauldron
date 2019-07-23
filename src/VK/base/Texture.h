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

#pragma once

#include "Device.h"
#include "UploadHeap.h"
#include "ResourceViewHeaps.h"
#include "..\VulkanMemoryAllocator\vk_mem_alloc.h"
#include "misc\DDSLoader.h"

namespace CAULDRON_VK
{
    // This class provides functionality to create a 2D-texture/Render Target.

    class Texture
    {
    public:
        Texture();
        virtual       ~Texture();
        virtual void OnDestroy();

        // load file into heap
        INT32 Init(Device *pDevice, VkImageCreateInfo *pCreateInfo, char *name = NULL);
        INT32 InitRendertarget(Device *pDevice, uint32_t width, uint32_t height, VkFormat format, VkSampleCountFlagBits msaa, VkImageUsageFlags usage, bool bUAV, char *name = NULL);
        INT32 InitDepthStencil(Device *pDevice, uint32_t width, uint32_t height, VkFormat format, VkSampleCountFlagBits msaa, char *name = NULL);
        bool InitFromFile(Device* pDevice, UploadHeap* pUploadHeap, const char *szFilename, bool useSRGB = false, float cutOff = 1.0f);

        VkImage Resource() { return m_pResource; }

        void CreateRTV(VkImageView *pRV, int mipLevel = -1);
        void CreateSRV(VkImageView *pImageView, int mipLevel = -1);
        void CreateDSV(VkImageView *pView);
        void CreateCubeSRV(VkImageView *pImageView);

        uint32_t GetWidth() { return m_header.width; }
        uint32_t GetHeight() { return m_header.height; }
        uint32_t GetMipCount() { return m_header.mipMapCount; }

        VkFormat GetFormat() { return m_format; }
    private:
        Device         *m_pDevice = NULL;

#ifdef USE_VMA
        VmaAllocation    m_ImageAlloc = VK_NULL_HANDLE;
#else
        VkDeviceMemory   m_deviceMemory = VK_NULL_HANDLE;
#endif
        VkFormat         m_format;
        VkImage          m_pResource = VK_NULL_HANDLE;

        IMG_INFO  m_header;

    protected:

        struct FootPrint
        {
            UINT8* pixels;
            uint32_t width, height, offset;
        } footprints[6][12];


        VkImage CreateTextureCommitted(Device* pDevice, UploadHeap* pUploadHeap, const char *pName, bool useSRGB = false);
        void LoadAndUpload(Device* pDevice, UploadHeap* pUploadHeap, ImgLoader *pDds, VkImage pTexture2D);

        void    PatchFmt24To32Bit(unsigned char *pDst, unsigned char *pSrc, UINT32 pixelCount);
        UINT32  GetPixelSize(DXGI_FORMAT fmt) const;
        bool    isCubemap()const;
    };
}