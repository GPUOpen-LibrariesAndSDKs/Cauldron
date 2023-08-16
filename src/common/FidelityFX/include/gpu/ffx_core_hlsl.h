// This file is part of the FidelityFX SDK.
//
// Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

/// Pack 2x32-bit floating point values in a single 32bit value.
/// 
/// This function first converts each component of <c><i>value</i></c> into their nearest 16-bit floating
/// point representation, and then stores the X and Y components in the lower and upper 16 bits of the
/// 32bit unsigned integer respectively.
///
/// @param [in] value               A 2-dimensional floating point value to convert and pack.
/// 
/// @returns
/// A packed 32bit value containing 2 16bit floating point values.
/// 
/// @ingroup HLSL
FfxUInt32 packHalf2x16(FfxFloat32x2 value)
{
    return f32tof16(value.x) | (f32tof16(value.y) << 16);
}

/// Broadcast a scalar value to a 2-dimensional floating point vector.
///
/// @param [in] value               The value to to broadcast.
///
/// @returns
/// A 2-dimensional floating point vector with <c><i>value</i></c> in each component.
///
/// @ingroup HLSL
FfxFloat32x2 broadcast2(FfxFloat32 value)
{
    return FfxFloat32x2(value, value);
}

/// Broadcast a scalar value to a 3-dimensional floating point vector.
///
/// @param [in] value               The value to to broadcast.
///
/// @returns
/// A 3-dimensional floating point vector with <c><i>value</i></c> in each component.
///
/// @ingroup HLSL
FfxFloat32x3 broadcast3(FfxFloat32 value)
{
    return FfxFloat32x3(value, value, value);
}

/// Broadcast a scalar value to a 4-dimensional floating point vector.
///
/// @param [in] value               The value to to broadcast.
///
/// @returns
/// A 4-dimensional floating point vector with <c><i>value</i></c> in each component.
///
/// @ingroup HLSL
FfxFloat32x4 broadcast4(FfxFloat32 value)
{
    return FfxFloat32x4(value, value, value, value);
}

/// Broadcast a scalar value to a 2-dimensional unsigned integer vector.
///
/// @param [in] value               The value to to broadcast.
///
/// @returns
/// A 2-dimensional unsigned integer vector with <c><i>value</i></c> in each component.
///
/// @ingroup HLSL
FfxUInt32x2 broadcast2(FfxUInt32 value)
{
    return FfxUInt32x2(value, value);
}

/// Broadcast a scalar value to a 3-dimensional unsigned integer vector.
///
/// @param [in] value               The value to to broadcast.
///
/// @returns
/// A 3-dimensional unsigned integer vector with <c><i>value</i></c> in each component.
///
/// @ingroup HLSL
FfxUInt32x3 broadcast3(FfxUInt32 value)
{
    return FfxUInt32x3(value, value, value);
}

/// Broadcast a scalar value to a 4-dimensional unsigned integer vector.
///
/// @param [in] value               The value to to broadcast.
///
/// @returns
/// A 4-dimensional unsigned integer vector with <c><i>value</i></c> in each component.
///
/// @ingroup HLSL
FfxUInt32x4 broadcast4(FfxUInt32 value)
{
    return FfxUInt32x4(value, value, value, value);
}

FfxUInt32 bitfieldExtract(FfxUInt32 src, FfxUInt32 off, FfxUInt32 bits)
{
    FfxUInt32 mask = (1u << bits) - 1;
    return (src >> off) & mask;
}

FfxUInt32 bitfieldInsert(FfxUInt32 src, FfxUInt32 ins, FfxUInt32 mask)
{
    return (ins & mask) | (src & (~mask));
}

FfxUInt32 bitfieldInsertMask(FfxUInt32 src, FfxUInt32 ins, FfxUInt32 bits)
{
    FfxUInt32 mask = (1u << bits) - 1;
    return (ins & mask) | (src & (~mask));
}

/// Compute the factional part of a decimal value.
///
/// This function calculates <c><i>x - floor(x)</i></c>. Where <c><i>floor</i></c> is the intrinsic HLSL function.
///
/// NOTE: This function should compile down to a single <c><i>V_MAX3_F32</i></c> operation on GCN/RDNA hardware. It is
/// worth further noting that this function is intentionally distinct from the HLSL <c><i>frac</i></c> intrinsic
/// function.
///
/// @param [in] x               The value to compute the fractional part from.
/// 
/// @returns
/// The fractional part of <c><i>x</i></c>.
///
/// @ingroup HLSL
FfxFloat32 fract(FfxFloat32 x)
{
    return x - floor(x);
}

