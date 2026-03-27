#include <cls/util/error.h>
#include <cls/util/types.h>
#include <cls/util/xxhash32.h>
#include <stdint.h>
#include <string.h>

#define XXH_PRIME32_1 0x9E3779B1U
#define XXH_PRIME32_2 0x85EBCA77U
#define XXH_PRIME32_3 0xC2B2AE3DU
#define XXH_PRIME32_4 0x27D4EB2FU
#define XXH_PRIME32_5 0x165667B1U

static u32 rotl32(u32 x, int r) {
    return (x << r) | (x >> (32 - r));
}

static int read32le(u32 *out, const void *data) {
    if (!out || !data)
        return CLS_NULLPTR;

    const u8 *p = (const u8 *)data;
    *out =
        ((u32)p[0]) | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
    return CLS_SUCCESS;
}

static u32 xxh32_round(u32 acc, u32 input) {
    acc += input * XXH_PRIME32_2;
    acc = rotl32(acc, 13);
    acc *= XXH_PRIME32_1;
    return acc;
}

static u32 xxh32_avalanche(u32 h32) {
    h32 ^= h32 >> 15;
    h32 *= XXH_PRIME32_2;
    h32 ^= h32 >> 13;
    h32 *= XXH_PRIME32_3;
    h32 ^= h32 >> 16;
    return h32;
}

int cls_xxhash32(u32 *hash, const void *input, size_t length, u32 seed) {
    const u8 *p = input;
    const u8 *const b_end = p + length;
    u32 h32;

    if (length >= 16) {
        const u8 *const limit = b_end - 16;
        u32 v1 = seed + XXH_PRIME32_1 + XXH_PRIME32_2;
        u32 v2 = seed + XXH_PRIME32_2;
        u32 v3 = seed + 0;
        u32 v4 = seed - XXH_PRIME32_1;

        do {
            u32 val = 0;
            int error = read32le(&val, p);
            if (error)
                return error;
            v1 = xxh32_round(v1, val);
            p += 4;

            error = read32le(&val, p);
            if (error)
                return error;
            v2 = xxh32_round(v2, val);
            p += 4;

            error = read32le(&val, p);
            if (error)
                return error;
            v3 = xxh32_round(v3, val);
            p += 4;

            error = read32le(&val, p);
            if (error)
                return error;
            v4 = xxh32_round(v4, val);
            p += 4;
        } while (p <= limit);

        h32 = rotl32(v1, 1) + rotl32(v2, 7) + rotl32(v3, 12) + rotl32(v4, 18);
    } else {
        h32 = seed + XXH_PRIME32_5;
    }

    h32 += (u32)length;

    while (p + 4 <= b_end) {
        u32 val = 0;
        int error = read32le(&val, p);
        if (error)
            return error;
        h32 += val * XXH_PRIME32_3;
        h32 = rotl32(h32, 17) * XXH_PRIME32_4;
        p += 4;
    }

    while (p < b_end) {
        h32 += (*p) * XXH_PRIME32_5;
        h32 = rotl32(h32, 11) * XXH_PRIME32_1;
        p++;
    }

    *hash = xxh32_avalanche(h32);
    return CLS_SUCCESS;
}
