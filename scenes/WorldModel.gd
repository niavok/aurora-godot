extends Node2D

const WORLD_BLOCK_SIZE = 1000
#const WORLD_BLOCK_DEPTH = 1000.0
const WORLD_BLOCK_DEPTH = 1.0
const WORLD_BLOCK_INTERFACE_SECTION = WORLD_BLOCK_SIZE * WORLD_BLOCK_DEPTH
const WORLD_BLOCK_COUNT_X = 10
const WORLD_BLOCK_COUNT_Y = 50
const WORLD_BLOCK_COUNT = WORLD_BLOCK_COUNT_X * WORLD_BLOCK_COUNT_Y
const WORLD_SIZE = Vector2(WORLD_BLOCK_SIZE * WORLD_BLOCK_COUNT_X, WORLD_BLOCK_COUNT_Y * WORLD_BLOCK_COUNT_Y)


const RENDER_DEBUG_SCALE = 0.04
const RENDER_DEBUG_VELOCITY_SCALE = 15
const RENDER_DEBUG_OFFSET = Vector2(-2200, -1700)



const blocks = []

var rng = RandomNumberGenerator.new()

var world_processing_thread
var world_processing_thread_running

class Block:

	# static
	var gas_volume

	# dynamic
	var input_internal_energy = 0
	var output_internal_energy = 0
	var input_gas_composition_N = []
	var output_gas_composition_N = []
	var kinetic_energy = 0
	var velocity_direction = Vector2()
	var pending_input_internal_energy
	var pending_input_gas_composition_N = []
	var pending_kinetic_energy = []

	# Cache
	var pressure
	var temperature
	var N
	var energy_per_k
	var mass
	var input_volume = 0.0
	var output_volume = 0.0
	var input_mass = 0.0
	var output_mass = 0.0
	var input_elastic_energy
	var output_elastic_energy
	var input_thermal_energy
	var output_thermal_energy
	var input_energy_per_k
	var output_energy_per_k
	var give_volume_orders = []
	var velocity_cache
	var velocity_amplitude
	var pressure_gradient

	func init_empty():
		input_internal_energy = 0
		output_internal_energy = 0
		kinetic_energy = 0
		velocity_direction = Vector2()
		velocity_cache = Vector2()
		input_gas_composition_N.resize(PhysicalConstants.Gas.Count)
		output_gas_composition_N.resize(PhysicalConstants.Gas.Count)
		pending_input_gas_composition_N.resize(PhysicalConstants.Gas.Count)
		pending_input_internal_energy = 0

		for gas in range(PhysicalConstants.Gas.Count):
			input_gas_composition_N[gas] = 0
			output_gas_composition_N[gas] = 0
			pending_input_gas_composition_N[gas] = 0

	func init_with_gas(gas : int, temperature : float, pressure : float):
		init_empty()

		var totalN = PhysicalConstants.estimate_gas_N(gas_volume, pressure, temperature)
		output_gas_composition_N[gas] = totalN
		output_internal_energy = PhysicalConstants.estimate_internal_energy(gas, totalN, temperature, gas_volume);


	func update_cache():

		N = 0
		input_energy_per_k = 0
		output_energy_per_k = 0
		input_mass = 0
		output_mass = 0

		for gas in range(PhysicalConstants.Gas.Count):
			var input_N = input_gas_composition_N[gas]
			var output_N = output_gas_composition_N[gas]
			var gas_N = input_N + output_N

			if gas_N > 0:

				var gas_specific_heat_by_mole = PhysicalConstants.gas_specific_heat_by_mole[gas]
				var gas_mass_by_mole = PhysicalConstants.gas_mass_by_mole[gas]

				N += gas_N
				input_energy_per_k += input_N * gas_specific_heat_by_mole
				output_energy_per_k += output_N * gas_specific_heat_by_mole
				input_mass += input_N * gas_mass_by_mole;
				output_mass += output_N * gas_mass_by_mole;

		energy_per_k = input_energy_per_k + output_energy_per_k
		mass = input_mass + output_mass

		var internal_energy = input_internal_energy + output_internal_energy

		if N == 0:
			pressure = 0
			temperature = 0
			pressure_gradient = 0
		else:
			if input_mass == 0:
				input_thermal_energy = 0
			else:
				input_thermal_energy =  input_internal_energy / ( 1 + PhysicalConstants.gas_elastic_coef * PhysicalConstants.gas_constant * N / input_energy_per_k)
			if output_mass == 0:
				output_thermal_energy = 0
			else:
				output_thermal_energy =  output_internal_energy / ( 1 + PhysicalConstants.gas_elastic_coef * PhysicalConstants.gas_constant * N / output_energy_per_k)

			input_elastic_energy = input_internal_energy - input_thermal_energy
			output_elastic_energy = output_internal_energy - output_thermal_energy

			if input_elastic_energy > output_elastic_energy:
				input_volume = gas_volume / (1+ output_elastic_energy / input_elastic_energy)
				output_volume = gas_volume - input_volume
				pressure = input_elastic_energy / (PhysicalConstants.gas_elastic_coef * input_volume)
			else:
				output_volume = gas_volume / (1+ input_elastic_energy / output_elastic_energy)
				input_volume = gas_volume - output_volume
				pressure = output_elastic_energy / (PhysicalConstants.gas_elastic_coef * output_volume)

			var elastic_energy = input_elastic_energy + output_elastic_energy;
			var thermal_energy = input_thermal_energy + output_thermal_energy;

			temperature = thermal_energy / energy_per_k
			var density = mass / gas_volume
			pressure_gradient = density * PhysicalConstants.gravity

		if kinetic_energy > 0:
			velocity_amplitude = sqrt(kinetic_energy * 2 / mass)
			velocity_cache = velocity_direction * velocity_amplitude

	func add_give_volume_order(output_block : Block, volume : float, delta_height : float):
		give_volume_orders.append([output_block, volume, delta_height])


	func apply_gas_movement():
		var volume_to_give = 0
		for give_order in give_volume_orders:
			volume_to_give += give_order[1]

		var total_given_mass = 0
		var given_mass_by_order = []

		if volume_to_give < output_volume:
			for give_order in give_volume_orders:
				var output_volume_ratio = give_order[1] /  output_volume
				var internal_energy_to_give = output_volume_ratio * output_internal_energy
				var output_block = give_order[0]
				output_block.pending_input_internal_energy += internal_energy_to_give

				var given_mass_for_order = 0

				for gas in range(PhysicalConstants.Gas.Count):
					var N_to_give = output_volume_ratio * output_gas_composition_N[gas]
					output_block.pending_input_gas_composition_N[gas] += N_to_give
					given_mass_for_order += N_to_give * PhysicalConstants.gas_mass_by_mole[gas]
				total_given_mass += given_mass_for_order
				given_mass_by_order.append(given_mass_for_order)

			# remove given_composition
			var remaining_volume_ratio = 1 - volume_to_give / output_volume
			output_internal_energy *= remaining_volume_ratio
			for gas in range(PhysicalConstants.Gas.Count):
				output_gas_composition_N[gas] *= remaining_volume_ratio
		else:
			# give all output, part between order

			for give_order in give_volume_orders:
				var output_volume_ratio = give_order[1] /  volume_to_give
				var internal_energy_to_give = output_volume_ratio * output_internal_energy
				var output_block = give_order[0]
				output_block.pending_input_internal_energy += internal_energy_to_give

				var given_mass_for_order = 0

				for gas in range(PhysicalConstants.Gas.Count):
					var N_to_give = output_volume_ratio * output_gas_composition_N[gas]
					output_block.pending_input_gas_composition_N[gas] += N_to_give
					given_mass_for_order += N_to_give * PhysicalConstants.gas_mass_by_mole[gas]
				total_given_mass += given_mass_for_order
				given_mass_by_order.append(given_mass_for_order)

			# swap output
			var remaining_volume_to_give = volume_to_give - output_volume
			var remaining_volume_to_give_ratio = remaining_volume_to_give / volume_to_give

			output_internal_energy = input_internal_energy
			input_internal_energy = 0
			for gas in range(PhysicalConstants.Gas.Count):
				output_gas_composition_N[gas] = input_gas_composition_N[gas]
				input_gas_composition_N[gas] = 0
			output_volume = input_volume

			# give missing
			var give_order_index = 0
			for give_order in give_volume_orders:
				var output_volume_ratio = remaining_volume_to_give_ratio * give_order[1]/ output_volume
				var internal_energy_to_give = output_volume_ratio * output_internal_energy
				var output_block = give_order[0]
				output_block.pending_input_internal_energy += internal_energy_to_give

				var given_mass_for_order = 0

				for gas in range(PhysicalConstants.Gas.Count):
					var N_to_give = output_volume_ratio * output_gas_composition_N[gas]
					output_block.pending_input_gas_composition_N[gas] += N_to_give
					given_mass_for_order += N_to_give * PhysicalConstants.gas_mass_by_mole[gas]
				total_given_mass += given_mass_for_order
				given_mass_by_order[give_order_index] += given_mass_for_order
				give_order_index += 1

			# remove given composition
			var remaining_volume_ratio = 1 - remaining_volume_to_give / output_volume
			output_internal_energy *= remaining_volume_ratio
			for gas in range(PhysicalConstants.Gas.Count):
				output_gas_composition_N[gas] *= remaining_volume_ratio


		# Transfert part of kinetic energy depending on the given mass

		for give_order_index in range(give_volume_orders.size()):
			var given_mass_ratio = given_mass_by_order[give_order_index] / mass
			var given_kinetic_energy = kinetic_energy * given_mass_ratio
			give_volume_orders[give_order_index][0].give_kinetic_energy(given_kinetic_energy, velocity_direction)

		var remaining_kinetic_energy_ratio = 1 - total_given_mass / mass
		kinetic_energy *= remaining_kinetic_energy_ratio

		# compute potiential energydifference
		for give_order_index in range(give_volume_orders.size()):
			var delta_height = give_volume_orders[give_order_index][2]
			if delta_height != 0:
				var output_block = give_volume_orders[give_order_index][0]
				var potential_energy_diff = delta_height * given_mass_by_order[give_order_index] * PhysicalConstants.gravity
				var volume_ratio = give_volume_orders[give_order_index][1] / gas_volume
				output_block.pending_input_internal_energy += potential_energy_diff * volume_ratio
				output_internal_energy += potential_energy_diff * (1-volume_ratio)
		give_volume_orders = []

	func integrate_pending():
		input_internal_energy += pending_input_internal_energy

		for gas in range(PhysicalConstants.Gas.Count):
			input_gas_composition_N[gas] += pending_input_gas_composition_N[gas]
			pending_input_gas_composition_N[gas] = 0

		pending_input_internal_energy = 0
		input_gas_composition_N

		if pending_kinetic_energy.size() > 0:
			var kinetic_energy_vector = kinetic_energy * velocity_direction
			var total_kinetic_energy = kinetic_energy
			for kinetic_energy_addition in pending_kinetic_energy:
				var impulse_vector = kinetic_energy_addition[0] * kinetic_energy_addition[1]
				total_kinetic_energy += kinetic_energy_addition[0]
				kinetic_energy_vector += impulse_vector

			var new_kinetic_energy = kinetic_energy_vector.length()
			velocity_direction = kinetic_energy_vector.normalized()

			var kinetic_energy_diff = total_kinetic_energy - new_kinetic_energy
			input_internal_energy += kinetic_energy_diff
			pending_kinetic_energy = []

	func give_kinetic_energy(energy, direction):
		pending_kinetic_energy.append([energy, direction])

	func draw_velocity_debug(canvas : CanvasItem, pos : Vector2):
		var center_pos = pos + Vector2(WORLD_BLOCK_SIZE, WORLD_BLOCK_SIZE) * 0.5
		var velocity_pos = center_pos + velocity_cache * RENDER_DEBUG_VELOCITY_SCALE

		canvas.draw_line(RENDER_DEBUG_OFFSET + center_pos * RENDER_DEBUG_SCALE, RENDER_DEBUG_OFFSET + velocity_pos * RENDER_DEBUG_SCALE, Color(1.0,0.0,0.0), 1)

	func draw_debug_tile(canvas : CanvasItem, pos : Vector2):

		var pressure_opacity = clamp(pressure / (100000*1.0), 0.0, 1.0)
		var temperature_red = clamp(temperature / 3000.0, 0.0, 1.0)

		var color = Color(temperature_red, 0.5, 1.0, pressure_opacity)

		canvas.draw_rect(Rect2(RENDER_DEBUG_OFFSET + pos * RENDER_DEBUG_SCALE, Vector2(WORLD_BLOCK_SIZE, WORLD_BLOCK_SIZE) * RENDER_DEBUG_SCALE), color, true)

		#var temp_text = String.format("%10.3f" % temperature)

		var temp_text = "%10.1f" % temperature
		#var pressure_text = "%10.1f" % ((pressure-100000))
		var pressure_text = "%10.2f" % (pressure/100000)

		canvas.draw_string(canvas.font, RENDER_DEBUG_OFFSET + pos * RENDER_DEBUG_SCALE + Vector2(0, 10), temp_text, Color(1.0,1.0,1.0,1.0))
		canvas.draw_string(canvas.font, RENDER_DEBUG_OFFSET + pos * RENDER_DEBUG_SCALE + Vector2(0, 25), pressure_text, Color(1.0,1.0,1.0,1.0))

	func get_pressure_at_relative_altitude(relative_altitude : float):
		return pressure + pressure_gradient * relative_altitude

