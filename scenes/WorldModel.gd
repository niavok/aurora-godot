extends Node2D

const WORLD_BLOCK_SIZE = 10.0
const WORLD_BLOCK_COUNT_X = 1000
const WORLD_BLOCK_COUNT_Y = 200
const WORLD_BLOCK_COUNT = WORLD_BLOCK_COUNT_X * WORLD_BLOCK_COUNT_Y
const WORLD_SIZE = Vector2(WORLD_BLOCK_SIZE * WORLD_BLOCK_COUNT_X, WORLD_BLOCK_COUNT_Y * WORLD_BLOCK_COUNT_Y)

const blocks = []

var rng = RandomNumberGenerator.new()

class Block:
	pass


const RENDER_DEBUG_SCALE = 0.15
const RENDER_DEBUG_OFFSET = Vector2(-2200, 0)


class AtmosphericBlock extends Block:
	func draw(canvas : CanvasItem, pos : Vector2):
		canvas.draw_rect(Rect2(RENDER_DEBUG_OFFSET + pos * RENDER_DEBUG_SCALE, Vector2(WORLD_BLOCK_SIZE, WORLD_BLOCK_SIZE) * RENDER_DEBUG_SCALE), Color(0.5, 0.5, 1.0), true)


class GroundBlock extends Block:
	func draw(canvas : CanvasItem, pos : Vector2):
		canvas.draw_rect(Rect2(RENDER_DEBUG_OFFSET + pos * RENDER_DEBUG_SCALE, Vector2(WORLD_BLOCK_SIZE, WORLD_BLOCK_SIZE) * RENDER_DEBUG_SCALE), Color(1.0, 0.5, 0.5), true)

class SurfaceBlock extends Block:
	func draw(canvas : CanvasItem, pos : Vector2):
		canvas.draw_rect(Rect2(RENDER_DEBUG_OFFSET + pos * RENDER_DEBUG_SCALE, Vector2(WORLD_BLOCK_SIZE, WORLD_BLOCK_SIZE) * RENDER_DEBUG_SCALE), Color(0.5, 1.0, 0.5), true)


func _ready():
	print("init world model")
	rng.randomize()
	#for i in range(WORLD_BLOCK_COUNT):
	#	blocks.append(Block.new())
	blocks.resize(WORLD_BLOCK_COUNT)


	var altitude = 1500.0
	var sloop_rate = 0

	for x in range (WORLD_BLOCK_COUNT_X):
		sloop_rate += rng.randfn(0.0, 1.0)

		sloop_rate = clamp(sloop_rate, -10, 10)
		altitude += sloop_rate

		if altitude < 200 or altitude > WORLD_BLOCK_COUNT_Y * WORLD_BLOCK_SIZE - 200:
			altitude = clamp(altitude, 200, WORLD_BLOCK_COUNT_Y * WORLD_BLOCK_SIZE - 200)
			sloop_rate /= 2

		var max_atm_block = int(altitude / WORLD_BLOCK_SIZE)

		for y in range(max_atm_block):
			var block = AtmosphericBlock.new()

			blocks[x + y  * WORLD_BLOCK_COUNT_X] = block

		for y in range(max_atm_block+1, WORLD_BLOCK_COUNT_Y):
			var block = GroundBlock.new()

			blocks[x + y  * WORLD_BLOCK_COUNT_X] = block

		blocks[x + max_atm_block  * WORLD_BLOCK_COUNT_X] = SurfaceBlock.new()

	print("init world done")


func _draw():
	for x in range (WORLD_BLOCK_COUNT_X):
		for y in range(WORLD_BLOCK_COUNT_Y):
			var block = blocks[x + y  * WORLD_BLOCK_COUNT_X]
			block.draw(self, Vector2(x * WORLD_BLOCK_SIZE, y * WORLD_BLOCK_SIZE))

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):

	for i in range (WORLD_BLOCK_COUNT):
		block[i].process_export()

	for i in range (WORLD_BLOCK_COUNT):
		block[i].apply_export()

	process_import()

	pass
