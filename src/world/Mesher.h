#pragma once

#include <vector>

#include "Chunk.h"
#include "render/Texture.h"

struct Face {
    int dx, dy, dz;
};

constexpr Face FACES[6] = {
    {1, 0, 0},  // +X (east)
    {-1, 0, 0}, // -X (west)
    {0, 1, 0},  // +Y (up)
    {0, -1, 0}, // -Y (down)
    {0, 0, 1},  // +Z (south)
    {0, 0, -1}  // -Z (north)
};

constexpr int CORNERS[6][4][3] = {
    {{1,0,1},{1,0,0},{1,1,0},{1,1,1}},  // +X
    {{0,0,0},{0,0,1},{0,1,1},{0,1,0}},  // -X
    {{0,1,1},{1,1,1},{1,1,0},{0,1,0}},  // +Y
    {{0,0,0},{1,0,0},{1,0,1},{0,0,1}},  // -Y
    {{0,0,1},{1,0,1},{1,1,1},{0,1,1}},  // +Z
    {{1,0,0},{0,0,0},{0,1,0},{1,1,0}},  // -Z
};

constexpr float UVS[4][2] = {{0,0}, {1,0}, {1,1}, {0,1}};
constexpr float FACE_SHADE[6] = {0.8f, 0.8f, 1.0f, 0.5f, 0.7f, 0.7f};

struct Vertex {
    float x, y, z; // position in chunk-space
    float u, v; // texture coordinate
    float shade; // brightness multiplier
};

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

bool isAir(const World& world, const ChunkPos& pos, const Chunk* c, int x, int y, int z) {
    // For now, we just assume border chunks are air.
    if (x >= 0 && x < CHUNK_SIZE 
     && y >= 0 && y < CHUNK_SIZE
     && z >= 0 && z < CHUNK_SIZE) {
        return c->get(x, y, z) == 0;
    }

    int wx = pos.x * CHUNK_SIZE + x;
    int wy = pos.y * CHUNK_SIZE + y;
    int wz = pos.z * CHUNK_SIZE + z;
    return world.getBlock(wx, wy, wz) == 0; 
}

void addFace(MeshData& mesh, int x, int y, int z, int face, int tile) {
    uint32_t base = (uint32_t)mesh.vertices.size();

    int tx = tile % ATLAS_TILES;
    int ty = tile / ATLAS_TILES;

    for (int i = 0; i < 4; i++) {
        float u = (tx + UVS[i][0]) / ATLAS_TILES;
        float v = (ty + 1.0f - UVS[i][1]) / ATLAS_TILES;

        mesh.vertices.push_back({
            (float)(x + CORNERS[face][i][0]),
            (float)(y + CORNERS[face][i][1]),
            (float)(z + CORNERS[face][i][2]),
            u, v,
            FACE_SHADE[face]
        });
    }

    mesh.indices.insert(mesh.indices.end(),
        { base, base + 1, base + 2, base + 2, base + 3, base });
}

MeshData meshChunk(const World& world, const ChunkPos& pos) {
    MeshData mesh;
    Chunk* c = world.getChunk(pos);
    
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            for (int y = 0; y < CHUNK_SIZE; y++) {
                if (!isAir(world, pos, c, x, y, z)) {
                    for (int i = 0; i < 6; i++) {
                        if (isAir(world, pos, c, x + FACES[i].dx, y + FACES[i].dy, z + FACES[i].dz)) {
                            addFace(mesh, x, y, z, i, c->get(x, y, z));
                        }
                    }
                }
            }
        }
    }

    return mesh;
}