class AtmosphericBlock extends Block:

	func init():
		gas_volume = WORLD_BLOCK_DEPTH * WORLD_BLOCK_SIZE * WORLD_BLOCK_SIZE
		init_empty()
		#init_with_gas(PhysicalConstants.Gas.Nitrogen, 274, 1e5)
		init_with_gas(PhysicalConstants.Gas.Nitrogen, 300, 0.13e5)

	func draw(canvas : CanvasItem, pos : Vector2):
		draw_debug_tile(canvas, pos)

		draw_velocity_debug(canvas, pos)



class GroundBlock extends Block:
	func init():
		gas_volume = WORLD_BLOCK_DEPTH * WORLD_BLOCK_SIZE * WORLD_BLOCK_SIZE
		init_empty()
		init_with_gas(PhysicalConstants.Gas.Nitrogen, 300, 0.13e5)
		# TODO proportionnal

	func draw(canvas : CanvasItem, pos : Vector2):
		draw_debug_tile(canvas, pos)
		#canvas.draw_rect(Rect2(RENDER_DEBUG_OFFSET + pos * RENDER_DEBUG_SCALE, Vector2(WORLD_BLOCK_SIZE, WORLD_BLOCK_SIZE) * RENDER_DEBUG_SCALE), Color(1.0, 0.5, 0.5), true)
		draw_velocity_debug(canvas, pos)

