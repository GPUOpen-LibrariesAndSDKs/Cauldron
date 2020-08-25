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
#include "../json/json.h"
#include "../Misc/Camera.h"
#include "GltfStructures.h"

// The GlTF file is loaded in 2 steps
//
// 1) loading the GPU agnostic data that is common to all passes (This is done in the GLTFCommon class you can see here below)
//     - nodes
//     - scenes
//     - animations
//     - binary buffers
//     - skinning skeletons
//
// 2) loading the GPU specific data that is common to any pass (This is done in the GLTFCommonVK class)
//     - textures
//

using json = nlohmann::json;

struct GLTFCommonTransformed
{
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
    XMMATRIX  mInverseCameraViewProj;
    XMVECTOR  cameraPos;
    float     iblFactor;
    float     emmisiveFactor;
    float     invScreenResolution[2];

    uint32_t  padding[3];
    uint32_t  lightCount;
    Light     lights[80];
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
    std::vector<LightInstance> m_lightInstances;
    std::vector<tfCamera> m_cameras;

    std::vector<tfNode> m_nodes;

    std::vector<tfAnimation> m_animations;
    std::vector<char *> m_buffersData;

    const json *m_pAccessors;
    const json *m_pBufferViews;

    std::vector<XMMATRIX> m_animatedMats;       // object space matrices of each node after being animated

    // we keep the data for the last 2 frames, this is for computing motion vectors
    GLTFCommonTransformed m_transformedData[2];
    GLTFCommonTransformed *m_pCurrentFrameTransformedData;
    GLTFCommonTransformed *m_pPreviousFrameTransformedData;

    per_frame m_perFrameData;

    bool Load(const std::string &path, const std::string &filename);
    void Unload();

    // misc functions
    int FindMeshSkinId(int meshId) const;
    int GetInverseBindMatricesBufferSizeByID(int id) const;
    void GetBufferDetails(int accessor, tfAccessor *pAccessor) const;
    void GetAttributesAccessors(const json &gltfAttributes, std::vector<char*> *pStreamNames, std::vector<tfAccessor> *pAccessors) const;

    // transformation and animation functions
    void SetAnimationTime(uint32_t animationIndex, float time);
    void TransformScene(int sceneIndex, XMMATRIX world);
    per_frame *SetPerFrameData(const Camera &cam);
    bool GetCamera(uint32_t cameraIdx, Camera *pCam) const;
    tfNodeIdx AddNode(const tfNode& node);
    int AddLight(const tfNode& node, const tfLight& light);
private:
    void InitTransformedData(); //this is called after loading the data from the GLTF
    void TransformNodes(const tfNode *pRootNode, XMMATRIX world, const std::vector<tfNodeIdx> *pNodes, GLTFCommonTransformed *pTransformed) const;
};
