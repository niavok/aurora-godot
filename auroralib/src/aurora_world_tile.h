#ifndef AURORA_WORLD_TILE_H
#define AURORA_WORLD_TILE_H

#include "aurora_types.h"

namespace godot {
namespace aurora {

class AuroraWorldTile
{
public:
	bool SetTileMaterial(TileMaterial material); // True if changed
	TileMaterial GetTileMaterial() const;

private:
	TileMaterial m_tileMaterial;
};

}
}
#endif