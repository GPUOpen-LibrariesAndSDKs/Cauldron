// AMD Cauldron code
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

#ifdef ID_SKINNING_MATRICES

struct Matrix2
{
    mat4 m_current;
    mat4 m_previous;
};


layout (std140, binding = ID_SKINNING_MATRICES) uniform perSkeleton
{
    Matrix2 u_ModelMatrix[200];
} myPerSkeleton;

mat4 GetCurrentSkinningMatrix(vec4 Weights, uvec4 Joints)
{
    mat4 skinningMatrix =
        Weights.x * myPerSkeleton.u_ModelMatrix[Joints.x].m_current +
        Weights.y * myPerSkeleton.u_ModelMatrix[Joints.y].m_current +
        Weights.z * myPerSkeleton.u_ModelMatrix[Joints.z].m_current +
        Weights.w * myPerSkeleton.u_ModelMatrix[Joints.w].m_current;
    return skinningMatrix;
}

mat4 GetPreviousSkinningMatrix(vec4 Weights, uvec4 Joints)
{
    mat4 skinningMatrix =
        Weights.x * myPerSkeleton.u_ModelMatrix[Joints.x].m_previous +
        Weights.y * myPerSkeleton.u_ModelMatrix[Joints.y].m_previous +
        Weights.z * myPerSkeleton.u_ModelMatrix[Joints.z].m_previous +
        Weights.w * myPerSkeleton.u_ModelMatrix[Joints.w].m_previous;
    return skinningMatrix;
}

#endif
