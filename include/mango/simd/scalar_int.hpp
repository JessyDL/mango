/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

#ifdef MANGO_SIMD_INT_SCALAR

namespace mango {
namespace simd {

    // -----------------------------------------------------------------
    // uint32x4
    // -----------------------------------------------------------------

    // shuffle

    template <int x, int y, int z, int w>
    static inline uint32x4 uint32x4_shuffle(uint32x4 v)
    {
        // .generic
        const uint32* src = reinterpret_cast<const uint32 *>(&v);
        uint32x4 n = { src[x], src[y], src[z], src[w] };
        return n;
    }

    template <>
    inline uint32x4 uint32x4_shuffle<0, 1, 2, 3>(uint32x4 v)
    {
        // .xyzw
        return v;
    }

    // indexed access

    template <int Index>
    static inline uint32x4 uint32x4_set_component(uint32x4 a, uint32 s);

    template <>
    inline uint32x4 uint32x4_set_component<0>(uint32x4 a, uint32 s)
    {
        a.x = s;
        return a;
    }

    template <>
    inline uint32x4 uint32x4_set_component<1>(uint32x4 a, uint32 s)
    {
        a.y = s;
        return a;
    }

    template <>
    inline uint32x4 uint32x4_set_component<2>(uint32x4 a, uint32 s)
    {
        a.z = s;
        return a;
    }

    template <>
    inline uint32x4 uint32x4_set_component<3>(uint32x4 a, uint32 s)
    {
        a.w = s;
        return a;
    }

    template <int Index>
    static inline uint32 uint32x4_get_component(uint32x4 a);

    template <>
    inline uint32 uint32x4_get_component<0>(uint32x4 a)
    {
        return a.x;
    }

    template <>
    inline uint32 uint32x4_get_component<1>(uint32x4 a)
    {
        return a.y;
    }

    template <>
    inline uint32 uint32x4_get_component<2>(uint32x4 a)
    {
        return a.z;
    }

    template <>
    inline uint32 uint32x4_get_component<3>(uint32x4 a)
    {
        return a.w;
    }

    static inline uint32x4 uint32x4_zero()
    {
        uint32x4 temp = { 0, 0, 0, 0 };
        return temp;
    }

    static inline uint32x4 uint32x4_set1(uint32 s)
    {
        uint32x4 temp = { s, s, s, s };
        return temp;
    }

    static inline uint32x4 uint32x4_set4(uint32 x, uint32 y, uint32 z, uint32 w)
    {
        uint32x4 temp = { x, y, z, w };
        return temp;
    }

    static inline uint32x4 uint32x4_uload(const uint32* source)
    {
        uint32x4 temp = { source[0], source[1], source[2], source[3] };
        return temp;
    }

    static inline void uint32x4_ustore(uint32* dest, uint32x4 a)
    {
        dest[0] = a.x;
        dest[1] = a.y;
        dest[2] = a.z;
        dest[3] = a.w;
    }

    // logical

    static inline uint32x4 uint32x4_and(uint32x4 a, uint32x4 b)
    {
        uint32x4 v;
        v.x = a.x & b.x;
        v.y = a.y & b.y;
        v.z = a.z & b.z;
        v.w = a.w & b.w;
        return v;
    }

    static inline uint32x4 uint32x4_nand(uint32x4 a, uint32x4 b)
    {
        uint32x4 v;
        v.x = ~a.x & b.x;
        v.y = ~a.y & b.y;
        v.z = ~a.z & b.z;
        v.w = ~a.w & b.w;
        return v;
    }

    static inline uint32x4 uint32x4_or(uint32x4 a, uint32x4 b)
    {
        uint32x4 v;
        v.x = a.x | b.x;
        v.y = a.y | b.y;
        v.z = a.z | b.z;
        v.w = a.w | b.w;
        return v;
    }

    static inline uint32x4 uint32x4_xor(uint32x4 a, uint32x4 b)
    {
        uint32x4 v;
        v.x = a.x ^ b.x;
        v.y = a.y ^ b.y;
        v.z = a.z ^ b.z;
        v.w = a.w ^ b.w;
        return v;
    }

    static inline uint32x4 uint32x4_compare_eq(uint32x4 a, uint32x4 b)
    {
        uint32x4 v;
        v.x = a.x == b.x ? 0xffffffff : 0;
        v.y = a.y == b.y ? 0xffffffff : 0;
        v.z = a.z == b.z ? 0xffffffff : 0;
        v.w = a.w == b.w ? 0xffffffff : 0;
        return v;
    }

    static inline uint32x4 uint32x4_compare_gt(uint32x4 a, uint32x4 b)
    {
        uint32x4 v;
        v.x = a.x > b.x ? 0xffffffff : 0;
        v.y = a.y > b.y ? 0xffffffff : 0;
        v.z = a.z > b.z ? 0xffffffff : 0;
        v.w = a.w > b.w ? 0xffffffff : 0;
        return v;
    }