/// Compute the factional part of a decimal value.
///
/// This function calculates <c><i>x - floor(x)</i></c>. Where <c><i>floor</i></c> is the intrinsic HLSL function.
///
/// NOTE: This function should compile down to a single <c><i>V_MAX3_F32</i></c> operation on GCN/RDNA hardware. It is
/// worth further noting that this function is intentionally distinct from the HLSL <c><i>frac</i></c> intrinsic
/// function.
///
/// @param [in] x               The value to compute the fractional part from.
///
/// @returns
/// The fractional part of <c><i>x</i></c>.
///
/// @ingroup HLSL
FfxFloat32x2 fract(FfxFloat32x2 x)
{
    return x - floor(x);
}

/// Compute the factional part of a decimal value.
///
/// This function calculates <c><i>x - floor(x)</i></c>. Where <c><i>floor</i></c> is the intrinsic HLSL function.
///
/// NOTE: This function should compile down to a single <c><i>V_MAX3_F32</i></c> operation on GCN/RDNA hardware. It is
/// worth further noting that this function is intentionally distinct from the HLSL <c><i>frac</i></c> intrinsic
/// function.
///
/// @param [in] x               The value to compute the fractional part from.
///
/// @returns
/// The fractional part of <c><i>x</i></c>.
///
/// @ingroup HLSL
FfxFloat32x3 fract(FfxFloat32x3 x)
{
    return x - floor(x);
}

/// Compute the factional part of a decimal value.
///
/// This function calculates <c><i>x - floor(x)</i></c>. Where <c><i>floor</i></c> is the intrinsic HLSL function.
///
/// NOTE: This function should compile down to a single <c><i>V_MAX3_F32</i></c> operation on GCN/RDNA hardware. It is
/// worth further noting that this function is intentionally distinct from the HLSL <c><i>frac</i></c> intrinsic 
/// function.
///
/// @param [in] x               The value to compute the fractional part from.
///
/// @returns
/// The fractional part of <c><i>x</i></c>.
///
/// @ingroup HLSL
FfxFloat32x4 fract(FfxFloat32x4 x)
{
    return x - floor(x);
}

/// Compute the maximum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MAX3_F32</i></c> operation on GCN/RDNA hardware.
/// 
/// @param [in] x               The first value to include in the max calculation.
/// @param [in] y               The second value to include in the max calcuation.
/// @param [in] z               The third value to include in the max calcuation.
/// 
/// @returns
/// The maximum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSL
FfxFloat32 ffxMax3(FfxFloat32 x, FfxFloat32 y, FfxFloat32 z)
{
    return max(x, max(y, z));
}

/// Compute the maximum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MAX3_F32</i></c> operation on GCN/RDNA hardware.
/// 
/// @param [in] x               The first value to include in the max calculation.
/// @param [in] y               The second value to include in the max calcuation.
/// @param [in] z               The third value to include in the max calcuation.
///
/// @returns
/// The maximum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSL
FfxFloat32x2 ffxMax3(FfxFloat32x2 x, FfxFloat32x2 y, FfxFloat32x2 z)
{
    return max(x, max(y, z));
}

/// Compute the maximum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MAX3_F32</i></c> operation on GCN/RDNA hardware.
/// 
/// @param [in] x               The first value to include in the max calculation.
/// @param [in] y               The second value to include in the max calcuation.
/// @param [in] z               The third value to include in the max calcuation.
///
/// @returns
/// The maximum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSL
FfxFloat32x3 ffxMax3(FfxFloat32x3 x, FfxFloat32x3 y, FfxFloat32x3 z)
{
    return max(x, max(y, z));
}

/// Compute the maximum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MAX3_F32</i></c> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the max calculation.
/// @param [in] y               The second value to include in the max calcuation.
/// @param [in] z               The third value to include in the max calcuation.
///
/// @returns
/// The maximum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSL
FfxFloat32x4 ffxMax3(FfxFloat32x4 x, FfxFloat32x4 y, FfxFloat32x4 z)
{
    return max(x, max(y, z));
}

