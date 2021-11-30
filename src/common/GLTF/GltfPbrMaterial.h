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
#include "GltfHelpers.h"
#include "Base/ShaderCompiler.h"

struct PBRMaterialParametersConstantBuffer
{
    math::Vector4 m_emissiveFactor;

    // pbrMetallicRoughness
    math::Vector4 m_baseColorFactor;
    math::Vector4 m_metallicRoughnessValues;

    // KHR_materials_pbrSpecularGlossiness
    math::Vector4 m_DiffuseFactor;
    math::Vector4 m_specularGlossinessFactor;
};


struct PBRMaterialParameters
{
    bool     m_doubleSided = false;
    bool     m_blending = false;

    DefineList m_defines;

    PBRMaterialParametersConstantBuffer m_params;
};


// Read GLTF material and store it in our structure
//
void SetDefaultMaterialParamters(PBRMaterialParameters *pPbrMaterialParameters);
void ProcessMaterials(const json::object_t &material, PBRMaterialParameters *tfmat, std::map<std::string, int> &textureIds);
bool DoesMaterialUseSemantic(DefineList &defines, const std::string semanticName);
bool ProcessGetTextureIndexAndTextCoord(const json::object_t &material, const std::string &textureName, int *pIndex, int *pTexCoord);
void GetSrgbAndCutOffOfImageGivenItsUse(int imageIndex, const json &materials, std::unordered_map<int, int> const &textureToImage, bool *pSrgbOut, float *pCutoff);