class SurfaceBlock extends Block:

	func init():
		gas_volume = WORLD_BLOCK_DEPTH * WORLD_BLOCK_SIZE * WORLD_BLOCK_SIZE
		init_empty()
		init_with_gas(PhysicalConstants.Gas.Nitrogen, 300, 0.13e5)
		# TODO depend on ground composition

	func draw(canvas : CanvasItem, pos : Vector2):
		#canvas.draw_rect(Rect2(RENDER_DEBUG_OFFSET + pos * RENDER_DEBUG_SCALE, Vector2(WORLD_BLOCK_SIZE, WORLD_BLOCK_SIZE) * RENDER_DEBUG_SCALE), Color(0.5, 1.0, 0.5), true)
		draw_debug_tile(canvas, pos)
		draw_velocity_debug(canvas, pos)

var font

func _ready():
	print("init world model")
	rng.randomize()
	#for i in range(WORLD_BLOCK_COUNT):
	#	blocks.append(Block.new())
	blocks.resize(WORLD_BLOCK_COUNT)

	var label = Label.new()
	font = label.get_font("")

	#font = load("res://fonts/DebugFont.tres")

#	font = DynamicFont.new()
#	font.font_data = load("res://fonts/RobotoMono-Regular.ttf")
#	font.size = 8

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

	for i in range (WORLD_BLOCK_COUNT):
		var block = blocks[i]
		block.init()
		block.update_cache()



	print("init world done")

	world_processing_thread = Thread.new()
	world_processing_thread_running = true
	#world_processing_thread.start(self, "process_world")

