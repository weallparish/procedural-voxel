#pragma once
#include <cstdint>
#include <cassert>

constexpr int CHUNK_SIZE = 16;
constexpr int CHUNK_VOLUME = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;

using BlockID = uint16_t;

struct Chunk {
    BlockID blocks[CHUNK_VOLUME] = {};

    static int index(int x, int y, int z) {
        // Make sure we are reading memory in the chunk's bounds.
        assert(x >= 0 && x < CHUNK_SIZE 
            && y >= 0 && y < CHUNK_SIZE
            && z >= 0 && z < CHUNK_SIZE);

        return x + z * CHUNK_SIZE + y * CHUNK_SIZE * CHUNK_SIZE;
    }

    BlockID get(int x, int y, int z) const {
        return blocks[index(x, y, z)];
    }

    void set(int x, int y, int z, BlockID id) {
        blocks[index(x, y, z)] = id;
    }
};

