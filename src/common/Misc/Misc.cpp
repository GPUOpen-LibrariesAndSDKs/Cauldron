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

#include "stdafx.h"
#include "Misc.h"

//
// Get current time in milliseconds
//
double MillisecondsNow()
{
    static LARGE_INTEGER s_frequency;
    static BOOL s_use_qpc = QueryPerformanceFrequency(&s_frequency);
    double milliseconds = 0;

    if (s_use_qpc)
    {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        milliseconds = double(1000.0 * now.QuadPart) / s_frequency.QuadPart;
    }
    else
    {
        milliseconds = double(GetTickCount());
    }

    return milliseconds;
}

class MessageBuffer
{
public:
    MessageBuffer(size_t len) :
        m_dynamic(len > STATIC_LEN ? len : 0),
        m_ptr(len > STATIC_LEN ? m_dynamic.data() : m_static)
    {
    }
    char* Data() { return m_ptr; }

private:
    static const size_t STATIC_LEN = 256;
    char m_static[STATIC_LEN];
    std::vector<char> m_dynamic;
    char* m_ptr;
};

//
// Formats a string
//
std::string format(const char* format, ...)
{
    va_list args;
    va_start(args, format);
#ifndef _MSC_VER
    size_t size = std::snprintf(nullptr, 0, format, args) + 1; // Extra space for '\0'
    MessageBuffer buf(size);
    std::vsnprintf(buf.Data(), size, format, args);
    va_end(args);
    return std::string(buf.Data(), buf.Data() + size - 1); // We don't want the '\0' inside
#else
    const size_t size = (size_t)_vscprintf(format, args) + 1;
    MessageBuffer buf(size);
    vsnprintf_s(buf.Data(), size, _TRUNCATE, format, args);
    va_end(args);
    return std::string(buf.Data(), buf.Data() + size - 1);
#endif
}

void Trace(const std::string &str)
{
    std::mutex mutex;
    std::unique_lock<std::mutex> lock(mutex);
    OutputDebugStringA(str.c_str());
}

void Trace(const char* pFormat, ...)
{
    std::mutex mutex;
    std::unique_lock<std::mutex> lock(mutex);

    va_list args;

    // generate formatted string
    va_start(args, pFormat);
    const size_t bufLen = (size_t)_vscprintf(pFormat, args) + 2;
    MessageBuffer buf(bufLen);
    vsnprintf_s(buf.Data(), bufLen, bufLen, pFormat, args);
    va_end(args);
    strcat_s(buf.Data(), bufLen, "\n");

    OutputDebugStringA(buf.Data());
}

//
//  Reads a file into a buffer
//
bool ReadFile(const char *name, char **data, size_t *size, bool isbinary)
{
    FILE *file;

    //Open file
    if (fopen_s(&file, name, isbinary ? "rb" : "r") != 0)
    {
        return false;
    }

    //Get file length
    fseek(file, 0, SEEK_END);
    size_t fileLen = ftell(file);
    fseek(file, 0, SEEK_SET);

    // if ascii add one more char to accomodate for the \0
    if (!isbinary)
        fileLen++;

    //Allocate memory
    char *buffer = (char *)malloc(std::max<size_t>(fileLen, 1));
    if (!buffer)
    {
        fclose(file);
        return false;
    }

    //Read file contents into buffer
    size_t bytesRead = 0;
    if(fileLen > 0)
    {
        bytesRead = fread(buffer, 1, fileLen, file);
    }
    fclose(file);

    if (!isbinary)
    {
        buffer[bytesRead] = 0;    
        fileLen = bytesRead;
    }

    *data = buffer;
    if (size != NULL)
        *size = fileLen;

    return true;
}

bool SaveFile(const char *name, void const*data, size_t size, bool isbinary)
{
    FILE *file;
    if (fopen_s(&file, name, isbinary ? "wb" : "w") == 0)
    {
        fwrite(data, size, 1, file);
        fclose(file);
        return true;
    }

    return false;
}