/// Compute the maximum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MAX3_F32</i></c> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the max calculation.
/// @param [in] y               The second value to include in the max calcuation.
/// @param [in] z               The third value to include in the max calcuation.
///
/// @returns
/// The maximum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSL
FfxUInt32 ffxMax3(FfxUInt32 x, FfxUInt32 y, FfxUInt32 z)
{
    return max(x, max(y, z));
}

/// Compute the maximum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MAX3_F32</i></c> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the max calculation.
/// @param [in] y               The second value to include in the max calcuation.
/// @param [in] z               The third value to include in the max calcuation.
///
/// @returns
/// The maximum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSL
FfxUInt32x2 ffxMax3(FfxUInt32x2 x, FfxUInt32x2 y, FfxUInt32x2 z)
{
    return max(x, max(y, z));
}

/// Compute the maximum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MAX3_F32</i></c> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the max calculation.
/// @param [in] y               The second value to include in the max calcuation.
/// @param [in] z               The third value to include in the max calcuation.
///
/// @returns
/// The maximum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSL
FfxUInt32x3 ffxMax3(FfxUInt32x3 x, FfxUInt32x3 y, FfxUInt32x3 z)
{
    return max(x, max(y, z));
}

/// Compute the maximum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MAX3_F32</i></c> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the max calculation.
/// @param [in] y               The second value to include in the max calcuation.
/// @param [in] z               The third value to include in the max calcuation.
///
/// @returns
/// The maximum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSL
FfxUInt32x4 ffxMax3(FfxUInt32x4 x, FfxUInt32x4 y, FfxUInt32x4 z)
{
    return max(x, max(y, z));
}

/// Compute the median of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MED3_F32</i></c> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the median calculation.
/// @param [in] y               The second value to include in the median calcuation.
/// @param [in] z               The third value to include in the median calcuation.
///
/// @returns
/// The median value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSL
FfxFloat32 ffxMed3(FfxFloat32 x, FfxFloat32 y, FfxFloat32 z)
{
    return max(min(x, y), min(max(x, y), z));
}

/// Compute the median of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MED3_F32</i></c> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the median calculation.
/// @param [in] y               The second value to include in the median calcuation.
/// @param [in] z               The third value to include in the median calcuation.
///
/// @returns
/// The median value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSL
FfxFloat32x2 ffxMed3(FfxFloat32x2 x, FfxFloat32x2 y, FfxFloat32x2 z)
{
    return max(min(x, y), min(max(x, y), z));
}

/// Compute the median of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MED3_F32</i></c> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the median calculation.
/// @param [in] y               The second value to include in the median calcuation.
/// @param [in] z               The third value to include in the median calcuation.
///
/// @returns
/// The median value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSL
FfxFloat32x3 ffxMed3(FfxFloat32x3 x, FfxFloat32x3 y, FfxFloat32x3 z)
{
    return max(min(x, y), min(max(x, y), z));
}

/// Compute the median of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MED3_F32</i></c> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the median calculation.
/// @param [in] y               The second value to include in the median calcuation.
/// @param [in] z               The third value to include in the median calcuation.
///
/// @returns
/// The median value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSL
FfxFloat32x4 ffxMed3(FfxFloat32x4 x, FfxFloat32x4 y, FfxFloat32x4 z)
{
    return max(min(x, y), min(max(x, y), z));
}

/// Compute the minimum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MIN3_F32</i></c> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the min calculation.
/// @param [in] y               The second value to include in the min calcuation.
/// @param [in] z               The third value to include in the min calcuation.
///
/// @returns
/// The minimum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSL
FfxFloat32 ffxMin3(FfxFloat32 x, FfxFloat32 y, FfxFloat32 z)
{
    return min(x, min(y, z));
}

/// Compute the minimum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MIN3_F32</i></c> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the min calculation.
/// @param [in] y               The second value to include in the min calcuation.
/// @param [in] z               The third value to include in the min calcuation.
///
/// @returns
/// The minimum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSL
FfxFloat32x2 ffxMin3(FfxFloat32x2 x, FfxFloat32x2 y, FfxFloat32x2 z)
{
    return min(x, min(y, z));
}

/// Compute the minimum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MIN3_F32</c></i> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the min calculation.
/// @param [in] y               The second value to include in the min calcuation.
/// @param [in] z               The third value to include in the min calcuation.
///
/// @returns
/// The minimum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSL
FfxFloat32x3 ffxMin3(FfxFloat32x3 x, FfxFloat32x3 y, FfxFloat32x3 z)
{
    return min(x, min(y, z));
}

