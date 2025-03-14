/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "simd.hpp"

namespace mango {
namespace simd {

    // 128 bit gather

    static inline f32x4 gather4(const f32* address, s32x4 offset)
    {
        auto s0 = address[get_component<0>(offset)];
        auto s1 = address[get_component<1>(offset)];
        auto s2 = address[get_component<2>(offset)];
        auto s3 = address[get_component<3>(offset)];
        return f32x4_set(s0, s1, s2, s3);
    }

    static inline f64x2 gather2(const f64* address, s32x4 offset)
    {
        auto s0 = address[get_component<0>(offset)];
        auto s1 = address[get_component<1>(offset)];
        return f64x2_set2(s0, s1);
    }

    static inline u32x4 gather4(const u32* address, s32x4 offset)
    {
        auto s0 = address[get_component<0>(offset)];
        auto s1 = address[get_component<1>(offset)];
        auto s2 = address[get_component<2>(offset)];
        auto s3 = address[get_component<3>(offset)];
        return u32x4_set(s0, s1, s2, s3);
    }

    static inline s32x4 gather4(const s32* address, s32x4 offset)
    {
        auto s0 = address[get_component<0>(offset)];
        auto s1 = address[get_component<1>(offset)];
        auto s2 = address[get_component<2>(offset)];
        auto s3 = address[get_component<3>(offset)];
        return s32x4_set(s0, s1, s2, s3);
    }

    static inline u64x2 gather2(const u64* address, s32x4 offset)
    {
        auto s0 = address[get_component<0>(offset)];
        auto s1 = address[get_component<1>(offset)];
        return u64x2_set2(s0, s1);
    }

    static inline s64x2 gather2(const s64* address, s32x4 offset)
    {
        auto s0 = address[get_component<0>(offset)];
        auto s1 = address[get_component<1>(offset)];
        return s64x2_set2(s0, s1);
    }

    // 256 bit gather

    static inline f32x8 gather8(const f32* address, s32x8 offset)
    {
        f32x4 a = gather4(address, get_low(offset));
        f32x4 b = gather4(address, get_high(offset));
        return combine(a, b);
    }

    static inline f64x4 gather4(const f64* address, s32x4 offset)
    {
        auto s0 = address[get_component<0>(offset)];
        auto s1 = address[get_component<1>(offset)];
        auto s2 = address[get_component<2>(offset)];
        auto s3 = address[get_component<3>(offset)];
        return f64x4_set(s0, s1, s2, s3);
    }

    static inline u32x8 gather8(const u32* address, s32x8 offset)
    {
        u32x4 a = gather4(address, get_low(offset));
        u32x4 b = gather4(address, get_high(offset));
        return u32x8(a, b);
    }

    static inline s32x8 gather8(const s32* address, s32x8 offset)
    {
        s32x4 a = gather4(address, get_low(offset));
        s32x4 b = gather4(address, get_high(offset));
        return s32x8(a, b);
    }

    static inline u64x4 gather4(const u64* address, s32x4 offset)
    {
        auto s0 = address[get_component<0>(offset)];
        auto s1 = address[get_component<1>(offset)];
        auto s2 = address[get_component<2>(offset)];
        auto s3 = address[get_component<3>(offset)];
        return u64x4_set(s0, s1, s2, s3);
    }

    static inline s64x4 gather4(const s64* address, s32x4 offset)
    {
        auto s0 = address[get_component<0>(offset)];
        auto s1 = address[get_component<1>(offset)];
        auto s2 = address[get_component<2>(offset)];
        auto s3 = address[get_component<3>(offset)];
        return s64x4_set(s0, s1, s2, s3);
    }

    // 512 bit gather

    static inline f32x16 gather16(const f32* address, s32x16 offset)
    {
        f32x16 result;
        result.lo = gather8(address, offset.lo);
        result.hi = gather8(address, offset.hi);
        return result;
    }

    static inline f64x8 gather8(const f64* address, s32x8 offset)
    {
        f64x8 result;
        result.lo = gather4(address, offset.lo);
        result.hi = gather4(address, offset.hi);
        return result;
    }

