#include "aurora_world_chunk.h"

#include <cassert>

namespace godot {
namespace aurora {

void AuroraWorldChunk::SetTileMaterial(AVectorI relativeTileCoord, TileMaterial material)
{
	if (m_isHomogeneous)
	{
		if (m_chunkMaterial == material)
		{
			return;
		}
		else
		{
			SpitChunk();
			AVectorI localBlockCoord;
			AuroraWorldBlock& block = GetBlockAndLocalCoord(relativeTileCoord, localBlockCoord);
			block.SetTileMaterial(localBlockCoord, material);
		}
	}
	else
	{
		AVectorI localBlockCoord;
		AuroraWorldBlock& block = GetBlockAndLocalCoord(relativeTileCoord, localBlockCoord);
		if (block.SetTileMaterial(localBlockCoord, material))
		{
			if (!block.IsHomogeneous())
			{
				TryMergeChunk();
			}
		}
	}
}

void AuroraWorldChunk::SpitChunk()
{
	assert(m_isHomogeneous);

	m_blocks.resize(BLOCK_PER_CHUNK);
	for (int index = 0; index < BLOCK_PER_CHUNK; index++)
	{
		m_blocks[index].SetBlockMaterial(m_chunkMaterial);
	}

	m_isHomogeneous = false;
}

void AuroraWorldChunk::TryMergeChunk()
{
	assert(!m_isHomogeneous);

	for (int index = 1; index < BLOCK_PER_CHUNK; index++)
	{
		if (!m_blocks[index].IsHomogeneous())
		{
			return;
		}
	}

	m_chunkMaterial = m_blocks[0].GetBlockMaterial();
	for (int index = 1; index < BLOCK_PER_CHUNK; index++)
	{
		if (m_blocks[index].GetBlockMaterial() != m_chunkMaterial)
		{
			return;
		}
	}

	// Go for merge
	m_blocks.clear();
	m_blocks.shrink_to_fit();
	m_isHomogeneous = true;
}

AuroraWorldBlock& AuroraWorldChunk::GetBlockAndLocalCoord(AVectorI relativeTileCoord, AVectorI& localBlockCoord)
{
	localBlockCoord.x = relativeTileCoord.x % TILE_PER_BLOCK_LINE;
	localBlockCoord.y = relativeTileCoord.y % TILE_PER_BLOCK_LINE;

	int blockIndexX = relativeTileCoord.x / BLOCK_PER_CHUNK_LINE;
	int blockIndexY = relativeTileCoord.y / BLOCK_PER_CHUNK_LINE;

	return m_blocks[blockIndexX + blockIndexY * BLOCK_PER_CHUNK_LINE];
}



}
}