/// Compute the minimum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MIN3_F32</c></i> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the min calculation.
/// @param [in] y               The second value to include in the min calcuation.
/// @param [in] z               The third value to include in the min calcuation.
///
/// @returns
/// The minimum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSL
FfxFloat32x4 ffxMin3(FfxFloat32x4 x, FfxFloat32x4 y, FfxFloat32x4 z)
{
    return min(x, min(y, z));
}

/// Compute the minimum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MIN3_F32</c></i> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the min calculation.
/// @param [in] y               The second value to include in the min calcuation.
/// @param [in] z               The third value to include in the min calcuation.
///
/// @returns
/// The minimum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSL
FfxUInt32 ffxMin3(FfxUInt32 x, FfxUInt32 y, FfxUInt32 z)
{
    return min(x, min(y, z));
}

/// Compute the minimum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MIN3_F32</c></i> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the min calculation.
/// @param [in] y               The second value to include in the min calcuation.
/// @param [in] z               The third value to include in the min calcuation.
///
/// @returns
/// The minimum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSL
FfxUInt32x2 ffxMin3(FfxUInt32x2 x, FfxUInt32x2 y, FfxUInt32x2 z)
{
    return min(x, min(y, z));
}

/// Compute the minimum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MIN3_F32</c></i> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the min calculation.
/// @param [in] y               The second value to include in the min calcuation.
/// @param [in] z               The third value to include in the min calcuation.
///
/// @returns
/// The minimum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSL
FfxUInt32x3 ffxMin3(FfxUInt32x3 x, FfxUInt32x3 y, FfxUInt32x3 z)
{
    return min(x, min(y, z));
}

/// Compute the minimum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MIN3_F32</c></i> operation on GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the min calculation.
/// @param [in] y               The second value to include in the min calcuation.
/// @param [in] z               The third value to include in the min calcuation.
///
/// @returns
/// The minimum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup HLSL
FfxUInt32x4 ffxMin3(FfxUInt32x4 x, FfxUInt32x4 y, FfxUInt32x4 z)
{
    return min(x, min(y, z));
}


FfxUInt32 AShrSU1(FfxUInt32 a, FfxUInt32 b)
{
    return FfxUInt32(FfxInt32(a) >> FfxInt32(b));
}

//==============================================================================================================================
//                                                          HLSL HALF
//==============================================================================================================================
#if defined(FFX_HALF)

//==============================================================================================================================
// Need to use manual unpack to get optimal execution (don't use packed types in buffers directly).
// Unpack requires this pattern: https://gpuopen.com/first-steps-implementing-fp16/
FfxFloat16x2 ffxUint32ToFloat16x2(FfxUInt32 x)
{
    FfxFloat32x2 t = f16tof32(FfxUInt32x2(x & 0xFFFF, x >> 16));
    return FfxFloat16x2(t);
}
FfxFloat16x4 ffxUint32x2ToFloat16x4(FfxUInt32x2 x)
{
    return FfxFloat16x4(ffxUint32ToFloat16x2(x.x), ffxUint32ToFloat16x2(x.y));
}
FfxUInt16x2 ffxUint32ToUint16x2(FfxUInt32 x)
{
    FfxUInt32x2 t = FfxUInt32x2(x & 0xFFFF, x >> 16);
    return FfxUInt16x2(t);
}
FfxUInt16x4 ffxUint32x2ToUint16x4(FfxUInt32x2 x)
{
    return FfxUInt16x4(ffxUint32ToUint16x2(x.x), ffxUint32ToUint16x2(x.y));
}
#define FFX_UINT32_TO_FLOAT16X2(x) ffxUint32ToFloat16x2(FfxUInt32(x))
#define FFX_UINT32X2_TO_FLOAT16X4(x) ffxUint32x2ToFloat16x4(FfxUInt32x2(x))
#define FFX_UINT32_TO_UINT16X2(x) ffxUint32ToUint16x2(FfxUInt32(x))
#define FFX_UINT32X2_TO_UINT16X4(x) ffxUint32x2ToUint16x4(FfxUInt32x2(x))
//------------------------------------------------------------------------------------------------------------------------------
FfxUInt32 ffxFloat16x2ToUint32(FfxFloat16x2 x)
{
    return f32tof16(x.x) + (f32tof16(x.y) << 16);
}
FfxUInt32x2 ffxFloat16x4ToUint32x2(FfxFloat16x4 x)
{
    return FfxUInt32x2(ffxFloat16x2ToUint32(x.xy), ffxFloat16x2ToUint32(x.zw));
}
FfxUInt32 ffxUint16x2ToUint32(FfxUInt16x2 x)
{
    return FfxUInt32(x.x) + (FfxUInt32(x.y) << 16);
}
FfxUInt32x2 ffxUint16x4ToUint32x2(FfxUInt16x4 x)
{
    return FfxUInt32x2(ffxUint16x2ToUint32(x.xy), ffxUint16x2ToUint32(x.zw));
}
#define FFX_FLOAT16X2_TO_UINT32(x) ffxFloat16x2ToUint32(FfxFloat16x2(x))
#define FFX_FLOAT16X4_TO_UINT32X2(x) ffxFloat16x4ToUint32x2(FfxFloat16x4(x))
#define FFX_UINT16X2_TO_UINT32(x) ffxUint16x2ToUint32(FfxUInt16x2(x))
#define FFX_UINT16X4_TO_UINT32X2(x) ffxUint16x4ToUint32x2(FfxUInt16x4(x))

