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

#if !defined(FFX_SKIP_EXT)
#if defined(FFX_HALF)
    #extension GL_EXT_shader_16bit_storage : require
    #extension GL_EXT_shader_explicit_arithmetic_types : require
#endif // #if defined(FFX_HALF)

#if defined(FFX_LONG)
    #extension GL_ARB_gpu_shader_int64 : require
    #extension GL_NV_shader_atomic_int64 : require
#endif // #if defined(FFX_LONG)

#if defined(FFX_WAVE)
    #extension GL_KHR_shader_subgroup_arithmetic : require
    #extension GL_KHR_shader_subgroup_ballot : require
    #extension GL_KHR_shader_subgroup_quad : require
    #extension GL_KHR_shader_subgroup_shuffle : require
#endif // #if defined(FFX_WAVE)
#endif // #if !defined(FFX_SKIP_EXT)

/// Interprets the bit pattern of x as a floating-point number.
///
/// @param [in] value               The input value.
///
/// @returns
/// The input interpreted as a floating-point number.
///
/// @ingroup GLSL
FfxFloat32 asfloat(FfxUInt32 x)
{
    return uintBitsToFloat(x);
}

/// Interprets the bit pattern of x as a floating-point number.
///
/// @param [in] value               The input value.
///
/// @returns
/// The input interpreted as a floating-point number.
///
/// @ingroup GLSL
FfxFloat32x2 asfloat(FfxUInt32x2 x)
{
    return uintBitsToFloat(x);
}

/// Interprets the bit pattern of x as a floating-point number.
///
/// @param [in] value               The input value.
///
/// @returns
/// The input interpreted as a floating-point number.
///
/// @ingroup GLSL
FfxFloat32x3 asfloat(FfxUInt32x3 x)
{
    return uintBitsToFloat(x);
}

/// Interprets the bit pattern of x as a floating-point number.
///
/// @param [in] value               The input value.
///
/// @returns
/// The input interpreted as a floating-point number.
///
/// @ingroup GLSL
FfxFloat32x4 asfloat(FfxUInt32x4 x)
{
    return uintBitsToFloat(x);
}

/// Interprets the bit pattern of x as an unsigned integer.
///
/// @param [in] value               The input value.
///
/// @returns
/// The input interpreted as an unsigned integer.
///
/// @ingroup GLSL
FfxUInt32 asuint(FfxFloat32 x)
{
    return floatBitsToUint(x);
}

/// Interprets the bit pattern of x as an unsigned integer.
///
/// @param [in] value               The input value.
///
/// @returns
/// The input interpreted as an unsigned integer.
///
/// @ingroup GLSL
FfxUInt32x2 asuint(FfxFloat32x2 x)
{
    return floatBitsToUint(x);
}

/// Interprets the bit pattern of x as an unsigned integer.
///
/// @param [in] value               The input value.
///
/// @returns
/// The input interpreted as an unsigned integer.
///
/// @ingroup GLSL
FfxUInt32x3 asuint(FfxFloat32x3 x)
{
    return floatBitsToUint(x);
}

/// Interprets the bit pattern of x as an unsigned integer.
///
/// @param [in] value               The input value.
///
/// @returns
/// The input interpreted as an unsigned integer.
///
/// @ingroup GLSL
FfxUInt32x4 asuint(FfxFloat32x4 x)
{
    return floatBitsToUint(x);
}

/// Convert a 32bit IEEE 754 floating point value to its nearest 16bit equivalent.
///
/// @param [in] value               The value to convert.
/// 
/// @returns
/// The nearest 16bit equivalent of <c><i>value</i></c>.
/// 
/// @ingroup GLSL
FfxUInt32 f32tof16(FfxFloat32 value)
{
    return packHalf2x16(FfxFloat32x2(value, 0.0));
}

