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

#pragma once
#include "ThreadPool.h"

// This is a poor's man multithreaded lib. This is how it works:
//
// Each task is invoked by the app thread using the Async class, this class executes the shader compilation in a new thread.
// To prevent context switches we need to limit the number of running threads to the number of cores. 
// This is done by a global counter that keeps track of the number of running threads. 
// This counter gets incremented when the thread is running a task and decremented when it finishes.
// It is also decremented when a thread is put into Wait mode and incremented when a thread is signaled AND there is a core available to resume the thread.
// If all cores are busy the app thread is put to Wait to prevent it from spawning more threads.

class Sync
{
    int m_count = 0;
    std::mutex m_mutex;
    std::condition_variable condition;
public:
    int Inc()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_count++;
        return m_count;
    }

    int Dec()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_count--;
        if (m_count == 0)
            condition.notify_all();
        return m_count;
    }

    int Get()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_count;
    }

    void Reset()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_count = 0;
        condition.notify_all();
    }

    void Wait()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (m_count != 0)
            condition.wait(lock);
    }

};

class Async
{
    static int s_activeThreads;
    static int s_maxThreads;
    static std::mutex s_mutex;
    static std::condition_variable s_condition;
    static bool s_bExiting;

    std::function<void()> m_job;
    Sync *m_pSync;
    std::thread *m_pThread;

public:
    Async(std::function<void()> job, Sync *pSync = NULL);
    ~Async();
    static void Wait(Sync *pSync);
};

class AsyncPool
{
    std::vector<Async *> m_pool;
public:
    ~AsyncPool();
    void Flush();
    void AddAsyncTask(std::function<void()> job, Sync *pSync = NULL);
};

void ExecAsyncIfThereIsAPool(AsyncPool *pAsyncPool, std::function<void()> job);