#if defined(FFX_HLSL_6_2) && !defined(FFX_NO_16_BIT_CAST)
    #define FFX_TO_UINT16(x) asuint16(x)
    #define FFX_TO_UINT16X2(x) asuint16(x)
    #define FFX_TO_UINT16X3(x) asuint16(x)
    #define FFX_TO_UINT16X4(x) asuint16(x)
#else
    #define FFX_TO_UINT16(a) FfxUInt16(f32tof16(FfxFloat32(a)))
    #define FFX_TO_UINT16X2(a) FfxUInt16x2(FFX_TO_UINT16((a).x), FFX_TO_UINT16((a).y))
    #define FFX_TO_UINT16X3(a) FfxUInt16x3(FFX_TO_UINT16((a).x), FFX_TO_UINT16((a).y), FFX_TO_UINT16((a).z))
    #define FFX_TO_UINT16X4(a) FfxUInt16x4(FFX_TO_UINT16((a).x), FFX_TO_UINT16((a).y), FFX_TO_UINT16((a).z), FFX_TO_UINT16((a).w))
#endif // #if defined(FFX_HLSL_6_2) && !defined(FFX_NO_16_BIT_CAST)

#if defined(FFX_HLSL_6_2) && !defined(FFX_NO_16_BIT_CAST)
    #define FFX_TO_FLOAT16(x) asfloat16(x)
    #define FFX_TO_FLOAT16X2(x) asfloat16(x)
    #define FFX_TO_FLOAT16X3(x) asfloat16(x)
    #define FFX_TO_FLOAT16X4(x) asfloat16(x)
#else
    #define FFX_TO_FLOAT16(a) FfxFloat16(f16tof32(FfxUInt32(a)))
    #define FFX_TO_FLOAT16X2(a) FfxFloat16x2(FFX_TO_FLOAT16((a).x), FFX_TO_FLOAT16((a).y))
    #define FFX_TO_FLOAT16X3(a) FfxFloat16x3(FFX_TO_FLOAT16((a).x), FFX_TO_FLOAT16((a).y), FFX_TO_FLOAT16((a).z))
    #define FFX_TO_FLOAT16X4(a) FfxFloat16x4(FFX_TO_FLOAT16((a).x), FFX_TO_FLOAT16((a).y), FFX_TO_FLOAT16((a).z), FFX_TO_FLOAT16((a).w))
#endif // #if defined(FFX_HLSL_6_2) && !defined(FFX_NO_16_BIT_CAST)

