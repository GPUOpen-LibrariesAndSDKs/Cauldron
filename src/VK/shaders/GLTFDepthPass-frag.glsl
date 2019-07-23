#version 450

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

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

precision highp float;

#ifdef ID_4PS_TEXCOORD_0
    layout (location = ID_4PS_TEXCOORD_0) in  vec2 v_UV;
#endif

#ifdef ID_baseColorTexture
    layout (set=1, binding = ID_baseColorTexture) uniform sampler2D u_BaseColorSampler;
#endif

void main()
{
#ifdef ID_baseColorTexture  
    vec4 baseColor = texture(u_BaseColorSampler, v_UV);

    #if defined(DEF_alphaMode_BLEND) 
        if (baseColor.a == 0)
            discard;
    #elif defined(DEF_alphaMode_MASK) && defined(DEF_alphaCutoff)
        if (baseColor.a < DEF_alphaCutoff)
            discard;
    #else 
        //OPAQUE
    #endif
#endif
}
