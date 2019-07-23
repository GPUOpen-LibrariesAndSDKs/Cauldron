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
#include "..\json\json.h"
#include "..\..\common\Misc\Camera.h"
#include "GltfStructures.h"

// The GlTF file is loaded in 2 steps
//
// 1) loading the GPU agnostic data that is common to all pass (This is done in the GLTFCommon class you can see here below)
//     - nodes
//     - scenes
//     - animations
//     - binary buffers
//     - skining skeletons
//
// 2) loading the GPU specific data that is common to any pass (This is done in the GLTFCommonVK class)
//     - textures
//

using json = nlohmann::json;

struct GLTFCommonTransformed
{    
    std::vector<XMMATRIX> m_animatedMats;       // objext space matrices of each node after being animated
    std::vector<XMMATRIX> m_worldSpaceMats;     // world space matrices of each node after processing the hierarchy
    std::map<int, std::vector<XMMATRIX>> m_worldSpaceSkeletonMats; // skinning matrices, following the m_jointsNodeIdx order
};

//
// Structures holding the per frame constant buffer data. 
//
struct Light
{
    XMMATRIX      mLightViewProj;

    float         direction[3];
    float         range;

    float         color[3];
    float         intensity;
    
    float         position[3];
    float         innerConeCos;
    
    float         outerConeCos;
    uint32_t      type;
    float         depthBias;
    uint32_t      shadowMapIndex = -1;
};

const uint32_t LightType_Directional = 0;
const uint32_t LightType_Point = 1;
const uint32_t LightType_Spot = 2;

struct per_frame
{
    XMMATRIX  mCameraViewProj;
    XMVECTOR  cameraPos;
    float     iblFactor;
    float     emmisiveFactor;

    uint32_t  padding;
    uint32_t  lightCount;
    Light     lights[4];
};

//
// GLTFCommon, common stuff that is API agnostic
//
class GLTFCommon
{
public:
    json j3;

    std::string m_path;
    std::vector<tfScene> m_scenes;
    std::vector<tfMesh> m_meshes;
    std::vector<tfSkins> m_skins;
    std::vector<tfLight> m_lights;
    std::vector<tfCamera> m_cameras;

    std::vector<tfNode> m_nodes;

    std::vector<tfAnimation> m_animations;
    std::vector<char *> m_buffersData;

    const json::array_t *m_pAccessors;
    const json::array_t *m_pBufferViews;

    GLTFCommonTransformed m_transformedData;

    per_frame m_perFrameData;

    bool Load(const std::string &path, const std::string &filename);
    void Unload();

    // misc functions
    int FindMeshSkinId(int meshId);
    int GetInverseBindMatricesBufferSizeByID(int id);
    void GetBufferDetails(int accessor, tfAccessor *pAccessor);

    // transformation and animation functions
    void InitTransformedData();
    void SetAnimationTime(uint32_t animationIndex, float time);
    void TransformScene(int sceneIndex, XMMATRIX world);
    per_frame *SetPerFrameData(uint32_t cameraIdx);
};

