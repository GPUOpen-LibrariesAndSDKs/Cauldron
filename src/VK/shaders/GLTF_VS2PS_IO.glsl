// AMD Cauldron code
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

#ifndef VS2PS_struct
#define VS2PS_struct

struct VS2PS
{
#ifdef ID_COLOR_0
    vec3 Color0;
#endif

#ifdef ID_TEXCOORD_0
    vec2 UV0;
#endif

#ifdef ID_TEXCOORD_1
    vec2 UV1;
#endif

#ifdef ID_NORMAL
    vec3 Normal;
#endif

#ifdef ID_TANGENT
    vec3 Tangent;
    vec3 Binormal;
#endif

	vec3 WorldPos;

#ifdef HAS_MOTION_VECTORS
	vec4 CurrPosition;
	vec4 PrevPosition;
#endif

};

#endif
