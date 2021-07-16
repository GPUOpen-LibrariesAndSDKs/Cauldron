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

Texture2D Input : register(t0);
Texture2D MotionVectors : register(t1);
RWTexture2D<float4> Result : register(u0);

[numthreads(WIDTH, HEIGHT, DEPTH)]
void main(uint3 threadID : SV_DispatchThreadID)
{
    uint2 coord = threadID.xy;

/*    
    int steps = 25;

    float2 velocity = -1000 * MotionVectors.Load(int3(coord, 0)) / float(steps);
    velocity = float2(velocity.x, -velocity.y);

    float4 vColor = float4(0, 0, 0, 0);

    for (int i = 0; i < steps; ++i)
    {
        vColor += Input.Load(int3(coord + ((float)i)*velocity, 0));
    }

    Result[coord] = vColor / float(steps);
*/
    float ax = ((float) coord.x) / 20.0;
    float ay = ((float) coord.y) / 20.0;
    int2 distortion = int2(20 * sin(ay), 20 * cos(ax));
    
    Result[coord] = Input.Load(int3(coord + distortion, 0));
}