//==============================================================================================================================
FfxFloat16 ffxBroadcastFloat16(FfxFloat16 a)
{
    return FfxFloat16(a);
}
FfxFloat16x2 ffxBroadcastFloat16x2(FfxFloat16 a)
{
    return FfxFloat16x2(a, a);
}
FfxFloat16x3 ffxBroadcastFloat16x3(FfxFloat16 a)
{
    return FfxFloat16x3(a, a, a);
}
FfxFloat16x4 ffxBroadcastFloat16x4(FfxFloat16 a)
{
    return FfxFloat16x4(a, a, a, a);
}
#define FFX_BROADCAST_FLOAT16(a) ffxBroadcastFloat16(FfxFloat16(a))
#define FFX_BROADCAST_FLOAT16X2(a) ffxBroadcastFloat16x2(FfxFloat16(a))
#define FFX_BROADCAST_FLOAT16X3(a) ffxBroadcastFloat16x3(FfxFloat16(a))
#define FFX_BROADCAST_FLOAT16X4(a) ffxBroadcastFloat16x4(FfxFloat16(a))
//------------------------------------------------------------------------------------------------------------------------------
FfxUInt16 ffxBroadcastUint16(FfxUInt16 a)
{
    return FfxUInt16(a);
}
FfxUInt16x2 ffxBroadcastUint16x2(FfxUInt16 a)
{
    return FfxUInt16x2(a, a);
}
FfxUInt16x3 ffxBroadcastUint16x3(FfxUInt16 a)
{
    return FfxUInt16x3(a, a, a);
}
FfxUInt16x4 ffxBroadcastUint16x4(FfxUInt16 a)
{
    return FfxUInt16x4(a, a, a, a);
}
#define FFX_BROADCAST_UINT16(a) ffxBroadcastUint16(FfxUInt16(a))
#define FFX_BROADCAST_UINT16X2(a) ffxBroadcastUint16x2(FfxUInt16(a))
#define FFX_BROADCAST_UINT16X3(a) ffxBroadcastUint16x3(FfxUInt16(a))
#define FFX_BROADCAST_UINT16X4(a) ffxBroadcastUint16x4(FfxUInt16(a))
//==============================================================================================================================
FfxUInt16 ffxAbsHalf(FfxUInt16 a)
{
    return FfxUInt16(abs(FfxInt16(a)));
}
FfxUInt16x2 ffxAbsHalf(FfxUInt16x2 a)
{
    return FfxUInt16x2(abs(FfxInt16x2(a)));
}
FfxUInt16x3 ffxAbsHalf(FfxUInt16x3 a)
{
    return FfxUInt16x3(abs(FfxInt16x3(a)));
}
FfxUInt16x4 ffxAbsHalf(FfxUInt16x4 a)
{
    return FfxUInt16x4(abs(FfxInt16x4(a)));
}
//------------------------------------------------------------------------------------------------------------------------------
FfxFloat16 ffxClampHalf(FfxFloat16 x, FfxFloat16 n, FfxFloat16 m)
{
    return max(n, min(x, m));
}
FfxFloat16x2 ffxClampHalf(FfxFloat16x2 x, FfxFloat16x2 n, FfxFloat16x2 m)
{
    return max(n, min(x, m));
}
FfxFloat16x3 ffxClampHalf(FfxFloat16x3 x, FfxFloat16x3 n, FfxFloat16x3 m)
{
    return max(n, min(x, m));
}
FfxFloat16x4 ffxClampHalf(FfxFloat16x4 x, FfxFloat16x4 n, FfxFloat16x4 m)
{
    return max(n, min(x, m));
}
//------------------------------------------------------------------------------------------------------------------------------
// V_FRACT_F16 (note DX frac() is different).
FfxFloat16 ffxFractHalf(FfxFloat16 x)
{
    return x - floor(x);
}
FfxFloat16x2 ffxFractHalf(FfxFloat16x2 x)
{
    return x - floor(x);
}
FfxFloat16x3 ffxFractHalf(FfxFloat16x3 x)
{
    return x - floor(x);
}
FfxFloat16x4 ffxFractHalf(FfxFloat16x4 x)
{
    return x - floor(x);
}
//------------------------------------------------------------------------------------------------------------------------------
FfxFloat16 ffxLerpHalf(FfxFloat16 x, FfxFloat16 y, FfxFloat16 a)
{
    return lerp(x, y, a);
}
FfxFloat16x2 ffxLerpHalf(FfxFloat16x2 x, FfxFloat16x2 y, FfxFloat16x2 a)
{
    return lerp(x, y, a);
}
FfxFloat16x3 ffxLerpHalf(FfxFloat16x3 x, FfxFloat16x3 y, FfxFloat16x3 a)
{
    return lerp(x, y, a);
}
FfxFloat16x4 ffxLerpHalf(FfxFloat16x4 x, FfxFloat16x4 y, FfxFloat16x4 a)
{
    return lerp(x, y, a);
}
//------------------------------------------------------------------------------------------------------------------------------
FfxFloat16 ffxMax3Half(FfxFloat16 x, FfxFloat16 y, FfxFloat16 z)
{
    return max(x, max(y, z));
}
FfxFloat16x2 ffxMax3Half(FfxFloat16x2 x, FfxFloat16x2 y, FfxFloat16x2 z)
{
    return max(x, max(y, z));
}
FfxFloat16x3 ffxMax3Half(FfxFloat16x3 x, FfxFloat16x3 y, FfxFloat16x3 z)
{
    return max(x, max(y, z));
}
FfxFloat16x4 ffxMax3Half(FfxFloat16x4 x, FfxFloat16x4 y, FfxFloat16x4 z)
{
    return max(x, max(y, z));
}
//------------------------------------------------------------------------------------------------------------------------------
FfxUInt16 ffxMaxHalf(FfxUInt16 a, FfxUInt16 b)
{
    return FfxUInt16(max(FfxInt32(a), FfxInt32(b)));
}
FfxUInt16x2 ffxMaxHalf(FfxUInt16x2 a, FfxUInt16x2 b)
{
    return FfxUInt16x2(max(FfxInt32x2(a), FfxInt32x2(b)));
}
FfxUInt16x3 ffxMaxHalf(FfxUInt16x3 a, FfxUInt16x3 b)
{
    return FfxUInt16x3(max(FfxInt32x3(a), FfxInt32x3(b)));
}
FfxUInt16x4 ffxMaxHalf(FfxUInt16x4 a, FfxUInt16x4 b)
{
    return FfxUInt16x4(max(FfxInt32x4(a), FfxInt32x4(b)));
}
//------------------------------------------------------------------------------------------------------------------------------
FfxFloat16 ffxMin3Half(FfxFloat16 x, FfxFloat16 y, FfxFloat16 z)
{
    return min(x, min(y, z));
}
FfxFloat16x2 ffxMin3Half(FfxFloat16x2 x, FfxFloat16x2 y, FfxFloat16x2 z)
{
    return min(x, min(y, z));
}
FfxFloat16x3 ffxMin3Half(FfxFloat16x3 x, FfxFloat16x3 y, FfxFloat16x3 z)
{
    return min(x, min(y, z));
}
FfxFloat16x4 ffxMin3Half(FfxFloat16x4 x, FfxFloat16x4 y, FfxFloat16x4 z)
{
    return min(x, min(y, z));
}
//------------------------------------------------------------------------------------------------------------------------------
FfxUInt16 ffxMinHalf(FfxUInt16 a, FfxUInt16 b)
{
    return FfxUInt16(min(FfxInt32(a), FfxInt32(b)));
}
FfxUInt16x2 ffxMinHalf(FfxUInt16x2 a, FfxUInt16x2 b)
{
    return FfxUInt16x2(min(FfxInt32x2(a), FfxInt32x2(b)));
}
FfxUInt16x3 ffxMinHalf(FfxUInt16x3 a, FfxUInt16x3 b)
{
    return FfxUInt16x3(min(FfxInt32x3(a), FfxInt32x3(b)));
}
FfxUInt16x4 ffxMinHalf(FfxUInt16x4 a, FfxUInt16x4 b)
{
    return FfxUInt16x4(min(FfxInt32x4(a), FfxInt32x4(b)));
}
//------------------------------------------------------------------------------------------------------------------------------
FfxFloat16 ffxReciprocalHalf(FfxFloat16 x)
{
    return rcp(x);
}
FfxFloat16x2 ffxReciprocalHalf(FfxFloat16x2 x)
{
    return rcp(x);
}
FfxFloat16x3 ffxReciprocalHalf(FfxFloat16x3 x)
{
    return rcp(x);
}
FfxFloat16x4 ffxReciprocalHalf(FfxFloat16x4 x)
{
    return rcp(x);
}
//------------------------------------------------------------------------------------------------------------------------------
FfxFloat16 ffxReciprocalSquareRootHalf(FfxFloat16 x)
{
    return rsqrt(x);
}
FfxFloat16x2 ffxReciprocalSquareRootHalf(FfxFloat16x2 x)
{
    return rsqrt(x);
}
FfxFloat16x3 ffxReciprocalSquareRootHalf(FfxFloat16x3 x)
{
    return rsqrt(x);
}
FfxFloat16x4 ffxReciprocalSquareRootHalf(FfxFloat16x4 x)
{
    return rsqrt(x);
}
//------------------------------------------------------------------------------------------------------------------------------
FfxFloat16 ffxSaturateHalf(FfxFloat16 x)
{
    return saturate(x);
}
FfxFloat16x2 ffxSaturateHalf(FfxFloat16x2 x)
{
    return saturate(x);
}
FfxFloat16x3 ffxSaturateHalf(FfxFloat16x3 x)
{
    return saturate(x);
}
FfxFloat16x4 ffxSaturateHalf(FfxFloat16x4 x)
{
    return saturate(x);
}
//------------------------------------------------------------------------------------------------------------------------------
FfxUInt16 ffxBitShiftRightHalf(FfxUInt16 a, FfxUInt16 b)
{
    return FfxUInt16(FfxInt16(a) >> FfxInt16(b));
}
FfxUInt16x2 ffxBitShiftRightHalf(FfxUInt16x2 a, FfxUInt16x2 b)
{
    return FfxUInt16x2(FfxInt16x2(a) >> FfxInt16x2(b));
}
FfxUInt16x3 ffxBitShiftRightHalf(FfxUInt16x3 a, FfxUInt16x3 b)
{
    return FfxUInt16x3(FfxInt16x3(a) >> FfxInt16x3(b));
}
FfxUInt16x4 ffxBitShiftRightHalf(FfxUInt16x4 a, FfxUInt16x4 b)
{
    return FfxUInt16x4(FfxInt16x4(a) >> FfxInt16x4(b));
}
#endif // #if defined(FFX_HALF)

