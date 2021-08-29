#ifndef AURORA_WORLD_H
#define AURORA_WORLD_H

#include "aurora_utils.h"
#include "aurora_types.h"
#include "aurora_world_chunk.h"
#include <vector>

namespace godot {
namespace aurora {

class AuroraWorld
{
public:
	AuroraWorld();
	AVectorI GetTileCount() const; // Tile of 10 x 10 cm
	AVectorI GetBlockCount() const; // Block is 16x16 tiles so 1,6 x 1,6 m
	AVectorI GetChunkCount() const; // Chunck is 16 x 16 blocks so 25,6 x 25,6 m 
	float GetTileSize() const;

	void Step(float dt);

	void SetTileMaterial(AVectorI tileCoord, TileMaterial material);

	AuroraWorldChunk& GetChunkAndLocalCoord(AVectorI tileCoord, AVectorI& localChunkCoord);

private:
	AVectorI m_chunkCount;
	std::vector<AuroraWorldChunk> m_chunks;
};

}
}
#endif