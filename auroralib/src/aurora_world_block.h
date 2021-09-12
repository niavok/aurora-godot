#ifndef AURORA_WORLD_CHUNK_H
#define AURORA_WORLD_CHUNK_H

#include <vector>
#include <functional>

#include "aurora_types.h"
#include "aurora_utils.h"
#include "aurora_world_tile.h"

namespace godot {
namespace aurora {

class AuroraWorldBlock
{
public:
	bool SetTileMaterial(AVectorI relativeTileCoord, TileMaterial material);
	bool SetTileMaterial(ARectI relativeTileRect, TileMaterial material);
	bool SetBlockMaterial(TileMaterial material);

	bool IsHomogeneous() const;
	AuroraWorldTile& GetTile(AVectorI relativeTileCoord);

	TileMaterial GetBlockMaterial() const;

	bool SelectTiles(ARectI rectTileCoord, std::function<bool(AuroraWorldTile&)> callback);


private:

	void SplitBlock();
	void TryMergeBlock();

	bool m_isHomogeneous = true;
	TileMaterial m_blockMaterial = TileMaterial::Air;
	std::vector<AuroraWorldTile> m_tiles;
};

}
}
#endif