/// Broadcast a scalar value to a 2-dimensional floating point vector.
///
/// @param [in] value               The value to to broadcast.
///
/// @returns
/// A 2-dimensional floating point vector with <c><i>value</i></c> in each component.
///
/// @ingroup GLSL
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
/// @ingroup GLSL
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
/// @ingroup GLSL
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
/// @ingroup GLSL
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
/// @ingroup GLSL
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
/// @ingroup GLSL
FfxUInt32x4 broadcast4(FfxUInt32 value)
{
    return FfxUInt32x4(value, value, value, value);
}

///
///
/// @ingroup GLSL
FfxUInt32 bitfieldExtract(FfxUInt32 src, FfxUInt32 off, FfxUInt32 bits)
{
    return bitfieldExtract(src, FfxInt32(off), FfxInt32(bits));
}

///
///
/// @ingroup GLSL
FfxUInt32 bitfieldInsert(FfxUInt32 src, FfxUInt32 ins, FfxUInt32 mask)
{
    return (ins & mask) | (src & (~mask));
}

// Proxy for V_BFI_B32 where the 'mask' is set as 'bits', 'mask=(1<<bits)-1', and 'bits' needs to be an immediate.
///
///
/// @ingroup GLSL
FfxUInt32 bitfieldInsertMask(FfxUInt32 src, FfxUInt32 ins, FfxUInt32 bits)
{
    return bitfieldInsert(src, ins, 0, FfxInt32(bits));
}

/// Compute the linear interopation between three values.
///
/// Implemented by calling the GLSL <c><i>mix</i></c> instrinsic function. Implements the
/// following math:
///
///     (1 - t) * x + t * y
///
/// @param [in] x               The first value to include in the max calculation.
/// @param [in] y               The second value to include in the max calcuation.
/// @param [in] z               The third value to include in the max calcuation.
///
/// @returns
/// The maximum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup GLSL
FfxFloat32 lerp(FfxFloat32 x, FfxFloat32 y, FfxFloat32 t)
{
    return mix(x, y, t);
}

/// Compute the linear interopation between three values.
///
/// Implemented by calling the GLSL <c><i>mix</i></c> instrinsic function. Implements the
/// following math:
///
///     (1 - t) * x + t * y
///
/// @param [in] x               The first value to include in the max calculation.
/// @param [in] y               The second value to include in the max calcuation.
/// @param [in] z               The third value to include in the max calcuation.
///
/// @returns
/// The maximum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup GLSL
FfxFloat32x2 lerp(FfxFloat32x2 x, FfxFloat32x2 y, FfxFloat32x2 t)
{
    return mix(x, y, t);
}

/// Compute the linear interopation between three values.
///
/// Implemented by calling the GLSL <c><i>mix</i></c> instrinsic function. Implements the
/// following math:
///
///     (1 - t) * x + t * y
///
/// @param [in] x               The first value to include in the max calculation.
/// @param [in] y               The second value to include in the max calcuation.
/// @param [in] z               The third value to include in the max calcuation.
///
/// @returns
/// The maximum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup GLSL
FfxFloat32x3 lerp(FfxFloat32x3 x, FfxFloat32x3 y, FfxFloat32x3 t)
{
    return mix(x, y, t);
}

/// Compute the linear interopation between three values.
///
/// Implemented by calling the GLSL <c><i>mix</i></c> instrinsic function. Implements the
/// following math:
/// 
///     (1 - t) * x + t * y
/// 
/// @param [in] x               The first value to include in the max calculation.
/// @param [in] y               The second value to include in the max calcuation.
/// @param [in] z               The third value to include in the max calcuation.
///
/// @returns
/// The maximum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup GLSL
FfxFloat32x4 lerp(FfxFloat32x4 x, FfxFloat32x4 y, FfxFloat32x4 t)
{
    return mix(x, y, t);
}

/// Compute the maximum of three values.
///
/// NOTE: This function should compile down to a single V_MAX3_F32 operation on
/// GCN or RDNA hardware.
///
/// @param [in] x               The first value to include in the max calculation.
/// @param [in] y               The second value to include in the max calcuation.
/// @param [in] z               The third value to include in the max calcuation.
///
/// @returns
/// The maximum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup GLSL
FfxFloat32 ffxMax3(FfxFloat32 x, FfxFloat32 y, FfxFloat32 z)
{
    return max(x, max(y, z));
}