func _exit_tree():
	world_processing_thread_running = false
	#world_processing_thread.wait_to_finish()

func _draw():
	for x in range (WORLD_BLOCK_COUNT_X):
		for y in range(WORLD_BLOCK_COUNT_Y):
			var block = blocks[x + y  * WORLD_BLOCK_COUNT_X]
			block.draw(self, Vector2(x * WORLD_BLOCK_SIZE, y * WORLD_BLOCK_SIZE))


var paused  = true

func _process(delta):

	if Input.is_action_just_pressed("world_toogle_pause"):
		paused = !paused

	if not paused or Input.is_action_just_pressed("world_step"):
		step_world(0.1)
		update()

	if Input.is_action_just_pressed("world_toogle_pause") or Input.is_action_just_pressed("world_step"):
		var total_internal_energy = 0
		var total_kinetic_energy = 0
		var total_elastic_energy = 0
		var total_thermal_energy = 0

		var total_gas_N = []
		for gas in range(PhysicalConstants.Gas.Count):
			total_gas_N.append(0)

		for block in blocks:
			total_internal_energy += block.input_internal_energy + block.output_internal_energy
			total_thermal_energy += block.input_thermal_energy + block.output_thermal_energy
			total_elastic_energy += block.input_elastic_energy + block.output_elastic_energy
			total_kinetic_energy += block.kinetic_energy
			for gas in range(PhysicalConstants.Gas.Count):
				total_gas_N[gas] += block.input_gas_composition_N[gas] + block.output_gas_composition_N[gas]

		var total_energy = total_internal_energy + total_kinetic_energy

		print("====")
		print("total_internal_energy %s TJ" % (total_internal_energy / 1e9))
		print("total_kinetic_energy %s MJ" % (total_kinetic_energy/ 1e6))
		print("total_thermal_energy %s TJ" % (total_thermal_energy/ 1e9))
		print("total_elastic_energy %s TJ" % (total_elastic_energy/ 1e9))
		print("total_gas N:")
		for gas in range(PhysicalConstants.Gas.Count):
			print("- %s Mmol" % (total_gas_N[gas] / 1e6) )
		print("total_energy %s TJ" % (total_energy/ 1e9))

