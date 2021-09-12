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

	AVectorI chunkCoord;
	for (chunkCoord.x = 0; chunkCoord.x < m_chunkCount.x; chunkCoord.x++)
	{
		for (chunkCoord.y = 0; chunkCoord.y < m_chunkCount.y; chunkCoord.y++)
		{
			AuroraWorldChunk& chunk = GetChunk(chunkCoord);
			chunk.Init(chunkCoord);
		}
	}
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

AuroraWorldChunk& AuroraWorld::GetChunkAndLocalCoord(AVectorI tileCoord, AVectorI& localChunkCoord)
{
	localChunkCoord.x = (TILE_PER_CHUNK_LINE + tileCoord.x % TILE_PER_CHUNK_LINE) % TILE_PER_CHUNK_LINE;
	localChunkCoord.y = (TILE_PER_CHUNK_LINE + tileCoord.y % TILE_PER_CHUNK_LINE) % TILE_PER_CHUNK_LINE;

	int chunkIndexX = tileCoord.x / TILE_PER_CHUNK_LINE;
	int chunkIndexY = tileCoord.y / TILE_PER_CHUNK_LINE;

	if (chunkIndexX < 0 || chunkIndexX >= m_chunkCount.x)
	{
		chunkIndexX = (m_chunkCount.x + chunkIndexX % m_chunkCount.x) % m_chunkCount.x;
	}

	assert(chunkIndexY >= 0);
	assert(chunkIndexY < m_chunkCount.y);
	assert(chunkIndexX >= 0);
	assert(chunkIndexX < m_chunkCount.x);

	return m_chunks[chunkIndexX + chunkIndexY * m_chunkCount.x];
}

void AuroraWorld::SetTileMaterial(AVectorI tileCoord, TileMaterial material)
{
	AVectorI localChunkCoord;
	AuroraWorldChunk& chunk = GetChunkAndLocalCoord(tileCoord, localChunkCoord);
	chunk.SetTileMaterial(localChunkCoord, material);
}

void AuroraWorld::SetTileMaterial(ARectI tileRect, TileMaterial material)
{
	SelectChunks(tileRect, [this, material](AuroraWorldChunk& chunk, ARectI& localRect)
		{
			chunk.SetTileMaterial(localRect, material);
		});
}

void AuroraWorld::Step(float dt)
{

}

AuroraWorldChunk& AuroraWorld::GetChunk(AVectorI chunkCoord)
{
	return m_chunks[chunkCoord.x + chunkCoord.y * m_chunkCount.x];
}

void AuroraWorld::SelectChunks(ARectI rectTileCoord, std::function<void(AuroraWorldChunk&, ARectI&)> callback)
{
	AVectorI localStartChunkCoord;
	AuroraWorldChunk& startChunk = GetChunkAndLocalCoord(rectTileCoord.position, localStartChunkCoord);

	AVectorI localEndChunkCoord;
	AuroraWorldChunk& endChunk = GetChunkAndLocalCoord(rectTileCoord.position + rectTileCoord.size - AVectorI(1) , localEndChunkCoord);

	AVectorI chunkCount = AVectorI(1,1) + rectTileCoord.size / TILE_PER_CHUNK_LINE;
	
	
	AVectorI chunkCoord;
	for (chunkCoord.x = startChunk.GetChunkCoord().x; chunkCoord.x <= endChunk.GetChunkCoord().x; chunkCoord.x++)
	{
		for (chunkCoord.y = startChunk.GetChunkCoord().y; chunkCoord.y <= endChunk.GetChunkCoord().y; chunkCoord.y++)
		{
			AuroraWorldChunk& chunk = GetChunk(chunkCoord);
			ARectI chunkTileRect = chunk.GetTileRect();
			ARectI intesection = chunkTileRect.Intersection(rectTileCoord);
			ARectI localIntesection = ARectI(intesection.position - chunkTileRect.position, intesection.size);
			callback(chunk, localIntesection);
		}
	}

}


}
}