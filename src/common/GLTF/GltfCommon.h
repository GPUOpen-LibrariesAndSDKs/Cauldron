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
#include "json.h"
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

// Define a maximum number of shadows supported in a scene (note, these are only for spots and directional)
static const uint32_t MaxLightInstances = 80;
static const uint32_t MaxShadowInstances = 32;

class Matrix2
{
    math::Matrix4 m_current;
    math::Matrix4 m_previous;
public:
    void Set(const math::Matrix4& m) { m_previous = m_current; m_current = m; }
    math::Matrix4 GetCurrent() const { return m_current; }
    math::Matrix4 GetPrevious() const { return m_previous; }
};

//
// Structures holding the per frame constant buffer data. 
//
struct Light
{
    math::Matrix4   mLightViewProj;
    math::Matrix4   mLightView;

    float         direction[3];
    float         range;

    float         color[3];
    float         intensity;

    float         position[3];
    float         innerConeCos;

    float         outerConeCos;
    uint32_t      type;
    float         depthBias;
    int32_t       shadowMapIndex = -1;
};


const uint32_t LightType_Directional = 0;
const uint32_t LightType_Point = 1;
const uint32_t LightType_Spot = 2;

struct per_frame
{
    math::Matrix4 mCameraCurrViewProj;
    math::Matrix4 mCameraPrevViewProj;
    math::Matrix4  mInverseCameraCurrViewProj;
    math::Vector4  cameraPos;
    float     iblFactor;
    float     emmisiveFactor;
    float     invScreenResolution[2];

    math::Vector4 wireframeOptions;

    float     mCameraCurrJitter[2];
    float     mCameraPrevJitter[2];

    Light     lights[MaxLightInstances];
    uint32_t  lightCount;
    float     lodBias = 0.0f;
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

    std::vector<math::Matrix4> m_animatedMats;       // object space matrices of each node after being animated

    std::vector<Matrix2> m_worldSpaceMats;     // world space matrices of each node after processing the hierarchy
    std::map<int, std::vector<Matrix2>> m_worldSpaceSkeletonMats; // skinning matrices, following the m_jointsNodeIdx order

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
    void TransformScene(int sceneIndex, const math::Matrix4& world);
    per_frame *SetPerFrameData(const Camera& cam);
    bool GetCamera(uint32_t cameraIdx, Camera *pCam) const;
    tfNodeIdx AddNode(const tfNode& node);
    int AddLight(const tfNode& node, const tfLight& light);

private:
    void InitTransformedData(); //this is called after loading the data from the GLTF
    void TransformNodes(const math::Matrix4& world, const std::vector<tfNodeIdx> *pNodes);
    math::Matrix4 ComputeDirectionalLightOrthographicMatrix(const math::Matrix4& mLightView);
};
