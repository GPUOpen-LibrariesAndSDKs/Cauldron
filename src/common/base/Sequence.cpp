// AMD Cauldron code
// 
// Copyright(c) 2017 Advanced Micro Devices, Inc.All rights reserved.
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
#include "Sequence.h"

void BenchmarkSequence::ReadKeyframes(const json& jSequence, float timeStart, float timeEnd)
{
    m_timeStart = jSequence.value("timeStart", timeStart);
    m_timeEnd = jSequence.value("timeEnd", timeEnd);

    m_keyFrames.clear();

    const json& keyFrames = jSequence["keyFrames"];
    for (const json& jkf : keyFrames)
    {
        KeyFrame key = {};
        key.m_camera = -1;

        key.m_time = jkf["time"];

        auto find = jkf.find("camera");
        if(find != jkf.cend())
        { 
            key.m_camera = find.value();
        }

        if (key.m_camera == -1)
        {
            const json& jfrom = jkf["from"];
            key.m_from = math::Vector4(jfrom[0], jfrom[1], jfrom[2], 0);

            const json& jto = jkf["to"];
            key.m_to = math::Vector4(jto[0], jto[1], jto[2], 0);
        }
        
        key.m_screenShotName = jkf.value("screenShotName", "");

        m_keyFrames.push_back(key);
    }
}

float BenchmarkSequence::GetNextKeyTime(float time)
{
    for (int i = 0; i < m_keyFrames.size(); i++)
    {
        KeyFrame key = m_keyFrames[i];
        if (key.m_time > time)
        {
            return key.m_time;
        }
    }

    return -1;
}

const BenchmarkSequence::KeyFrame BenchmarkSequence::GetNextKeyFrame(float time) const
{
    for (int i = 0; i < m_keyFrames.size(); i++)
    {
        const KeyFrame& key = m_keyFrames[i];
        if (key.m_time >= time)
        {
            return key;
        }
    }

    // key frame not found, return empty
    KeyFrame invalidKey = KeyFrame{ 0 };
    invalidKey.m_time = -1.0f;
    return invalidKey;
}
