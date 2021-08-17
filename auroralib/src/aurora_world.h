#ifndef AURORA_WORLD_H
#define AURORA_WORLD_H

#include "aurora_types.h"
#include <vector>

namespace godot {
namespace aurora {

class AuroraChunk
{

};

class AuroraWorld
{
public:
	AuroraWorld();
	AVector GetWorldSize() const; // Size in tile of 10 x 10 cm
	AVector GetBlockCount() const; // Block is 16x16 tiles so 1,6 x 1,6 m
	AVector GetChunkCount() const; // Chunck is 16 x 16 blocks so 25,6 x 25,6 m 

private:
	AVector m_chunkCount;

	std::vector<AuroraChunk> m_chunks;
};

}
}
#endif