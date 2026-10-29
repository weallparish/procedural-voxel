#pragma once

#include <vector>
#include <algorithm>

#include "World.h"
#include "TerrainGen.h"
#include "core/Camera.h"

struct WantedChunk {
    ChunkPos pos;
    int distance;
};

struct ChunkStreamer {
    int radius = 5;
    std::vector<WantedChunk> chunkRequests;
    ChunkPos lastChunk;

    int seed;
    BlockRegistry registry;

    int chunkLimitPerFrame = 3;

    TerrainGen tg;

    ChunkStreamer(int s, BlockRegistry& reg) : tg(s, reg) {
        seed = s;
        registry = reg;
    }

    std::vector<ChunkPos> update(World& world, Camera cam) {
        ChunkPos c = world.worldPosToChunkPos(int(cam.pos.x), int(cam.pos.y), int(cam.pos.z));
        std::vector<ChunkPos> removedChunks;

        // Rebuild list of wanted chunks.
        if (lastChunk != c) {
            lastChunk = c;

            for (int dz = -radius; dz <= radius; dz++) {
                for (int dx = -radius; dx <= radius; dx++) {
                    int distanceSquared = dx * dx + dz * dz;

                    if (distanceSquared > (radius+1) * (radius+1)) continue;

                    for (int dy = -3; dy <= 3; dy++) {
                        chunkRequests.push_back({{c.x + dx, dy, c.z + dz}, distanceSquared});
                    }
                    
                }
            }

            std::sort(chunkRequests.begin(), chunkRequests.end(), [](const WantedChunk& a, const WantedChunk& b) {return a.distance < b.distance;});

            for (auto it = world.chunks.begin(); it != world.chunks.end(); it++) {
                int distance = (it->first.x - c.x) * (it->first.x - c.x) + (it->first.z - c.z) * (it->first.z - c.z);
                if (distance > (radius + 3) * (radius + 3)) {
                    removedChunks.push_back(it->first);
                    it = world.chunks.erase(it);

                    if (it == world.chunks.end()) {
                        break;
                    }
                }
            }
        }

        // Generate up to chunkLimitPerFrame chunks.
        int budget = chunkLimitPerFrame;
        for (auto chunkRequest : chunkRequests) {
            if (budget == 0) break;

            tg.generateChunk(world.createChunk(chunkRequest.pos), chunkRequest.pos);

            budget--;
        }

        for (int i = chunkLimitPerFrame; i >= 0; i--) {
            if (i >= chunkRequests.size()) {
                continue;
            }

            chunkRequests.erase(chunkRequests.begin() + i);
        }

        return removedChunks;
    }
};