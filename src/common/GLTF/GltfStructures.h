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

//
// This file holds all the structures/classes used to load a glTF model
//

class tfAccessor
{
public:
    const void *m_data = NULL;
    int m_count = 0;
    int m_stride;
    int m_dimension;
    int m_type;

    math::Vector4 m_min;
    math::Vector4 m_max;

    const void *Get(int i) const
    {
        if (i >= m_count)
            i = m_count - 1;

        return (const char*)m_data + m_stride * i;
    }

    int FindClosestFloatIndex(float val) const
    {
        int ini = 0;
        int fin = m_count - 1;

        while (ini <= fin)
        {
            int mid = (ini + fin) / 2;
            float v = *(const float*)Get(mid);

            if (val < v)
                fin = mid - 1;
            else if (val > v)
                ini = mid + 1;
            else
                return mid;
        }

        return fin;
    }
};

struct tfPrimitives
{
    math::Vector4 m_center;
    math::Vector4 m_radius;
};

struct tfMesh
{
    std::vector<tfPrimitives> m_pPrimitives;
};

struct Transform
{
    math::Matrix4   m_rotation = math::Matrix4::identity();
    math::Vector4   m_translation = math::Vector4(0, 0, 0, 0);
    math::Vector4   m_scale = math::Vector4(1, 1, 1, 0);

    void LookAt(math::Vector4 source, math::Vector4 target)
    { 
        m_rotation = math::inverse(math::Matrix4::lookAt(math::Point3(source.getXYZ()), math::Point3(target.getXYZ()), math::Vector3(0, 1, 0)));
        m_translation = m_rotation.getCol(3);  m_rotation.setCol(3, math::Vector4(0, 0, 0, 1)); 
    }

    math::Matrix4 GetWorldMat() const 
    { 
        return math::Matrix4::translation(m_translation.getXYZ()) * m_rotation * math::Matrix4::scale(m_scale.getXYZ());
    }
};

typedef int tfNodeIdx;

struct tfNode
{
    std::vector<tfNodeIdx> m_children;

    int skinIndex = -1;
    int meshIndex = -1;
    int channel = -1;
    bool bIsJoint = false;

    std::string m_name;

    Transform m_transform;
};

struct NodeMatrixPostTransform
{
    tfNode *pN; math::Matrix4 m;
};

struct tfScene
{
    std::vector<tfNodeIdx> m_nodes;
};

struct tfSkins
{
    tfAccessor m_InverseBindMatrices;
    tfNode *m_pSkeleton = NULL;
    std::vector<int> m_jointsNodeIdx;
};

class tfSampler
{
public:
    tfAccessor m_time;
    tfAccessor m_value;

    void SampleLinear(float time, float *frac, float **pCurr, float **pNext) const
    {
        int curr_index = m_time.FindClosestFloatIndex(time);
        int next_index = std::min<int>(curr_index + 1, m_time.m_count - 1);

        if (curr_index < 0) curr_index++;

        if (curr_index == next_index)
        {
            *frac = 0;
            *pCurr = (float*)m_value.Get(curr_index);
            *pNext = (float*)m_value.Get(next_index);
            return;
        }

        float curr_time = *(float*)m_time.Get(curr_index);
        float next_time = *(float*)m_time.Get(next_index);

        *pCurr = (float*)m_value.Get(curr_index);
        *pNext = (float*)m_value.Get(next_index);
        *frac = (time - curr_time) / (next_time - curr_time);
        assert(*frac >= 0 && *frac <= 1.0);
    }
};

class tfChannel
{
public:
    ~tfChannel()
    {
        delete m_pTranslation;
        delete m_pRotation;
        delete m_pScale;
    }

    tfSampler *m_pTranslation;
    tfSampler *m_pRotation;
    tfSampler *m_pScale;
};

struct tfAnimation
{
    float m_duration;
    std::map<int, tfChannel> m_channels;
};

struct tfLight
{
    enum LightType { LIGHT_DIRECTIONAL, LIGHT_POINTLIGHT, LIGHT_SPOTLIGHT };

    LightType m_type = LIGHT_DIRECTIONAL;

    //tfNodeIdx m_nodeIndex = -1;

    math::Vector4 m_color;
    float       m_range;
    float       m_intensity = 0.0f;
    float       m_innerConeAngle = 0.0f;
    float       m_outerConeAngle = 0.0f;
    uint32_t    m_shadowResolution = 1024;
    float       m_bias = 70.0f / 100000.0f;
};

struct LightInstance
{
    int m_lightId = -1;
    tfNodeIdx m_nodeIndex = -1;
};

struct tfCamera
{
    enum LightType { CAMERA_PERSPECTIVE };
    float yfov, zfar, znear;

    tfNodeIdx m_nodeIndex = -1;
};