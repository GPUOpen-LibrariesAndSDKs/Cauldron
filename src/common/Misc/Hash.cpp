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
#include "Hash.h"

//
// Compute a hash of an array
//
size_t Hash(const void *ptr, size_t size, size_t result)
{
    for (size_t i = 0; i < size; ++i)
    {
        result = (result * 16777619) ^ ((char *)ptr)[i];
    }

    return result;
}

size_t HashString(const char *str, size_t result)
{
    return Hash(str, strlen(str), result);
}

size_t HashString(const std::string &str, size_t result)
{
    return HashString(str.c_str(), result);
}

size_t HashInt(const int type, size_t result) { return Hash(&type, sizeof(int), result); }
size_t HashFloat(const float type, size_t result) { return Hash(&type, sizeof(float), result); }
size_t HashPtr(const void *type, size_t result) { return Hash(&type, sizeof(void *), result); }