//==============================================================================================================================
//                                                         HLSL WAVE
//==============================================================================================================================
#if defined(FFX_WAVE)
// Where 'x' must be a compile time literal.
FfxFloat32 AWaveXorF1(FfxFloat32 v, FfxUInt32 x)
{
    return WaveReadLaneAt(v, WaveGetLaneIndex() ^ x);
}
FfxFloat32x2 AWaveXorF2(FfxFloat32x2 v, FfxUInt32 x)
{
    return WaveReadLaneAt(v, WaveGetLaneIndex() ^ x);
}
FfxFloat32x3 AWaveXorF3(FfxFloat32x3 v, FfxUInt32 x)
{
    return WaveReadLaneAt(v, WaveGetLaneIndex() ^ x);
}
FfxFloat32x4 AWaveXorF4(FfxFloat32x4 v, FfxUInt32 x)
{
    return WaveReadLaneAt(v, WaveGetLaneIndex() ^ x);
}
FfxUInt32 AWaveXorU1(FfxUInt32 v, FfxUInt32 x)
{
    return WaveReadLaneAt(v, WaveGetLaneIndex() ^ x);
}
FfxUInt32x2 AWaveXorU1(FfxUInt32x2 v, FfxUInt32 x)
{
    return WaveReadLaneAt(v, WaveGetLaneIndex() ^ x);
}
FfxUInt32x3 AWaveXorU1(FfxUInt32x3 v, FfxUInt32 x)
{
    return WaveReadLaneAt(v, WaveGetLaneIndex() ^ x);
}
FfxUInt32x4 AWaveXorU1(FfxUInt32x4 v, FfxUInt32 x)
{
    return WaveReadLaneAt(v, WaveGetLaneIndex() ^ x);
}

