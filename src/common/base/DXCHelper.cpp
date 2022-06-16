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
#include <D3DCompiler.h>
#include <dxcapi.h>
#include "Misc/Misc.h"
#include "DXCHelper.h"
#include "ShaderCompiler.h"
#include "ShaderCompilerCache.h"
#include "Misc/Error.h"

#define USE_DXC_SPIRV_FROM_DISK

void CompileMacros(const DefineList *pMacros, std::vector<D3D_SHADER_MACRO> *pOut)
{
    if (pMacros != NULL)
    {
        for (auto it = pMacros->begin(); it != pMacros->end(); it++)
        {
            D3D_SHADER_MACRO macro;
            macro.Name = it->first.c_str();
            macro.Definition = it->second.c_str();
            pOut->push_back(macro);
        }
    }
}

DxcCreateInstanceProc s_dxc_create_func;

bool InitDirectXCompiler()
{
    std::string fullshaderCompilerPath = "dxcompiler.dll";
    std::string fullshaderDXILPath = "dxil.dll";

    HMODULE dxil_module = ::LoadLibrary(fullshaderDXILPath.c_str());

    HMODULE dxc_module = ::LoadLibrary(fullshaderCompilerPath.c_str());
    s_dxc_create_func = (DxcCreateInstanceProc)::GetProcAddress(dxc_module, "DxcCreateInstance");

    return s_dxc_create_func != NULL;
}

interface Includer : public ID3DInclude
{
public:
    virtual ~Includer() {}

    HRESULT Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes)
    {
        std::string fullpath = GetShaderCompilerLibDir() + pFileName;
        return ReadFile(fullpath.c_str(), (char**)ppData, (size_t*)pBytes, false) ? S_OK : E_FAIL;
    }
    HRESULT Close(LPCVOID pData)
    {
        free((void*)pData);
        return S_OK;
    }
};

interface IncluderDxc : public IDxcIncludeHandler
{
    IDxcLibrary *m_pLibrary;
public:
    IncluderDxc(IDxcLibrary *pLibrary) : m_pLibrary(pLibrary) {}
    HRESULT QueryInterface(const IID &, void **) { return S_OK; }
    ULONG AddRef() { return 0; }
    ULONG Release() { return 0; }
    HRESULT LoadSource(LPCWSTR pFilename, IDxcBlob **ppIncludeSource)
    {
        char fullpath[1024];
        sprintf_s<1024>(fullpath, ("%s\\%S"), GetShaderCompilerLibDir().c_str(), pFilename);

        LPCVOID pData;
        size_t bytes;
        HRESULT hr = ReadFile(fullpath, (char**)&pData, (size_t*)&bytes, false) ? S_OK : E_FAIL;

        if (hr == E_FAIL)
        {
            // return the failure here instead of crashing on CreateBlobWithEncodingFromPinned 
            // to be able to report the error to the output console
            return hr;
        }

        IDxcBlobEncoding *pSource;
        ThrowIfFailed(m_pLibrary->CreateBlobWithEncodingFromPinned((LPBYTE)pData, (UINT32)bytes, CP_UTF8, &pSource));
        *ppIncludeSource = pSource;

        return S_OK;
    }
};