    static inline u32x16 gather16(const u32* address, s32x16 offset)
    {
        u32x16 result;
        result.lo = gather8(address, offset.lo);
        result.hi = gather8(address, offset.hi);
        return result;
    }

    static inline s32x16 gather16(const s32* address, s32x16 offset)
    {
        s32x16 result;
        result.lo = gather8(address, offset.lo);
        result.hi = gather8(address, offset.hi);
        return result;
    }

    static inline u64x8 gather8(const u64* address, s32x8 offset)
    {
        u64x8 result;
        result.lo = gather4(address, offset.lo);
        result.hi = gather4(address, offset.hi);
        return result;
    }

    static inline s64x8 gather8(const s64* address, s32x8 offset)
    {
        s64x8 result;
        result.lo = gather4(address, offset.lo);
        result.hi = gather4(address, offset.hi);
        return result;
    }

    // 128 bit masked gather

    static inline f32x4 gather4(const f32* address, s32x4 offset, f32x4 value, mask32x4 mask)
    {
        return select(mask, gather4(address, offset), value);
    }

    static inline f64x2 gather2(const f64* address, s32x4 offset, f64x2 value, mask64x2 mask)
    {
        return select(mask, gather2(address, offset), value);
    }

    static inline u32x4 gather4(const u32* address, s32x4 offset, u32x4 value, mask32x4 mask)
    {
        return select(mask, gather4(address, offset), value);
    }

    static inline s32x4 gather4(const s32* address, s32x4 offset, s32x4 value, mask32x4 mask)
    {
        return select(mask, gather4(address, offset), value);
    }

    static inline u64x2 gather2(const u64* address, s32x4 offset, u64x2 value, mask64x2 mask)
    {
        return select(mask, gather2(address, offset), value);
    }

    static inline s64x2 gather2(const s64* address, s32x4 offset, s64x2 value, mask64x2 mask)
    {
        return select(mask, gather2(address, offset), value);
    }

    // 256 bit masked gather

    static inline f32x8 gather8(const f32* address, s32x8 offset, f32x8 value, mask32x8 mask)
    {
        return select(mask, gather8(address, offset), value);
    }

    static inline f64x4 gather4(const f64* address, s32x4 offset, f64x4 value, mask64x4 mask)
    {
        return select(mask, gather4(address, offset), value);
    }

    static inline u32x8 gather8(const u32* address, s32x8 offset, u32x8 value, mask32x8 mask)
    {
        return select(mask, gather8(address, offset), value);
    }

    static inline s32x8 gather8(const s32* address, s32x8 offset, s32x8 value, mask32x8 mask)
    {
        return select(mask, gather8(address, offset), value);
    }

    static inline u64x4 gather4(const u64* address, s32x4 offset, u64x4 value, mask64x4 mask)
    {
        return select(mask, gather4(address, offset), value);
    }

    static inline s64x4 gather4(const s64* address, s32x4 offset, s64x4 value, mask64x4 mask)
    {
        return select(mask, gather4(address, offset), value);
    }

    // 512 bit masked gather

    static inline f32x16 gather16(const f32* address, s32x16 offset, f32x16 value, mask32x16 mask)
    {
        return select(mask, gather16(address, offset), value);
    }

    static inline f64x8 gather8(const f64* address, s32x8 offset, f64x8 value, mask64x8 mask)
    {
        return select(mask, gather8(address, offset), value);
    }

    static inline u32x16 gather16(const u32* address, s32x16 offset, u32x16 value, mask32x16 mask)
    {
        return select(mask, gather16(address, offset), value);
    }

    static inline s32x16 gather16(const s32* address, s32x16 offset, s32x16 value, mask32x16 mask)
    {
        return select(mask, gather16(address, offset), value);
    }

    static inline u64x8 gather8(const u64* address, s32x8 offset, u64x8 value, mask64x8 mask)
    {
        return select(mask, gather8(address, offset), value);
    }

    static inline s64x8 gather8(const s64* address, s32x8 offset, s64x8 value, mask64x8 mask)
    {
        return select(mask, gather8(address, offset), value);
    }

} // namespace simd
} // namespace mango