/// Compute the maximum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MAX3_F32</i></c> operation on
/// GCN or RDNA hardware.
///
/// @param [in] x               The first value to include in the max calculation.
/// @param [in] y               The second value to include in the max calcuation.
/// @param [in] z               The third value to include in the max calcuation.
///
/// @returns
/// The maximum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup GLSL
FfxFloat32x2 ffxMax3(FfxFloat32x2 x, FfxFloat32x2 y, FfxFloat32x2 z)
{
    return max(x, max(y, z));
}

/// Compute the maximum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MAX3_F32</i></c> operation on
/// GCN or RDNA hardware.
///
/// @param [in] x               The first value to include in the max calculation.
/// @param [in] y               The second value to include in the max calcuation.
/// @param [in] z               The third value to include in the max calcuation.
///
/// @returns
/// The maximum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup GLSL
FfxFloat32x3 ffxMax3(FfxFloat32x3 x, FfxFloat32x3 y, FfxFloat32x3 z)
{
    return max(x, max(y, z));
}

/// Compute the maximum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MAX3_F32</i></c> operation on
/// GCN or RDNA hardware.
///
/// @param [in] x               The first value to include in the max calculation.
/// @param [in] y               The second value to include in the max calcuation.
/// @param [in] z               The third value to include in the max calcuation.
///
/// @returns
/// The maximum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup GLSL
FfxFloat32x4 ffxMax3(FfxFloat32x4 x, FfxFloat32x4 y, FfxFloat32x4 z)
{
    return max(x, max(y, z));
}

/// Compute the maximum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MAX3_F32</i></c> operation on
/// GCN or RDNA hardware.
///
/// @param [in] x               The first value to include in the max calculation.
/// @param [in] y               The second value to include in the max calcuation.
/// @param [in] z               The third value to include in the max calcuation.
///
/// @returns
/// The maximum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup GLSL
FfxUInt32 ffxMax3(FfxUInt32 x, FfxUInt32 y, FfxUInt32 z)
{
    return max(x, max(y, z));
}

/// Compute the maximum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MAX3_F32</i></c> operation on
/// GCN or RDNA hardware.
///
/// @param [in] x               The first value to include in the max calculation.
/// @param [in] y               The second value to include in the max calcuation.
/// @param [in] z               The third value to include in the max calcuation.
///
/// @returns
/// The maximum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup GLSL
FfxUInt32x2 ffxMax3(FfxUInt32x2 x, FfxUInt32x2 y, FfxUInt32x2 z)
{
    return max(x, max(y, z));
}

/// Compute the maximum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MAX3_F32</i></c> operation on
/// GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the max calculation.
/// @param [in] y               The second value to include in the max calcuation.
/// @param [in] z               The third value to include in the max calcuation.
///
/// @returns
/// The maximum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup GLSL
FfxUInt32x3 ffxMax3(FfxUInt32x3 x, FfxUInt32x3 y, FfxUInt32x3 z)
{
    return max(x, max(y, z));
}

/// Compute the maximum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MAX3_F32</i></c> operation on
/// GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the max calculation.
/// @param [in] y               The second value to include in the max calcuation.
/// @param [in] z               The third value to include in the max calcuation.
///
/// @returns
/// The maximum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup GLSL
FfxUInt32x4 ffxMax3(FfxUInt32x4 x, FfxUInt32x4 y, FfxUInt32x4 z)
{
    return max(x, max(y, z));
}

/// Compute the median of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MED3_F32</i></c> operation on
/// GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the median calculation.
/// @param [in] y               The second value to include in the median calcuation.
/// @param [in] z               The third value to include in the median calcuation.
///
/// @returns
/// The median value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup GLSL
FfxFloat32 ffxMed3(FfxFloat32 x, FfxFloat32 y, FfxFloat32 z)
{
    return max(min(x, y), min(max(x, y), z));
}

