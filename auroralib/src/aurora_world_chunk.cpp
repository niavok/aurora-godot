#include "aurora_world_chunk.h"

#include <cassert>

namespace godot {
namespace aurora {

void AuroraWorldChunk::Init(AVectorI chunkCoord)
{
	m_chunkCoord = chunkCoord;
}


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
			SplitChunk();
			AVectorI localBlockTileCoord;
			AVectorI blockCoord;
			AuroraWorldBlock& block = GetBlockAndLocalCoord(relativeTileCoord, localBlockTileCoord, blockCoord);
			block.SetTileMaterial(localBlockTileCoord, material);
		}
	}
	else
	{
		AVectorI localBlockTileCoord;
		AVectorI blockCoord;
		AuroraWorldBlock& block = GetBlockAndLocalCoord(relativeTileCoord, localBlockTileCoord, blockCoord);
		if (block.SetTileMaterial(localBlockTileCoord, material))
		{
			if (!block.IsHomogeneous())
			{
				TryMergeChunk();
			}
		}
	}
}

void AuroraWorldChunk::SetTileMaterial(ARectI relativeTileRect, TileMaterial material)
{
	ARectI WholeChunk(AVectorI(0), AVectorI(TILE_PER_CHUNK_LINE));


	if (relativeTileRect == WholeChunk)
	{
		if (IsHomogeneous())
		{
			if (GetChunkMaterial() == material)
			{
				// Nothing to do
			}
			else
			{
				m_chunkMaterial = material;
			}
		}
		else
		{
			// Force merge
			m_blocks.clear();
			m_blocks.shrink_to_fit();
			m_isHomogeneous = true;
			m_chunkMaterial = material;
		}
	}
	else
	{
		if (IsHomogeneous())
		{
			if (GetChunkMaterial() == material)
			{
				// Nothing to do
				return;
			}
			else
			{
				SplitChunk();
			}
		}

		SelectBlocks(relativeTileRect, [this, material](AuroraWorldBlock& block, ARectI& localRect)
			{
				block.SetTileMaterial(localRect, material);
			});

		if (!IsHomogeneous())
		{
			TryMergeChunk();
		}
	}

	
}


void AuroraWorldChunk::SplitChunk()
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

	for (int index = 0; index < BLOCK_PER_CHUNK; index++)
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

AuroraWorldBlock& AuroraWorldChunk::GetBlockAndLocalCoord(AVectorI relativeTileCoord, AVectorI& localBlockTileCoord, AVectorI& blockCoord)
{
	localBlockTileCoord.x = (relativeTileCoord.x % TILE_PER_BLOCK_LINE);
	localBlockTileCoord.y = (relativeTileCoord.y % TILE_PER_BLOCK_LINE);

	blockCoord.x = relativeTileCoord.x / BLOCK_PER_CHUNK_LINE;
	blockCoord.y = relativeTileCoord.y / BLOCK_PER_CHUNK_LINE;

	assert(blockCoord.x >= 0);
	assert(blockCoord.x < BLOCK_PER_CHUNK_LINE);
	assert(blockCoord.y >= 0);
	assert(blockCoord.y < BLOCK_PER_CHUNK_LINE);

	return m_blocks[blockCoord.x + blockCoord.y * BLOCK_PER_CHUNK_LINE];
}



AuroraWorldBlock& AuroraWorldChunk::GetBlock(AVectorI blockCoord)
{
	assert(!m_isHomogeneous);
	return m_blocks[blockCoord.x + blockCoord.y * BLOCK_PER_CHUNK_LINE];
}

void AuroraWorldChunk::SelectBlocks(ARectI rectTileCoord, std::function<void(AuroraWorldBlock&, ARectI&)> callback)
{
	AVectorI localStartBlockCoord;
	AVectorI startBlockCoord;
	AuroraWorldBlock& startBlock = GetBlockAndLocalCoord(rectTileCoord.position, localStartBlockCoord, startBlockCoord);

	AVectorI localEndBlockCoord;
	AVectorI endBlockCoord;
	AuroraWorldBlock& endBlock = GetBlockAndLocalCoord(rectTileCoord.position + rectTileCoord.size - AVectorI(1), localEndBlockCoord, endBlockCoord);

	AVectorI chunkCount = AVectorI(1, 1) + rectTileCoord.size / TILE_PER_CHUNK_LINE;


	AVectorI blockCoord;
	for (blockCoord.x = startBlockCoord.x; blockCoord.x <= endBlockCoord.x; blockCoord.x++)
	{
		for (blockCoord.y = startBlockCoord.y; blockCoord.y <= endBlockCoord.y; blockCoord.y++)
		{
			AuroraWorldBlock& block = GetBlock(blockCoord);
			ARectI blockTileRect(blockCoord * TILE_PER_BLOCK_LINE, TILE_PER_BLOCK_LINE);
			ARectI intesection = blockTileRect.Intersection(rectTileCoord);
			ARectI localIntesection = ARectI(intesection.position - blockTileRect.position, intesection.size);
			callback(block, localIntesection);
		}
	}
}


}
}