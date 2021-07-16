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
#include "ShaderCompilerHelper.h"
#include "base/ShaderCompilerCache.h"
#include "Misc/AsyncCache.h"
#include <codecvt>
#include <locale>

namespace CAULDRON_DX12
{
#define USE_MULTITHREADED_CACHE 

    Cache<D3D12_SHADER_BYTECODE> s_shaderCache;

    void DestroyShadersInTheCache()
    {
        s_shaderCache.ForEach([](const Cache<D3D12_SHADER_BYTECODE>::DatabaseType::iterator& it)
        {
            free((void*)(it->second.m_data.pShaderBytecode));
        });
    }

    void CreateShaderCache()
    {
        PWSTR path = NULL;
        SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &path);
        std::wstring sShaderCachePathW = std::wstring(path) + L"\\AMD\\Cauldron\\ShaderCacheDX";
        CreateDirectoryW((std::wstring(path) + L"\\AMD").c_str(), 0);
        CreateDirectoryW((std::wstring(path) + L"\\AMD\\Cauldron").c_str(), 0);
        CreateDirectoryW((std::wstring(path) + L"\\AMD\\Cauldron\\ShaderCacheDX").c_str(), 0);

        InitShaderCompilerCache("ShaderLibDX", std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(sShaderCachePathW));
    }

    void DestroyShaderCache(Device *pDevice)
    {
        pDevice;
        DestroyShadersInTheCache();
    }

    bool DXCompile(const char *pSrcCode,
        const DefineList* pDefines,
        const char *pEntryPoint,
        const char *pParams,
        D3D12_SHADER_BYTECODE* pOutBytecode)
    {
        //compute hash
        //
        size_t hash;
        hash = HashShaderString((GetShaderCompilerLibDir() + "\\").c_str() , pSrcCode);
        hash = Hash(pEntryPoint, strlen(pEntryPoint), hash);
        hash = Hash(pParams, strlen(pParams), hash);
        if (pDefines != NULL)
        {
            hash = pDefines->Hash(hash);
        }

#ifdef USE_MULTITHREADED_CACHE
        // Compile if not in cache
        //
        if (s_shaderCache.CacheMiss(hash, pOutBytecode))
#endif
        {
            char *SpvData = NULL;
            size_t SpvSize = 0;

            DXCompileToDXO(hash, pSrcCode, pDefines, pEntryPoint, pParams, &SpvData, &SpvSize);

            assert(SpvSize != 0);
            pOutBytecode->BytecodeLength = SpvSize;
            pOutBytecode->pShaderBytecode = SpvData;

#ifdef USE_MULTITHREADED_CACHE
            s_shaderCache.UpdateCache(hash, pOutBytecode);
#endif
        }

        return true;
    }

    bool CompileShaderFromString(
        const char *pShaderCode,
        const DefineList* pDefines,
        const char *pEntrypoint,
        const char *pParams,
        D3D12_SHADER_BYTECODE* pOutBytecode)
    {
        assert(strlen(pShaderCode) > 0);

        return DXCompile(pShaderCode, pDefines, pEntrypoint, pParams, pOutBytecode);
    }

    bool CompileShaderFromFile(
        const char *pFilename,
        const DefineList* pDefines,
        const char *pEntryPoint,
        const char *pParams,
        D3D12_SHADER_BYTECODE* pOutBytecode)
    {
        char *pShaderCode;
        size_t size;

        //append path
        char fullpath[1024];
        sprintf_s(fullpath, "%s\\%s", GetShaderCompilerLibDir().c_str(), pFilename);

    bool result = false;
    if (ReadFile(fullpath, &pShaderCode, &size, false))
    {
        result = CompileShaderFromString(pShaderCode, pDefines, pEntryPoint, pParams, pOutBytecode);
        free(pShaderCode);
    }

    assert(result && "Some of the shaders have not been copied to the bin folder, try rebuilding the solution.");

return result;
    }

}