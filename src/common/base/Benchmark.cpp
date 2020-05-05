// AMD AMDUtils code
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
#include "Benchmark.h"

float Benchmark(const std::vector<TimeStamp> &timeStamps, float time, float duration)
{
    static uint32_t frames = 0;
    if (frames < 200) // render the scene for 200 frames before starting the animation.
    {
        frames++;
        return 0.0f;
    }

    static FILE *f = NULL;
    if (f == NULL)
    {
        fopen_s(&f, "res.csv", "a");

        //save heathers
        fprintf(f, "time");
        for (uint32_t i = 1; i < timeStamps.size(); i++)
            fprintf(f, ", %s", timeStamps[i].m_label.c_str());
        fprintf(f, "\n");
        time = 0.0f;
    }

    //save timings
    fprintf(f, "%f", time);
    for (uint32_t i = 1; i < timeStamps.size(); i++)
        fprintf(f, ", %f", ((float)(timeStamps[i].m_microseconds - timeStamps[i - 1].m_microseconds)));
    fprintf(f, "\n");

    // are we done yet?
    if (duration == 0)
        duration = 5;

    if (time > duration)
    {
        fclose(f);
        exit(0);
    }

    return time;
}