# Called every frame. 'delta' is the elapsed time since the previous frame.
func process_world(userdata):
	print("process_world")
	#var plop = 0
	#var plip = 2 / plop
	while world_processing_thread_running:
		 step_world(0.1)

func step_world(delta_time):

	var first_block = blocks[0]
	#print("step world model")

	# compute pressure again ?

	#for x in range (WORLD_BLOCK_COUNT_X):
	#	for y in range (WORLD_BLOCK_COUNT_Y):
	#		compute_gas_block_movement(delta_time, x, y, get_block_at(x, y))

	for x in range (WORLD_BLOCK_COUNT_X):
		for y in range (WORLD_BLOCK_COUNT_Y - 1):
			compute_gas_transition(delta_time, get_block_at_with_wrapping(x, y), get_block_at_with_wrapping(x, y), 1, 0)
			compute_gas_transition(delta_time, get_block_at_with_wrapping(x, y), get_block_at_with_wrapping(x, y), 0, 1)
	for x in range (WORLD_BLOCK_COUNT_X):
		compute_gas_transition(delta_time, get_block_at_with_wrapping(x, WORLD_BLOCK_COUNT_Y - 1), get_block_at_with_wrapping(x, WORLD_BLOCK_COUNT_Y - 1), 1, 0)


	for i in range (WORLD_BLOCK_COUNT):
		blocks[i].apply_gas_movement()

# TODO diffusion, decantation


	for i in range (WORLD_BLOCK_COUNT):
		var block = blocks[i]
		block.integrate_pending()
		block.update_cache()


	# export molecules based one tempature -> velocity repartition -> movement on side
	# https://fr.wikipedia.org/wiki/Loi_de_distribution_des_vitesses_de_Maxwell
	# elastique energy