    static inline uint32x4 uint32x4_select(uint32x4 mask, uint32x4 a, uint32x4 b)
    {
        uint32x4 v;
        v.x = (mask.x & a.x) | (~mask.x & b.x);
        v.y = (mask.y & a.y) | (~mask.y & b.y);
        v.z = (mask.z & a.z) | (~mask.z & b.z);
        v.w = (mask.w & a.w) | (~mask.w & b.w);
        return v;
    }

    static inline uint32x4 uint32x4_min(uint32x4 a, uint32x4 b)
    {
        uint32x4 v;
        v.x = std::min(a.x, b.x);
        v.y = std::min(a.y, b.y);
        v.z = std::min(a.z, b.z);
        v.w = std::min(a.w, b.w);
        return v;
    }

    static inline uint32x4 uint32x4_max(uint32x4 a, uint32x4 b)
    {
        uint32x4 v;
        v.x = std::max(a.x, b.x);
        v.y = std::max(a.y, b.y);
        v.z = std::max(a.z, b.z);
        v.w = std::max(a.w, b.w);
        return v;
    }

    // -----------------------------------------------------------------
    // int32x4
    // -----------------------------------------------------------------

    // shuffle

    template <int x, int y, int z, int w>
    static inline int32x4 int32x4_shuffle(int32x4 v)
    {
        // .generic
        const int32* src = reinterpret_cast<const int32 *>(&v);
        int32x4 n = { src[x], src[y], src[z], src[w] };
        return n;
    }

    template <>
    inline int32x4 int32x4_shuffle<0, 1, 2, 3>(int32x4 v)
    {
        // .xyzw
        return v;
    }

    // indexed access

    template <int Index>
    static inline int32x4 int32x4_set_component(int32x4 a, int32 s);

    template <>
    inline int32x4 int32x4_set_component<0>(int32x4 a, int32 s)
    {
        a.x = s;
        return a;
    }

    template <>
    inline int32x4 int32x4_set_component<1>(int32x4 a, int32 s)
    {
        a.y = s;
        return a;
    }

    template <>
    inline int32x4 int32x4_set_component<2>(int32x4 a, int32 s)
    {
        a.z = s;
        return a;
    }

    template <>
    inline int32x4 int32x4_set_component<3>(int32x4 a, int32 s)
    {
        a.w = s;
        return a;
    }

    template <int Index>
    static inline int32 int32x4_get_component(int32x4 a);

    template <>
    inline int32 int32x4_get_component<0>(int32x4 a)
    {
        return a.x;
    }

    template <>
    inline int32 int32x4_get_component<1>(int32x4 a)
    {
        return a.y;
    }

    template <>
    inline int32 int32x4_get_component<2>(int32x4 a)
    {
        return a.z;
    }

    template <>
    inline int32 int32x4_get_component<3>(int32x4 a)
    {
        return a.w;
    }

    static inline int32x4 int32x4_zero()
    {
        int32x4 temp = { 0, 0, 0, 0 };
        return temp;
    }

    static inline int32x4 int32x4_set1(int s)
    {
        int32x4 temp = { s, s, s, s };
        return temp;
    }

    static inline int32x4 int32x4_set4(int x, int y, int z, int w)
    {
        int32x4 temp = { x, y, z, w };
        return temp;
    }

    static inline int32x4 int32x4_uload(const int* source)
    {
        int32x4 temp = { source[0], source[1], source[2], source[3] };
        return temp;
    }

    static inline void int32x4_ustore(int* dest, int32x4 a)
    {
        dest[0] = a.x;
        dest[1] = a.y;
        dest[2] = a.z;
        dest[3] = a.w;
    }

    static inline int32x4 int32x4_neg(int32x4 a)
    {
        int32x4 v = { -a.x, -a.y, -a.z, -a.w };
        return v;
    }

    static inline int32x4 int32x4_add(int32x4 a, int32x4 b)
    {
        int32x4 v;
        v.x = a.x + b.x;
        v.y = a.y + b.y;
        v.z = a.z + b.z;
        v.w = a.w + b.w;
        return v;
    }

    static inline int32x4 int32x4_sub(int32x4 a, int32x4 b)
    {
        int32x4 v;
        v.x = a.x - b.x;
        v.y = a.y - b.y;
        v.z = a.z - b.z;
        v.w = a.w - b.w;
        return v;
    }

    // logical

    static inline int32x4 int32x4_and(int32x4 a, int32x4 b)
    {
        int32x4 v;
        v.x = a.x & b.x;
        v.y = a.y & b.y;
        v.z = a.z & b.z;
        v.w = a.w & b.w;
        return v;
    }

