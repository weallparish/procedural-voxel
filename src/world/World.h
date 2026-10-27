#pragma once

#include <unordered_map>
#include <memory>

#include "Chunk.h"

struct ChunkPos {
    int x, y, z;
    bool operator==(const ChunkPos& o) const {
        return x == o.x && y == o.y && z == o.z;
    }
};

struct ChunkPosHash {
    size_t operator()(const ChunkPos& p) const {
        size_t h = std::hash<int>()(p.x);
        h ^= std::hash<int>()(p.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<int>()(p.z) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

struct World {
    std::unordered_map<ChunkPos, std::unique_ptr<Chunk>, ChunkPosHash> chunks;

    inline int floorDiv(int a, int b) const {
        int q = a / b;
        
        // If there is a decimal left over and a and b aren't the same sign.
        if (a % b != 0 && ((a < 0) != (b < 0))) {
            return  q - 1;
        }
        else {
            return q;
        }
    }

    inline int floorMod(int a, int b) const {
        int m = a % b;

        // If there is a decimal left over and a and b aren't the same sign.
        if (a % b != 0 && ((a < 0) != (b < 0))) {
            return m + b;
        }
        else {
            return m;
        }
    }

    Chunk* createChunk(const ChunkPos& pos) {
        auto& slot = chunks[pos];
        if (!slot) slot = std::make_unique<Chunk>();
        return slot.get();
    }

    Chunk* getChunk(const ChunkPos& pos) const {
        auto it = chunks.find(pos);
        if (it == chunks.end()) return nullptr;
        return it->second.get();
    }

    BlockID getBlock(int x, int y, int z) const {
        const Chunk* c = getChunk({floorDiv(x, CHUNK_SIZE), floorDiv(y, CHUNK_SIZE), floorDiv(z, CHUNK_SIZE)});

        if (!c) {
            return 0; // Non-existing chunks are assumed to have air.
        }
       
        int cX = floorMod(x, CHUNK_SIZE);
        int cY = floorMod(y, CHUNK_SIZE);
        int cZ = floorMod(z, CHUNK_SIZE);

        return c->get(cX, cY, cZ);
    }

    void setBlock(int x, int y, int z, BlockID block) {
        ChunkPos chunkPos = {floorDiv(x, CHUNK_SIZE), floorDiv(y, CHUNK_SIZE), floorDiv(z, CHUNK_SIZE)};
        Chunk* c = getChunk(chunkPos);

        if (!c) {
            c = createChunk(chunkPos);
        }
       
        int cX = floorMod(x, CHUNK_SIZE);
        int cY = floorMod(y, CHUNK_SIZE);
        int cZ = floorMod(z, CHUNK_SIZE);

        assert(c && "setBlock: chunk is null");
        c->set(cX, cY, cZ, block);
    }
};