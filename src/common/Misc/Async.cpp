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
#include "AsyncCache.h"
#include "Misc.h"
#include <future>
#include "CPUUserMarkers.h"

//
//
//

Async::Async(std::function<void()> job, Sync *pSync, const wchar_t* jobDescription) :
    m_job{job},
    m_pSync{pSync},
    m_jobDescription{jobDescription}
{
    if (m_pSync)
        m_pSync->Inc();

    {
        std::unique_lock<std::mutex> lock(s_AsyncMutex);

        while (s_activeThreads >= s_maxThreads)
        {
            s_condition.wait(lock);
        }
        
        s_activeThreads++;
    }

    m_future = std::async([this,jobDescription]()
    {
        // Obtain previous thread name and store it.

        {
            CPUUserMarker marker(jobDescription);
            m_job();
        }

        {
            std::lock_guard<std::mutex> lock(s_AsyncMutex);
            s_activeThreads--;
        }        

        s_condition.notify_all();

        if (m_pSync)
            m_pSync->Dec();

    });
}

Async::~Async()
{
    m_future.wait();
}

void Async::Wait(Sync *pSync)
{    
    if (pSync->Get() == 0)
        return;     

    {
        std::lock_guard <std::mutex> lock(s_AsyncMutex);
        s_activeThreads--;
    }

    s_condition.notify_all();

    pSync->Wait();

    {
        std::unique_lock<std::mutex> lock(s_AsyncMutex);

        s_condition.wait(lock, []
        {
            return s_activeThreads<s_maxThreads;
        });

        s_activeThreads++;
    }    
}

//
// Basic async pool
//

AsyncPool::~AsyncPool()
{
    Flush();
}

void AsyncPool::Flush()
{
    std::lock_guard <std::mutex> lock(m_AsyncPoolMutex);
    for (int i = 0; i < m_pool.size(); i++)
        delete m_pool[i];
    m_pool.clear();
}

void AsyncPool::AddAsyncTask(std::function<void()> job, Sync *pSync, const wchar_t* jobDescription)
{
    std::lock_guard <std::mutex> lock(m_AsyncPoolMutex);
    m_pool.push_back( new Async(job, pSync, jobDescription) );
}


//
// ExecAsyncIfThereIsAPool, will use async if there is a pool, otherwise will run the task synchronously
void ExecAsyncIfThereIsAPool(AsyncPool* pAsyncPool, const wchar_t* jobDescription, std::function<void()> job)
{
    // use MT if there is a pool
    if (pAsyncPool != NULL)
    {
        pAsyncPool->AddAsyncTask(job, nullptr, jobDescription);
    }
    else
    {
        job();
    }
}

//
// ExecAsyncIfThereIsAPool, will use async if there is a pool, otherwise will run the task synchronously
void ExecAsyncIfThereIsAPool(AsyncPool *pAsyncPool, std::function<void()> job)
{
    // use MT if there is a pool
    if (pAsyncPool != NULL)
    {
        pAsyncPool->AddAsyncTask(job, nullptr, nullptr);
    }
    else
    {
        job();
    }
}

//
// Some static functions
//
int Async::s_activeThreads = 0;
std::mutex Async::s_AsyncMutex;
std::condition_variable Async::s_condition;
int Async::s_maxThreads = std::thread::hardware_concurrency();
