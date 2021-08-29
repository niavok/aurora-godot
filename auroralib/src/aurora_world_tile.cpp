#include "aurora_world.h"

namespace godot {
namespace aurora {

bool AuroraWorldTile::SetTileMaterial(TileMaterial material)
{
	if (m_tileMaterial != material)
	{
		m_tileMaterial = material;
		return true;
	}
	else
	{
		return false;
	}
}

TileMaterial AuroraWorldTile::GetTileMaterial() const
{
	return m_tileMaterial;
}

}
}