/// Compute the median of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MED3_F32</i></c> operation on
/// GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the median calculation.
/// @param [in] y               The second value to include in the median calcuation.
/// @param [in] z               The third value to include in the median calcuation.
///
/// @returns
/// The median value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup GLSL
FfxFloat32x2 ffxMed3(FfxFloat32x2 x, FfxFloat32x2 y, FfxFloat32x2 z)
{
    return max(min(x, y), min(max(x, y), z));
}

/// Compute the median of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MED3_F32</i></c> operation on
/// GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the median calculation.
/// @param [in] y               The second value to include in the median calcuation.
/// @param [in] z               The third value to include in the median calcuation.
///
/// @returns
/// The median value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup GLSL
FfxFloat32x3 ffxMed3(FfxFloat32x3 x, FfxFloat32x3 y, FfxFloat32x3 z)
{
    return max(min(x, y), min(max(x, y), z));
}

/// Compute the median of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MED3_F32</i></c> operation on
/// GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the median calculation.
/// @param [in] y               The second value to include in the median calcuation.
/// @param [in] z               The third value to include in the median calcuation.
///
/// @returns
/// The median value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup GLSL
FfxFloat32x4 ffxMed3(FfxFloat32x4 x, FfxFloat32x4 y, FfxFloat32x4 z)
{
    return max(min(x, y), min(max(x, y), z));
}

/// Compute the minimum of three values.
///
/// NOTE: This function should compile down to a single <c><i>V_MIN3_F32</i></c> operation on
/// GCN and RDNA hardware.
///
/// @param [in] x               The first value to include in the min calculation.
/// @param [in] y               The second value to include in the min calcuation.
/// @param [in] z               The third value to include in the min calcuation.
///
/// @returns
/// The minimum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup GLSL
FfxFloat32 ffxMin3(FfxFloat32 x, FfxFloat32 y, FfxFloat32 z)
{
    return min(x, min(y, z));
}

/// Compute the minimum of three values.
///
/// NOTE: This function should compile down to a single V_MIN3_F32 operation on
/// GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the min calculation.
/// @param [in] y               The second value to include in the min calcuation.
/// @param [in] z               The third value to include in the min calcuation.
///
/// @returns
/// The minimum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup GLSL
FfxFloat32x2 ffxMin3(FfxFloat32x2 x, FfxFloat32x2 y, FfxFloat32x2 z)
{
    return min(x, min(y, z));
}

/// Compute the minimum of three values.
///
/// NOTE: This function should compile down to a single V_MIN3_F32 operation on
/// GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the min calculation.
/// @param [in] y               The second value to include in the min calcuation.
/// @param [in] z               The third value to include in the min calcuation.
///
/// @returns
/// The minimum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup GLSL
FfxFloat32x3 ffxMin3(FfxFloat32x3 x, FfxFloat32x3 y, FfxFloat32x3 z)
{
    return min(x, min(y, z));
}

/// Compute the minimum of three values.
///
/// NOTE: This function should compile down to a single V_MIN3_F32 operation on
/// GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the min calculation.
/// @param [in] y               The second value to include in the min calcuation.
/// @param [in] z               The third value to include in the min calcuation.
///
/// @returns
/// The minimum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup GLSL
FfxFloat32x4 ffxMin3(FfxFloat32x4 x, FfxFloat32x4 y, FfxFloat32x4 z)
{
    return min(x, min(y, z));
}

/// Compute the minimum of three values.
///
/// NOTE: This function should compile down to a single V_MIN3_F32 operation on
/// GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the min calculation.
/// @param [in] y               The second value to include in the min calcuation.
/// @param [in] z               The third value to include in the min calcuation.
///
/// @returns
/// The minimum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup GLSL
FfxUInt32 ffxMin3(FfxUInt32 x, FfxUInt32 y, FfxUInt32 z)
{
    return min(x, min(y, z));
}

