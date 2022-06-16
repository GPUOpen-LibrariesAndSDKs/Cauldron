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
#include "Camera.h"
#include "Misc.h"

Camera::Camera()
{
    m_PrevProj = math::Matrix4::identity();
    m_Proj = math::Matrix4::identity();
    m_PrevView = math::Matrix4::identity();
    m_View = math::Matrix4::identity();
    m_PrevProjJittered = math::Matrix4::identity();
    m_ProjJittered = math::Matrix4::identity();
    m_eyePos = math::Vector4(0, 0, 0, 0);
    m_distance = -1;
    m_speed = 0;
    m_LastMoveDir = math::Vector4(0, 0, 0, 0);
}

//--------------------------------------------------------------------------------------
//
// OnCreate 
//
//--------------------------------------------------------------------------------------
void Camera::SetFov(float fovV, uint32_t width, uint32_t height, float nearPlane, float farPlane)
{
    SetFov(fovV, width * 1.f / height, nearPlane, farPlane);
}

void Camera::SetFov(float fovV, float aspectRatio, float nearPlane, float farPlane)
{
    m_aspectRatio = aspectRatio;

    m_near = nearPlane;
    m_far = farPlane;

    m_fovV = fovV;
    m_fovH = std::min<float>(m_fovV * aspectRatio, XM_PI / 2.0f);
    m_fovV = m_fovH / aspectRatio;

    m_Proj = math::Matrix4::perspective(fovV, m_aspectRatio, nearPlane, farPlane);
    m_ProjJittered = m_Proj;
}

void Camera::SetFov(float fovV, uint32_t width, uint32_t height, float nearPlane)
{
    SetFov(fovV, width * 1.f / height, nearPlane);
}

void Camera::SetFov(float fovV, float aspectRatio, float nearPlane)
{
    m_aspectRatio = aspectRatio;

    m_near = nearPlane;
    m_far = FLT_MAX;

    m_fovV = fovV;
    m_fovH = std::min<float>(m_fovV * aspectRatio, XM_PI / 2.0f);
    m_fovV = m_fovH / aspectRatio;

    const float cotHalfFovY = cosf(0.5f * m_fovV) / sinf(0.5f * m_fovV);
    const float m00 = cotHalfFovY / aspectRatio;
    const float m11 = cotHalfFovY;

    math::Vector4 c0(m00, 0.f, 0.f, 0.f);
    math::Vector4 c1(0.f, m11, 0.f, 0.f);
    math::Vector4 c2(0.f, 0.f, 0.f, -1.f);
    math::Vector4 c3(0.f, 0.f, m_near, 0.f);

    m_Proj.setCol0(c0);
    m_Proj.setCol1(c1);
    m_Proj.setCol2(c2);
    m_Proj.setCol3(c3);

    m_ProjJittered = m_Proj;
}

void Camera::SetMatrix(const math::Matrix4& cameraMatrix)
{
    m_eyePos = cameraMatrix.getCol3();
    LookAt(m_eyePos, m_eyePos + cameraMatrix * math::Vector4(0, 0, 1, 0));
}

//--------------------------------------------------------------------------------------
//
// LookAt, use this functions before calling update functions
//
//--------------------------------------------------------------------------------------
void Camera::LookAt(const math::Vector4& eyePos, const math::Vector4& lookAt)
{
    m_eyePos = eyePos;
    m_View = LookAtRH(eyePos, lookAt);
    m_distance = math::SSE::length(lookAt - eyePos);

	math::Vector4 zBasis = m_View.getRow(2);
	m_yaw = atan2f(zBasis.getX(), zBasis.getZ());
	float fLen = sqrtf(zBasis.getZ() * zBasis.getZ() + zBasis.getX() * zBasis.getX());
	m_pitch = atan2f(zBasis.getY(), fLen);
}

void Camera::LookAt(float yaw, float pitch, float distance, const math::Vector4& at )
{   
    LookAt(at + PolarToVector(yaw, pitch)*distance, at);
}

