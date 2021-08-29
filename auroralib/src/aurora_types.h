#ifndef AURORA_TYPES_H
#define AURORA_TYPES_H

namespace godot {
namespace aurora {

constexpr int TILE_PER_BLOCK_LINE = 16;
constexpr int BLOCK_PER_CHUNK_LINE = 16;
constexpr int TILE_PER_CHUNK_LINE = TILE_PER_BLOCK_LINE * BLOCK_PER_CHUNK_LINE;

constexpr int TILE_PER_BLOCK = TILE_PER_BLOCK_LINE * TILE_PER_BLOCK_LINE;

constexpr int BLOCK_PER_CHUNK = BLOCK_PER_CHUNK_LINE * BLOCK_PER_CHUNK_LINE;

enum class TileMaterial
{
	Vaccum,
	Air,
	Aerock
};

}
}
#endif