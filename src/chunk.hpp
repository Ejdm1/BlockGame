#include <string>
#include <iostream>
#include <thread>
#include <fstream>
#include <FastNoise/FastNoise.h>
#include <glm/glm.hpp>
#include <simd/simd.h>

const int chunkLine = 32;
const bool regenerate = false;
const int renderDistance = 9;

struct Chunk {
    int chunkPos;
    int blocks[128][16][16];
};

struct ChunkToGPU {
    int chunkPos;
    int blocks [16*16*128];
};

struct NuberOfBlocksInChunk {
    int nuberOfBlocks[chunkLine*chunkLine];
};

struct NoiseMaps {
    std::vector<float> noiseMap = {};
};

struct ChunkClass {
    void generateChunk(std::unordered_map<std::string, int> texturesDict);
    std::vector<ChunkToGPU> chunkToGPU = {};
    std::string mapFileName = "map" + std::to_string(chunkLine) + "x" + std::to_string(chunkLine);
    int chunkCount = chunkLine * chunkLine;
    NuberOfBlocksInChunk numberOfBlocksInChunk;
    int seed;
    int blockCounter = 0;
};
//Saveing block data into single int x,y,z,blockID(4bits x, 8bits y, 4bits z, 14bits blockID)//
inline int blockFace(glm::vec3 pos, int block_id) {
    int data = 0;
    if (pos.x > 15) {pos.x = 0;}
    if (pos.y > 255) {pos.y = 0;}
    if (pos.z > 15) {pos.z = 0;}

    data |= ((int)pos.x & 0xf) << 0;
    data |= ((int)pos.y & 0xff) << 4;
    data |= ((int)pos.z & 0xf) << 12;
    data |= (block_id & 0x3fff) << 18;

    return data;
}
///////////////////////////////////////////////////////////////////////////////////////////////
//////////////////Move bits in integer to get stored values//////////////////////
inline float MoveBits(int bitAmount, int bitsRight, int data) {
    int shiftedRight = data >> bitsRight;
    int bitMask = (1 << bitAmount) -1;
    int temp = shiftedRight & bitMask;
    return static_cast<float>(temp);
}
/////////////////////////////////////////////////////////////////////////////////
////////Get blocks position in chunk x,y,z(4bits x, 8bits y, 4bits z)////////////
inline simd::float3 GetPos(int data) {
    return simd::float3{MoveBits(4, 0, data), MoveBits(8, 4, data), MoveBits(4, 12, data)};
}
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////Get texture ID(max 14bits)//////////////////////////////
inline int GetBlockId(int data) {
    return MoveBits(14, 18, data);
}
/////////////////////////////////////////////////////////////////////////////////
//////////////Get chunks position in world space x,y(8bits each)/////////////////
inline glm::vec2 GetChunkPosition(int chunkData) {
    glm::vec2 posVec;
    posVec.y = MoveBits(8,0,chunkData);
    posVec.x = MoveBits(8,8,chunkData);
    if(posVec.x > 128) {
        posVec.x = (posVec.x * -1) + 128;
    };
    if(posVec.y > 128) {
        posVec.y = (posVec.y * -1) + 128;
    };
    return posVec;
}
/////////////////////////////////////////////////////////////////////////////////
////////////////////////Create sign bit for chunks position//////////////////////
inline int ChunkPosition(int posInX, int posInY) {
    if(posInX < 0) {
        posInX = (posInX * -1) + 128;
    }
    if(posInY < 0) {
        posInY = (posInY * -1) + 128;
    }
    int pos = 0;
    pos |= (posInX & 0xff) << 0;
    pos |= (posInY & 0xff) << 8;
    
    return pos;
}
/////////////////////////////////////////////////////////////////////////////////