// AMD Workgraph Rasterizer code
// 
// Copyright(c) 2024 Advanced Micro Devices, Inc.All rights reserved.
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

#define	min3(x, y, z)	min(min(x, y), z)
#define	max3(x, y, z)	max(max(x, y), z)

//--------------------------------------------------------------------------------------

// Depends on mad() being implemented as a fma()
float DifferenceOfProducts(in float a, in float b, in float c, in float d)
{
#if PRECISE && FMA
	// From pbrt: https://pharr.org/matt/blog/2019/11/03/difference-of-floats
	float cd = c * d;
	float err = mad(-c, d, cd);
	float dop = mad(a, b, -cd);
	return dop + err;
#else
	return a * b - c * d;
#endif
}

float3 SafeCross(in float3 v1, in float3 v2)
{
	return float3(
		DifferenceOfProducts(v1.y, v2.z, v1.z, v2.y),
		DifferenceOfProducts(v1.z, v2.x, v1.x, v2.z),
		DifferenceOfProducts(v1.x, v2.y, v1.y, v2.x)
	);
}

float SafeDeterminant(in float3 v0, in float3 v1, in float3 v2)
{
	return dot(v0, SafeCross(v1, v2));
}

void SafeInverse(in float det, in float3 v0, in float3 v1, in float3 v2, out float3 o0, out float3 o1, out float3 o2)
{
#if !PRECISE
	det = 1.0f / det;
#endif

	o0.x = DifferenceOfProducts(v1.y, v2.z, v2.y, v1.z);
	o0.y = DifferenceOfProducts(v0.z, v2.y, v0.y, v2.z);
	o0.z = DifferenceOfProducts(v0.y, v1.z, v0.z, v1.y);

	o1.x = DifferenceOfProducts(v1.z, v2.x, v1.x, v2.z);
	o1.y = DifferenceOfProducts(v0.x, v2.z, v0.z, v2.x);
	o1.z = DifferenceOfProducts(v1.x, v0.z, v0.x, v1.z);

	o2.x = DifferenceOfProducts(v1.x, v2.y, v2.x, v1.y);
	o2.y = DifferenceOfProducts(v2.x, v0.y, v0.x, v2.y);
	o2.z = DifferenceOfProducts(v0.x, v1.y, v1.x, v0.y);

#if PRECISE
	o0 /= det;
	o1 /= det;
	o2 /= det;
#else
	o0 *= det;
	o1 *= det;
	o2 *= det;
#endif
}
