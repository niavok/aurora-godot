#include "aurora_world.h"

#include <cassert>

namespace godot {
namespace aurora {

static const int chunkCountXPower = 10;
static const int chunkCountYPower = 9;

AuroraWorld::AuroraWorld()
	: m_chunkCount(1<< chunkCountXPower, 1 << chunkCountYPower)
{
	m_chunks.resize(m_chunkCount.Area());
}

AVectorI AuroraWorld::GetTileCount() const
{
	return m_chunkCount << 8;
}

AVectorI AuroraWorld::GetBlockCount() const
{
	return m_chunkCount << 4;
}

AVectorI AuroraWorld::GetChunkCount() const
{
	return m_chunkCount;
}

float AuroraWorld::GetTileSize() const
{
	return 10.f;
}

AuroraWorldChunk& AuroraWorld::GetChunkAndLocalCoord(AVectorI tileCoord, AVectorI& localChunkCoord)
{
	localChunkCoord.x = tileCoord.x % TILE_PER_CHUNK_LINE;
	localChunkCoord.y = tileCoord.y % TILE_PER_CHUNK_LINE;

	int chunkIndexX = tileCoord.x / TILE_PER_CHUNK_LINE;
	int chunkIndexY = tileCoord.y / TILE_PER_CHUNK_LINE;

	if (chunkIndexX < 0 || chunkIndexX >= m_chunkCount.x)
	{
		chunkIndexX = chunkIndexX % m_chunkCount.x;
	}

	assert(chunkIndexY >= 0);
	assert(chunkIndexY < m_chunkCount.y);

	return m_chunks[chunkIndexX + chunkIndexY * m_chunkCount.x];
}

void AuroraWorld::SetTileMaterial(AVectorI tileCoord, TileMaterial material)
{
	AVectorI localChunkCoord;
	AuroraWorldChunk& chunk = GetChunkAndLocalCoord(tileCoord, localChunkCoord);
	chunk.SetTileMaterial(localChunkCoord, material);
}

void AuroraWorld::Step(float dt)
{

}

}
}