/// Compute the minimum of three values.
///
/// NOTE: This function should compile down to a single V_MIN3_F32 operation on
/// GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the min calculation.
/// @param [in] y               The second value to include in the min calcuation.
/// @param [in] z               The third value to include in the min calcuation.
///
/// @returns
/// The minimum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup GLSL
FfxUInt32x2 ffxMin3(FfxUInt32x2 x, FfxUInt32x2 y, FfxUInt32x2 z)
{
    return min(x, min(y, z));
}

/// Compute the minimum of three values.
///
/// NOTE: This function should compile down to a single V_MIN3_F32 operation on
/// GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the min calculation.
/// @param [in] y               The second value to include in the min calcuation.
/// @param [in] z               The third value to include in the min calcuation.
///
/// @returns
/// The minimum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup GLSL
FfxUInt32x3 ffxMin3(FfxUInt32x3 x, FfxUInt32x3 y, FfxUInt32x3 z)
{
    return min(x, min(y, z));
}

/// Compute the minimum of three values.
///
/// NOTE: This function should compile down to a single V_MIN3_F32 operation on
/// GCN/RDNA hardware.
///
/// @param [in] x               The first value to include in the min calculation.
/// @param [in] y               The second value to include in the min calcuation.
/// @param [in] z               The third value to include in the min calcuation.
///
/// @returns
/// The minimum value of <c><i>x</i></c>, <c><i>y</i></c>, and <c><i>z</i></c>.
///
/// @ingroup GLSL
FfxUInt32x4 ffxMin3(FfxUInt32x4 x, FfxUInt32x4 y, FfxUInt32x4 z)
{
    return min(x, min(y, z));
}

/// Compute the reciprocal of a value.
///
/// NOTE: This function is only provided for GLSL. In HLSL the intrinsic function <c><i>rcp</i></c> can be used.
///
/// @param [in] x               The value to compute the reciprocal for.
///
/// @returns
/// The reciprocal value of <c><i>x</i></c>.
/// 
/// @ingroup GLSL
FfxFloat32 rcp(FfxFloat32 x)
{
    return FfxFloat32(1.0) / x;
}

/// Compute the reciprocal of a value.
///
/// NOTE: This function is only provided for GLSL. In HLSL the intrinsic function <c><i>rcp</i></c> can be used.
///
/// @param [in] x               The value to compute the reciprocal for.
///
/// @returns
/// The reciprocal value of <c><i>x</i></c>.
///
/// @ingroup GLSL
FfxFloat32x2 rcp(FfxFloat32x2 x)
{
    return broadcast2(1.0) / x;
}

/// Compute the reciprocal of a value.
///
/// NOTE: This function is only provided for GLSL. In HLSL the intrinsic function <c><i>rcp</i></c> can be used.
///
/// @param [in] x               The value to compute the reciprocal for.
///
/// @returns
/// The reciprocal value of <c><i>x</i></c>.
///
/// @ingroup GLSL
FfxFloat32x3 rcp(FfxFloat32x3 x)
{
    return broadcast3(1.0) / x;
}

/// Compute the reciprocal of a value.
///
/// NOTE: This function is only provided for GLSL. In HLSL the intrinsic function <c><i>rcp</i></c> can be used.
///
/// @param [in] x               The value to compute the reciprocal for.
///
/// @returns
/// The reciprocal value of <c><i>x</i></c>.
///
/// @ingroup GLSL
FfxFloat32x4 rcp(FfxFloat32x4 x)
{
    return broadcast4(1.0) / x;
}

/// Compute the reciprocal square root of a value.
///
/// NOTE: This function is only provided for GLSL. In HLSL the intrinsic function <c><i>rsqrt</i></c> can be used.
///
/// @param [in] x               The value to compute the reciprocal for.
///
/// @returns
/// The reciprocal square root value of <c><i>x</i></c>.
///
/// @ingroup GLSL
FfxFloat32 rsqrt(FfxFloat32 x)
{
    return FfxFloat32(1.0) / sqrt(x);
}

