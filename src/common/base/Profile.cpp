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
#include "Profile.h"
#include "Sequence.h"
#include "..\Misc\Misc.h"

struct PerformanceProfile
{
	int warmUpFrames;
	int durationFrames;
	int runningTimeWhenNoAnimation;
	std::string filterLabel;
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

static PerformanceProfile pf;

static void SaveGPUTicks(int frame, std::string filterLabel, const std::vector<TimeStamp>& timeStamps)
{    
    if (pf.m_saveHeaders)
    {
        //save headers
        fprintf(pf.f, "frame");
        for (uint32_t i = 0; i < timeStamps.size(); i++)
            if (filterLabel.empty() || filterLabel == timeStamps[i].m_label)
                fprintf(pf.f, ", %s", timeStamps[i].m_label.c_str());
        fprintf(pf.f, "\n");
        frame = 0;

        pf.m_saveHeaders = false;
    }

    //save timings
    fprintf(pf.f, "%d", frame);
	for (uint32_t i = 0; i < timeStamps.size(); i++)
		if (filterLabel.empty() || filterLabel == timeStamps[i].m_label)
            fprintf(pf.f, ", %llu", timeStamps[i].m_ticks);
    fprintf(pf.f, "\n");
}

//
//
//
void ProfileConfig(const json& profile, int cameraId, GLTFCommon *pGltfLoader, const std::string& deviceName, const std::string& driverVersion)
{
    if (profile.is_null())
    {
        Trace("Profile section not found in command line, the scene needs a profile section for this to work\n");
        exit(0);
    }

    pf.f = NULL;
    pf.frame = 0;
    // the number of frames to run before the benchmark starts
	pf.warmUpFrames = profile.value("warmUpFrames", 200);
	pf.durationFrames = profile.value("durationFrames", 200);
    pf.exitWhenTimeEnds = profile.value("exitWhenTimeEnds", true);
    pf.filterLabel = profile.value("filter", "");
    
    //get filename and open it
    std::string resultsFilename = profile.value("resultsFilename", "res.csv");
    pf.m_saveHeaders = true;
    if(fopen_s(&pf.f, resultsFilename.c_str(), "w") != 0)
    {
        Trace(format("The file %s cannot be opened\n", resultsFilename.c_str()));
        exit(0);
    }

    fprintf(pf.f, "#deviceName %s\n", deviceName.c_str());
    fprintf(pf.f, "#driverVersion %s\n", driverVersion.c_str());

    pf.timeStep = profile.value("timeStep", 1.0f);

    // Set default timeStart/timeEnd
    pf.timeStart = 0;
    pf.timeEnd = 0;
    if ((pGltfLoader!=NULL) && (pGltfLoader->m_animations.size() > 0))
    {
        //if there is an animation take the endTime from the animation
        pf.timeEnd = pGltfLoader->m_animations[0].m_duration;
    }

    //override those values if set
    pf.timeStart = profile.value("timeStart", pf.timeStart);
    pf.timeEnd = profile.value("timeEnd", pf.timeEnd);
    pf.time = pf.timeStart;

    // Sets the camera and its animation:
    //
    pf.m_animationFound = false;
    pf.cameraId = cameraId;
    if ((pGltfLoader == NULL) || cameraId==-1)
    {
        if (profile.find("sequence") != profile.end())
        {
            //camera will use the sequence
            const json& sequence = profile["sequence"];
            pf.m_sequence.ReadKeyframes(sequence, pf.timeStart, pf.timeEnd);
            pf.timeStart = pf.m_sequence.GetTimeStart();
            pf.timeEnd = pf.m_sequence.GetTimeEnd();
            pf.m_animationFound = true;
            pf.cameraId = -1;
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
        pf.m_animationFound = true;
    }

    pf.m_nextTime = 0;
    pf.m_pGltfLoader = pGltfLoader;    
}

float ProfileLoop(const std::vector<TimeStamp> &timeStamps, Camera *pCam, std::string& outScreenShotName)
{
    if (pf.frame < pf.warmUpFrames) // warmup
    {
        pf.frame++;
        return pf.time;
    }

    if ((pf.frame - pf.warmUpFrames) > pf.durationFrames) // are we done yet?
    {
        fclose(pf.f);

        if (pf.exitWhenTimeEnds)
        {
            PostQuitMessage(0);
            return pf.time; 
        }
    }

    SaveGPUTicks(pf.frame - pf.warmUpFrames, pf.filterLabel, timeStamps);
        
    // animate camera
    if (pf.m_animationFound && (pCam != NULL))
    {
        // if GLTF has camera with cameraID then use that camera and its animation
        if (pf.cameraId >= 0)
        {                
            pf.m_pGltfLoader->GetCamera(pf.cameraId, pCam);
        }
        else
        {
            // cameraID is -1, then use our sequence
            if (pf.time >= pf.m_nextTime)
            {
                pf.m_nextTime = pf.m_sequence.GetNextKeyTime(pf.time);

                const BenchmarkSequence::KeyFrame keyFrame = pf.m_sequence.GetNextKeyFrame(pf.time);
                
                const bool bValidKeyframe = keyFrame.m_time >= 0;
                if (bValidKeyframe)
                {
                    if (keyFrame.m_camera == -1)
                    {
                        pCam->LookAt(keyFrame.m_from, keyFrame.m_to);
                    }
                    else
                    {
                        pf.m_pGltfLoader->GetCamera(keyFrame.m_camera, pCam);
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

    float time = pf.time;
	pf.time += pf.timeStep;
	pf.frame++;
    return time;
}


