#pragma once

#include <unordered_map>
#include <memory>

#include "Chunk.h"
#include "utils/VoxelMath.h"

struct ChunkPos {
    int x, y, z;

    float squaredMagnitude() {
        return x^2 + y^2 + z^2;
    }

    glm::vec3 operator-(const ChunkPos c) const {
        return glm::vec3{x - c.x, y - c.y, z - c.z};
    }

    glm::vec3 operator*(const int c) const {
        return glm::vec3{x * c, y * c, z * c};
    }

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

    ChunkPos worldPosToChunkPos(int x, int y, int z) const {
        ChunkPos pos = {floorDiv(x, CHUNK_SIZE), floorDiv(y, CHUNK_SIZE), floorDiv(z, CHUNK_SIZE)};
        return pos;
    }

    BlockID getBlock(int x, int y, int z) const {
        const Chunk* c = getChunk(worldPosToChunkPos(x, y, z));

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
        setNeighborsDirty(c, chunkPos, cX, cY, cZ);
    }

    void setNeighborsDirty(Chunk* chunk, ChunkPos pos, int x, int y, int z) {
        chunk->isDirty = true;

        if (x == 0) {
            getChunk(ChunkPos(pos.x - 1, pos.y, pos.z))->isDirty = true;
        }
        else if (x == 15) {
            getChunk(ChunkPos(pos.x + 1, pos.y, pos.z))->isDirty = true;
        }

        if (y == 0) {
            getChunk(ChunkPos(pos.x, pos.y - 1, pos.z))->isDirty = true;
        }
        else if (y == 15) {
            getChunk(ChunkPos(pos.x, pos.y + 1, pos.z))->isDirty = true;
        }

        if (z == 0) {
            getChunk(ChunkPos(pos.x, pos.y, pos.z - 1))->isDirty = true;
        }
        else if (z == 15) {
            getChunk(ChunkPos(pos.x, pos.y, pos.z + 1))->isDirty = true;
        }
    }
};