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
    layout (std140, binding = ID_SKINNING_MATRICES) uniform perSkeleton
    {
        mat4 u_ModelMatrix[200];
    } myPerSkeleton;
#endif

#ifdef ID_4VS_WEIGHTS_0
    layout (location = ID_4VS_WEIGHTS_0) in  vec4 a_Weights0;
#endif

#ifdef ID_4VS_WEIGHTS_1
    layout (location = ID_4VS_WEIGHTS_1) in  vec4 a_Weights1;
#endif

#ifdef ID_4VS_JOINTS_0
    layout (location = ID_4VS_JOINTS_0) in  uvec4 a_Joints0;
#endif

#ifdef ID_4VS_JOINTS_1
    layout (location = ID_4VS_JOINTS_1) in  uvec4 a_Joints1;
#endif

mat4 ApplySkinning(in mat4 transMatrix)
{

#ifdef ID_4VS_WEIGHTS_0

    mat4 skinMatrix = 
        a_Weights0.x * myPerSkeleton.u_ModelMatrix[a_Joints0.x] +
        a_Weights0.y * myPerSkeleton.u_ModelMatrix[a_Joints0.y] +
        a_Weights0.z * myPerSkeleton.u_ModelMatrix[a_Joints0.z] +
        a_Weights0.w * myPerSkeleton.u_ModelMatrix[a_Joints0.w];
        
#ifdef ID_4VS_WEIGHTS_1
    skinMatrix += 
        a_Weights1.x * myPerSkeleton.u_ModelMatrix[a_Joints1.x] +
        a_Weights1.y * myPerSkeleton.u_ModelMatrix[a_Joints1.y] +
        a_Weights1.z * myPerSkeleton.u_ModelMatrix[a_Joints1.z] +
        a_Weights1.w * myPerSkeleton.u_ModelMatrix[a_Joints1.w];
#endif

    return transMatrix * skinMatrix;
#else    
    return transMatrix;
#endif
}