//
// Launch a process, captures stderr into a file
//
bool LaunchProcess(const char* commandLine, const char* filenameErr)
{
    char cmdLine[1024];
    strcpy_s<1024>(cmdLine, commandLine);

    // create a pipe to get possible errors from the process
    //
    HANDLE g_hChildStd_OUT_Rd = NULL;
    HANDLE g_hChildStd_OUT_Wr = NULL;

    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;
    if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))
        return false;

    // launch process
    //
    PROCESS_INFORMATION pi = {};
    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdError = g_hChildStd_OUT_Wr;
    si.hStdOutput = g_hChildStd_OUT_Wr;
    si.wShowWindow = SW_HIDE;

    if (CreateProcessA(NULL, cmdLine, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
    {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(g_hChildStd_OUT_Wr);

        ULONG rc;
        if (GetExitCodeProcess(pi.hProcess, &rc))
        {
            if (rc == 0)
            {
                DeleteFileA(filenameErr);
                return true;
            }
            else
            {
                Trace(format("*** Process %s returned an error, see %s ***\n\n", commandLine, filenameErr));

                // save errors to disk
                std::ofstream ofs(filenameErr, std::ofstream::out);

                for (;;)
                {
                    DWORD dwRead;
                    char chBuf[2049];
                    BOOL bSuccess = ReadFile(g_hChildStd_OUT_Rd, chBuf, 2048, &dwRead, NULL);
                    chBuf[dwRead] = 0;
                    if (!bSuccess || dwRead == 0) break;

                    Trace(chBuf);

                    ofs << chBuf;
                }

                ofs.close();
            }
        }

        CloseHandle(g_hChildStd_OUT_Rd);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    else
    {
        Trace(format("*** Can't launch: %s \n", commandLine));
    }

    return false;
}

//
// Frustrum culls an AABB. The culling is done in clip space. 
//
bool CameraFrustumToBoxCollision(const XMMATRIX mCameraViewProj, const XMVECTOR boxCenter, const XMVECTOR boxExtent)
{
    float ex = XMVectorGetX(boxExtent);
    float ey = XMVectorGetY(boxExtent);
    float ez = XMVectorGetZ(boxExtent);

    XMVECTOR p[8];
    p[0] = XMVector4Transform((boxCenter + XMVectorSet(ex, ey, ez, 0)), mCameraViewProj);
    p[1] = XMVector4Transform((boxCenter + XMVectorSet(ex, ey, -ez, 0)), mCameraViewProj);
    p[2] = XMVector4Transform((boxCenter + XMVectorSet(ex, -ey, ez, 0)), mCameraViewProj);
    p[3] = XMVector4Transform((boxCenter + XMVectorSet(ex, -ey, -ez, 0)), mCameraViewProj);
    p[4] = XMVector4Transform((boxCenter + XMVectorSet(-ex, ey, ez, 0)), mCameraViewProj);
    p[5] = XMVector4Transform((boxCenter + XMVectorSet(-ex, ey, -ez, 0)), mCameraViewProj);
    p[6] = XMVector4Transform((boxCenter + XMVectorSet(-ex, -ey, ez, 0)), mCameraViewProj);
    p[7] = XMVector4Transform((boxCenter + XMVectorSet(-ex, -ey, -ez, 0)), mCameraViewProj);

    uint32_t left = 0;
    uint32_t right = 0;
    uint32_t top = 0;
    uint32_t bottom = 0;
    uint32_t back = 0;
    for (int i = 0; i < 8; i++)
    {
        float x = XMVectorGetX(p[i]);
        float y = XMVectorGetY(p[i]);
        float z = XMVectorGetZ(p[i]);
        float w = XMVectorGetW(p[i]);

        if (x < -w) left++;
        if (x > w) right++;
        if (y < -w) bottom++;
        if (y > w) top++;
        if (z < 0) back++;
    }

    return left == 8 || right == 8 || top == 8 || bottom == 8 || back == 8;
}
