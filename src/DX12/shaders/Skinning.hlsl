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

//--------------------------------------------------------------------------------------
//  Constant buffers
//--------------------------------------------------------------------------------------
#ifdef ID_SKINNING_MATRICES

struct Matrix2
{
    matrix m_current;
    matrix m_previous;
};

cbuffer cbPerSkeleton : register(CB(ID_SKINNING_MATRICES))
{
    Matrix2 myPerSkeleton_u_ModelMatrix[200];
};

matrix GetCurrentSkinningMatrix(float4 Weights, uint4 Joints)
{
    matrix skinningMatrix =
        Weights.x * myPerSkeleton_u_ModelMatrix[Joints.x].m_current +
        Weights.y * myPerSkeleton_u_ModelMatrix[Joints.y].m_current +
        Weights.z * myPerSkeleton_u_ModelMatrix[Joints.z].m_current +
        Weights.w * myPerSkeleton_u_ModelMatrix[Joints.w].m_current;
    return skinningMatrix;
}

matrix GetPreviousSkinningMatrix(float4 Weights, uint4 Joints)
{
    matrix skinningMatrix =
        Weights.x * myPerSkeleton_u_ModelMatrix[Joints.x].m_previous +
        Weights.y * myPerSkeleton_u_ModelMatrix[Joints.y].m_previous +
        Weights.z * myPerSkeleton_u_ModelMatrix[Joints.z].m_previous +
        Weights.w * myPerSkeleton_u_ModelMatrix[Joints.w].m_previous;
    return skinningMatrix;
}

#endif