bool DXCompileToDXO(size_t hash,
    const char *pSrcCode,
    const DefineList* pDefines,
    const char *pEntryPoint,
    const char *pParams,
    char **outSpvData,
    size_t *outSpvSize)
{
    //detect output bytecode type (DXBC/SPIR-V) and use proper extension
    std::string filenameOut;
    {
        auto found = std::string(pParams).find("-spirv ");
        if (found == std::string::npos)
            filenameOut = GetShaderCompilerCacheDir() + format("\\%p.dxo", hash);
        else
            filenameOut = GetShaderCompilerCacheDir() + format("\\%p.spv", hash);
    }

#ifdef USE_DXC_SPIRV_FROM_DISK
    if (ReadFile(filenameOut.c_str(), outSpvData, outSpvSize, true) && *outSpvSize > 0)
    {
        //Trace(format("thread 0x%04x compile: %p disk\n", GetCurrentThreadId(), hash));
        return true;
    }
#endif

    // create hlsl file for shader compiler to compile
    //
    std::string filenameHlsl = GetShaderCompilerCacheDir() + format("\\%p.hlsl", hash);
    std::ofstream ofs(filenameHlsl, std::ofstream::out);
    ofs << pSrcCode;
    ofs.close();

    std::string filenamePdb = GetShaderCompilerCacheDir() + format("\\%p.lld", hash);

    // get defines
    //
    
    struct DefineElement
    {
        wchar_t name[128];
        wchar_t value[128];
    };
    std::vector<DefineElement> defineElements(pDefines ? pDefines->size() : 0);
    std::vector<DxcDefine> defines(pDefines ? pDefines->size() : 0);

    int defineCount = 0;
    if (pDefines != NULL)
    {
        for (auto it = pDefines->begin(); it != pDefines->end(); it++)
        {
            auto& defineElement = defineElements[defineCount];
            swprintf_s<128>(defineElement.name, L"%S", it->first.c_str());
            swprintf_s<128>(defineElement.value, L"%S", it->second.c_str());

            defines[defineCount].Name = defineElement.name;
            defines[defineCount].Value = defineElement.value;
            defineCount++;
        }
    }

    // check whether DXCompiler is initialized
    if (s_dxc_create_func == nullptr)
    {
        Trace("Error: s_dxc_create_func() is null, have you called InitDirectXCompiler() ?");
        return false;
    }

    IDxcLibrary *pLibrary;
    ThrowIfFailed(s_dxc_create_func(CLSID_DxcLibrary, IID_PPV_ARGS(&pLibrary)));

    IDxcBlobEncoding *pSource;
    ThrowIfFailed(pLibrary->CreateBlobWithEncodingFromPinned((LPBYTE)pSrcCode, (UINT32)strlen(pSrcCode), CP_UTF8, &pSource));

    IDxcCompiler2* pCompiler;
    ThrowIfFailed(s_dxc_create_func(CLSID_DxcCompiler, IID_PPV_ARGS(&pCompiler)));

    IncluderDxc Includer(pLibrary);

    std::vector<LPCWSTR> ppArgs;
    // splits params string into an array of strings
    {

        char params[1024];
        strcpy_s<1024>(params, pParams);

        char *next_token;
        char *token = strtok_s(params, " ", &next_token);
        while (token != NULL) {
            wchar_t wide_str[1024];
            swprintf_s<1024>(wide_str, L"%S", token);

            const size_t wide_str2_len = wcslen(wide_str) + 1;
            wchar_t *wide_str2 = (wchar_t *)malloc(wide_str2_len * sizeof(wchar_t));
            wcscpy_s(wide_str2, wide_str2_len, wide_str);
            ppArgs.push_back(wide_str2);

            token = strtok_s(NULL, " ", &next_token);
        }
    }

    wchar_t  pEntryPointW[256];
    swprintf_s<256>(pEntryPointW, L"%S", pEntryPoint);

    IDxcOperationResult *pResultPre;
    HRESULT res1 = pCompiler->Preprocess(pSource, L"", NULL, 0, defines.data(), defineCount, &Includer, &pResultPre);
    if (res1 == S_OK)
    {
        Microsoft::WRL::ComPtr<IDxcBlob> pCode1;
        pResultPre->GetResult(pCode1.GetAddressOf());
        std::string preprocessedCode = "";
        preprocessedCode = "// dxc -E" + std::string(pEntryPoint) + " " + std::string(pParams) + " " + filenameHlsl + "\n\n";
        if (pDefines)
        {
            for (auto it = pDefines->begin(); it != pDefines->end(); it++)
                preprocessedCode += "#define " + it->first + " " + it->second + "\n";
        }
        preprocessedCode += std::string((char *)pCode1->GetBufferPointer());
        preprocessedCode += "\n";
        SaveFile(filenameHlsl.c_str(), preprocessedCode.c_str(), preprocessedCode.size(), false);

        IDxcOperationResult* pOpRes;
        HRESULT res;

        if (false)
        {
            Microsoft::WRL::ComPtr<IDxcBlob> pPDB;
            LPWSTR pDebugBlobName[1024];
            res = pCompiler->CompileWithDebug(pSource, NULL, pEntryPointW, L"", ppArgs.data(), (UINT32)ppArgs.size(), defines.data(), defineCount, &Includer, &pOpRes, pDebugBlobName, pPDB.GetAddressOf());

            // Setup the correct name for the PDB
            if (pPDB)
            {                
                char pPDBName[1024];
                sprintf_s(pPDBName, "%s\\%ls", GetShaderCompilerCacheDir().c_str(), *pDebugBlobName);
                SaveFile(pPDBName, pPDB->GetBufferPointer(), pPDB->GetBufferSize(), true);
            }
        }
        else
        {
            res = pCompiler->Compile(pSource, NULL, pEntryPointW, L"", ppArgs.data(), (UINT32)ppArgs.size(), defines.data(), defineCount, &Includer, &pOpRes);
        }

        // Clean up allocated memory
        while (!ppArgs.empty())
        {
            LPCWSTR pWString = ppArgs.back();
            ppArgs.pop_back();
            free((void*)pWString);
        }

        pSource->Release();
        pLibrary->Release();
        pCompiler->Release();

        IDxcBlob *pResult = NULL;
        IDxcBlobEncoding *pError = NULL;
        if (pOpRes != NULL)
        {
            pOpRes->GetResult(&pResult);
            pOpRes->GetErrorBuffer(&pError);
            pOpRes->Release();
        }

        if (pResult != NULL && pResult->GetBufferSize() > 0)
        {
            *outSpvSize = pResult->GetBufferSize();
            *outSpvData = (char *)malloc(*outSpvSize);

            memcpy(*outSpvData, pResult->GetBufferPointer(), *outSpvSize);

            pResult->Release();

            // Make sure pError doesn't leak if it was allocated
            if (pError)
                pError->Release();

#ifdef USE_DXC_SPIRV_FROM_DISK
            SaveFile(filenameOut.c_str(), *outSpvData, *outSpvSize, true);
#endif
            return true;
        }
        else
        {
            IDxcBlobEncoding *pErrorUtf8;
            pLibrary->GetBlobAsUtf8(pError, &pErrorUtf8);

            Trace("*** Error compiling %p.hlsl ***\n", hash);

            std::string filenameErr = GetShaderCompilerCacheDir() + format("\\%p.err", hash);
            SaveFile(filenameErr.c_str(), pErrorUtf8->GetBufferPointer(), pErrorUtf8->GetBufferSize(), false);

            std::string errMsg = std::string((char*)pErrorUtf8->GetBufferPointer(), pErrorUtf8->GetBufferSize());
            Trace(errMsg);

            // Make sure pResult doesn't leak if it was allocated
            if (pResult)
                pResult->Release();

            pErrorUtf8->Release();
        }
    }

    return false;
}

