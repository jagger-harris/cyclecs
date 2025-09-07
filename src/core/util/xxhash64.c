#include "core/util/xxhash64.h"
#include <stdint.h>
#include <string.h>

#define XXH_PRIME64_1 0x9E3779B185EBCA87ULL
#define XXH_PRIME64_2 0xC2B2AE3D27D4EB4FULL
#define XXH_PRIME64_3 0x165667B19E3779F9ULL
#define XXH_PRIME64_4 0x85EBCA77C2B2AE63ULL
#define XXH_PRIME64_5 0x27D4EB2F165667C5ULL

static u64 rotl64(u64 x, int r) {
    return (x << r) | (x >> (64 - r));
}

static u64 read64le(const void *ptr) {
    const u8 *p = (const u8 *)ptr;
    return ((u64)p[0]) | ((u64)p[1] << 8) | ((u64)p[2] << 16) |
           ((u64)p[3] << 24) | ((u64)p[4] << 32) | ((u64)p[5] << 40) |
           ((u64)p[6] << 48) | ((u64)p[7] << 56);
}

static u32 read32le(const void *ptr) {
    const u8 *p = (const u8 *)ptr;
    return ((u32)p[0]) | ((u32)p[1] << 8) | ((u32)p[2] << 16) |
           ((u32)p[3] << 24);
}

static u64 xxh64_round(u64 acc, u64 input) {
    acc += input * XXH_PRIME64_2;
    acc = rotl64(acc, 31);
    acc *= XXH_PRIME64_1;
    return acc;
}

static u64 xxh64_merge_round(u64 acc, u64 val) {
    val = xxh64_round(0, val);
    acc ^= val;
    acc = acc * XXH_PRIME64_1 + XXH_PRIME64_4;
    return acc;
}

static u64 xxh64_avalanche(u64 h64) {
    h64 ^= h64 >> 33;
    h64 *= XXH_PRIME64_2;
    h64 ^= h64 >> 29;
    h64 *= XXH_PRIME64_3;
    h64 ^= h64 >> 32;
    return h64;
}

u64 xxhash64(const void *input, size_t length, u64 seed) {
    const u8 *p = input;
    const u8 *const bEnd = p + length;
    u64 h64;

    if (length >= 32) {
        const u8 *const limit = bEnd - 32;
        u64 v1 = seed + XXH_PRIME64_1 + XXH_PRIME64_2;
        u64 v2 = seed + XXH_PRIME64_2;
        u64 v3 = seed + 0;
        u64 v4 = seed - XXH_PRIME64_1;

        do {
            v1 = xxh64_round(v1, read64le(p));
            p += 8;
            v2 = xxh64_round(v2, read64le(p));
            p += 8;
            v3 = xxh64_round(v3, read64le(p));
            p += 8;
            v4 = xxh64_round(v4, read64le(p));
            p += 8;
        } while (p <= limit);

        h64 = rotl64(v1, 1) + rotl64(v2, 7) + rotl64(v3, 12) + rotl64(v4, 18);
        h64 = xxh64_merge_round(h64, v1);
        h64 = xxh64_merge_round(h64, v2);
        h64 = xxh64_merge_round(h64, v3);
        h64 = xxh64_merge_round(h64, v4);
    } else {
        h64 = seed + XXH_PRIME64_5;
    }

    h64 += (u64)length;

    while (p + 8 <= bEnd) {
        u64 k1 = xxh64_round(0, read64le(p));
        h64 ^= k1;
        h64 = rotl64(h64, 27) * XXH_PRIME64_1 + XXH_PRIME64_4;
        p += 8;
    }

    if (p + 4 <= bEnd) {
        h64 ^= (u64)(read32le(p)) * XXH_PRIME64_1;
        h64 = rotl64(h64, 23) * XXH_PRIME64_2 + XXH_PRIME64_3;
        p += 4;
    }

    while (p < bEnd) {
        h64 ^= (*p) * XXH_PRIME64_5;
        h64 = rotl64(h64, 11) * XXH_PRIME64_1;
        p++;
    }

    return xxh64_avalanche(h64);
}
