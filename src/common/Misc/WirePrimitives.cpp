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
#include "WirePrimitives.h"
#include "Misc/Camera.h"

void GenerateSphere(int sides, std::vector<unsigned short> &outIndices, std::vector<float> &outVertices)
{
    int i = 0;

    outIndices.clear();
    outVertices.clear();

    for (int roll = 0; roll < sides; roll++)
    {
        for (int pitch = 0; pitch < sides; pitch++)
        {
            outIndices.push_back(i);
            outIndices.push_back(i + 1);
            outIndices.push_back(i);
            outIndices.push_back(i + 2);
            i += 3;

            math::Vector4 v1 = PolarToVector((roll    ) * (2.0f * XM_PI) / sides, (pitch    ) * (2.0f * XM_PI) / sides);
            math::Vector4 v2 = PolarToVector((roll + 1) * (2.0f * XM_PI) / sides, (pitch    ) * (2.0f * XM_PI) / sides);
            math::Vector4 v3 = PolarToVector((roll    ) * (2.0f * XM_PI) / sides, (pitch + 1) * (2.0f * XM_PI) / sides);

            outVertices.push_back(v1.getX()); outVertices.push_back(v1.getY()); outVertices.push_back(v1.getZ());
            outVertices.push_back(v2.getX()); outVertices.push_back(v2.getY()); outVertices.push_back(v2.getZ());
            outVertices.push_back(v3.getX()); outVertices.push_back(v3.getY()); outVertices.push_back(v3.getZ());
        }
    }
}

void GenerateBox(std::vector<unsigned short> &outIndices, std::vector<float> &outVertices)
{
    outIndices.clear();
    outVertices.clear();

    std::vector<unsigned short> indices =
    {
        0,1, 1,2, 2,3, 3,0,
        4,5, 5,6, 6,7, 7,4,
        0,4,
        1,5,
        2,6,
        3,7
    };

    std::vector<float> vertices =
    {
        -1,  -1,   1,
        1,  -1,   1,
        1,   1,   1,
        -1,   1,   1,
        -1,  -1,  -1,
        1,  -1,  -1,
        1,   1,  -1,
        -1,   1,  -1,
    };

    outIndices = indices;
    outVertices = vertices;
}