    static inline int32x4 int32x4_nand(int32x4 a, int32x4 b)
    {
        int32x4 v;
        v.x = ~a.x & b.x;
        v.y = ~a.y & b.y;
        v.z = ~a.z & b.z;
        v.w = ~a.w & b.w;
        return v;
    }

    static inline int32x4 int32x4_or(int32x4 a, int32x4 b)
    {
        int32x4 v;
        v.x = a.x | b.x;
        v.y = a.y | b.y;
        v.z = a.z | b.z;
        v.w = a.w | b.w;
        return v;
    }

    static inline int32x4 int32x4_xor(int32x4 a, int32x4 b)
    {
        int32x4 v;
        v.x = a.x ^ b.x;
        v.y = a.y ^ b.y;
        v.z = a.z ^ b.z;
        v.w = a.w ^ b.w;
        return v;
    }

    // shift

    static inline int32x4 int32x4_sll(int32x4 a, int b)
    {
        int32x4 v;
        v.x = static_cast<uint32>(a.x) << b;
        v.y = static_cast<uint32>(a.y) << b;
        v.z = static_cast<uint32>(a.z) << b;
        v.w = static_cast<uint32>(a.w) << b;
        return v;
    }

    static inline int32x4 int32x4_srl(int32x4 a, int b)
    {
        int32x4 v;
        v.x = static_cast<uint32>(a.x) >> b;
        v.y = static_cast<uint32>(a.y) >> b;
        v.z = static_cast<uint32>(a.z) >> b;
        v.w = static_cast<uint32>(a.w) >> b;
        return v;
    }

    static inline int32x4 int32x4_sra(int32x4 a, int b)
    {
        int32x4 v;
        v.x = a.x >> b;
        v.y = a.y >> b;
        v.z = a.z >> b;
        v.w = a.w >> b;
        return v;
    }

    // compare

    static inline int32x4 int32x4_compare_eq(int32x4 a, int32x4 b)
    {
        int32x4 v;
        v.x = a.x == b.x ? 0xffffffff : 0;
        v.y = a.y == b.y ? 0xffffffff : 0;
        v.z = a.z == b.z ? 0xffffffff : 0;
        v.w = a.w == b.w ? 0xffffffff : 0;
        return v;
    }

    static inline int32x4 int32x4_compare_gt(int32x4 a, int32x4 b)
    {
        int32x4 v;
        v.x = a.x > b.x ? 0xffffffff : 0;
        v.y = a.y > b.y ? 0xffffffff : 0;
        v.z = a.z > b.z ? 0xffffffff : 0;
        v.w = a.w > b.w ? 0xffffffff : 0;
        return v;
    }

    static inline int32x4 int32x4_select(int32x4 mask, int32x4 a, int32x4 b)
    {
        int32x4 v;
        v.x = (mask.x & a.x) | (~mask.x & b.x);
        v.y = (mask.y & a.y) | (~mask.y & b.y);
        v.z = (mask.z & a.z) | (~mask.z & b.z);
        v.w = (mask.w & a.w) | (~mask.w & b.w);
        return v;
    }

    static inline uint32 int32x4_get_mask(int32x4 a)
    {
        const uint32 x = a.x & 0x80000000;
        const uint32 y = a.y & 0x80000000;
        const uint32 z = a.z & 0x80000000;
        const uint32 w = a.w & 0x80000000;
        const uint32 mask = (x >> 31) | (y >> 30) | (z >> 29) | (w >> 28);
        return mask;
    }

    static inline int32x4 int32x4_min(int32x4 a, int32x4 b)
    {
        int32x4 v;
        v.x = std::min(a.x, b.x);
        v.y = std::min(a.y, b.y);
        v.z = std::min(a.z, b.z);
        v.w = std::min(a.w, b.w);
        return v;
    }

    static inline int32x4 int32x4_max(int32x4 a, int32x4 b)
    {
        int32x4 v;
        v.x = std::max(a.x, b.x);
        v.y = std::max(a.y, b.y);
        v.z = std::max(a.z, b.z);
        v.w = std::max(a.w, b.w);
        return v;
    }

    static inline uint32 int32x4_pack(int32x4 s)
    {
        const uint32 x = byteclamp(s.x);
        const uint32 y = byteclamp(s.y);
        const uint32 z = byteclamp(s.z);
        const uint32 w = byteclamp(s.w);
        return x | (y << 8) | (z << 16) | (w << 24);
    }

    static inline int32x4 int32x4_unpack(uint32 s)
    {
        int32x4 v;
        v.x = (s >> 0) & 0xff;
        v.y = (s >> 8) & 0xff;
        v.z = (s >> 16) & 0xff;
        v.w = (s >> 24);
        return v;
    }

} // namespace simd
} // namespace mango

#endif // MANGO_SIMD_INT_SCALAR
