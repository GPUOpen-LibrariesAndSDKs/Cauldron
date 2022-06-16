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

#pragma once

#include "../../libs/vectormath/vectormath.hpp"

class Camera
{
public:
    Camera();
    void SetMatrix(const math::Matrix4& cameraMatrix);
    void LookAt(const math::Vector4& eyePos, const math::Vector4& lookAt);
    void LookAt(float yaw, float pitch, float distance, const math::Vector4& at);
    void SetFov(float fov, uint32_t width, uint32_t height, float nearPlane, float farPlane);
    void SetFov(float fov, float aspectRatio, float nearPlane, float farPlane);
    void SetFov(float fov, uint32_t width, uint32_t height, float nearPlane);   // inverted depth with infinite zFar
    void SetFov(float fov, float aspectRatio, float nearPlane);                 // inverted depth with infinite zFar
    void UpdateCameraPolar(float yaw, float pitch, float x, float y, float distance, const bool *keyDown = NULL /* keyDown[256] */, double deltaTime = 0.0);
    void UpdateCameraWASD(float yaw, float pitch, const bool keyDown[256], double deltaTime);

    math::Matrix4 GetView() const { return m_View; }
    math::Matrix4 GetPrevView() const { return m_PrevView; }
    math::Matrix4 GetViewport() const { return m_Viewport; }
    math::Vector4 GetPosition() const { return m_eyePos;   }

    math::Vector4 GetDirection()    const { return math::Vector4((math::transpose(m_View) * math::Vector4(0.0f, 0.0f, 1.0f, 0.0f)).getXYZ(), 0); }
    math::Vector4 GetUp()           const { return math::Vector4((math::transpose(m_View) * math::Vector4(0.0f, 1.0f, 0.0f, 0.0f)).getXYZ(), 0); }
    math::Vector4 GetSide()         const { return math::Vector4((math::transpose(m_View) * math::Vector4(1.0f, 1.0f, 0.0f, 0.0f)).getXYZ(), 0); }
    math::Matrix4 GetProjection()   const { return m_ProjJittered; }
    math::Matrix4 GetPrevProjection()   const { return m_PrevProjJittered; }

    float GetFovH() const { return m_fovH; }
    float GetFovV() const { return m_fovV; }

    float GetAspectRatio() const { return m_aspectRatio; }

    float GetNearPlane() const { return m_near; }
    float GetFarPlane() const { return m_far; }

    float GetYaw() const { return m_yaw; }
    float GetPitch() const { return m_pitch; }
    float GetDistance() const { return m_distance; }

    void SetSpeed( float speed ) { m_speed = speed; }
    void SetProjectionJitter(float jitterX, float jitterY);
    void SetProjectionJitter(uint32_t width, uint32_t height, uint32_t &seed);
    void UpdatePreviousMatrices() { m_PrevView = m_View; m_PrevProj = m_Proj; m_PrevProjJittered = m_ProjJittered;  }

private:
    math::Vector4       m_LastMoveDir;
    math::Matrix4       m_View;
    math::Matrix4       m_Proj;
    math::Matrix4       m_ProjJittered;
    math::Matrix4       m_PrevView;
    math::Matrix4       m_PrevProj;
    math::Matrix4       m_PrevProjJittered;
    math::Matrix4       m_Viewport;
    math::Vector4       m_eyePos;
    float               m_distance;
    float               m_fovV, m_fovH;
    float               m_near, m_far;
    float               m_aspectRatio;

    float               m_speed = 1.0f;
    float               m_yaw = 0.0f;
    float               m_pitch = 0.0f;
    float               m_roll = 0.0f;
};

math::Vector4 PolarToVector(float roll, float pitch);
math::Matrix4 LookAtRH(const math::Vector4& eyePos, const math::Vector4& lookAt);
math::Vector4 MoveWASD(const bool keyDown[256]);
