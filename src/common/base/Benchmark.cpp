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
#include "Benchmark.h"
#include "Sequence.h"
#include "..\Misc\Misc.h"

struct Benchmark
{
    int warmUpFrames;
    int runningTimeWhenNoAnimation;
    FILE *f = NULL;
    float timeStep;
    float time;
    float timeStart, timeEnd;
    int frame;
    bool exitWhenTimeEnds;
    int cameraId;

    GLTFCommon *m_pGltfLoader;

    BenchmarkSequence m_sequence;

    bool m_animationFound = false;
    bool m_saveHeaders = true;
    float m_nextTime = 0;
};

static Benchmark bm;

static void SaveTimestamps(float time, const std::vector<TimeStamp> &timeStamps)
{    
    if (bm.m_saveHeaders)
    {
        //save headers
        fprintf(bm.f, "time");
        for (uint32_t i = 0; i < timeStamps.size(); i++)
            fprintf(bm.f, ", %s", timeStamps[i].m_label.c_str());
        fprintf(bm.f, "\n");
        time = 0.0f;

        bm.m_saveHeaders = false;
    }

    //save timings
    fprintf(bm.f, "%f", time);
    for (uint32_t i = 0; i < timeStamps.size(); i++)
        fprintf(bm.f, ", %f", (float)timeStamps[i].m_microseconds);
    fprintf(bm.f, "\n");
}

//
//
//
void BenchmarkConfig(const json& benchmark, int cameraId, GLTFCommon *pGltfLoader, const std::string& deviceName, const std::string& driverVersion)
{
    if (benchmark.is_null())
    {
        Trace("Benchmark section not found in json, the scene needs a benchmark section for this to work\n");
        exit(0);
    }

    bm.f = NULL;
    bm.frame = 0;
    // the number of frames to run before the benchmark starts
    bm.warmUpFrames = benchmark.value("warmUpFrames", 200);    
    bm.exitWhenTimeEnds = benchmark.value("exitWhenTimeEnds", true);
    
    //get filename and open it
    std::string resultsFilename = benchmark.value("resultsFilename", "res.csv");
    bm.m_saveHeaders = true;
    if(fopen_s(&bm.f, resultsFilename.c_str(), "w") != 0)
    {
        Trace(format("The file %s cannot be opened\n", resultsFilename.c_str()));
        exit(0);
    }

    fprintf(bm.f, "#deviceName %s\n", deviceName.c_str());
    fprintf(bm.f, "#driverVersion %s\n", driverVersion.c_str());

    bm.timeStep = benchmark.value("timeStep", 1.0f);

    // Set default timeStart/timeEnd
    bm.timeStart = 0;
    bm.timeEnd = 0;
    if ((pGltfLoader!=NULL) && (pGltfLoader->m_animations.size() > 0))
    {
        //if there is an animation take the endTime from the animation
        bm.timeEnd = pGltfLoader->m_animations[0].m_duration;
    }

    //override those values if set
    bm.timeStart = benchmark.value("timeStart", bm.timeStart);
    bm.timeEnd = benchmark.value("timeEnd", bm.timeEnd);
    bm.time = bm.timeStart;

    // Sets the camera and its animation:
    //
    bm.m_animationFound = false;
    bm.cameraId = cameraId;
    if ((pGltfLoader == NULL) || cameraId==-1)
    {
        if (benchmark.find("sequence") != benchmark.end())
        {
            //camera will use the sequence
            const json& sequence = benchmark["sequence"];
            bm.m_sequence.ReadKeyframes(sequence, bm.timeStart, bm.timeEnd);
            bm.timeStart = bm.m_sequence.GetTimeStart();
            bm.timeEnd = bm.m_sequence.GetTimeEnd();
            bm.m_animationFound = true;
            bm.cameraId = -1;
        }
        else
        {
            // will use no sequence, will use the default static camera
        }
    }
    else
    {
        // a camera from the GLTF will be used
        // check such a camera exist, otherwise show an error and quit
        Camera Cam;
        if (pGltfLoader->GetCamera(cameraId, &Cam) == false)
        {
            Trace(format("The cameraId %i doesn't exist in the GLTF\n", cameraId));
            exit(0);
        }
        bm.m_animationFound = true;
    }

    bm.m_nextTime = 0;
    bm.m_pGltfLoader = pGltfLoader;    
}

float BenchmarkLoop(const std::vector<TimeStamp> &timeStamps, Camera *pCam, std::string& outScreenShotName)
{
    if (bm.frame < bm.warmUpFrames) // warmup
    {
        bm.frame++;
        return bm.time;
    }

    if (bm.time > bm.timeEnd) // are we done yet?
    {
        fclose(bm.f);

        if (bm.exitWhenTimeEnds)
        {
            PostQuitMessage(0);
            return bm.time; 
        }
    }

    SaveTimestamps(bm.time, timeStamps);
        
    // animate camera
    if (bm.m_animationFound && (pCam != NULL))
    {
        // if GLTF has camera with cameraID then use that camera and its animation
        if (bm.cameraId >= 0)
        {                
            bm.m_pGltfLoader->GetCamera(bm.cameraId, pCam);
        }
        else
        {
            // cameraID is -1, then use our sequence
            if (bm.time >= bm.m_nextTime)
            {
                bm.m_nextTime = bm.m_sequence.GetNextKeyTime(bm.time);

                const BenchmarkSequence::KeyFrame keyFrame = bm.m_sequence.GetNextKeyFrame(bm.time);
                
                const bool bValidKeyframe = keyFrame.m_time >= 0;
                if (bValidKeyframe)
                {
                    if (keyFrame.m_camera == -1)
                    {
                        pCam->LookAt(keyFrame.m_from, keyFrame.m_to);
                    }
                    else
                    {
                        bm.m_pGltfLoader->GetCamera(keyFrame.m_camera, pCam);
                    }

                    const bool bShouldTakeScreenshot = !keyFrame.m_screenShotName.empty();
                    if (bShouldTakeScreenshot)
                    {
                        outScreenShotName = keyFrame.m_screenShotName;
                    }
                }
            }
        }
    }

    float time = bm.time;
    bm.time += bm.timeStep;
    return time;
}


