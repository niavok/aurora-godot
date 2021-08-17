#ifndef AURORA_TYPES_H
#define AURORA_TYPES_H

namespace godot {
namespace aurora {

struct AVector
{
	AVector();
	AVector(int x, int y);

	int x;
	int y;

	int Area() const;
	AVector operator<<(int shift) const;
};

}
}
#endif