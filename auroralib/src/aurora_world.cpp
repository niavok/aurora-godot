#include "aurora_world.h"

namespace godot {
namespace aurora {

static const int chunkCountXPower = 9;
static const int chunkCountYPower = 8;

AuroraWorld::AuroraWorld()
	: m_chunkCount(1<< chunkCountXPower, 1 << chunkCountYPower)
{
	m_chunks.resize(m_chunkCount.Area());
}

AVector AuroraWorld::GetWorldSize() const
{
	return m_chunkCount << 8;
}

AVector AuroraWorld::GetBlockCount() const
{
	return m_chunkCount << 4;
}

AVector AuroraWorld::GetChunkCount() const
{
	return m_chunkCount;
}


}
}