#if defined(FFX_HALF)
FfxFloat16x2 ffxWaveXorFloat16x2(FfxFloat16x2 v, FfxUInt32 x)
{
    return FFX_UINT32_TO_FLOAT16X2(WaveReadLaneAt(FFX_FLOAT16X2_TO_UINT32(v), WaveGetLaneIndex() ^ x));
}
FfxFloat16x4 ffxWaveXorFloat16x4(FfxFloat16x4 v, FfxUInt32 x)
{
    return FFX_UINT32X2_TO_FLOAT16X4(WaveReadLaneAt(FFX_FLOAT16X4_TO_UINT32X2(v), WaveGetLaneIndex() ^ x));
}
FfxUInt16x2 ffxWaveXorUint16x2(FfxUInt16x2 v, FfxUInt32 x)
{
    return FFX_UINT32_TO_UINT16X2(WaveReadLaneAt(FFX_UINT16X2_TO_UINT32(v), WaveGetLaneIndex() ^ x));
}
FfxUInt16x4 ffxWaveXorUint16x4(FfxUInt16x4 v, FfxUInt32 x)
{
    return AW4_FFX_UINT32(WaveReadLaneAt(FFX_UINT32_AW4(v), WaveGetLaneIndex() ^ x));
}
#endif // #if defined(FFX_HALF)
#endif // #if defined(FFX_WAVE)
