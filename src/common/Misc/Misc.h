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

double MillisecondsNow();
std::string format(const char* format, ...);
bool ReadFile(const char *name, char **data, size_t *size, bool isbinary);
bool SaveFile(const char *name, void const*data, size_t size, bool isbinary);
void Trace(const std::string &str);
void Trace(const char* pFormat, ...);
bool LaunchProcess(const char* commandLine, const char* filenameErr);
inline void GetXYZ(float *f, XMVECTOR v)
{
    f[0] = XMVectorGetX(v);
    f[1] = XMVectorGetY(v);
    f[2] = XMVectorGetZ(v);
}
bool CameraFrustumToBoxCollision(const XMMATRIX mCameraViewProj, const XMVECTOR boxCenter, const XMVECTOR boxExtent);

// align val to the next multiple of alignment
template<typename T> inline T AlignUp(T val, T alignment)
{
    return (val + alignment - (T)1) & ~(alignment - (T)1);
}
// align val to the previous multiple of alignment
template<typename T> inline T AlignDown(T val, T alignment)
{
    return val & ~(alignment - (T)1);
}
template<typename T> inline T DivideRoundingUp(T a, T b)
{
    return (x + y - (T)1) / y;
}

class Profile
{
    double m_startTime;
    const char *m_label;
public:
    Profile(const char *label) {
        m_startTime = MillisecondsNow(); m_label = label;
    }
    ~Profile() {
        Trace(format("*** %s  %f ms\n", m_label, (MillisecondsNow() - m_startTime)));
    }
};
