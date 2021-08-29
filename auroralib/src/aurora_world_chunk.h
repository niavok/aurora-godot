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
	void SetTileMaterial(AVectorI relativeTileCoord, TileMaterial material);
	AuroraWorldBlock& GetBlockAndLocalCoord(AVectorI relativeTileCoord, AVectorI& localBlockCoord);

private:

	void SpitChunk();
	void TryMergeChunk();

	bool m_isHomogeneous;
	TileMaterial m_chunkMaterial;
	std::vector<AuroraWorldBlock> m_blocks;
};


}
}
#endif