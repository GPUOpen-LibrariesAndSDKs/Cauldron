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
#include "../json/json.h"
#include "../common/Misc/Camera.h"
#include "../common/GLTF/GltfCommon.h"

using json = nlohmann::json;

class BenchmarkSequence
{
public:
    struct KeyFrame
    {
        float m_time;
        int m_camera;
        math::Vector4 m_from, m_to;
        std::string m_screenShotName;
    };

    void ReadKeyframes(const json& jSequence, float timeStart, float timeEnd);
    float GetTimeStart() { return m_timeStart; }
    float GetTimeEnd() { return m_timeEnd; }

    //
    // Given a time return the time of the next keyframe, that way we know how long we have to wait until we apply the next keyframe
    //
    float GetNextKeyTime(float time);
    const KeyFrame GetNextKeyFrame(float time) const;

private:
    float m_timeStart;
    float m_timeEnd;

    std::vector<KeyFrame> m_keyFrames;
};