//--------------------------------------------------------------------------------------
//
// UpdateCamera
//
//--------------------------------------------------------------------------------------
void Camera::UpdateCameraWASD(float yaw, float pitch, const bool keyDown[256], double deltaTime)
{   
    auto movedir = MoveWASD(keyDown);
    // Smooth movement
    if (abs(movedir.getX()) > 0.1f || abs(movedir.getY()) > 0.1f || abs(movedir.getZ()) > 0.1f) {
      if (keyDown[VK_SHIFT])
        m_speed = m_speed + (5.0f - m_speed) * (1.0f - std::exp(-(float)deltaTime*1.0f));
      else
        m_speed = m_speed + (2.0f - m_speed) * (1.0f - std::exp(-(float)deltaTime*3.0f));
      
    }
    else
      m_speed = m_speed + (0.0f - m_speed) * (1.0f - std::exp(-(float)deltaTime*8.0f));

    m_LastMoveDir = m_LastMoveDir + (movedir - m_LastMoveDir) * (1.0f - std::exp(-(float)deltaTime*3.0f));

    m_eyePos += math::transpose(m_View) * (m_LastMoveDir * m_speed * (float)deltaTime);
    math::Vector4 dir = PolarToVector(yaw, pitch) * m_distance;
    LookAt(GetPosition(), GetPosition() - dir);
}

void Camera::UpdateCameraPolar(float yaw, float pitch, float x, float y, float distance, const bool *keyDown, double deltaTime)
{
    pitch = std::max(-XM_PIDIV2 + 1e-3f, std::min(pitch, XM_PIDIV2 - 1e-3f));

    // Trucks camera, moves the camera parallel to the view plane.
    m_eyePos += GetSide() * x * distance / 10.0f;
    m_eyePos += GetUp() * y * distance / 10.0f;

    // Orbits camera, rotates a camera about the target
    math::Vector4 dir = GetDirection();
    math::Vector4 pol = PolarToVector(yaw, pitch);

    if (keyDown)
        m_eyePos += math::transpose(m_View) * (MoveWASD(keyDown) * m_speed * (float)deltaTime);

    math::Vector4 at = m_eyePos - (dir * m_distance);

    LookAt(at + (pol * distance), at);
}

//--------------------------------------------------------------------------------------
//
// SetProjectionJitter
//
//--------------------------------------------------------------------------------------

void Camera::SetProjectionJitter(float jitterX, float jitterY)
{
    math::Matrix4 jitterMat(math::Matrix3::identity(), Vectormath::Vector3(jitterX, jitterY, 0));
    m_ProjJittered = jitterMat * m_Proj;
}

void Camera::SetProjectionJitter(uint32_t width, uint32_t height, uint32_t &sampleIndex)
{
    static const auto CalculateHaltonNumber = [](uint32_t index, uint32_t base)
    {
        float f = 1.0f, result = 0.0f;

        for (uint32_t i = index; i > 0;)
        {
            f /= static_cast<float>(base);
            result = result + f * static_cast<float>(i % base);
            i = static_cast<uint32_t>(floorf(static_cast<float>(i) / static_cast<float>(base)));
        }

        return result;
    };

    sampleIndex = (sampleIndex + 1) % 16;   // 16x TAA

    float jitterX = CalculateHaltonNumber(sampleIndex + 1, 2) - 0.5f;
    float jitterY = CalculateHaltonNumber(sampleIndex + 1, 3) - 0.5f;

    jitterX /= static_cast<float>(width);
    jitterY /= static_cast<float>(height);

    SetProjectionJitter(jitterX, jitterY);
}

//--------------------------------------------------------------------------------------
//
// Get a vector pointing in the direction of yaw and pitch
//
//--------------------------------------------------------------------------------------
math::Vector4 PolarToVector(float yaw, float pitch)
{
    return math::Vector4(sinf(yaw) * cosf(pitch), sinf(pitch), cosf(yaw) * cosf(pitch), 0);
}

math::Matrix4 LookAtRH(const math::Vector4& eyePos, const math::Vector4& lookAt)
{
    return math::Matrix4::lookAt(math::toPoint3(eyePos), math::toPoint3(lookAt), math::Vector3(0, 1, 0));
}

math::Vector4 MoveWASD(const bool keyDown[256])
{ 
    float scale = 1.0f;
    float x = 0, y = 0, z = 0;

    if (keyDown['W'])
    {
        z = -scale;
    }
    if (keyDown['S'])
    {
        z = scale;
    }
    if (keyDown['A'])
    {
        x = -scale;
    }
    if (keyDown['D'])
    {
        x = scale;
    }
    if (keyDown['E'])
    {
        y = scale;
    }
    if (keyDown['Q'])
    {
        y = -scale;
    }

    float len = std::max(1.0e-3f, sqrtf(x * x + y * y + z * z));

    return math::Vector4(x / len, y / len, z / len, 0.0f);
}
