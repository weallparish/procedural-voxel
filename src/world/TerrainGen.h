#pragma once

#include <Cpp/FastNoiseLite.h>

#include "Chunk.h"
#include "World.h"

#define BASE_HEIGHT 32
#define HEIGHT_RANGE 20

struct TerrainGen {
    FastNoiseLite fnl;

    BlockID grass;
    BlockID dirt;
    BlockID stone;

    TerrainGen(int seed, BlockRegistry& reg) {
        fnl.SetSeed(seed);
        fnl.SetFractalOctaves(2);

        grass = reg.idOf("grass");
        dirt = reg.idOf("dirt");
        stone = reg.idOf("stone");
    }

    void generateChunk(Chunk* chunk, const ChunkPos& pos) {
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                float worldX = pos.x * CHUNK_SIZE + x;
                float worldZ = pos.z * CHUNK_SIZE + z;

                float noise = fnl.GetNoise(worldX, worldZ);

                int height = int(BASE_HEIGHT + noise * HEIGHT_RANGE);

                for (int y = 0; y < CHUNK_SIZE; y++) {
                    int worldY = pos.y * CHUNK_SIZE + y;

                    if (worldY <= height - 3) {
                        chunk->set(x, y, z, stone);
                    }
                    else if (worldY <= height - 1) {
                        chunk->set(x, y, z, dirt);
                    }
                    else if (worldY <= height) {
                        chunk->set(x, y, z, grass);
                    }
                    else {
                        break;
                    }
                }
            }
        }
        
    }
};