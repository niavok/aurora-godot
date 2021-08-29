#include "aurora_world_block.h"

#include <cassert>

namespace godot {
namespace aurora {

bool AuroraWorldBlock::SetTileMaterial(AVectorI relativeTileCoord, TileMaterial material)
{
	if (m_isHomogeneous)
	{
		if (m_blockMaterial == material)
		{
			return false;
		}
		else
		{
			SplitBlock();
			AuroraWorldTile& tile = GetTile(relativeTileCoord);
			bool changed = tile.SetTileMaterial(material);
			assert(changed);
			return true;
		}
	}
	else
	{
		AuroraWorldTile& tile = GetTile(relativeTileCoord);
		if (tile.SetTileMaterial(material))
		{
			TryMergeBlock();
			return true;
		}
		else
		{
			return false;
		}
	}

}

bool AuroraWorldBlock::IsHomogeneous() const
{
	return m_isHomogeneous;
}

AuroraWorldTile& AuroraWorldBlock::GetTile(AVectorI relativeTileCoord)
{
	assert(!m_isHomogeneous);
	assert(relativeTileCoord.x >= 0);
	assert(relativeTileCoord.y >= 0);
	assert(relativeTileCoord.x < TILE_PER_BLOCK_LINE);
	assert(relativeTileCoord.y < TILE_PER_BLOCK_LINE);

	return m_tiles[relativeTileCoord.x + relativeTileCoord.y * TILE_PER_BLOCK_LINE];
}

void AuroraWorldBlock::SplitBlock()
{
	assert(m_isHomogeneous);

	m_tiles.resize(TILE_PER_BLOCK);
	for (int index = 0; index < TILE_PER_BLOCK; index++)
	{
		m_tiles[index].SetTileMaterial(m_blockMaterial);
	}

	m_isHomogeneous = false;
}

void AuroraWorldBlock::TryMergeBlock()
{
	assert(!m_isHomogeneous);

	m_blockMaterial = m_tiles[0].GetTileMaterial();
	for (int index = 1; index < TILE_PER_BLOCK; index++)
	{
		if (m_tiles[index].GetTileMaterial() != m_blockMaterial)
		{
			return;
		}
	}

	// Go for merge
	m_tiles.clear();
	m_tiles.shrink_to_fit();
	m_isHomogeneous = true;

}

TileMaterial AuroraWorldBlock::GetBlockMaterial() const
{
	assert(m_isHomogeneous);
	return m_blockMaterial;
}


bool AuroraWorldBlock::SetBlockMaterial(TileMaterial material)
{
	if (m_isHomogeneous && m_blockMaterial == material)
	{
		return false;
	}

	if (!m_isHomogeneous)
	{
		// Force homogeneous
		m_tiles.clear();
		m_tiles.shrink_to_fit();
		m_isHomogeneous = true;
	}

	m_blockMaterial = material;

	return true;
}




}
}