func get_relative_force(block_x : int, block_y : int, dx : int, dy: int, dir_and_coef : Vector2, local_block : Block):
	var neightbourg_block : Block = get_block_at_with_wrapping(block_x + dx, block_y + dy)
	if not neightbourg_block:
		return Vector2()

	var local_pressure_at_altitude = local_block.pressure + local_block.pressure_gradient * dy * WORLD_BLOCK_SIZE *0.5
	var neightbourg_pressure_at_altitude = neightbourg_block.pressure - neightbourg_block.pressure_gradient * dy * WORLD_BLOCK_SIZE *0.5

	var delta_pressure = neightbourg_pressure_at_altitude - local_pressure_at_altitude # Pa
	var force = delta_pressure * WORLD_BLOCK_INTERFACE_SECTION  # Pa * m2 = N
	var directional_force = force * dir_and_coef
	return directional_force


func compute_gas_block_movement(delta_time : float, block_x : int, block_y : int, block : Block):

	var forces = Vector2()

	if block.mass > 0:
		forces += get_relative_force(block_x, block_y, -1, -1, Vector2( 0.5, 0.5), block)
		forces += get_relative_force(block_x, block_y,  1, -1, Vector2(-0.5, 0.5), block)
		forces += get_relative_force(block_x, block_y,  1,  1, Vector2(-0.5,-0.5), block)
		forces += get_relative_force(block_x, block_y, -1,  1, Vector2( 0.5,-0.5), block)

		forces += get_relative_force(block_x, block_y,  1,  0, Vector2(-1,0), block)
		forces += get_relative_force(block_x, block_y, -1,  0, Vector2(1,0), block)
		forces += get_relative_force(block_x, block_y,  0,  1, Vector2(0,-1), block)
		forces += get_relative_force(block_x, block_y,  0, -1, Vector2(0,1), block)


		var forces_acceleration = forces / block.mass

		var acceleration = Vector2(0, 9.81) + forces_acceleration


		var new_velocity = block.velocity_cache + acceleration * delta_time

		# debug dumping
		new_velocity *= 0.95


		var top_block = get_block_at_with_wrapping(block_x, block_y -1)
		var bottom_block = get_block_at_with_wrapping(block_x, block_y + 1)
		var left_block =  get_block_at_with_wrapping(block_x - 1, block_y)
		var right_block = get_block_at_with_wrapping(block_x + 1, block_y)

		if not top_block and new_velocity.y < 0:
			new_velocity.y = 0
		if not bottom_block and new_velocity.y > 0:
			new_velocity.y = 0

		var new_kinetic_energy = 0.5 * block.mass * new_velocity.length_squared()

		var kinetic_energy_diff = new_kinetic_energy - block.kinetic_energy

		var internal_energy = block.input_internal_energy + block.output_internal_energy

		block.kinetic_energy = new_kinetic_energy
		block.velocity_direction =new_velocity.normalized()

		var remaining_internal_energy = internal_energy - kinetic_energy_diff

		var new_energy_ratio = remaining_internal_energy / internal_energy
		block.input_internal_energy *= new_energy_ratio
		block.output_internal_energy *= new_energy_ratio

		# Apply movement
		#TODO
		var block_movement = new_velocity * delta_time

		block_movement.x = clamp(block_movement.x, -WORLD_BLOCK_SIZE*0.5, WORLD_BLOCK_SIZE*0.5)
		block_movement.y = clamp(block_movement.y, -WORLD_BLOCK_SIZE*0.5, WORLD_BLOCK_SIZE*0.5)

		# Top and bottom volume


		var top_volume = block_movement.y * (WORLD_BLOCK_SIZE - abs(block_movement.x)) * WORLD_BLOCK_DEPTH
		var left_volume = block_movement.x * (WORLD_BLOCK_SIZE - abs(block_movement.y)) * WORLD_BLOCK_DEPTH

		if top_block:
			transfert_block_volume(block, top_block, -top_volume * 0.5, -WORLD_BLOCK_SIZE)
		if bottom_block:
			transfert_block_volume(block, bottom_block, top_volume * 0.5, WORLD_BLOCK_SIZE)
		if left_block:
			transfert_block_volume(block, left_block, left_volume * 0.5, 0)
		if right_block:
			transfert_block_volume(block, right_block, -left_volume * 0.5, 0)

		if sign(block_movement.y) ==  sign(block_movement.x):
			# Top left diagonal
			var diagonal_volume = sign(block_movement.x) * block_movement.y * block_movement.x * WORLD_BLOCK_DEPTH

			var top_left_block = get_block_at_with_wrapping(block_x-1, block_y -1)
			var bottom_right_block = get_block_at_with_wrapping(block_x + 1, block_y + 1)
			if top_left_block:
				transfert_block_volume(block, top_left_block, -diagonal_volume * 0.5, -WORLD_BLOCK_SIZE)
			if bottom_right_block:
				transfert_block_volume(block, bottom_right_block, diagonal_volume * 0.5, WORLD_BLOCK_SIZE)
		else:
			# Top right diagonal
			var diagonal_volume = sign(block_movement.x) * block_movement.y * block_movement.x * WORLD_BLOCK_DEPTH

			var top_right_block = get_block_at_with_wrapping(block_x+1, block_y -1)
			var bottom_left_block = get_block_at_with_wrapping(block_x - 1, block_y + 1)
			if top_right_block:
				transfert_block_volume(block, top_right_block, -diagonal_volume * 0.5, -WORLD_BLOCK_SIZE)
			if bottom_left_block:
				transfert_block_volume(block, bottom_left_block, diagonal_volume * 0.5, WORLD_BLOCK_SIZE)

