#ifndef AURORA_WORLD_CHUNK_H
#define AURORA_WORLD_CHUNK_H

#include <vector>

#include "aurora_types.h"
#include "aurora_utils.h"
#include "aurora_world_tile.h"

namespace godot {
namespace aurora {

class AuroraWorldBlock
{
public:
	bool SetTileMaterial(AVectorI relativeTileCoord, TileMaterial material);
	bool SetBlockMaterial(TileMaterial material);

	bool IsHomogeneous() const;

	AuroraWorldTile& GetTile(AVectorI relativeTileCoord);

	TileMaterial GetBlockMaterial() const;

private:

	void SplitBlock();
	void TryMergeBlock();

	bool m_isHomogeneous;
	TileMaterial m_blockMaterial;
	std::vector<AuroraWorldTile> m_tiles;
};

}
}
#endif