/// Compute the reciprocal square root of a value.
///
/// NOTE: This function is only provided for GLSL. In HLSL the intrinsic function <c><i>rsqrt</i></c> can be used.
///
/// @param [in] x               The value to compute the reciprocal for.
///
/// @returns
/// The reciprocal square root value of <c><i>x</i></c>.
///
/// @ingroup GLSL
FfxFloat32x2 rsqrt(FfxFloat32x2 x)
{
    return broadcast2(1.0) / sqrt(x);
}

/// Compute the reciprocal square root of a value.
///
/// NOTE: This function is only provided for GLSL. In HLSL the intrinsic function <c><i>rsqrt</i></c> can be used.
///
/// @param [in] x               The value to compute the reciprocal for.
///
/// @returns
/// The reciprocal square root value of <c><i>x</i></c>.
///
/// @ingroup GLSL
FfxFloat32x3 rsqrt(FfxFloat32x3 x)
{
    return broadcast3(1.0) / sqrt(x);
}

/// Compute the reciprocal square root of a value.
///
/// NOTE: This function is only provided for GLSL. In HLSL the intrinsic function <c><i>rsqrt</i></c> can be used.
///
/// @param [in] x               The value to compute the reciprocal for.
///
/// @returns
/// The reciprocal square root value of <c><i>x</i></c>.
///
/// @ingroup GLSL
FfxFloat32x4 rsqrt(FfxFloat32x4 x)
{
    return broadcast4(1.0) / sqrt(x);
}

///
///
/// @ingroup GLSL
FfxFloat32 saturate(FfxFloat32 x)
{
    return clamp(x, FfxFloat32(0.0), FfxFloat32(1.0));
}

///
///
/// @ingroup GLSL
FfxFloat32x2 saturate(FfxFloat32x2 x)
{
    return clamp(x, broadcast2(0.0), broadcast2(1.0));
}

///
///
/// @ingroup GLSL
FfxFloat32x3 saturate(FfxFloat32x3 x)
{
    return clamp(x, broadcast3(0.0), broadcast3(1.0));
}

///
///
/// @ingroup GLSL
FfxFloat32x4 saturate(FfxFloat32x4 x)
{
    return clamp(x, broadcast4(0.0), broadcast4(1.0));
}

FfxUInt32 AShrSU1(FfxUInt32 a, FfxUInt32 b)
{
    return FfxUInt32(FfxInt32(a) >> FfxInt32(b));
}

#if defined(FFX_HALF)

#define FFX_UINT32_TO_FLOAT16X2(x) unpackFloat2x16(FfxUInt32(x))