func compute_gas_transition(delta_time : float, block_a : Block, block_b : Block, dx : int, dy : int):
	var delta_pressure = block_b.get_pressure_at_relative_altitude(-dy * WORLD_BLOCK_SIZE * 0.5) - block_a.get_pressure_at_relative_altitude(dy * WORLD_BLOCK_SIZE * 0.5)

	var transition_direction = Vector2(dx, dy)

	var transition_mass = 0.5 * (block_a.mass + block_b.mass)

	var transition_velocity = 0.5 * (block_a.velocity_cache * block_a.mass + block_b.velocity_cache * block_b.mass) / transition_mass

	var force = transition_direction * delta_pressure * WORLD_BLOCK_INTERFACE_SECTION # Pa * m2 = N

	var acceleration = force / transition_mass

	if dy != 0:
		acceleration += Vector2(0, 9.81)

	var new_transition_velocity = transition_velocity + acceleration * delta_time

	var linear_transition_velocity = new_transition_velocity.dot(transition_direction)

	var linear_translation = linear_transition_velocity * delta_time
	var linear_translation_clamped = clamp(linear_translation,-WORLD_BLOCK_SIZE * 0.5, WORLD_BLOCK_SIZE * 0.5)
	var volume_displaced = linear_translation_clamped * WORLD_BLOCK_INTERFACE_SECTION

	transfert_block_volume(block_a, block_b, volume_displaced, dy * WORLD_BLOCK_SIZE)


func get_block_at(block_x : int, block_y : int):
	assert(block_x > -1)
	assert(block_y > -1)
	assert(block_x < WORLD_BLOCK_COUNT_X)
	assert(block_y < WORLD_BLOCK_COUNT_Y)

	return blocks[block_x + block_y * WORLD_BLOCK_COUNT_X]

func get_block_at_with_wrapping(block_x : int, block_y : int):
	if block_y < 0 or block_y >= WORLD_BLOCK_COUNT_Y:
		return null

	if block_x < 0:
		block_x += WORLD_BLOCK_COUNT_X
	elif block_x >= WORLD_BLOCK_COUNT_X:
		 block_x -= WORLD_BLOCK_COUNT_X


	return get_block_at(block_x, block_y)

func transfert_block_volume(block_a, block_b, signed_volume, delta_height):

	var input_block
	var output_block
	var volume = abs(signed_volume)

	if volume == 0:
		return

	if signed_volume > 0:
		input_block = block_a
		output_block = block_b
	else:
		input_block = block_b
		output_block = block_a

	if volume >  input_block.gas_volume:
		print("give overflow")

	input_block.add_give_volume_order(output_block, volume, sign(signed_volume) * delta_height)




