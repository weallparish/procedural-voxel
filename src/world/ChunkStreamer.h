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

    bool built = false;

    TerrainGen tg;

    ChunkStreamer(int s, BlockRegistry& reg) : tg(s, reg) {
        seed = s;
        registry = reg;
    }

    std::vector<ChunkPos> update(World& world, Camera cam) {
        ChunkPos c = world.worldPosToChunkPos(int(std::floor(cam.pos.x)), int(std::floor(cam.pos.y)), int(std::floor(cam.pos.z)));
        std::vector<ChunkPos> removedChunks;

        // Rebuild list of wanted chunks.
        if (lastChunk != c || !built) {
            built = true;
            lastChunk = c;

            chunkRequests.clear();

            for (int dz = -(radius+1); dz <= (radius+1); dz++) {
                for (int dx = -(radius+1); dx <= (radius+1); dx++) {
                    for (int dy = -(radius + 1); dy <= (radius + 1); dy++) {
                        int distanceSquared = dx * dx + dy* dy + dz * dz;

                        if (distanceSquared > (radius+1) * (radius+1)) continue;

                        chunkRequests.push_back({{c.x + dx, c.y + dy, c.z + dz}, distanceSquared});
                    }
                    
                    
                    
                }
            }

            std::sort(chunkRequests.begin(), chunkRequests.end(), [](const WantedChunk& a, const WantedChunk& b) {return a.distance < b.distance;});

            for (auto it = world.chunks.begin(); it != world.chunks.end();) {
                int distance = (it->first.x - c.x) * (it->first.x - c.x) + (it->first.y - c.y) * (it->first.y - c.y) + (it->first.z - c.z) * (it->first.z - c.z);
                if (distance > (radius + 3) * (radius + 3)) {
                    removedChunks.push_back(it->first);
                    it = world.chunks.erase(it);
                }
                else {
                    it++;
                }
            }
        }

        // Generate up to chunkLimitPerFrame chunks.
        int budget = chunkLimitPerFrame;
        for (auto chunkRequest : chunkRequests) {
            if (budget == 0) break;

            if (world.chunks.contains(chunkRequest.pos)) continue;

            tg.generateChunk(world.createChunk(chunkRequest.pos), chunkRequest.pos);

            budget--;
        }

        return removedChunks;
    }
};