FfxFloat16x4 ffxUint32x2ToFloat16x4(FfxUInt32x2 x)
{
    return FfxFloat16x4(unpackFloat2x16(x.x), unpackFloat2x16(x.y));
}
#define FFX_UINT32X2_TO_FLOAT16X4(x) ffxUint32x2ToFloat16x4(FfxUInt32x2(x))
#define FFX_UINT32_TO_UINT16X2(x) unpackUint2x16(FfxUInt32(x))
#define FFX_UINT32X2_TO_UINT16X4(x) unpackUint4x16(pack64(FfxUInt32x2(x)))
//------------------------------------------------------------------------------------------------------------------------------
#define FFX_FLOAT16X2_TO_UINT32(x) packFloat2x16(FfxFloat16x2(x))
FfxUInt32x2 ffxFloat16x4ToUint32x2(FfxFloat16x4 x)
{
    return FfxUInt32x2(packFloat2x16(x.xy), packFloat2x16(x.zw));
}
#define FFX_FLOAT16X4_TO_UINT32X2(x) ffxFloat16x4ToUint32x2(FfxFloat16x4(x))
#define FFX_UINT16X2_TO_UINT32(x) packUint2x16(FfxUInt16x2(x))
#define FFX_UINT16X4_TO_UINT32X2(x) unpack32(packUint4x16(FfxUInt16x4(x)))
//==============================================================================================================================
#define FFX_TO_UINT16(x) halfBitsToUint16(FfxFloat16(x))
#define FFX_TO_UINT16X2(x) halfBitsToUint16(FfxFloat16x2(x))
#define FFX_TO_UINT16X3(x) halfBitsToUint16(FfxFloat16x3(x))
#define FFX_TO_UINT16X4(x) halfBitsToUint16(FfxFloat16x4(x))
//------------------------------------------------------------------------------------------------------------------------------
#define FFX_TO_FLOAT16(x) uint16BitsToHalf(FfxUInt16(x))
#define FFX_TO_FLOAT16X2(x) uint16BitsToHalf(FfxUInt16x2(x))
#define FFX_TO_FLOAT16X3(x) uint16BitsToHalf(FfxUInt16x3(x))
#define FFX_TO_FLOAT16X4(x) uint16BitsToHalf(FfxUInt16x4(x))
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
    return clamp(x, n, m);
}
FfxFloat16x2 ffxClampHalf(FfxFloat16x2 x, FfxFloat16x2 n, FfxFloat16x2 m)
{
    return clamp(x, n, m);
}
FfxFloat16x3 ffxClampHalf(FfxFloat16x3 x, FfxFloat16x3 n, FfxFloat16x3 m)
{
    return clamp(x, n, m);
}
FfxFloat16x4 ffxClampHalf(FfxFloat16x4 x, FfxFloat16x4 n, FfxFloat16x4 m)
{
    return clamp(x, n, m);
}
//------------------------------------------------------------------------------------------------------------------------------
FfxFloat16 ffxFractHalf(FfxFloat16 x)
{
    return fract(x);
}
FfxFloat16x2 ffxFractHalf(FfxFloat16x2 x)
{
    return fract(x);
}
FfxFloat16x3 ffxFractHalf(FfxFloat16x3 x)
{
    return fract(x);
}
FfxFloat16x4 ffxFractHalf(FfxFloat16x4 x)
{
    return fract(x);
}
//------------------------------------------------------------------------------------------------------------------------------
FfxFloat16 ffxLerpHalf(FfxFloat16 x, FfxFloat16 y, FfxFloat16 a)
{
    return mix(x, y, a);
}
FfxFloat16x2 ffxLerpHalf(FfxFloat16x2 x, FfxFloat16x2 y, FfxFloat16x2 a)
{
    return mix(x, y, a);
}
FfxFloat16x3 ffxLerpHalf(FfxFloat16x3 x, FfxFloat16x3 y, FfxFloat16x3 a)
{
    return mix(x, y, a);
}
FfxFloat16x4 ffxLerpHalf(FfxFloat16x4 x, FfxFloat16x4 y, FfxFloat16x4 a)
{
    return mix(x, y, a);
}
//------------------------------------------------------------------------------------------------------------------------------
// No packed version of ffxMax3.
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
// No packed version of ffxMin3.
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
    return FFX_BROADCAST_FLOAT16(1.0) / x;
}
FfxFloat16x2 ffxReciprocalHalf(FfxFloat16x2 x)
{
    return FFX_BROADCAST_FLOAT16X2(1.0) / x;
}
FfxFloat16x3 ffxReciprocalHalf(FfxFloat16x3 x)
{
    return FFX_BROADCAST_FLOAT16X3(1.0) / x;
}
FfxFloat16x4 ffxReciprocalHalf(FfxFloat16x4 x)
{
    return FFX_BROADCAST_FLOAT16X4(1.0) / x;
}
//------------------------------------------------------------------------------------------------------------------------------
FfxFloat16 ffxReciprocalSquareRootHalf(FfxFloat16 x)
{
    return FFX_BROADCAST_FLOAT16(1.0) / sqrt(x);
}
FfxFloat16x2 ffxReciprocalSquareRootHalf(FfxFloat16x2 x)
{
    return FFX_BROADCAST_FLOAT16X2(1.0) / sqrt(x);
}
FfxFloat16x3 ffxReciprocalSquareRootHalf(FfxFloat16x3 x)
{
    return FFX_BROADCAST_FLOAT16X3(1.0) / sqrt(x);
}
FfxFloat16x4 ffxReciprocalSquareRootHalf(FfxFloat16x4 x)
{
    return FFX_BROADCAST_FLOAT16X4(1.0) / sqrt(x);
}
//------------------------------------------------------------------------------------------------------------------------------
FfxFloat16 ffxSaturateHalf(FfxFloat16 x)
{
    return clamp(x, FFX_BROADCAST_FLOAT16(0.0), FFX_BROADCAST_FLOAT16(1.0));
}
FfxFloat16x2 ffxSaturateHalf(FfxFloat16x2 x)
{
    return clamp(x, FFX_BROADCAST_FLOAT16X2(0.0), FFX_BROADCAST_FLOAT16X2(1.0));
}
FfxFloat16x3 ffxSaturateHalf(FfxFloat16x3 x)
{
    return clamp(x, FFX_BROADCAST_FLOAT16X3(0.0), FFX_BROADCAST_FLOAT16X3(1.0));
}
FfxFloat16x4 ffxSaturateHalf(FfxFloat16x4 x)
{
    return clamp(x, FFX_BROADCAST_FLOAT16X4(0.0), FFX_BROADCAST_FLOAT16X4(1.0));
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

#if defined(FFX_WAVE)
// Where 'x' must be a compile time literal.
FfxFloat32 AWaveXorF1(FfxFloat32 v, FfxUInt32 x)
{
    return subgroupShuffleXor(v, x);
}
FfxFloat32x2 AWaveXorF2(FfxFloat32x2 v, FfxUInt32 x)
{
    return subgroupShuffleXor(v, x);
}
FfxFloat32x3 AWaveXorF3(FfxFloat32x3 v, FfxUInt32 x)
{
    return subgroupShuffleXor(v, x);
}
FfxFloat32x4 AWaveXorF4(FfxFloat32x4 v, FfxUInt32 x)
{
    return subgroupShuffleXor(v, x);
}
FfxUInt32 AWaveXorU1(FfxUInt32 v, FfxUInt32 x)
{
    return subgroupShuffleXor(v, x);
}
FfxUInt32x2 AWaveXorU2(FfxUInt32x2 v, FfxUInt32 x)
{
    return subgroupShuffleXor(v, x);
}
FfxUInt32x3 AWaveXorU3(FfxUInt32x3 v, FfxUInt32 x)
{
    return subgroupShuffleXor(v, x);
}
FfxUInt32x4 AWaveXorU4(FfxUInt32x4 v, FfxUInt32 x)
{
    return subgroupShuffleXor(v, x);
}

//------------------------------------------------------------------------------------------------------------------------------
#if defined(FFX_HALF)
FfxFloat16x2 ffxWaveXorFloat16x2(FfxFloat16x2 v, FfxUInt32 x)
{
    return FFX_UINT32_TO_FLOAT16X2(subgroupShuffleXor(FFX_FLOAT16X2_TO_UINT32(v), x));
}
FfxFloat16x4 ffxWaveXorFloat16x4(FfxFloat16x4 v, FfxUInt32 x)
{
    return FFX_UINT32X2_TO_FLOAT16X4(subgroupShuffleXor(FFX_FLOAT16X4_TO_UINT32X2(v), x));
}
FfxUInt16x2 ffxWaveXorUint16x2(FfxUInt16x2 v, FfxUInt32 x)
{
    return FFX_UINT32_TO_UINT16X2(subgroupShuffleXor(FFX_UINT16X2_TO_UINT32(v), x));
}
FfxUInt16x4 ffxWaveXorUint16x4(FfxUInt16x4 v, FfxUInt32 x)
{
    return FFX_UINT32X2_TO_UINT16X4(subgroupShuffleXor(FFX_UINT16X4_TO_UINT32X2(v), x));
}
#endif // #if defined(FFX_HALF)
#endif // #if defined(FFX_WAVE)
