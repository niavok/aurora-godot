#ifndef AURORA_WORLD_BLOCK_H
#define AURORA_WORLD_BLOCK_H

#include <vector>

#include "aurora_types.h"
#include "aurora_utils.h"
#include "aurora_world_block.h"

namespace godot {
namespace aurora {

class AuroraWorldChunk
{
public:
	void Init(AVectorI chunkCoord);
	void SetTileMaterial(AVectorI relativeTileCoord, TileMaterial material);
	AuroraWorldBlock& GetBlockAndLocalCoord(AVectorI relativeTileCoord, AVectorI& localBlockCoord);
	AVectorI const& GetChunkCoord() const { return m_chunkCoord; }
	bool IsHomogeneous() const { return m_isHomogeneous; }
	TileMaterial GetChunkMaterial() const { return m_chunkMaterial; }
	AuroraWorldBlock& GetBlock(AVectorI blockCoord);

private:

	void SpitChunk();
	void TryMergeChunk();

	AVectorI m_chunkCoord;

	bool m_isHomogeneous = true;
	TileMaterial m_chunkMaterial = TileMaterial::Air;
	std::vector<AuroraWorldBlock> m_blocks;
};


}
}
#endif